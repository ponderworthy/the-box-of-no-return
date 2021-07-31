/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
 *   Copyright (C) 2011-2013 Andreas Persson                               *
 *   Copyright (C) 2014-2017 Christian Schoenebeck                         *
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

#include "AudioOutputDeviceCoreAudio.h"

#include "../../common/global_private.h"

namespace LinuxSampler {

    void AudioOutputDeviceCoreAudio::AudioQueueListener (
            void *inUserData, AudioQueueRef inAQ, AudioQueuePropertyID inID
    ) {
        switch(inID) {
            case kAudioQueueProperty_IsRunning:
                
                break;
            case kAudioQueueProperty_CurrentDevice:
                
                break;
        }
    }

    void AudioOutputDeviceCoreAudio::HandleOutputBuffer (
        void                 *aqData,
        AudioQueueRef        inAQ,
        AudioQueueBufferRef  inBuffer
    ) {
        AQPlayerState* pAqData = (AQPlayerState*) aqData;
        if (atomic_read(&(pAqData->mIsRunning)) == 0) {
            AudioQueueFlush(pAqData->mQueue);
            AudioQueueStop (pAqData->mQueue, true);
            return;
        }

        if(atomic_read(&(pAqData->pDevice->restartQueue))) return;

        uint bufferSize = pAqData->pDevice->uiBufferSize;

        // let all connected engines render 'fragmentSize' sample points
        pAqData->pDevice->RenderAudio(bufferSize);

        Float32* pDataBuf = (Float32*)(inBuffer->mAudioData);

        uint uiCoreAudioChannels = pAqData->pDevice->uiCoreAudioChannels;
        for (int c = 0; c < uiCoreAudioChannels; c++) {
            float* in  = pAqData->pDevice->Channels[c]->Buffer();
            for (int i = 0, o = c; i < bufferSize; i++ , o += uiCoreAudioChannels) {
                pDataBuf[o] = in[i];
            }
        }

        inBuffer->mAudioDataByteSize = (uiCoreAudioChannels * 4) * bufferSize;

        OSStatus res = AudioQueueEnqueueBuffer(pAqData->mQueue, inBuffer, 0, NULL);
        if(res) std::cerr << "AudioQueueEnqueueBuffer: Error " << res << std::endl;
    }

    void AudioOutputDeviceCoreAudio::DeviceChanged() {
        dmsg(1,("Restarting audio queue..."));
        atomic_set(&restartQueue, 1);
    }

