/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#include <sstream>

#include "Sampler.h"

#include "common/global_private.h"
#include "engines/EngineFactory.h"
#include "engines/EngineChannelFactory.h"
#include "plugins/InstrumentEditorFactory.h"
#include "drivers/audio/AudioOutputDeviceFactory.h"
#include "drivers/midi/MidiInputDeviceFactory.h"
#include "drivers/midi/MidiInstrumentMapper.h"
#include "common/Features.h"
#include "network/lscpserver.h"

namespace LinuxSampler {

    // ******************************************************************
    // * SamplerChannel

    SamplerChannel::SamplerChannel(Sampler* pS) {
        pSampler           = pS;
        pEngineChannel     = NULL;
        pAudioOutputDevice = NULL;
        iMidiPort          = 0;
        midiChannel        = midi_chan_all;
        iIndex             = -1;
    }

    SamplerChannel::~SamplerChannel() {
        if (pEngineChannel) {
            Engine* engine = pEngineChannel->GetEngine();
            if (pAudioOutputDevice) pAudioOutputDevice->Disconnect(engine);

            if (pEngineChannel) {
                pEngineChannel->DisconnectAllMidiInputPorts();
                
                if (pAudioOutputDevice) pEngineChannel->DisconnectAudioOutputDevice();
                EngineChannelFactory::Destroy(pEngineChannel);

                // reconnect engine if it still exists
                const std::set<Engine*>& engines = EngineFactory::EngineInstances();
                if (engines.find(engine) != engines.end()) pAudioOutputDevice->Connect(engine);
            }
        }
    }

    void SamplerChannel::SetEngineType(String EngineType) throw (Exception) {
        dmsg(2,("SamplerChannel: Assigning engine type..."));

        if (pEngineChannel) {
            if (!strcasecmp(pEngineChannel->EngineName().c_str(), EngineType.c_str())) {
                dmsg(2,("OK\n"));
                return;
            }
        }

        fireEngineToBeChanged();

        // create new engine channel
        EngineChannel* pNewEngineChannel = EngineChannelFactory::Create(EngineType);
        if (!pNewEngineChannel) throw Exception("Unknown engine type");

        // remember current MIDI input connections
        std::vector<MidiInputPort*> vMidiInputs = GetMidiInputPorts();
        midi_chan_t midiChannel = GetMidiInputChannel();
        
        try {
            pNewEngineChannel->SetSamplerChannel(this);

            // disconnect old engine channel
            if (pEngineChannel) {
                Engine* engine = pEngineChannel->GetEngine();
                if (pAudioOutputDevice) pAudioOutputDevice->Disconnect(engine);

                pEngineChannel->DisconnectAllMidiInputPorts();
                if (pAudioOutputDevice) pEngineChannel->DisconnectAudioOutputDevice();
                EngineChannelFactory::Destroy(pEngineChannel);
                pEngineChannel = NULL;

                // reconnect engine if it still exists
                const std::set<Engine*>& engines = EngineFactory::EngineInstances();
                if (engines.find(engine) != engines.end()) pAudioOutputDevice->Connect(engine);
            }

            // connect new engine channel
            if (pAudioOutputDevice) {
                pNewEngineChannel->Connect(pAudioOutputDevice);
                pAudioOutputDevice->Connect(pNewEngineChannel->GetEngine());
            }
            pNewEngineChannel->SetMidiChannel(midiChannel);
            for (int i = 0; i < vMidiInputs.size(); ++i) {
                pNewEngineChannel->Connect(vMidiInputs[i]);
            }
        } catch (...) {
            EngineChannelFactory::Destroy(pNewEngineChannel);
            throw; // re-throw the same exception
        }
        pEngineChannel = pNewEngineChannel;

        // from now on get MIDI input ports from EngineChannel object
        this->vMidiInputs.clear();
        this->iMidiPort        = 0;

        pEngineChannel->StatusChanged(true);
        fireEngineChanged();
        dmsg(2,("OK\n"));
    }

