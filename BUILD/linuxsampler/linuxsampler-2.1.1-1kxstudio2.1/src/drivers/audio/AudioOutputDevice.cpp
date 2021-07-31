/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#include "AudioOutputDeviceFactory.h"
#include "AudioOutputDevice.h"
#include "../../common/global_private.h"
#include "../../common/IDGenerator.h"

namespace LinuxSampler {

// *************** ParameterActive ***************
// *

    AudioOutputDevice::ParameterActive::ParameterActive() : DeviceCreationParameterBool() {
        InitWithDefault();
    }

    AudioOutputDevice::ParameterActive::ParameterActive(String s) : DeviceCreationParameterBool(s) {
    }

    String AudioOutputDevice::ParameterActive::Description() {
        return "Enable / disable device";
    }

    bool AudioOutputDevice::ParameterActive::Fix() {
        return false;
    }

    bool AudioOutputDevice::ParameterActive::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDevice::ParameterActive::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<bool> AudioOutputDevice::ParameterActive::DefaultAsBool(std::map<String,String> Parameters) {
        return true;
    }

    void AudioOutputDevice::ParameterActive::OnSetValue(bool b) throw (Exception) {
        if (b) ((AudioOutputDevice*)pDevice)->Play();
        else ((AudioOutputDevice*)pDevice)->Stop();
    }

    String AudioOutputDevice::ParameterActive::Name() {
        return "ACTIVE";
    }



// *************** ParameterSampleRate ***************
// *

    AudioOutputDevice::ParameterSampleRate::ParameterSampleRate() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDevice::ParameterSampleRate::ParameterSampleRate(String s) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDevice::ParameterSampleRate::Description() {
        return "Output sample rate";
    }

    bool AudioOutputDevice::ParameterSampleRate::Fix() {
        return true;
    }

    bool AudioOutputDevice::ParameterSampleRate::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDevice::ParameterSampleRate::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<int> AudioOutputDevice::ParameterSampleRate::DefaultAsInt(std::map<String,String> Parameters) {
        return 44100;
    }

    optional<int> AudioOutputDevice::ParameterSampleRate::RangeMinAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    optional<int> AudioOutputDevice::ParameterSampleRate::RangeMaxAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    std::vector<int> AudioOutputDevice::ParameterSampleRate::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    int AudioOutputDevice::ParameterSampleRate::ValueAsInt() {
        return (pDevice) ? (int) ((AudioOutputDevice*)pDevice)->SampleRate()
                         : DeviceCreationParameterInt::ValueAsInt();
    }

    void AudioOutputDevice::ParameterSampleRate::OnSetValue(int i) throw (Exception) {
        /* cannot happen, as parameter is fix */
    }

    String AudioOutputDevice::ParameterSampleRate::Name() {
        return "SAMPLERATE";
    }



// *************** ParameterChannels ***************
// *

    AudioOutputDevice::ParameterChannels::ParameterChannels() : DeviceCreationParameterInt() {
       InitWithDefault();
    }

    AudioOutputDevice::ParameterChannels::ParameterChannels(String s) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDevice::ParameterChannels::Description() {
        return "Number of output channels";
    }

    bool AudioOutputDevice::ParameterChannels::Fix() {
        return false;
    }

    bool AudioOutputDevice::ParameterChannels::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDevice::ParameterChannels::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<int> AudioOutputDevice::ParameterChannels::DefaultAsInt(std::map<String,String> Parameters) {
        return 2;
    }

    optional<int> AudioOutputDevice::ParameterChannels::RangeMinAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    optional<int> AudioOutputDevice::ParameterChannels::RangeMaxAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    std::vector<int> AudioOutputDevice::ParameterChannels::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDevice::ParameterChannels::OnSetValue(int i) throw (Exception) {
        ((AudioOutputDevice*)pDevice)->AcquireChannels(i);
    }

    String AudioOutputDevice::ParameterChannels::Name() {
        return "CHANNELS";
    }



// *************** AudioOutputDevice ***************
// *