    AudioOutputDeviceCoreAudio::AudioOutputDeviceCoreAudio (
                    std::map<String,DeviceCreationParameter*> Parameters
    ) : AudioOutputDevice(Parameters), Thread(true, true, 1, 0), CurrentDevice(0) {

        dmsg(2,("AudioOutputDeviceCoreAudio::AudioOutputDeviceCoreAudio()\n"));
        if(CAAudioDeviceListModel::GetModel()->GetOutputDeviceCount() < 1) {
            throw Exception("No audio output device found");
        }
        atomic_set(&pausedNew, 0);
        pausedOld = 0;
        atomic_set(&restartQueue, 0);

        uiCoreAudioChannels = ((DeviceCreationParameterInt*)Parameters["CHANNELS"])->ValueAsInt();
        uint samplerate     = ((DeviceCreationParameterInt*)Parameters["SAMPLERATE"])->ValueAsInt();
        uiBufferNumber      = ((DeviceCreationParameterInt*)Parameters["BUFFERS"])->ValueAsInt();
        uiBufferSize        = ((DeviceCreationParameterInt*)Parameters["BUFFERSIZE"])->ValueAsInt();
        int device = 0;
        try { device = ((ParameterDevice*)Parameters["DEVICE"])->GetDeviceIndex(); }
        catch(Exception x) { }

        CurrentDevice = CAAudioDeviceListModel::GetModel()->GetOutputDevice(device);
        CurrentDevice.AddListener(this);

        memset (&aqPlayerState.mDataFormat, 0, sizeof(AudioStreamBasicDescription));

        aqPlayerState.mDataFormat.mSampleRate = samplerate;
        aqPlayerState.mDataFormat.mFormatID = kAudioFormatLinearPCM;

        aqPlayerState.mDataFormat.mFormatFlags =
            kLinearPCMFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;

        int samplesize = 4;
        aqPlayerState.mDataFormat.mBytesPerPacket = uiCoreAudioChannels * samplesize;
        aqPlayerState.mDataFormat.mFramesPerPacket = 1;
        aqPlayerState.mDataFormat.mBytesPerFrame = uiCoreAudioChannels * samplesize;
        aqPlayerState.mDataFormat.mChannelsPerFrame = uiCoreAudioChannels;
        aqPlayerState.mDataFormat.mBitsPerChannel = 8 * samplesize;
        aqPlayerState.mDataFormat.mReserved = 0;

        aqPlayerState.mBuffers = new AudioQueueBufferRef[uiBufferNumber];

        aqPlayerState.bufferByteSize =
                MaxSamplesPerCycle() * aqPlayerState.mDataFormat.mBytesPerFrame;

        aqPlayerState.mNumPacketsToRead = MaxSamplesPerCycle();

        aqPlayerState.pDevice = this;
        aqPlayerState.mQueue = NULL;

        uint fragmentSize = MaxSamplesPerCycle();
        // create audio channels for this audio device to which the sampler engines can write to
        for (int i = 0; i < uiCoreAudioChannels; i++) {
            this->Channels.push_back(new AudioChannel(i, fragmentSize));
        }

        StartThread();

        if (!((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
		Stop();
	}
    }

    AudioOutputDeviceCoreAudio::~AudioOutputDeviceCoreAudio() {
        StopThread();

        atomic_set(&(aqPlayerState.mIsRunning), 0);
        {
            LockGuard lock(destroyMutex);
            AudioQueueDispose(aqPlayerState.mQueue, true);
        }
        delete [] aqPlayerState.mBuffers;

        CurrentDevice.RemoveListener(this);
    }

    String AudioOutputDeviceCoreAudio::Name() {
        return "COREAUDIO";
    }

    String AudioOutputDeviceCoreAudio::Description() {
        return "Apple CoreAudio";
    }

    String AudioOutputDeviceCoreAudio::Version() {
       String s = "$Revision: 3290 $";
       return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    void AudioOutputDeviceCoreAudio::Play() {
        atomic_set(&pausedNew, 0);
    }

    bool AudioOutputDeviceCoreAudio::IsPlaying() {
        return !atomic_read(&pausedNew);
    }

    void AudioOutputDeviceCoreAudio::Stop() {
        atomic_set(&pausedNew, 1);
    }

    uint AudioOutputDeviceCoreAudio::MaxSamplesPerCycle() {
        return uiBufferSize;
    }

    uint AudioOutputDeviceCoreAudio::SampleRate() {
        return aqPlayerState.mDataFormat.mSampleRate;
    }

    String AudioOutputDeviceCoreAudio::Driver() {
        return Name();
    }

    AudioChannel* AudioOutputDeviceCoreAudio::CreateChannel(uint ChannelNr) {
        // just create a mix channel
        return new AudioChannel(ChannelNr, Channel(ChannelNr % uiCoreAudioChannels));
    }

    void AudioOutputDeviceCoreAudio::FillBuffers() {
        for (int i = 0; i < uiBufferNumber; ++i) {
            HandleOutputBuffer (
                &aqPlayerState,
                aqPlayerState.mQueue,
                aqPlayerState.mBuffers[i]
            );
        }
    }

    void AudioOutputDeviceCoreAudio::PrimeAudioQueue() {
        OSStatus res = AudioQueuePrime(aqPlayerState.mQueue, 0, NULL);
        if(res) {
            String s = String("AudioQueuePrime: Error ") + ToString(res);
            throw Exception(s);
        }
    }

    void AudioOutputDeviceCoreAudio::CreateAndStartAudioQueue() throw(Exception) {
        OSStatus res = AudioQueueNewOutput (
            &aqPlayerState.mDataFormat,
            HandleOutputBuffer,
            &aqPlayerState,
            CFRunLoopGetCurrent(),
            kCFRunLoopCommonModes,
            0,
            &aqPlayerState.mQueue
        );

        if(res) {
            String s = String("AudioQueueNewOutput: Error ") + ToString(res);
            throw Exception(s);
        }

        CFStringRef devUID = CFStringCreateWithCString (
            NULL, CurrentDevice.GetUID().c_str(), kCFStringEncodingASCII
        );
        res = AudioQueueSetProperty (
            aqPlayerState.mQueue,
            kAudioQueueProperty_CurrentDevice,
            &devUID, sizeof(CFStringRef)
        );
        CFRelease(devUID);

        if(res) {
            String s = String("Failed to set audio device: ") + ToString(res);
            throw Exception(s);
        }

        for (int i = 0; i < uiBufferNumber; ++i) {
            res = AudioQueueAllocateBuffer (
                aqPlayerState.mQueue,
                aqPlayerState.bufferByteSize,
                &aqPlayerState.mBuffers[i]
            );

            if(res) {
                String s = String("AudioQueueAllocateBuffer: Error ");
                throw Exception(s + ToString(res));
            }
        }

        res = AudioQueueAddPropertyListener (
            aqPlayerState.mQueue,
            kAudioQueueProperty_CurrentDevice,
            AudioQueueListener,
            NULL
        );
        if(res) std::cerr << "Failed to register device change listener: " << res << std::endl;

        res = AudioQueueAddPropertyListener (
            aqPlayerState.mQueue,
            kAudioQueueProperty_IsRunning,
            AudioQueueListener,
            NULL
        );
        if(res) std::cerr << "Failed to register running listener: " << res << std::endl;

        Float32 gain = 1.0;

        res = AudioQueueSetParameter (
            aqPlayerState.mQueue,
            kAudioQueueParam_Volume,
            gain
        );

        if(res) std::cerr << "AudioQueueSetParameter: Error " << res << std::endl;

        atomic_set(&(aqPlayerState.mIsRunning), 1);
        FillBuffers();
        PrimeAudioQueue();

        res = AudioQueueStart(aqPlayerState.mQueue, NULL);
        if(res) {
            String s = String("AudioQueueStart: Error ") + ToString(res);
            throw Exception(s);
        }
    }

    void AudioOutputDeviceCoreAudio::DestroyAudioQueue() {
        AudioQueueFlush(aqPlayerState.mQueue);
        AudioQueueStop (aqPlayerState.mQueue, true);
        AudioQueueDispose(aqPlayerState.mQueue, true);
        aqPlayerState.mQueue = NULL;
    }

    /**
     * Entry point for the thread.
     */
    int AudioOutputDeviceCoreAudio::Main() {
        dmsg(1,("CoreAudio thread started\n"));
        OSStatus res;
        if(aqPlayerState.mQueue == NULL) {
            /*
             * Need to be run from this thread because of CFRunLoopGetCurrent()
             * which returns the CFRunLoop object for the current thread.
             */
            try { CreateAndStartAudioQueue(); }
            catch(Exception e) {
                std::cerr << "Failed to star audio queue: " + e.Message() << std::endl;
                return 0;
            }
        }

        {
            LockGuard lock(destroyMutex);
            do {
                if(atomic_read(&pausedNew) != pausedOld) {
                    pausedOld = atomic_read(&pausedNew);

                    if(pausedOld) {
                        res = AudioQueuePause(aqPlayerState.mQueue);
                        if(res) std::cerr << "AudioQueuePause: Error " << res << std::endl;
                    } else {
                        res = AudioQueuePrime(aqPlayerState.mQueue, 0, NULL);
                        if(res) std::cerr << "AudioQueuePrime: Error " << res << std::endl;
                        res = AudioQueueStart(aqPlayerState.mQueue, NULL);
                        if(res) std::cerr << "AudioQueueStart: Error " << res << std::endl;
                    }
                }

                if(atomic_read(&restartQueue)) {
                    DestroyAudioQueue();
                    CreateAndStartAudioQueue();
                    atomic_set(&restartQueue, 0);
                    dmsg(1,("Audio queue restarted"));
                }
            
                CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.2, false);
            } while (atomic_read(&(aqPlayerState.mIsRunning)));
        }

        dmsg(2,("CoreAudio thread stopped\n"));

        pthread_exit(NULL);
        return 0;
    }


// *************** ParameterDevice ***************
// *

    AudioOutputDeviceCoreAudio::ParameterDevice::ParameterDevice() : DeviceCreationParameterString() {
        InitWithDefault(); // use default device
    }

    AudioOutputDeviceCoreAudio::ParameterDevice::ParameterDevice(String s)
            throw (Exception) : DeviceCreationParameterString(s) {
    }

    String AudioOutputDeviceCoreAudio::ParameterDevice::Description() {
        return "Output device to be used";
    }

    bool AudioOutputDeviceCoreAudio::ParameterDevice::Fix() {
        return true;
    }

    bool AudioOutputDeviceCoreAudio::ParameterDevice::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*>
    AudioOutputDeviceCoreAudio::ParameterDevice::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    optional<String>
    AudioOutputDeviceCoreAudio::ParameterDevice::DefaultAsString(std::map<String,String> Parameters) {
        CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
        if (devs->GetOutputDeviceCount() < 1) {
            throw Exception("AudioOutputDeviceCoreAudio: Can't find any output device");
        }
        UInt32 idx = devs->GetOutputDeviceIndex(devs->GetDefaultOutputDeviceID());
        return CreateDeviceName(idx);
    }

    std::vector<String>
    AudioOutputDeviceCoreAudio::ParameterDevice::PossibilitiesAsString(std::map<String,String> Parameters) {
        std::vector<String> deviceNames;

        CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
        for(int i = 0; i < devs->GetOutputDeviceCount(); i++) {
            if(devs->GetOutputDevice(i).GetChannelNumber() < 1) continue;

            deviceNames.push_back(CreateDeviceName(i));
        }

        return deviceNames;
    }

    String AudioOutputDeviceCoreAudio::ParameterDevice::CreateDeviceName(int devIndex) {
        CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
        // Note that the space " " is used as delimiter to obtain the
        // device index. See GetDeviceIndex()
        return ToString(devIndex + 1) + " " + devs->GetOutputDevice(devIndex).GetName();
    }

    void AudioOutputDeviceCoreAudio::ParameterDevice::OnSetValue(String s) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceCoreAudio::ParameterDevice::Name() {
        return "DEVICE";
    }

    int AudioOutputDeviceCoreAudio::ParameterDevice::GetDeviceIndex() {
        String s = ValueAsString();
        if(s.empty()) return -1;
        int n = (int) s.find(' ');
        s = s.substr(0, n);
        return ToInt(s) - 1;
    }

// *************** ParameterSampleRate ***************
// *

    AudioOutputDeviceCoreAudio::ParameterSampleRate::ParameterSampleRate() :
                AudioOutputDevice::ParameterSampleRate::ParameterSampleRate() {

    }

    AudioOutputDeviceCoreAudio::ParameterSampleRate::ParameterSampleRate(String s) :
                AudioOutputDevice::ParameterSampleRate::ParameterSampleRate(s) {
        
    }

    std::map<String,DeviceCreationParameter*>
    AudioOutputDeviceCoreAudio::ParameterSampleRate::DependsAsParameters() {
        static ParameterDevice device;
        std::map<String,DeviceCreationParameter*> dependencies;
        dependencies[device.Name()] = &device;
        return dependencies;
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterSampleRate::DefaultAsInt(std::map<String,String> Parameters) {
        dmsg(2,("AudioOutputDeviceCoreAudio::ParameterSampleRate::DefaultAsInt()\n"));
        ParameterDevice dev(Parameters["DEVICE"]);
        int samplerate = 44100;

        try {
            int idx = dev.GetDeviceIndex();

            CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
            samplerate = devs->GetOutputDevice(idx).GetDefaultSamplerate();
        } catch(Exception e) { }

        return samplerate;
    }

    std::vector<int>
    AudioOutputDeviceCoreAudio::ParameterSampleRate::PossibilitiesAsInt(std::map<String,String> Parameters) {
        ParameterDevice dev(Parameters["DEVICE"]);
        std::vector<int> srates;

        try {
            int idx = dev.GetDeviceIndex();
            CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
            srates = devs->GetOutputDevice(idx).GetNominalSamplerates();
        } catch(Exception x) { }

        return srates;
    }


// *************** ParameterChannels ***************
// *

    AudioOutputDeviceCoreAudio::ParameterChannels::ParameterChannels() :
                    AudioOutputDevice::ParameterChannels::ParameterChannels() {
        
    }

    AudioOutputDeviceCoreAudio::ParameterChannels::ParameterChannels(String s) :
                    AudioOutputDevice::ParameterChannels::ParameterChannels(s) {
        
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterChannels::DefaultAsInt(std::map<String,String> Parameters) {
        dmsg(2,("AudioOutputDeviceCoreAudio::ParameterChannels::DefaultAsInt()\n"));
        ParameterDevice dev(Parameters["DEVICE"]);
        int chns = 2;

        try {
            int idx = dev.GetDeviceIndex();
            CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
            chns = devs->GetOutputDevice(idx).GetChannelNumber();
        } catch(Exception e) { }

        return chns;
    }

    std::vector<int>
    AudioOutputDeviceCoreAudio::ParameterChannels::PossibilitiesAsInt(std::map<String,String> Parameters) {
        ParameterDevice dev(Parameters["DEVICE"]);
        std::vector<int> chns;

        try {
            int idx = dev.GetDeviceIndex();
            CAAudioDeviceListModel* devs = CAAudioDeviceListModel::GetModel();
            for(int i = 1; i <= devs->GetOutputDevice(idx).GetChannelNumber(); i++) {
                chns.push_back(i);
            }
        } catch(Exception x) { }

        return chns;
    }

// *************** ParameterBuffers ***************
// *

    AudioOutputDeviceCoreAudio::ParameterBuffers::ParameterBuffers() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDeviceCoreAudio::ParameterBuffers::ParameterBuffers(String s)
            throw (Exception) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDeviceCoreAudio::ParameterBuffers::Description() {
        return "Number of audio buffer";
    }

    bool AudioOutputDeviceCoreAudio::ParameterBuffers::Fix() {
        return true;
    }

    bool AudioOutputDeviceCoreAudio::ParameterBuffers::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*>
    AudioOutputDeviceCoreAudio::ParameterBuffers::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBuffers::DefaultAsInt(std::map<String,String> Parameters) {
        return 3;
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBuffers::RangeMinAsInt(std::map<String,String> Parameters) {
        return 2;
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBuffers::RangeMaxAsInt(std::map<String,String> Parameters) {
        return 16;
    }

    std::vector<int>
    AudioOutputDeviceCoreAudio::ParameterBuffers::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDeviceCoreAudio::ParameterBuffers::OnSetValue(int i) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceCoreAudio::ParameterBuffers::Name() {
        return "BUFFERS";
    }

// *************** ParameterBufferSize ***************
// *

    AudioOutputDeviceCoreAudio::ParameterBufferSize::ParameterBufferSize() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDeviceCoreAudio::ParameterBufferSize::ParameterBufferSize(String s)
            throw (Exception) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDeviceCoreAudio::ParameterBufferSize::Description() {
        return "Size of the audio buffer in sample frames";
    }

    bool AudioOutputDeviceCoreAudio::ParameterBufferSize::Fix() {
        return true;
    }

    bool AudioOutputDeviceCoreAudio::ParameterBufferSize::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*>
    AudioOutputDeviceCoreAudio::ParameterBufferSize::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBufferSize::DefaultAsInt(std::map<String,String> Parameters) {
        return 256;
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBufferSize::RangeMinAsInt(std::map<String,String> Parameters) {
        return 32;
    }

    optional<int>
    AudioOutputDeviceCoreAudio::ParameterBufferSize::RangeMaxAsInt(std::map<String,String> Parameters) {
        return 4096;
    }

    std::vector<int>
    AudioOutputDeviceCoreAudio::ParameterBufferSize::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDeviceCoreAudio::ParameterBufferSize::OnSetValue(int i) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDeviceCoreAudio::ParameterBufferSize::Name() {
        return "BUFFERSIZE";
    }
} // namespace LinuxSampler