    void SamplerChannel::SetAudioOutputDevice(AudioOutputDevice* pDevice) throw (Exception) {
        if(pAudioOutputDevice == pDevice) return;

        // disconnect old device
        if (pAudioOutputDevice && pEngineChannel) {
            if (!pAudioOutputDevice->isAutonomousDevice())
                throw Exception("The audio output device '" + pAudioOutputDevice->Driver() + "' cannot be dropped from this sampler channel!");

            Engine* engine = pEngineChannel->GetEngine();
            pAudioOutputDevice->Disconnect(engine);

            pEngineChannel->DisconnectAudioOutputDevice();

            // reconnect engine if it still exists
            const std::set<Engine*>& engines = EngineFactory::EngineInstances();
            if (engines.find(engine) != engines.end()) pAudioOutputDevice->Connect(engine);
        }

        // connect new device
        pAudioOutputDevice = pDevice;
        if (pEngineChannel) {
            pEngineChannel->Connect(pAudioOutputDevice);
            pAudioOutputDevice->Connect(pEngineChannel->GetEngine());
        }
    }

    void SamplerChannel::Connect(MidiInputPort* pPort) throw (Exception) {
        if (!pPort) throw Exception("No MIDI input port provided");

        // prevent attempts to connect non-autonomous MIDI ports
        // (host plugins like VST, AU, LV2, DSSI)
        if (!pPort->GetDevice()->isAutonomousDevice())
            throw Exception("The MIDI input port '" + pPort->GetDevice()->Driver() + "' cannot be managed manually!");

        std::vector<MidiInputPort*> vMidiPorts = GetMidiInputPorts();

        // ignore if port is already connected
        for (int i = 0; i < vMidiPorts.size(); ++i) {
            if (vMidiPorts[i] == pPort) return;
        }

        // connect this new port
        if (pEngineChannel) {
            pEngineChannel->Connect(pPort);
        } else { // no engine channel yet, remember it for future connection ...
            const midi_conn_t c = {
                static_cast<uint>(pPort->GetDevice()->MidiInputDeviceID()),
                pPort->GetPortNumber()
            };
            this->vMidiInputs.push_back(c);
        }
    }

    void SamplerChannel::Disconnect(MidiInputPort* pPort) throw (Exception) {
        if (!pPort) return;

        // prevent attempts to alter channels with non-autonomous devices
        // (host plugins like VST, AU, LV2, DSSI)
        if (!pPort->GetDevice()->isAutonomousDevice())
            throw Exception("The MIDI input port '" + pPort->GetDevice()->Driver() + "' cannot be managed manually!");

        // disconnect this port
        if (pEngineChannel) {
            pEngineChannel->Disconnect(pPort);
        } else { // no engine channel yet, forget it regarding future connection ...
            const midi_conn_t c = {
                static_cast<uint>(pPort->GetDevice()->MidiInputDeviceID()),
                pPort->GetPortNumber()
            };
            for (ssize_t i = this->vMidiInputs.size() - 1; i >= 0; --i) {
                if (this->vMidiInputs[i] == c)
                    this->vMidiInputs.erase(this->vMidiInputs.begin() + i);
                // no break or return here, for safety reasons
                // (just in case there were really duplicates for some reason)
            }
        }
    }

    void SamplerChannel::DisconnectAllMidiInputPorts() throw (Exception) {
        std::vector<MidiInputPort*> vMidiPorts = GetMidiInputPorts();
        for (int i = 0; i < vMidiPorts.size(); ++i) Disconnect(vMidiPorts[i]);
    }

    std::vector<MidiInputPort*> SamplerChannel::GetMidiInputPorts() {
        std::vector<MidiInputPort*> v;
        if (pEngineChannel) {
            MidiInputPort* pPort = pEngineChannel->GetMidiInputPort(0);
            for (int i = 0; pPort; pPort = pEngineChannel->GetMidiInputPort(++i))
                v.push_back(pPort);
        } else {
            for (int i = 0; i < this->vMidiInputs.size(); ++i) {
                MidiInputPort* pPort = _getPortForID(this->vMidiInputs[i]);
                if (pPort) v.push_back(pPort);
            }
        }
        return v;
    }

    void SamplerChannel::SetMidiInputDevice(MidiInputDevice* pDevice) throw (Exception) {
       SetMidiInput(pDevice, 0, GetMidiInputChannel());
    }

    void SamplerChannel::SetMidiInputPort(int MidiPort) throw (Exception) {
       SetMidiInput(GetMidiInputDevice(), MidiPort, GetMidiInputChannel());
    }

    void SamplerChannel::SetMidiInputChannel(midi_chan_t MidiChannel) {
        if (!isValidMidiChan(MidiChannel)) throw Exception("Invalid MIDI channel (" + ToString(int(MidiChannel)) + ")");
        if (pEngineChannel) pEngineChannel->SetMidiChannel(MidiChannel);
        this->midiChannel = MidiChannel;
    }