    AudioOutputDevice::AudioOutputDevice(std::map<String,DeviceCreationParameter*> DriverParameters)
        : EnginesReader(Engines) {
        this->Parameters = DriverParameters;
        EffectChainIDs = new IDGenerator();
    }

    AudioOutputDevice::~AudioOutputDevice() {
        // delete all audio channels
        {
            std::vector<AudioChannel*>::iterator iter = Channels.begin();
            while (iter != Channels.end()) {
                delete *iter;
                iter++;
            }
            Channels.clear();
        }

        // delete all device parameters
        {
            std::map<String,DeviceCreationParameter*>::iterator iter = Parameters.begin();
            while (iter != Parameters.end()) {
                delete iter->second;
                iter++;
            }
            Parameters.clear();
        }

        // delete all master effect chains
        {
            std::vector<EffectChain*>::iterator iter = vEffectChains.begin();
            while (iter != vEffectChains.end()) {
                delete *iter;
                iter++;
            }
            vEffectChains.clear();
        }
        
        delete EffectChainIDs;
    }

    void AudioOutputDevice::Connect(Engine* pEngine) {
        std::set<Engine*>& engines = Engines.GetConfigForUpdate();
        if (engines.find(pEngine) == engines.end()) {
            engines.insert(pEngine);
            Engines.SwitchConfig().insert(pEngine);
            // make sure the engine knows about the connection
            //pEngine->Connect(this);
        }
    }

    void AudioOutputDevice::Disconnect(Engine* pEngine) {
        std::set<Engine*>& engines = Engines.GetConfigForUpdate();
        if (engines.find(pEngine) != engines.end()) { // if clause to prevent disconnect loop
            engines.erase(pEngine);
            Engines.SwitchConfig().erase(pEngine);
            // make sure the engine knows about the disconnection
            //pEngine->DisconnectAudioOutputDevice();
        }
    }
    
    void AudioOutputDevice::ReconnectAll() {
        // copy by value, not by reference here !
        std::set<Engine*> engines = Engines.GetConfigForUpdate();
        {
            std::set<Engine*>::iterator iterEngine = engines.begin();
            std::set<Engine*>::iterator end        = engines.end();
            for (; iterEngine != end; iterEngine++) {
                (*iterEngine)->ReconnectAudioOutputDevice();
            }
        }
        
        // update all effects as well
        for (std::vector<EffectChain*>::iterator it = vEffectChains.begin();
             it != vEffectChains.end(); ++it)
        {
            EffectChain* pChain = *it;
            pChain->Reconnect(this);
        }
    }

    AudioChannel* AudioOutputDevice::Channel(uint ChannelIndex) {
        return (ChannelIndex < Channels.size()) ? Channels[ChannelIndex] : NULL;
    }

    void AudioOutputDevice::AcquireChannels(uint Channels) {
        if (Channels > this->Channels.size()) {
            for (size_t c = this->Channels.size(); c < Channels; c++) {
                this->Channels.push_back(CreateChannel(uint(c)));
            }
        }
    }

