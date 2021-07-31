/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2014 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "AudioOutputDeviceAsio.h"
#include "AudioOutputDeviceFactory.h"

#define kMaxInputChannels 32
#define kMaxOutputChannels 32
#define ASIO_MAX_DEVICE_INFO 32

#define NUM_STANDARDSAMPLINGRATES 3  // 11.025, 22.05, 44.1
#define NUM_CUSTOMSAMPLINGRATES   9  // must be the same number of elements as in the array below
#define MAX_NUMSAMPLINGRATES  (NUM_STANDARDSAMPLINGRATES + NUM_CUSTOMSAMPLINGRATES)

// internal data storage
struct DriverInfo {
    // ASIOInit()
    ASIODriverInfo driverInfo;

    // ASIOGetChannels()
    long           numInputChannels;
    long           numOutputChannels;

    // ASIOGetBufferSize()
    long           minBufSize;
    long           maxBufSize;
    long           preferredBufSize;
    long           bufGranularity;
    long           chosenBufSize;

    // ASIOGetSampleRate()
    ASIOSampleRate sampleRate;
    std::vector<int> sampleRates;

    // ASIOOutputReady()
    bool           ASIOOutputReadySupported;

    // ASIOGetLatencies ()
    long           inputLatency;
    long           outputLatency;

    // ASIOCreateBuffers ()
    long           numInputBuffers;  // becomes number of actual created input buffers
    long           numOutputBuffers; // becomes number of actual created output buffers
    ASIOBufferInfo bufferInfos[kMaxInputChannels + kMaxOutputChannels];

    // ASIOGetChannelInfo()
    ASIOChannelInfo channelInfos[kMaxInputChannels + kMaxOutputChannels];
    // The above two arrays share the same indexing, as the data in them are linked together

    // Information from ASIOGetSamplePosition()
    // data is converted to double floats for easier use, however 64 bit integer can be used, too
    double         nanoSeconds;
    double         samples;
    double         tcSamples;   // time code samples

    // bufferSwitchTimeInfo()
    ASIOTime       tInfo;       // time info state
    unsigned long  sysRefTime;  // system reference time, when bufferSwitch() was called

    // Signal the end of processing in this example
    bool           stopped;
};

extern AsioDrivers* asioDrivers;
extern bool loadAsioDriver(char* name);

static DriverInfo asioDriverInfo = { { 0 }, 0 };

static bool asioDriverOpened = false;
static bool AudioOutputDeviceAsioInstantiated = false;
static String currentAsioDriverName;
static std::vector<String> asioDriverList;
static bool asioDriverListLoaded = false;