    void SamplerChannel::SetMidiInput(MidiInputDevice* pDevice, int iMidiPort, midi_chan_t MidiChannel) throw (Exception) {
        if (!pDevice) throw Exception("No MIDI input device assigned.");

        // apply new MIDI channel
        SetMidiInputChannel(MidiChannel);

        MidiInputPort* pNewPort = pDevice->GetPort(iMidiPort);
        if (!pNewPort) throw Exception("There is no MIDI input port with index " + ToString(iMidiPort) + ".");

        std::vector<MidiInputPort*> vMidiPorts = GetMidiInputPorts();

        // prevent attempts to remove non-autonomous MIDI ports
        // (host plugins like VST, AU, LV2, DSSI)
        for (int i = 0; i < vMidiPorts.size(); ++i) {
            if (vMidiPorts[i] == pNewPort) continue;
            if (!vMidiPorts[i]->GetDevice()->isAutonomousDevice())
                throw Exception("The MIDI input port '" + vMidiPorts[i]->GetDevice()->Driver() + "' cannot be altered on this sampler channel!");
        }

        if (pEngineChannel) {
            // remove all current connections
            pEngineChannel->DisconnectAllMidiInputPorts();
            // create the new connection (alone)
            pEngineChannel->Connect(pNewPort);
        } else { // if there is no engine channel yet, then store connection for future ...
            // delete all previously scheduled connections
            this->vMidiInputs.clear();
            // store the new connection (alone)
            const midi_conn_t c = {
                static_cast<uint>(pNewPort->GetDevice()->MidiInputDeviceID()),
                pNewPort->GetPortNumber()
            };
            this->vMidiInputs.push_back(c);
            this->iMidiPort = iMidiPort;
        }
    }

    EngineChannel* SamplerChannel::GetEngineChannel() {
        return pEngineChannel;
    }

    midi_chan_t SamplerChannel::GetMidiInputChannel() {
        if (pEngineChannel) this->midiChannel = pEngineChannel->MidiChannel();
        return this->midiChannel;
    }

    int SamplerChannel::GetMidiInputPort() {
        MidiInputPort* pMidiInputPort = (pEngineChannel) ? pEngineChannel->GetMidiInputPort(0) : NULL;
        if (pMidiInputPort) this->iMidiPort = (int) pMidiInputPort->GetPortNumber();
        return iMidiPort;
    }

    AudioOutputDevice* SamplerChannel::GetAudioOutputDevice() {
        return pAudioOutputDevice;
    }

    MidiInputDevice* SamplerChannel::GetMidiInputDevice() {
        if (pEngineChannel)
            return (pEngineChannel->GetMidiInputPort(0)) ? pEngineChannel->GetMidiInputPort(0)->GetDevice() : NULL;

        if (vMidiInputs.empty())
            return NULL;

        std::map<uint, MidiInputDevice*> mAllDevices = MidiInputDeviceFactory::Devices();
        if (!mAllDevices.count(vMidiInputs[0].deviceID))
            return NULL;

        return mAllDevices[vMidiInputs[0].deviceID];
    }

    uint SamplerChannel::Index() {
        if (iIndex >= 0) return iIndex;

        Sampler::SamplerChannelMap::iterator iter = pSampler->mSamplerChannels.begin();
        for (; iter != pSampler->mSamplerChannels.end(); iter++) {
            if (iter->second == this) {
                iIndex = iter->first;
                return iIndex;
            }
        }

        throw Exception("Internal error: SamplerChannel index not found");
    }

    Sampler* SamplerChannel::GetSampler() {
        return pSampler;
    }

    void SamplerChannel::AddEngineChangeListener(EngineChangeListener* l) {
        llEngineChangeListeners.AddListener(l);
    }

    void SamplerChannel::RemoveEngineChangeListener(EngineChangeListener* l) {
       llEngineChangeListeners.RemoveListener(l);
    }

    void SamplerChannel::RemoveAllEngineChangeListeners() {
       llEngineChangeListeners.RemoveAllListeners();
    }

    void SamplerChannel::fireEngineToBeChanged() {
        for (int i = 0; i < llEngineChangeListeners.GetListenerCount(); i++) {
            llEngineChangeListeners.GetListener(i)->EngineToBeChanged(Index());
        }
    }

    void SamplerChannel::fireEngineChanged() {
        for (int i = 0; i < llEngineChangeListeners.GetListenerCount(); i++) {
            llEngineChangeListeners.GetListener(i)->EngineChanged(Index());
        }
    }
    
