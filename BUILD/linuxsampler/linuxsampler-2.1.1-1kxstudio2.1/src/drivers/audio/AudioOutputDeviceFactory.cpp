/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2009 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "AudioOutputDeviceFactory.h"

#include "../../common/global_private.h"

#if HAVE_ALSA
# include "AudioOutputDeviceAlsa.h"
#endif // HAVE_ALSA

#if HAVE_JACK
# include "AudioOutputDeviceJack.h"
#endif // HAVE_JACK

#if HAVE_ARTS
# include "AudioOutputDeviceArts.h"
#endif // HAVE_ARTS

#if HAVE_ASIO
# include "AudioOutputDeviceAsio.h"
#endif // HAVE_ASIO

#if HAVE_COREAUDIO
# include "AudioOutputDeviceCoreAudio.h"
#endif // HAVE_COREAUDIO

namespace LinuxSampler {

    std::map<String, AudioOutputDeviceFactory::InnerFactory*> AudioOutputDeviceFactory::InnerFactories;
    std::map<String, DeviceParameterFactory*> AudioOutputDeviceFactory::ParameterFactories;

#if HAVE_ALSA
    REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDeviceAlsa);
    /* Common parameters for now they'll have to be registered here. */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterActive);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterSampleRate);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterChannels);
    /* Driver specific parameters */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterCard);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterFragments);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAlsa, ParameterFragmentSize);
#endif // HAVE_ALSA

#if HAVE_JACK
    REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDeviceJack);
    /* Common parameters for now they'll have to be registered here. */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceJack, ParameterActive);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceJack, ParameterSampleRate);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceJack, ParameterChannels);
    /* Driver specific parameters */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceJack, ParameterName);
#endif // HAVE_JACK

#if HAVE_ARTS
    REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDeviceArts);
    /* Common parameters for now they'll have to be registered here. */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceArts, ParameterActive);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceArts, ParameterSampleRate);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceArts, ParameterChannels);
    /* Driver specific parameters */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceArts, ParameterName);
#endif // HAVE_ARTS

#if HAVE_ASIO
    REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDeviceAsio);
    /* Common parameters for now they'll have to be registered here. */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAsio, ParameterActive);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAsio, ParameterSampleRate);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAsio, ParameterChannels);
    /* Driver specific parameters */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAsio, ParameterCard);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceAsio, ParameterFragmentSize);
#endif // HAVE_ASIO

#if HAVE_COREAUDIO
    REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDeviceCoreAudio);
    /* Common parameters for now they'll have to be registered here. */
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterActive);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterSampleRate);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterChannels);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterDevice);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterBuffers);
    REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDeviceCoreAudio, ParameterBufferSize);