namespace LinuxSampler {

// callback prototypes

ASIOCallbacks asioCallbacks;
AudioOutputDeviceAsio* GlobalAudioOutputDeviceAsioThisPtr;

template<int bitres>
static void floatToASIOSTInt32LSBXX(float* in, void* dest, int numSamples) {
    double pos_max_value = (1 << bitres) -1.0;
    double neg_max_value = -pos_max_value;
    int32_t* out = (int32_t*)dest;
    for (int i = 0; i < numSamples ; i++) {
        double sample_point = in[i] * pos_max_value;
        if (sample_point < neg_max_value) sample_point = neg_max_value;
        if (sample_point >  pos_max_value) sample_point =  pos_max_value;
        out[i] = (int16_t)sample_point;
    }
}

static void floatToASIOSTInt16LSB(float* in, void* dest, int numSamples) {
    int16_t* out = (int16_t*)dest;
    for (int i = 0; i < numSamples ; i++) {
        float sample_point = in[i] * 32768.0f;
        if (sample_point < -32768.0) sample_point = -32768.0;
        if (sample_point >  32767.0) sample_point =  32767.0;
        out[i] = (int16_t)sample_point;
    }
}

static void floatToASIOSTInt32LSB(float* in, void* dest, int numSamples) {
    int32_t* out = (int32_t*)dest;
    for (int i = 0; i < numSamples ; i++) {
        double sample_point = in[i] * 2147483648.0;
        if (sample_point < - 2147483648.0) sample_point = -2147483648.0;
        if (sample_point >  2147483647.0) sample_point =  2147483647.0;
        out[i] = (int32_t)(sample_point);
    }
}

std::vector<String> getAsioDriverNames();

static bool ASIO_loadAsioDriver(const char* name) {
    dmsg(2,("ASIO_loadAsioDriver: trying to load '%s'\n", name));
#ifdef  WINDOWS
    CoInitialize(0);
#endif
    return loadAsioDriver(const_cast<char*>(name));
}

int ASIO_OpenAndQueryDeviceInfo(String driverName, DriverInfo* driverInfo) {
    ASIOSampleRate possibleSampleRates[] =
        { 8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0,
          32000.0, 44100.0, 48000.0, 88200.0, 96000.0 };

    ASIOError asioError;

    // Call this function in order to fill the internal vector
    // containing the list of ASIO cards.

    // Since the reading of the list is performed only one time,
    // subsequent calls to this function will be automatically
    // ignored.

    // Calling getAsioDriverNames() is needed in the case of a
    // LinuxSampler client application first creating an ASIO device,
    // which calls ASIO_OpenAndQueryDeviceInfo(), and afterwards
    // retrieve the list of available ASIO cards, since
    // getAsioDriverNames() unloads the current driver and loads the
    // driver 'dummy' this would cause the audio stopping and forcing
    // the create the audio output device again.
    getAsioDriverNames();

    dmsg(2,("ASIO_OpenAndQueryDeviceInfo driverName='%s' current='%s'\n",
            driverName.c_str(), currentAsioDriverName.c_str()));
    if (driverName == currentAsioDriverName) {
        // if the given ASIO driver name == current ASIO driver name
        // and the driver was already opened then do nothing
        if (asioDriverOpened) {
            dmsg(2,("asioDriver already opened, doing nothing\n"));
            return 0;
        }
    }
    else {
        dmsg(2,("driverName != currentAsioDriverName, new asio driver specified, opening device...\n"));
        // if a new ASIO driver name was specified then first check if
        // we need to close the old one
        if (asioDriverOpened) {
            dmsg(2,("different asioDriver already opened, closing old one\n"));
            asioDriverOpened = false;
            ASIOExit(); // close the old ASIO driver
        }
    }
    currentAsioDriverName = driverName;

    memset(&driverInfo->driverInfo, 0, sizeof(ASIODriverInfo));
    driverInfo->driverInfo.asioVersion = 1;
    driverInfo->driverInfo.sysRef = NULL;

    // MUST BE CHECKED : to force fragments loading on Mac
    // ASIO_loadAsioDriver("dummy");

    dmsg(2,("Before ASIO_loadAsioDriver('%s')\n", driverName.c_str()));
    if (!ASIO_loadAsioDriver(driverName.c_str())) {
        dmsg(2,("ASIO_OpenAndQueryDeviceInfo could not loadAsioDriver %s\n", driverName.c_str()));
        return -1;
    }
    dmsg(2,("Before ASIOInit()\n"));
    if ((asioError = ASIOInit(&driverInfo->driverInfo)) != ASE_OK) {
        dmsg(2,("ASIO_OpenAndQueryDeviceInfo: ASIOInit returned %d for %s\n",
                int(asioError), driverName.c_str()));
        return asioError;
    }
    dmsg(2,("Before ASIOGetChannels()\n"));
    if ((asioError = ASIOGetChannels(&driverInfo->numInputChannels,
                                     &driverInfo->numOutputChannels)) != ASE_OK) {
        dmsg(1,("ASIO_OpenAndQueryDeviceInfo could not ASIOGetChannels for %s: %d\n",
                driverName.c_str(), int(asioError)));
        return asioError;
    }

    dmsg(2,("Before ASIOGetBufferSize()\n"));
    if ((asioError = ASIOGetBufferSize(&driverInfo->minBufSize,
                                       &driverInfo->maxBufSize,
                                       &driverInfo->preferredBufSize,
                                       &driverInfo->bufGranularity)) != ASE_OK) {
        dmsg(1,("ASIO_OpenAndQueryDeviceInfo could not ASIOGetBufferSize for %s: %d\n",
                driverName.c_str(), int(asioError)));
        return asioError;
    }

    dmsg(2,("ASIO_OpenAndQueryDeviceInfo: InputChannels = %ld\n", driverInfo->numInputChannels));
    dmsg(2,("ASIO_OpenAndQueryDeviceInfo: OutputChannels = %ld\n", driverInfo->numOutputChannels));

    // Loop through the possible sampling rates and check each to see
    // if the device supports it.
    driverInfo->sampleRates.clear();
    for (int index = 0; index < MAX_NUMSAMPLINGRATES; index++) {
        if (ASIOCanSampleRate(possibleSampleRates[index]) != ASE_NoClock) {
            dmsg(2,("ASIOCanSampleRate: possible sample rate = %ld\n",
                    (long)possibleSampleRates[index]));
            driverInfo->sampleRates.push_back((int)possibleSampleRates[index]);
        }
    }

    // get the channel infos for each output channel (including sample format)
    for (int i = 0; i < driverInfo->numOutputChannels; i++) {
        driverInfo->channelInfos[i].channel = i;
        driverInfo->channelInfos[i].isInput = ASIOFalse;
        ASIOGetChannelInfo(&driverInfo->channelInfos[i]);
        dmsg(2,("channelInfos[%d].type (sampleformat) = %d\n",
                i, int(driverInfo->channelInfos[i].type)));
    }

    dmsg(2,("ASIO_OpenAndQueryDeviceInfo: driver opened.\n"));
    asioDriverOpened = true;
    return ASE_OK;
}


std::vector<String> getAsioDriverNames() {
    char* names[ASIO_MAX_DEVICE_INFO];
    int numDrivers;

    if (asioDriverListLoaded) {
        dmsg(2,("getAsioDriverNames: ASIO driver list already loaded, doing returning cached list.\n"));
        return asioDriverList;
    }

    // MUST BE CHECKED : to force fragments loading on Mac
    ASIO_loadAsioDriver("dummy");

#if MAC
    numDrivers = asioDrivers->getNumFragments();
#elif WINDOWS
    numDrivers = asioDrivers->asioGetNumDev();
#endif

    for (int i = 0; i < ASIO_MAX_DEVICE_INFO; i++) {
        names[i] = new char[32];
        memset(names[i], 0, 32);
    }

    // Get names of all available ASIO drivers
    asioDrivers->getDriverNames(names, ASIO_MAX_DEVICE_INFO);
    dmsg(2,("getAsioDriverNames: numDrivers=%d\n", numDrivers));

    for (int i = 0; i < numDrivers; i++) {
        dmsg(2,("ASIO DRIVERLIST: i=%d  name='%s'\n", i, names[i]));

#if 1
        asioDriverList.push_back(names[i]);
#else
        // FIXME: We currently try what is the best method to exclude
        // not connected devices or not working drivers. The code
        // below is for testing only, it tries to load each ASIO
        // driver and if it gives an error it is not included in the
        // list of available drivers (asioDriverList).
        if (ASIO_loadAsioDriver(names[i])) {
            ASIOError asioError;
            if ((asioError = ASIOInit(&asioDriverInfo.driverInfo)) == ASE_OK) {
                asioDriverList.push_back(names[i]);
            }
            else {
                dmsg(2,("getDriverList: ASIOInit of driver %s gave Error %d! ignoring it.\n",
                        names[i], int(asioError)));
            }
        }
        else {
            dmsg(2,("getDriverList: load  driver %s failed! ignoring it.\n", names[i]));
        }
        // FIXME: we need to check this ASIOExit is needed (gave a
        // crash on a ASIO ADSP24(WDM) card so we commented it out)
        // ASIOExit();
#endif
        //currentAsioDriverName = "";
    }

    for (int i = 0; i < ASIO_MAX_DEVICE_INFO; i++) {
        delete[] names[i];
    }

    dmsg(2,("getAsioDriverNames: returing from function. asioDriverList.size()=%d\n",
            int(asioDriverList.size())));
    asioDriverListLoaded = true;
    return asioDriverList;
}

// currently not unsed
unsigned long get_sys_reference_time() {
    // get the system reference time
#if WINDOWS
    return timeGetTime();
#elif MAC
    static const double twoRaisedTo32 = 4294967296.;
    UnsignedWide ys;
    Microseconds(&ys);
    double r = ((double)ys.hi * twoRaisedTo32 + (double)ys.lo);
    return (unsigned long)(r / 1000.);
#endif
}


//----------------------------------------------------------------------------------
// conversion from 64 bit ASIOSample/ASIOTimeStamp to double float
#if NATIVE_INT64
#define ASIO64toDouble(a)  (a)
#else
const double twoRaisedTo32 = 4294967296.;
#define ASIO64toDouble(a)  ((a).lo + (a).hi * twoRaisedTo32)
#endif


ASIOTime* AudioOutputDeviceAsio::bufferSwitchTimeInfo(ASIOTime *timeInfo, long index,
                                                      ASIOBool processNow) {
    static long processedSamples = 0;
    // store the timeInfo for later use
    asioDriverInfo.tInfo = *timeInfo;

    // get the time stamp of the buffer, not necessary if no
    // synchronization to other media is required
    if (timeInfo->timeInfo.flags & kSystemTimeValid)
        asioDriverInfo.nanoSeconds = ASIO64toDouble(timeInfo->timeInfo.systemTime);
    else
        asioDriverInfo.nanoSeconds = 0;

    if (timeInfo->timeInfo.flags & kSamplePositionValid)
        asioDriverInfo.samples = ASIO64toDouble(timeInfo->timeInfo.samplePosition);
    else
        asioDriverInfo.samples = 0;

    if (timeInfo->timeCode.flags & kTcValid)
        asioDriverInfo.tcSamples = ASIO64toDouble(timeInfo->timeCode.timeCodeSamples);
    else
        asioDriverInfo.tcSamples = 0;

    // TODO: ignore for now. get the system reference time
    // asioDriverInfo.sysRefTime = get_sys_reference_time();

    // buffer size in samples
    long buffSize = asioDriverInfo.chosenBufSize;

    // tell LinuxSampler to  render a fragment of buffSize samples
    GlobalAudioOutputDeviceAsioThisPtr->RenderAudio(buffSize);

    // now write and convert the samples to the ASIO buffer
    for (int i = 0; i < asioDriverInfo.numOutputBuffers; i++)
    {
        // do processing for the outputs only
        switch (asioDriverInfo.channelInfos[i].type)
        {
        case ASIOSTInt16LSB:
            floatToASIOSTInt16LSB(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                  (int16_t*)asioDriverInfo.bufferInfos[i].buffers[index], buffSize);
            break;
        case ASIOSTInt24LSB:   // used for 20 bits as well
            memset (asioDriverInfo.bufferInfos[i].buffers[index], 0, buffSize * 3);
            break;
        case ASIOSTInt32LSB:
            floatToASIOSTInt32LSB(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                  (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index], buffSize);
            break;
        case ASIOSTFloat32LSB: // IEEE 754 32 bit float, as found on Intel x86 architecture
            throw AudioOutputException("ASIO Error: ASIOSTFloat32LSB not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTFloat64LSB: // IEEE 754 64 bit double float, as found on Intel x86 architecture
            throw AudioOutputException("ASIO Error: ASIOSTFloat64LSB not yet supported! report to LinuxSampler developers.");
            break;

            // These are used for 32 bit data buffer, with different
            // alignment of the data inside. 32 bit PCI bus systems
            // can more easily used with these.

        case ASIOSTInt32LSB16: // 32 bit data with 16 bit alignment
            floatToASIOSTInt32LSBXX<16>(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                        (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index],
                                        buffSize);
            break;
        case ASIOSTInt32LSB18: // 32 bit data with 18 bit alignment
            floatToASIOSTInt32LSBXX<18>(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                        (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index],
                                        buffSize);
            break;
        case ASIOSTInt32LSB20: // 32 bit data with 20 bit alignment
            floatToASIOSTInt32LSBXX<20>(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                        (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index],
                                        buffSize);
            break;
        case ASIOSTInt32LSB24: // 32 bit data with 24 bit alignment
            floatToASIOSTInt32LSBXX<24>(GlobalAudioOutputDeviceAsioThisPtr->Channels[i]->Buffer(),
                                        (int32_t*)asioDriverInfo.bufferInfos[i].buffers[index],
                                        buffSize);
            break;
        case ASIOSTInt16MSB:
            throw AudioOutputException("ASIO Error: ASIOSTInt16MSB not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTInt24MSB:   // used for 20 bits as well
            throw AudioOutputException("ASIO Error: ASIOSTInt24MSB not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTInt32MSB:
            throw AudioOutputException("ASIO Error: ASIOSTInt32MSB not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTFloat32MSB: // IEEE 754 32 bit float, as found on bigendian architecture
            throw AudioOutputException("ASIO Error: ASIOSTFloat32MSB not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTFloat64MSB: // IEEE 754 64 bit double float, as found on bigendian architecture
            throw AudioOutputException("ASIO Error: ASIOSTFloat64MSB not yet supported! report to LinuxSampler developers.");
            break;

            // These are used for 32 bit data buffer, with different
            // alignment of the data inside.  32 bit PCI bus systems
            // can more easily used with these.

        case ASIOSTInt32MSB16: // 32 bit data with 18 bit alignment
            throw AudioOutputException("ASIO Error: ASIOSTInt32MSB16 not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTInt32MSB18: // 32 bit data with 18 bit alignment
            throw AudioOutputException("ASIO Error: ASIOSTInt32MSB18 not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTInt32MSB20: // 32 bit data with 20 bit alignment
            throw AudioOutputException("ASIO Error: ASIOSTInt32MSB20 not yet supported! report to LinuxSampler developers.");
            break;
        case ASIOSTInt32MSB24: // 32 bit data with 24 bit alignment
            throw AudioOutputException("ASIO Error: ASIOSTInt32MSB24 not yet supported! report to LinuxSampler developers.");
            break;
        default:
            throw AudioOutputException("ASIO Error: unknown ASIOST sample format. report error.");
            break;
        }
    }

    // finally if the driver supports the ASIOOutputReady()
    // optimization, do it here, all data are in place
    if (asioDriverInfo.ASIOOutputReadySupported)
        ASIOOutputReady();

    processedSamples += buffSize;
    return 0L;
}

void AudioOutputDeviceAsio::bufferSwitch(long index, ASIOBool processNow) {
    // The actual processing callback.

    // As this is a "back door" into the bufferSwitchTimeInfo, a
    // timeInfo needs to be created, though it will only set the
    // timeInfo.samplePosition and timeInfo.systemTime fields and the
    // according flags.
    ASIOTime timeInfo;
    memset(&timeInfo, 0, sizeof(timeInfo));

    // Get the time stamp of the buffer. Not necessary if no
    // synchronization to other media is required.
    if (ASIOGetSamplePosition(&timeInfo.timeInfo.samplePosition,
                              &timeInfo.timeInfo.systemTime) == ASE_OK)
        timeInfo.timeInfo.flags = kSystemTimeValid | kSamplePositionValid;

    bufferSwitchTimeInfo(&timeInfo, index, processNow);
}

void sampleRateChanged(ASIOSampleRate sRate) {
    // Do whatever you need to do if the sample rate changed.
    // Usually this only happens during external sync.
    // Audio processing is not stopped by the driver, actual sample rate
    // might not have even changed, maybe only the sample rate status of an
    // AES/EBU or S/PDIF digital input at the audio device.
    // You might have to update time/sample related conversion routines, etc.
}

long asioMessages(long selector, long value, void* message, double* opt) {
    dmsg(2,("asioMessages selector=%ld value=%ld\n", selector, value));
    // currently the parameters "value", "message" and "opt" are not used.
    long ret = 0;
    switch (selector)
    {
    case kAsioSelectorSupported:
        if (value == kAsioResetRequest ||
            value == kAsioEngineVersion ||
            value == kAsioResyncRequest ||
            value == kAsioLatenciesChanged ||
            // the following three were added for ASIO 2.0, we don't necessarily have to support them
            value == kAsioSupportsTimeInfo ||
            value == kAsioSupportsTimeCode ||
            value == kAsioSupportsInputMonitor)
            ret = 1L;
        break;
    case kAsioResetRequest:
        // Defer the task and perform the reset of the driver during the next "safe" situation.
        // You cannot reset the driver right now, as this code is called from the driver.
        // Reset the driver is done by completely destruct is. I.e. ASIOStop(),
        // ASIODisposeBuffers(), Destruction
        // Afterwards you initialize the driver again.

        //GlobalAudioOutputDeviceAsioThisPtr->asioDriverInfo.stopped; // In this sample the processing will just stop
        ret = 1L;
        break;
    case kAsioResyncRequest:
        // This informs the application, that the driver encountered some non fatal data loss.
        // It is used for synchronization purposes of different media.
        // Added mainly to work around the Win16Mutex problems in Windows 95/98 with the
        // Windows Multimedia system, which could loose data because the Mutex was hold too long
        // by another thread.
        // However a driver can issue it in other situations, too.
        ret = 1L;
        break;
    case kAsioLatenciesChanged:
        // This will inform the host application that the drivers were latencies changed.
        // Beware, it this does not mean that the buffer sizes have changed!
        // You might need to update internal delay data.
        ret = 1L;
        break;
    case kAsioEngineVersion:
        // Return the supported ASIO version of the host application.
        // If a host applications does not implement this selector, ASIO 1.0 is assumed
        // by the driver
        ret = 2L;
        break;
    case kAsioSupportsTimeInfo:
        // Informs the driver whether the asioCallbacks.bufferSwitchTimeInfo() callback
        // is supported.
        // For compatibility with ASIO 1.0 drivers the host application should always support
        // the "old" bufferSwitch method, too.
        ret = 1;
        break;
    case kAsioSupportsTimeCode:
        // Informs the driver whether application is interested in time code info.
        // If an application does not need to know about time code, the driver has less work
        // to do.
        ret = 0;
        break;
    }
    return ret;
}


// *************** ParameterCard ***************
// *

AudioOutputDeviceAsio::ParameterCard::ParameterCard() : DeviceCreationParameterString() {
    InitWithDefault(); // use default card
}

AudioOutputDeviceAsio::ParameterCard::ParameterCard(String s) throw (Exception) : DeviceCreationParameterString(s) {
}

String AudioOutputDeviceAsio::ParameterCard::Description() {
    return "Sound card to be used";
}

bool AudioOutputDeviceAsio::ParameterCard::Fix() {
    return true;
}

bool AudioOutputDeviceAsio::ParameterCard::Mandatory() {
    return false;
}

std::map<String,DeviceCreationParameter*> AudioOutputDeviceAsio::ParameterCard::DependsAsParameters() {
    return std::map<String,DeviceCreationParameter*>(); // no dependencies
}

optional<String> AudioOutputDeviceAsio::ParameterCard::DefaultAsString(std::map<String,String> Parameters) {
    std::vector<String> cards = PossibilitiesAsString(Parameters);
    if (cards.empty()) throw Exception(
        "AudioOutputDeviceAsio: Can't find any ASIO-capable sound card. Make "
        "sure you have an ASIO driver installed for your sound device. If you "
        "are using a consumer sound card that does not provide an ASIO driver "
        "to be installed, then consider installing an alternative like ASIO4ALL."
    );

    // If currentAsioDriverName is empty then return the first card,
    // otherwise return the currentAsioDriverName. This avoids closing
    // the current ASIO driver when the LSCP client calls commands
    // like GET AUDIO_OUTPUT_DRIVER_PARAMETER INFO ASIO CARD, which
    // would load the default card (without this check it would always
    // be the same which would cause the above problem).
    if (currentAsioDriverName == "") {
        dmsg(2,("AudioOutputDeviceAsio::ParameterCard::DefaultAsString='%s' (first card by default)\n", cards[0].c_str()));
        return cards[0]; // first card by default
    }
    else {
        dmsg(2,("AudioOutputDeviceAsio::ParameterCard::DefaultAsString='%s'\n", currentAsioDriverName.c_str()));
        return currentAsioDriverName;
    }

}

std::vector<String> AudioOutputDeviceAsio::ParameterCard::PossibilitiesAsString(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterCard::PossibilitiesAsString:\n"));
    return getAsioDriverNames();
}

void AudioOutputDeviceAsio::ParameterCard::OnSetValue(String s) throw (Exception) {
    // not posssible, as parameter is fix
}

String AudioOutputDeviceAsio::ParameterCard::Name() {
    return "CARD";
}



// *************** ParameterSampleRate ***************
// *

AudioOutputDeviceAsio::ParameterSampleRate::ParameterSampleRate() : AudioOutputDevice::ParameterSampleRate::ParameterSampleRate() {
}

AudioOutputDeviceAsio::ParameterSampleRate::ParameterSampleRate(String s) : AudioOutputDevice::ParameterSampleRate::ParameterSampleRate(s) {
}

std::map<String,DeviceCreationParameter*> AudioOutputDeviceAsio::ParameterSampleRate::DependsAsParameters() {
    static ParameterCard card;
    std::map<String,DeviceCreationParameter*> dependencies;
    dependencies[card.Name()] = &card;
    return dependencies;
}

optional<int> AudioOutputDeviceAsio::ParameterSampleRate::DefaultAsInt(std::map<String,String> Parameters) {
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::DefaultAsInt returning optional<int>::nothing (parameter CARD not supplied)\n"));
        return optional<int>::nothing;
    }

    // return the default samplerate. first try 44100 then 48000, then
    // the first samplerate found in the list
    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    for (uint i = 0; i < asioDriverInfo.sampleRates.size(); i++) {
        if (asioDriverInfo.sampleRates[i] == 44100) {
            dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::DefaultAsInt returning 44100\n"));
            return 44100;
        }
        if (asioDriverInfo.sampleRates[i] == 48000) {
            dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::DefaultAsInt returning 48000\n"));
            return 48000;
        }
    }
    dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::DefaultAsInt returning %d\n",
            asioDriverInfo.sampleRates[0]));
    return asioDriverInfo.sampleRates[0];
}

std::vector<int> AudioOutputDeviceAsio::ParameterSampleRate::PossibilitiesAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::PossibilitiesAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::PossibilitiesAsInt returning empty vector (parameter CARD not supplied)\n"));
        return std::vector<int>();
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return std::vector<int>();

    for (uint i = 0; i < asioDriverInfo.sampleRates.size(); i++) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterSampleRate::PossibilitiesAsInt samplerate[%d]=%d\n", i, asioDriverInfo.sampleRates[i]));
    }

    return asioDriverInfo.sampleRates;
}



// *************** ParameterChannels ***************
// *

AudioOutputDeviceAsio::ParameterChannels::ParameterChannels() : AudioOutputDevice::ParameterChannels::ParameterChannels() {
    //InitWithDefault();
    // could not determine default value? ...
    //if (ValueAsInt() == 0) SetValue(2); // ... then (try) a common value
}

AudioOutputDeviceAsio::ParameterChannels::ParameterChannels(String s) : AudioOutputDevice::ParameterChannels::ParameterChannels(s) {
}

std::map<String,DeviceCreationParameter*> AudioOutputDeviceAsio::ParameterChannels::DependsAsParameters() {
    static ParameterCard card;
    std::map<String,DeviceCreationParameter*> dependencies;
    dependencies[card.Name()] = &card;
    return dependencies;
}

optional<int> AudioOutputDeviceAsio::ParameterChannels::DefaultAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::DefaultAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::DefaultAsInt returning optional<int>::nothing (CARD parameter not supplied)\n"));
        return optional<int>::nothing;
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::DefaultAsInt returning %ld\n",
            asioDriverInfo.numOutputChannels));
    return asioDriverInfo.numOutputChannels;
}