    /**
     * Takes a numeric MIDI device ID, port ID pair as argument and returns
     * the actual MIDI input port associated with that unique ID pair.
     */
    MidiInputPort* SamplerChannel::_getPortForID(const midi_conn_t& c) {
        std::map<uint, MidiInputDevice*> mAllDevices = MidiInputDeviceFactory::Devices();
        if (!mAllDevices.count(c.deviceID))
            return NULL;

        return mAllDevices[c.deviceID]->GetPort(c.portNr);
    }


    // ******************************************************************
    // * Sampler

    Sampler::Sampler() {
        eventHandler.SetSampler(this);
        uiOldTotalVoiceCount = uiOldTotalStreamCount = 0;
    }

    Sampler::~Sampler() {
        Reset();
    }

    uint Sampler::SamplerChannels() {
        return (uint) mSamplerChannels.size();
    }

    void Sampler::AddChannelCountListener(ChannelCountListener* l) {
        llChannelCountListeners.AddListener(l);
    }

    void Sampler::RemoveChannelCountListener(ChannelCountListener* l) {
       llChannelCountListeners.RemoveListener(l);
    }

    void Sampler::fireChannelCountChanged(int NewCount) {
        for (int i = 0; i < llChannelCountListeners.GetListenerCount(); i++) {
            llChannelCountListeners.GetListener(i)->ChannelCountChanged(NewCount);
        }
    }

    void Sampler::fireChannelAdded(SamplerChannel* pChannel) {
        for (int i = 0; i < llChannelCountListeners.GetListenerCount(); i++) {
            llChannelCountListeners.GetListener(i)->ChannelAdded(pChannel);
        }
    }

    void Sampler::fireChannelToBeRemoved(SamplerChannel* pChannel) {
        for (int i = 0; i < llChannelCountListeners.GetListenerCount(); i++) {
            llChannelCountListeners.GetListener(i)->ChannelToBeRemoved(pChannel);
        }
    }

    void Sampler::AddAudioDeviceCountListener(AudioDeviceCountListener* l) {
        llAudioDeviceCountListeners.AddListener(l);
    }

    void Sampler::RemoveAudioDeviceCountListener(AudioDeviceCountListener* l) {
        llAudioDeviceCountListeners.RemoveListener(l);
    }

    void Sampler::fireAudioDeviceCountChanged(int NewCount) {
        for (int i = 0; i < llAudioDeviceCountListeners.GetListenerCount(); i++) {
            llAudioDeviceCountListeners.GetListener(i)->AudioDeviceCountChanged(NewCount);
        }
    }

    void Sampler::AddMidiDeviceCountListener(MidiDeviceCountListener* l) {
        llMidiDeviceCountListeners.AddListener(l);
    }

    void Sampler::RemoveMidiDeviceCountListener(MidiDeviceCountListener* l) {
        llMidiDeviceCountListeners.RemoveListener(l);
    }

    void Sampler::fireMidiDeviceCountChanged(int NewCount) {
        for (int i = 0; i < llMidiDeviceCountListeners.GetListenerCount(); i++) {
            llMidiDeviceCountListeners.GetListener(i)->MidiDeviceCountChanged(NewCount);
        }
    }

    void Sampler::fireMidiDeviceToBeDestroyed(MidiInputDevice* pDevice) {
        for (int i = 0; i < llMidiDeviceCountListeners.GetListenerCount(); i++) {
            llMidiDeviceCountListeners.GetListener(i)->MidiDeviceToBeDestroyed(pDevice);
        }
    }

    void Sampler::fireMidiDeviceCreated(MidiInputDevice* pDevice) {
        for (int i = 0; i < llMidiDeviceCountListeners.GetListenerCount(); i++) {
            llMidiDeviceCountListeners.GetListener(i)->MidiDeviceCreated(pDevice);
        }
    }

    void Sampler::AddVoiceCountListener(VoiceCountListener* l) {
        llVoiceCountListeners.AddListener(l);
    }

    void Sampler::RemoveVoiceCountListener(VoiceCountListener* l) {
        llVoiceCountListeners.RemoveListener(l);
    }

    void Sampler::fireVoiceCountChanged(int ChannelId, int NewCount) {
        std::map<uint, uint>::iterator it = mOldVoiceCounts.find(ChannelId);
        if (it != mOldVoiceCounts.end()) {
            uint oldCount = it->second;
            if (NewCount == oldCount) return;
        }

        mOldVoiceCounts[ChannelId] = NewCount;

        for (int i = 0; i < llVoiceCountListeners.GetListenerCount(); i++) {
            llVoiceCountListeners.GetListener(i)->VoiceCountChanged(ChannelId, NewCount);
        }
    }