#endif // HAVE_COREAUDIO

    AudioOutputDeviceFactory::AudioOutputDeviceMap AudioOutputDeviceFactory::mAudioOutputDevices;

    /**
     * Creates a new audio output device for the given driver name and
     * parameters. Note, this method will also check whether it is actually
     * allowed to create an instance of the given driver on its own and throw
     * an Exception in case it is not allowed (this is the case for host plugin
     * drivers like VST, AU, DSSI, LV2).
     *
     * @param DriverName - name of the driver of which a new device shall be created
     * @param Parameters - device creation parameters which shall be passed to the driver's constructor
     *
     * @returns pointer to the new device instance
     * @throws Exception - in case the device could not be created
     *
     * @see CreatePrivate()
     */
    AudioOutputDevice* AudioOutputDeviceFactory::Create(String DriverName, std::map<String,String> Parameters) throw (Exception) {
        if (!InnerFactories.count(DriverName))
            throw Exception("There is no audio output driver '" + DriverName + "'.");
        if (!InnerFactories[DriverName]->isAutonomousDriver())
            throw Exception("You cannot directly create a new audio output device of the '" + DriverName + "' driver!");

        return CreatePrivate(DriverName, Parameters);
    }

    /**
     * Same as Create(), but this method won't check whether it is allowed to
     * create an instance of the given driver on its own. This method is
     * usually called by host plugins (e.g. VST, AU, DSSI, LV2) to actually
     * create their respective audio output devices for the sampler. Usually
     * one shouldn't call this method directly, but call Create() instead.
     */
    AudioOutputDevice* AudioOutputDeviceFactory::CreatePrivate(String DriverName, std::map<String,String> Parameters) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no audio output driver '" + DriverName + "'.");
        // let's see if we need to create parameters
        std::map<String,DeviceCreationParameter*> thisDeviceParams;
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            thisDeviceParams = pParamFactory->CreateAllParams(Parameters);
        } else {
            // no parameters are registered by the driver. Throw if any parameters were specified.
            if (Parameters.size() != 0) throw Exception("Driver '" + DriverName + "' does not have any parameters.");
        }

        // get a free device id
        int iDeviceId = -1;
        for (int i = 0; i >= 0; i++) { // seek for a free place starting from the beginning
            if (!mAudioOutputDevices[i]) {
                iDeviceId = i;
                mAudioOutputDevices.erase(i);
                break;
            }
        }
        if (iDeviceId < 0)
            throw Exception("Could not retrieve free device ID!");

        // now create the device using those parameters
        AudioOutputDevice* pDevice = InnerFactories[DriverName]->Create(thisDeviceParams);
        pDevice->setDeviceId(iDeviceId);
        // now attach all parameters to the newely created device.
        for (std::map<String,DeviceCreationParameter*>::iterator iter = thisDeviceParams.begin(); iter != thisDeviceParams.end(); iter++) {
            iter->second->Attach(pDevice);
        }

        // add new audio device to the audio device list
        mAudioOutputDevices[iDeviceId] = pDevice;

        return pDevice;
    }

    std::vector<String> AudioOutputDeviceFactory::AvailableDrivers() {
        std::vector<String> result;
        std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin();
        while (iter != InnerFactories.end()) {
            result.push_back(iter->first);
            iter++;
        }
        return result;
    }

    String AudioOutputDeviceFactory::AvailableDriversAsString() {
        std::vector<String> drivers = AvailableDrivers();
        String result;
        std::vector<String>::iterator iter = drivers.begin();
        for (; iter != drivers.end(); iter++) {
            if (result != "") result += ",";
            result += *iter;
        }
        return result;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceFactory::GetAvailableDriverParameters(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no audio output driver '" + DriverName + "'.");
        std::map<String,DeviceCreationParameter*> thisDeviceParams;
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            thisDeviceParams = pParamFactory->CreateAllParams();
        }
        return thisDeviceParams;
    }

    DeviceCreationParameter* AudioOutputDeviceFactory::GetDriverParameter(String DriverName, String ParameterName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no audio output driver '" + DriverName + "'.");
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            try { return pParamFactory->Create(ParameterName); }
            catch(Exception e) { }
        }
        throw Exception("Audio output driver '" + DriverName + "' does not have a parameter '" + ParameterName + "'.");
    }

    String AudioOutputDeviceFactory::GetDriverDescription(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no audio output driver '" + DriverName + "'.");
        return InnerFactories[DriverName]->Description();
    }

    String AudioOutputDeviceFactory::GetDriverVersion(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no audio output driver '" + DriverName + "'.");
        return InnerFactories[DriverName]->Version();
    }

    void AudioOutputDeviceFactory::Unregister(String DriverName) {
        std::map<String, InnerFactory*>::iterator iter = InnerFactories.find(DriverName);
        if (iter != InnerFactories.end()) {
            delete iter->second;
            InnerFactories.erase(iter);
        }

        std::map<String, DeviceParameterFactory*>::iterator iterpf = ParameterFactories.find(DriverName);
        if (iterpf != ParameterFactories.end()) {
            delete iterpf->second;
            ParameterFactories.erase(iterpf);
        }
    }

    std::map<uint, AudioOutputDevice*> AudioOutputDeviceFactory::Devices() {
        return mAudioOutputDevices;
    }

    /**
     * Destroys the given device. Usually this method shouldn't be called
     * directly, Sampler::DestroyAudioOutputDevice should be called instead,
     * since it also takes care whether some sampler channel is still using
     * the device, etc.
     */
    void AudioOutputDeviceFactory::Destroy(AudioOutputDevice* pDevice) throw (Exception) {
        if (pDevice && !pDevice->isAutonomousDevice())
            throw Exception("You cannot directly destroy this '" + pDevice->Driver() + "' device!");

        DestroyPrivate(pDevice);
    }

    void AudioOutputDeviceFactory::DestroyPrivate(AudioOutputDevice* pDevice) throw (Exception) {
        AudioOutputDeviceMap::iterator iter = mAudioOutputDevices.begin();
        for (; iter != mAudioOutputDevices.end(); iter++) {
            if (iter->second == pDevice) {
                // disable device
                pDevice->Stop();
                // remove device from the device list
                mAudioOutputDevices.erase(iter);
                // destroy and free device from memory
                delete pDevice;

                break;
            }
        }
    }

} // namespace LinuxSampler