optional<int> AudioOutputDeviceAsio::ParameterChannels::RangeMinAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMinAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMinAsInt returning optional<int>::nothing (CARD parameter not supplied)\n"));
        return optional<int>::nothing;
    }
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMinAsInt returning 1\n"));
    return 1;
}

optional<int> AudioOutputDeviceAsio::ParameterChannels::RangeMaxAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMaxAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMaxAsInt returning optional<int>::nothing (CARD parameter not supplied)\n"));
        return optional<int>::nothing;
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    dmsg(2,("AudioOutputDeviceAsio::ParameterChannels::RangeMaxAsInt returning %ld\n",
            asioDriverInfo.numOutputChannels));
    return asioDriverInfo.numOutputChannels;
}



// *************** ParameterFragmentSize ***************
// *

AudioOutputDeviceAsio::ParameterFragmentSize::ParameterFragmentSize() : DeviceCreationParameterInt() {
    InitWithDefault();
}

AudioOutputDeviceAsio::ParameterFragmentSize::ParameterFragmentSize(String s) throw (Exception) : DeviceCreationParameterInt(s) {
}

String AudioOutputDeviceAsio::ParameterFragmentSize::Description() {
    return "Size of each buffer fragment";
}

bool AudioOutputDeviceAsio::ParameterFragmentSize::Fix() {
    return true;
}