    void Sampler::AddStreamCountListener(StreamCountListener* l) {
        llStreamCountListeners.AddListener(l);
    }

    void Sampler::RemoveStreamCountListener(StreamCountListener* l) {
        llStreamCountListeners.RemoveListener(l);
    }

    void Sampler::fireStreamCountChanged(int ChannelId, int NewCount) {
        std::map<uint, uint>::iterator it = mOldStreamCounts.find(ChannelId);
        if (it != mOldStreamCounts.end()) {
            uint oldCount = it->second;
            if (NewCount == oldCount) return;
        }

        mOldStreamCounts[ChannelId] = NewCount;

        for (int i = 0; i < llStreamCountListeners.GetListenerCount(); i++) {
            llStreamCountListeners.GetListener(i)->StreamCountChanged(ChannelId, NewCount);
        }
    }

    void Sampler::AddBufferFillListener(BufferFillListener* l) {
        llBufferFillListeners.AddListener(l);
    }

    void Sampler::RemoveBufferFillListener(BufferFillListener* l) {
        llBufferFillListeners.RemoveListener(l);
    }

    void Sampler::fireBufferFillChanged(int ChannelId, String FillData) {
        for (int i = 0; i < llBufferFillListeners.GetListenerCount(); i++) {
            llBufferFillListeners.GetListener(i)->BufferFillChanged(ChannelId, FillData);
        }
    }

    void Sampler::AddTotalStreamCountListener(TotalStreamCountListener* l) {
        llTotalStreamCountListeners.AddListener(l);
    }

    void Sampler::RemoveTotalStreamCountListener(TotalStreamCountListener* l) {
        llTotalStreamCountListeners.RemoveListener(l);
    }

    void Sampler::fireTotalStreamCountChanged(int NewCount) {
        if (NewCount == uiOldTotalStreamCount) return;
        uiOldTotalStreamCount = NewCount;

        for (int i = 0; i < llTotalStreamCountListeners.GetListenerCount(); i++) {
            llTotalStreamCountListeners.GetListener(i)->TotalStreamCountChanged(NewCount);
        }
    }

    void Sampler::AddTotalVoiceCountListener(TotalVoiceCountListener* l) {
        llTotalVoiceCountListeners.AddListener(l);
    }

    void Sampler::RemoveTotalVoiceCountListener(TotalVoiceCountListener* l) {
        llTotalVoiceCountListeners.RemoveListener(l);
    }

    void Sampler::fireTotalVoiceCountChanged(int NewCount) {
        if (NewCount == uiOldTotalVoiceCount) return;
        uiOldTotalVoiceCount = NewCount;

        for (int i = 0; i < llTotalVoiceCountListeners.GetListenerCount(); i++) {
            llTotalVoiceCountListeners.GetListener(i)->TotalVoiceCountChanged(NewCount);
        }
    }

    void Sampler::AddFxSendCountListener(FxSendCountListener* l) {
        llFxSendCountListeners.AddListener(l);
    }

    void Sampler::RemoveFxSendCountListener(FxSendCountListener* l) {
        llFxSendCountListeners.RemoveListener(l);
    }

    void Sampler::fireFxSendCountChanged(int ChannelId, int NewCount) {
        for (int i = 0; i < llFxSendCountListeners.GetListenerCount(); i++) {
            llFxSendCountListeners.GetListener(i)->FxSendCountChanged(ChannelId, NewCount);
        }
    }

    void Sampler::EventHandler::EngineToBeChanged(int ChannelId) {
        // nothing to do here
    }

    void Sampler::EventHandler::EngineChanged(int ChannelId) {
        EngineChannel* engineChannel = pSampler->GetSamplerChannel(ChannelId)->GetEngineChannel();
        if(engineChannel == NULL) return;
        engineChannel->AddFxSendCountListener(this);
    }

    void Sampler::EventHandler::FxSendCountChanged(int ChannelId, int NewCount) {
        pSampler->fireFxSendCountChanged(ChannelId, NewCount);
    }