    uint AudioOutputDevice::ChannelCount() {
        return (uint) Channels.size();
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDevice::DeviceParameters() {
        return Parameters;
    }

    EffectChain* AudioOutputDevice::AddSendEffectChain() {
        EffectChain* pChain = new EffectChain(this, EffectChainIDs->create());
        vEffectChains.push_back(pChain);
        return pChain;
    }

    void AudioOutputDevice::RemoveSendEffectChain(uint iChain) throw (Exception) {
        if (iChain >= vEffectChains.size())
            throw Exception(
                "Could not remove send effect chain " + ToString(iChain) +
                ", index out of bounds"
            );
        std::vector<EffectChain*>::iterator iter = vEffectChains.begin();
        for (int i = 0; i < iChain; ++i) ++iter;
        EffectChainIDs->destroy((*iter)->ID());
        vEffectChains.erase(iter);
    }

    EffectChain* AudioOutputDevice::SendEffectChain(uint iChain) const {
        if (iChain >= vEffectChains.size()) return NULL;
        return vEffectChains[iChain];
    }

    EffectChain* AudioOutputDevice::SendEffectChainByID(uint iChainID) const {
        for (int i = 0; i < SendEffectChainCount(); i++) {
            if (SendEffectChain(i)->ID() == iChainID) return SendEffectChain(i);
        }

        return NULL;
    }

    uint AudioOutputDevice::SendEffectChainCount() const {
        return (uint) vEffectChains.size();
    }

    // TODO: to be removed
    EffectChain* AudioOutputDevice::AddMasterEffectChain() {
        return AddSendEffectChain();
    }

    // TODO: to be removed
    void AudioOutputDevice::RemoveMasterEffectChain(uint iChain) throw (Exception) {
        RemoveSendEffectChain(iChain);
    }

    // TODO: to be removed
    EffectChain* AudioOutputDevice::MasterEffectChain(uint iChain) const {
        return SendEffectChain(iChain);
    }

    // TODO: to be removed
    uint AudioOutputDevice::MasterEffectChainCount() const {
        return SendEffectChainCount();
    }
    
    float AudioOutputDevice::latency() {
        return float(MaxSamplesPerCycle()) / float(SampleRate());
    }

    int AudioOutputDevice::RenderAudio(uint Samples) {
        if (Channels.empty()) return 0;

        // reset all channels with silence
        {
            std::vector<AudioChannel*>::iterator iterChannels = Channels.begin();
            std::vector<AudioChannel*>::iterator end          = Channels.end();
            for (; iterChannels != end; iterChannels++)
                (*iterChannels)->Clear(Samples); // zero out audio buffer
        }
        // do the same for master effects
        {
            std::vector<EffectChain*>::iterator iterChains = vEffectChains.begin();
            std::vector<EffectChain*>::iterator end        = vEffectChains.end();
            for (; iterChains != end; ++iterChains)
                (*iterChains)->ClearAllChannels(); // zero out audio buffers
        }

        int result = 0;

        // let all connected engines render audio for the current audio fragment cycle
        const std::set<Engine*>& engines = EnginesReader.Lock();
        #if CONFIG_RT_EXCEPTIONS
        try
        #endif // CONFIG_RT_EXCEPTIONS
        {
            std::set<Engine*>::iterator iterEngine = engines.begin();
            std::set<Engine*>::iterator end        = engines.end();
            for (; iterEngine != end; iterEngine++) {
                int res = (*iterEngine)->RenderAudio(Samples);
                if (res != 0) result = res;
            }
        }
        #if CONFIG_RT_EXCEPTIONS
        catch (std::runtime_error se) {
            std::cerr << "std::runtime_error: " << se.what() << std::endl << std::flush;
            exit(EXIT_FAILURE);
        }
        #endif // CONFIG_RT_EXCEPTIONS
        EnginesReader.Unlock();

        // now that the engines (might) have left fx send signals for master
        // effects, render all master effects
        {
            std::vector<EffectChain*>::iterator iterChains = vEffectChains.begin();
            std::vector<EffectChain*>::iterator end        = vEffectChains.end();
            for (; iterChains != end; ++iterChains) {
                if (!(*iterChains)->EffectCount()) continue;
                (*iterChains)->RenderAudio(Samples);
                // mix the result of the last effect in the chain to the audio
                // output device channel(s)
                Effect* pLastEffect =
                    (*iterChains)->GetEffect((*iterChains)->EffectCount() - 1);
                for (int iChan = 0; iChan < pLastEffect->OutputChannelCount() && iChan < ChannelCount(); ++iChan)
                    pLastEffect->OutputChannel(iChan)->MixTo(Channel(iChan), Samples);
            }
        }

        return result;
    }

    int AudioOutputDevice::RenderSilence(uint Samples) {
        if (Channels.empty()) return 0;

        // reset all channels with silence
        {
            std::vector<AudioChannel*>::iterator iterChannels = Channels.begin();
            std::vector<AudioChannel*>::iterator end          = Channels.end();
            for (; iterChannels != end; iterChannels++)
                (*iterChannels)->Clear(Samples); // zero out audio buffer
        }

        return 0;
    }

} // namespace LinuxSampler