bool AudioOutputDeviceAsio::ParameterFragmentSize::Mandatory() {
    return false;
}

std::map<String,DeviceCreationParameter*> AudioOutputDeviceAsio::ParameterFragmentSize::DependsAsParameters() {
    static ParameterCard card;
    std::map<String,DeviceCreationParameter*> dependencies;
    dependencies[card.Name()] = &card;
    return dependencies;
}

optional<int> AudioOutputDeviceAsio::ParameterFragmentSize::DefaultAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::DefaultAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::DefaultAsInt returning optional<int>::nothing (no CARD parameter supplied\n"));
        return optional<int>::nothing;
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::DefaultAsInt returning %ld\n",
            asioDriverInfo.preferredBufSize));
    return asioDriverInfo.preferredBufSize;
}

optional<int> AudioOutputDeviceAsio::ParameterFragmentSize::RangeMinAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMinAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMinAsInt returning optional<int>::nothing (no CARD parameter supplied\n"));
        return optional<int>::nothing;
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMinAsInt returning %ld\n",
            asioDriverInfo.minBufSize));
    return asioDriverInfo.minBufSize;
}

optional<int> AudioOutputDeviceAsio::ParameterFragmentSize::RangeMaxAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMaxAsInt\n"));
    if (!Parameters.count("CARD")) {
        dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMaxAsInt returning optional<int>::nothing (no CARD parameter supplied\n"));
        return optional<int>::nothing;
    }

    ParameterCard card(Parameters["CARD"]);
    if (ASIO_OpenAndQueryDeviceInfo(card.ValueAsString(), &asioDriverInfo) != 0) return optional<int>::nothing;
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::RangeMaxAsInt returning %ld\n",
            asioDriverInfo.maxBufSize));
    return asioDriverInfo.maxBufSize;
}