    SamplerChannel* Sampler::AddSamplerChannel() {
        // if there's no sampler channel yet
        if (!mSamplerChannels.size()) {
            SamplerChannel* pChannel = new SamplerChannel(this);
            mSamplerChannels[0] = pChannel;
            fireChannelAdded(pChannel);
            fireChannelCountChanged(1);
            pChannel->AddEngineChangeListener(&eventHandler);
            return pChannel;
        }

        // get the highest used sampler channel index
        uint lastIndex = (--(mSamplerChannels.end()))->first;

        // check if we reached the index limit
        if (lastIndex + 1 < lastIndex) {
            // search for an unoccupied sampler channel index starting from 0
            for (uint i = 0; i < lastIndex; i++) {
                if (mSamplerChannels.find(i) != mSamplerChannels.end()) continue;
                // we found an unused index, so insert the new channel there
                SamplerChannel* pChannel = new SamplerChannel(this);
                mSamplerChannels[i] = pChannel;
                fireChannelAdded(pChannel);
                fireChannelCountChanged(SamplerChannels());
                pChannel->AddEngineChangeListener(&eventHandler);
                return pChannel;
            }
            throw Exception("Internal error: could not find unoccupied sampler channel index.");
        }

        // we have not reached the index limit so we just add the channel past the highest index
        SamplerChannel* pChannel = new SamplerChannel(this);
        mSamplerChannels[lastIndex + 1] = pChannel;
        fireChannelAdded(pChannel);
        fireChannelCountChanged(SamplerChannels());
        pChannel->AddEngineChangeListener(&eventHandler);
        return pChannel;
    }

    SamplerChannel* Sampler::GetSamplerChannel(uint uiSamplerChannel) {
        return (mSamplerChannels.find(uiSamplerChannel) != mSamplerChannels.end()) ? mSamplerChannels[uiSamplerChannel] : NULL;
    }

    std::map<uint, SamplerChannel*> Sampler::GetSamplerChannels() {
        return mSamplerChannels;
    }

    void Sampler::RemoveSamplerChannel(SamplerChannel* pSamplerChannel) {
        SamplerChannelMap::iterator iterChan = mSamplerChannels.begin();
        for (; iterChan != mSamplerChannels.end(); iterChan++) {
            if (iterChan->second == pSamplerChannel) {
                fireChannelToBeRemoved(pSamplerChannel);
                mOldVoiceCounts.erase(pSamplerChannel->Index());
                mOldStreamCounts.erase(pSamplerChannel->Index());
                pSamplerChannel->RemoveAllEngineChangeListeners();
                mSamplerChannels.erase(iterChan);
                delete pSamplerChannel;
                fireChannelCountChanged(SamplerChannels());
                return;
            }
        }
    }

    void Sampler::RemoveSamplerChannel(uint uiSamplerChannel) {
        SamplerChannel* pChannel = GetSamplerChannel(uiSamplerChannel);
        if (!pChannel) return;
        RemoveSamplerChannel(pChannel);
    }

    void Sampler::RemoveAllSamplerChannels() {
        /*
         * In maps iterator invalidation occurs when the iterator point
         * to the element that is being erased. So we need to copy the map
         * by calling GetSamplerChannels() to prevent that.
         */
        SamplerChannelMap chns = GetSamplerChannels();
        SamplerChannelMap::iterator iter = chns.begin();
        for(; iter != chns.end(); iter++) {
            RemoveSamplerChannel(iter->second);
        }
    }

    std::vector<String> Sampler::AvailableAudioOutputDrivers() {
        return AudioOutputDeviceFactory::AvailableDrivers();
    }

    std::vector<String> Sampler::AvailableMidiInputDrivers() {
        return MidiInputDeviceFactory::AvailableDrivers();
    }

    std::vector<String> Sampler::AvailableEngineTypes() {
        return EngineFactory::AvailableEngineTypes();
    }

    AudioOutputDevice* Sampler::CreateAudioOutputDevice(String AudioDriver, std::map<String,String> Parameters) throw (Exception) {
        // create new device
        AudioOutputDevice* pDevice = AudioOutputDeviceFactory::Create(AudioDriver, Parameters);

        fireAudioDeviceCountChanged(AudioOutputDevices());
        return pDevice;
    }

    uint Sampler::AudioOutputDevices() {
        return (uint) AudioOutputDeviceFactory::Devices().size();
    }

    uint Sampler::MidiInputDevices() {
        return (uint) MidiInputDeviceFactory::Devices().size();
    }

    std::map<uint, AudioOutputDevice*> Sampler::GetAudioOutputDevices() {
        return AudioOutputDeviceFactory::Devices();
    }

    std::map<uint, MidiInputDevice*> Sampler::GetMidiInputDevices() {
        return MidiInputDeviceFactory::Devices();
    }

    void Sampler::DestroyAudioOutputDevice(AudioOutputDevice* pDevice) throw (Exception) {
        if (pDevice) {
            // check if there are still sampler engines connected to this device
            for (SamplerChannelMap::iterator iterChan = mSamplerChannels.begin();
                 iterChan != mSamplerChannels.end(); iterChan++
            ) if (iterChan->second->GetAudioOutputDevice() == pDevice) throw Exception("Sampler channel " + ToString(iterChan->first) + " is still connected to the audio output device.");

            //TODO: should we add fireAudioDeviceToBeDestroyed() here ?
            AudioOutputDeviceFactory::Destroy(pDevice);
            fireAudioDeviceCountChanged(AudioOutputDevices());
        }
    }

    void Sampler::DestroyAllAudioOutputDevices() throw (Exception) {
        /*
         * In maps iterator invalidation occurs when the iterator point
         * to the element that is being erased. So we need to copy the map
         * by calling GetAudioOutputDevices() to prevent that.
         */
        std::map<uint, AudioOutputDevice*> devs = GetAudioOutputDevices();
        std::map<uint, AudioOutputDevice*>::iterator iter = devs.begin();
        for (; iter != devs.end(); iter++) {
            AudioOutputDevice* pDevice = iter->second;

            // skip non-autonomous devices
            if (!pDevice->isAutonomousDevice()) continue;

            DestroyAudioOutputDevice(pDevice);
        }
    }

    void Sampler::DestroyMidiInputDevice(MidiInputDevice* pDevice) throw (Exception) {
        if (pDevice) {
            // check if there are still sampler engines connected to this device
            for (SamplerChannelMap::iterator iterChan = mSamplerChannels.begin();
                 iterChan != mSamplerChannels.end(); ++iterChan)
            {
                std::vector<MidiInputPort*> vPorts = iterChan->second->GetMidiInputPorts();
                for (int k = 0; k < vPorts.size(); ++k)
                    if (vPorts[k]->GetDevice() == pDevice)
                        throw Exception("Sampler channel " + ToString(iterChan->first) + " is still connected to the midi input device.");
            }

            fireMidiDeviceToBeDestroyed(pDevice);
            MidiInputDeviceFactory::Destroy(pDevice);
            fireMidiDeviceCountChanged(MidiInputDevices());
        }
    }

    void Sampler::DestroyAllMidiInputDevices() throw (Exception) {
        /*
         * In maps iterator invalidation occurs when the iterator point
         * to the element that is being erased. So we need to copy the map
         * by calling GetMidiInputDevices() to prevent that.
         */
        std::map<uint, MidiInputDevice*> devs = GetMidiInputDevices();
        std::map<uint, MidiInputDevice*>::iterator iter = devs.begin();
        for (; iter != devs.end(); iter++) {
            MidiInputDevice* pDevice = iter->second;

            // skip non-autonomous devices
            if (!pDevice->isAutonomousDevice()) continue;

            DestroyMidiInputDevice(pDevice);
        }
    }

    MidiInputDevice* Sampler::CreateMidiInputDevice(String MidiDriver, std::map<String,String> Parameters) throw (Exception) {
        // create new device
        MidiInputDevice* pDevice = MidiInputDeviceFactory::Create(MidiDriver, Parameters, this);

        fireMidiDeviceCreated(pDevice);
        fireMidiDeviceCountChanged(MidiInputDevices());
        return pDevice;
    }

    int Sampler::GetDiskStreamCount() {
        int count = 0;
        std::set<Engine*>::iterator it = EngineFactory::EngineInstances().begin();

        for(; it != EngineFactory::EngineInstances().end(); it++) {
            count += (*it)->DiskStreamCount();
        }

        return count;
    }

    int Sampler::GetVoiceCount() {
        int count = 0;
        std::set<Engine*>::iterator it = EngineFactory::EngineInstances().begin();

        for(; it != EngineFactory::EngineInstances().end(); it++) {
            count += (*it)->VoiceCount();
        }

        return count;
    }

    int Sampler::GetGlobalMaxVoices() {
        return GLOBAL_MAX_VOICES; // see common/global_private.cpp
    }

    int Sampler::GetGlobalMaxStreams() {
        return GLOBAL_MAX_STREAMS; // see common/global_private.cpp
    }