std::vector<int> AudioOutputDeviceAsio::ParameterFragmentSize::PossibilitiesAsInt(std::map<String,String> Parameters) {
    dmsg(2,("AudioOutputDeviceAsio::ParameterFragmentSize::PossibilitiesAsInt\n"));
    return std::vector<int>();
}

void AudioOutputDeviceAsio::ParameterFragmentSize::OnSetValue(int i) throw (Exception) {
    // not posssible, as parameter is fix
}

String AudioOutputDeviceAsio::ParameterFragmentSize::Name() {
    return "FRAGMENTSIZE";
}



// *************** AudioOutputDeviceAsio ***************
// *

/**
 * Create and initialize Asio audio output device with given parameters.
 *
 * @param Parameters - optional parameters
 * @throws AudioOutputException  if output device cannot be opened
 */
AudioOutputDeviceAsio::AudioOutputDeviceAsio(std::map<String,DeviceCreationParameter*> Parameters) : AudioOutputDevice(Parameters) {
    if (AudioOutputDeviceAsioInstantiated) throw Exception("AudioOutputDeviceAsio: Sorry, only one ASIO card at time can be opened");
    AudioOutputDeviceAsioInstantiated = true;
    this->uiAsioChannels = ((DeviceCreationParameterInt*)Parameters["CHANNELS"])->ValueAsInt();
    this->uiSamplerate   = ((DeviceCreationParameterInt*)Parameters["SAMPLERATE"])->ValueAsInt();
    this->FragmentSize   = ((DeviceCreationParameterInt*)Parameters["FRAGMENTSIZE"])->ValueAsInt();
    String card          = ((DeviceCreationParameterString*)Parameters["CARD"])->ValueAsString();


    dmsg(2,("AudioOutputDeviceAsio::AudioOutputDeviceAsio constructor channels=%d samplerate=%d fragmentsize=%d card=%s\n",
            uiAsioChannels, uiSamplerate, FragmentSize, card.c_str()));

    asioIsPlaying = false;

    if (ASIO_OpenAndQueryDeviceInfo(card, &asioDriverInfo) != 0) {
        throw AudioOutputException("Error: ASIO Initialization error");
    }
    dmsg(2,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: after ASIO_OpenAndQueryDeviceInfo\n"));

    ASIOError asioError;
    if ((asioError = ASIOSetSampleRate(uiSamplerate)) != ASE_OK) {
        dmsg(1,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: ASIOSetSampleRate failed: %d\n", int(asioError)));
        throw AudioOutputException("Error: ASIOSetSampleRate: cannot set samplerate.");
    }
    dmsg(2,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: after ASIOSetSampleRate\n"));

    asioDriverInfo.ASIOOutputReadySupported = (ASIOOutputReady() == ASE_OK);
    dmsg(2,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: after ASIOOutputReady: %d\n",
             asioDriverInfo.ASIOOutputReadySupported));

    asioDriverInfo.numInputBuffers = 0;
    asioDriverInfo.numOutputBuffers = uiAsioChannels;
    asioDriverInfo.chosenBufSize = FragmentSize;

    for (uint i = 0; i < uiAsioChannels; i++) {
        asioDriverInfo.bufferInfos[i].isInput = ASIOFalse;
        asioDriverInfo.bufferInfos[i].channelNum = i;
    }

    asioCallbacks.bufferSwitch = &bufferSwitch;
    asioCallbacks.sampleRateDidChange = &sampleRateChanged;
    asioCallbacks.asioMessage = &asioMessages;
    asioCallbacks.bufferSwitchTimeInfo = &bufferSwitchTimeInfo;

    if ((asioError = ASIOCreateBuffers(asioDriverInfo.bufferInfos,
                                       asioDriverInfo.numOutputBuffers,
                                       asioDriverInfo.chosenBufSize,
                                       &asioCallbacks)) != ASE_OK) {
        dmsg(1,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: ASIOCreateBuffers failed: %d\n", int(asioError)));
        throw AudioOutputException("AudioOutputDeviceAsio: Error: ASIOCreateBuffers failed.");
    }
    dmsg(2,("AudioOutputDeviceAsio::AudioOutputDeviceAsio: after ASIOCreateBuffers\n"));

    // create audio channels for this audio device to which the sampler engines can write to
    for (uint i = 0; i < uiAsioChannels; i++) {
        Channels.push_back(new AudioChannel(i, FragmentSize));
    }

    // FIXME: temporary global variable used to store the this pointer
    // for the ASIO callbacks wanting to access the AudioOutputDeviceAsio
    // methods
    GlobalAudioOutputDeviceAsioThisPtr = this;

    if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
        Play();
    }
}

AudioOutputDeviceAsio::~AudioOutputDeviceAsio() {
    ASIOExit();
    asioDriverOpened = false;
    AudioOutputDeviceAsioInstantiated = false;
}

void AudioOutputDeviceAsio::Play() {
    dmsg(2,("AudioOutputDeviceAsio::Play()\n"));
    ASIOError asioError;
    if ((asioError = ASIOStart()) != ASE_OK) {
        asioIsPlaying = false;
        dmsg(1,("AudioOutputDeviceAsio::Play: ASIOStart failed: %d\n", int(asioError)));
        throw AudioOutputException("AudioOutputDeviceAsio: Error: ASIOStart failed");
    }
    else asioIsPlaying = true;
}

bool AudioOutputDeviceAsio::IsPlaying() {
    return asioIsPlaying;
}

void AudioOutputDeviceAsio::Stop() {
    dmsg(2,("AudioOutputDeviceAsio::Stop()\n"));
    ASIOStop();
    asioIsPlaying = false;
}

AudioChannel* AudioOutputDeviceAsio::CreateChannel(uint ChannelNr) {
    dmsg(2,("AudioOutputDeviceAsio::CreateChannel value=%d uiAsioChannels=%d\n",
            ChannelNr, uiAsioChannels));
    // just create a mix channel
    return new AudioChannel(ChannelNr, Channel(ChannelNr % uiAsioChannels));
}

uint AudioOutputDeviceAsio::MaxSamplesPerCycle() {
    dmsg(2,("AudioOutputDeviceAsio::MaxSamplesPerCycle value=%d\n", FragmentSize));
    return FragmentSize;
}

uint AudioOutputDeviceAsio::SampleRate() {
    dmsg(2,("AudioOutputDeviceAsio::SampleRate value=%d\n", uiSamplerate)); fflush(stdout);
    return uiSamplerate;
}

String AudioOutputDeviceAsio::Name() {
    return "ASIO";
}

String AudioOutputDeviceAsio::Driver() {
    return Name();
}

String AudioOutputDeviceAsio::Description() {
    return "Audio Streaming Input Output 2.2";
}

String AudioOutputDeviceAsio::Version() {
    String s = "$Revision: 2509 $";
    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
}

} // namespace LinuxSampler