    void Sampler::SetGlobalMaxVoices(int n) throw (Exception) {
        if (n < 1) throw Exception("Maximum voices may not be less than 1");
        GLOBAL_MAX_VOICES = n; // see common/global_private.cpp
        const std::set<Engine*>& engines = EngineFactory::EngineInstances();
        if (engines.size() > 0) {
            std::set<Engine*>::iterator iter = engines.begin();
            std::set<Engine*>::iterator end  = engines.end();
            for (; iter != end; ++iter) {
                (*iter)->SetMaxVoices(n);
            }
        }
    }

    void Sampler::SetGlobalMaxStreams(int n) throw (Exception) {
        if (n < 0) throw Exception("Maximum disk streams may not be negative");
        GLOBAL_MAX_STREAMS = n; // see common/global_private.cpp
        const std::set<Engine*>& engines = EngineFactory::EngineInstances();
        if (engines.size() > 0) {
            std::set<Engine*>::iterator iter = engines.begin();
            std::set<Engine*>::iterator end  = engines.end();
            for (; iter != end; ++iter) {
                (*iter)->SetMaxDiskStreams(n);
            }
        }
    }

    void Sampler::Reset() {
        // delete sampler channels
        try {
            RemoveAllSamplerChannels();
        }
        catch(...) {
            std::cerr << "Sampler::Reset(): Exception occured while trying to delete all sampler channels, exiting.\n" << std::flush;
            exit(EXIT_FAILURE);
        }

        // delete midi input devices
        try {
            DestroyAllMidiInputDevices();
        }
        catch(...) {
            std::cerr << "Sampler::Reset(): Exception occured while trying to delete all MIDI input devices, exiting.\n" << std::flush;
            exit(EXIT_FAILURE);
        }

        // delete audio output devices
        try {
            DestroyAllAudioOutputDevices();
        }
        catch(...) {
            std::cerr << "Sampler::Reset(): Exception occured while trying to delete all audio output devices, exiting.\n" << std::flush;
            exit(EXIT_FAILURE);
        }

        // delete MIDI instrument maps
        try {
            MidiInstrumentMapper::RemoveAllMaps();
        }
        catch(...) {
            std::cerr << "Sampler::Reset(): Exception occured while trying to delete all MIDI instrument maps, exiting.\n" << std::flush;
            exit(EXIT_FAILURE);
        }

        // unload all instrument editor DLLs
        InstrumentEditorFactory::ClosePlugins();
    }

    bool Sampler::EnableDenormalsAreZeroMode() {
        Features::detect();
        return Features::enableDenormalsAreZeroMode();
    }

    void Sampler::fireStatistics() {
        static const LSCPEvent::event_t eventsArr[] = {
            LSCPEvent::event_voice_count, LSCPEvent::event_stream_count,
            LSCPEvent::event_buffer_fill, LSCPEvent::event_total_voice_count
        };
        static const std::list<LSCPEvent::event_t> events(eventsArr, eventsArr + 4);

        if (LSCPServer::EventSubscribers(events))
        {
            LockGuard lock(LSCPServer::RTNotifyMutex);
            std::map<uint,SamplerChannel*> channels = GetSamplerChannels();
            std::map<uint,SamplerChannel*>::iterator iter = channels.begin();
            for (; iter != channels.end(); iter++) {
                SamplerChannel* pSamplerChannel = iter->second;
                EngineChannel* pEngineChannel = pSamplerChannel->GetEngineChannel();
                if (!pEngineChannel) continue;
                Engine* pEngine = pEngineChannel->GetEngine();
                if (!pEngine) continue;
                fireVoiceCountChanged(iter->first, pEngineChannel->GetVoiceCount());
                fireStreamCountChanged(iter->first, pEngineChannel->GetDiskStreamCount());
                fireBufferFillChanged(iter->first, pEngine->DiskStreamBufferFillPercentage());
            }

            fireTotalStreamCountChanged(GetDiskStreamCount());
            fireTotalVoiceCountChanged(GetVoiceCount());
        }
    }

#if defined(WIN32)
    static HINSTANCE dllInstance = NULL;

    String Sampler::GetInstallDir() {
        char buf[MAX_PATH + 1];
        if (GetModuleFileName(dllInstance, buf, MAX_PATH)) {
            String s(buf);
            size_t n = s.rfind('\\');
            if (n != String::npos) {
                return s.substr(0, n);
            }
        }
        return "";
    }
#endif
} // namespace LinuxSampler

#if defined(WIN32)
extern "C" {
    BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
    {
        switch (reason) {
        case DLL_PROCESS_ATTACH:
            LinuxSampler::dllInstance = instance;
            break;
        }
        return TRUE;
    }
}
#endif
