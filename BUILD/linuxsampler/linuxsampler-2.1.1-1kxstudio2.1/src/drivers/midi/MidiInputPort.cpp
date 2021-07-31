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

#include "MidiInputPort.h"

#include "../../common/global_private.h"
#include "MidiInstrumentMapper.h"
#include "../../Sampler.h"
#include "../../engines/EngineFactory.h"
#include "VirtualMidiDevice.h"

#include <algorithm>

namespace LinuxSampler {

// *************** ParameterName ***************
// *

    MidiInputPort::ParameterName::ParameterName(MidiInputPort* pPort) : DeviceRuntimeParameterString("Port " + ToString(pPort->GetPortNumber())) {
        this->pPort = pPort;
    }

    MidiInputPort::ParameterName::ParameterName(MidiInputPort* pPort, String val) : DeviceRuntimeParameterString(val) {
        this->pPort = pPort;
    }

    String MidiInputPort::ParameterName::Description() {
        return "Name for this port";
    }

    bool MidiInputPort::ParameterName::Fix() {
        return false;
    }

    std::vector<String> MidiInputPort::ParameterName::PossibilitiesAsString() {
        return std::vector<String>();
    }

    void MidiInputPort::ParameterName::OnSetValue(String s) throw (Exception) {
        return; /* FIXME: Nothing to do here */
    }



// *************** MidiInputPort ***************
// *

    MidiInputPort::~MidiInputPort() {
        std::map<String,DeviceRuntimeParameter*>::iterator iter = Parameters.begin();
        while (iter != Parameters.end()) {
            delete iter->second;
            iter++;
        }
        Parameters.clear();
    }

    MidiInputPort::MidiInputPort(MidiInputDevice* pDevice, int portNumber)
        : MidiChannelMapReader(MidiChannelMap),
          SysexListenersReader(SysexListeners),
          virtualMidiDevicesReader(virtualMidiDevices),
          noteOnVelocityFilterReader(noteOnVelocityFilter)
    {
        this->pDevice = pDevice;
        this->portNumber = portNumber;
        runningStatusBuf[0] = 0;
        Parameters["NAME"] = new ParameterName(this);
    }

    MidiInputDevice* MidiInputPort::GetDevice() {
        return pDevice;
    }

    uint MidiInputPort::GetPortNumber() {
        return portNumber;
    }

    std::map<String,DeviceRuntimeParameter*> MidiInputPort::PortParameters() {
        return Parameters;
    }

    void MidiInputPort::DispatchNoteOn(uint8_t Key, uint8_t Velocity, uint MidiChannel) {
        if (Key > 127 || Velocity > 127 || MidiChannel > 16) return;
        
        // apply velocity filter (if any)
        const std::vector<uint8_t>& velocityFilter = noteOnVelocityFilterReader.Lock();
        if (!velocityFilter.empty()) Velocity = velocityFilter[Velocity];
        noteOnVelocityFilterReader.Unlock();
        
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOn(Key, Velocity, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOn(Key, Velocity, MidiChannel);
        }
        MidiChannelMapReader.Unlock();

        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendNoteOnToDevice(Key, Velocity);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchNoteOn(uint8_t Key, uint8_t Velocity, uint MidiChannel, int32_t FragmentPos) {
        if (Key > 127 || Velocity > 127 || MidiChannel > 16) return;
        
        // apply velocity filter (if any)
        const std::vector<uint8_t>& velocityFilter = noteOnVelocityFilterReader.Lock();
        if (!velocityFilter.empty()) Velocity = velocityFilter[Velocity];
        noteOnVelocityFilterReader.Unlock();
        
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOn(Key, Velocity, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOn(Key, Velocity, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();

        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendNoteOnToDevice(Key, Velocity);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchNoteOff(uint8_t Key, uint8_t Velocity, uint MidiChannel) {
        if (Key > 127 || Velocity > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOff(Key, Velocity, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOff(Key, Velocity, MidiChannel);
        }
        MidiChannelMapReader.Unlock();

        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendNoteOffToDevice(Key, Velocity);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchNoteOff(uint8_t Key, uint8_t Velocity, uint MidiChannel, int32_t FragmentPos) {
        if (Key > 127 || Velocity > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOff(Key, Velocity, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendNoteOff(Key, Velocity, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();

        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendNoteOffToDevice(Key, Velocity);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchPitchbend(int Pitch, uint MidiChannel) {
        if (Pitch < -8192 || Pitch > 8191 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPitchbend(Pitch, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPitchbend(Pitch, MidiChannel);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchPitchbend(int Pitch, uint MidiChannel, int32_t FragmentPos) {
        if (Pitch < -8192 || Pitch > 8191 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPitchbend(Pitch, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPitchbend(Pitch, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchChannelPressure(uint8_t Value, uint MidiChannel) {
        if (Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendChannelPressure(Value, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendChannelPressure(Value, MidiChannel);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchChannelPressure(uint8_t Value, uint MidiChannel, int32_t FragmentPos) {
        if (Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendChannelPressure(Value, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendChannelPressure(Value, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint MidiChannel) {
        if (Key > 127 || Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPolyphonicKeyPressure(Key, Value, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPolyphonicKeyPressure(Key, Value, MidiChannel);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint MidiChannel, int32_t FragmentPos) {
        if (Key > 127 || Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPolyphonicKeyPressure(Key, Value, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendPolyphonicKeyPressure(Key, Value, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchControlChange(uint8_t Controller, uint8_t Value, uint MidiChannel) {
        if (Controller > 128 || Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendControlChange(Controller, Value, MidiChannel);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendControlChange(Controller, Value, MidiChannel);
        }
        MidiChannelMapReader.Unlock();
        
        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendCCToDevice(Controller, Value);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchControlChange(uint8_t Controller, uint8_t Value, uint MidiChannel, int32_t FragmentPos) {
        if (Controller > 128 || Value > 127 || MidiChannel > 16) return;
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendControlChange(Controller, Value, MidiChannel, FragmentPos);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendControlChange(Controller, Value, MidiChannel, FragmentPos);
        }
        MidiChannelMapReader.Unlock();
        
        // dispatch event to all low priority MIDI listeners
        const std::vector<VirtualMidiDevice*>& listeners =
            virtualMidiDevicesReader.Lock();
        for (int i = 0; i < listeners.size(); ++i)
            listeners[i]->SendCCToDevice(Controller, Value);
        virtualMidiDevicesReader.Unlock();
    }

    void MidiInputPort::DispatchSysex(void* pData, uint Size) {
        const std::set<Engine*> allEngines = SysexListenersReader.Lock();
        // dispatch event to all engine instances
        std::set<Engine*>::iterator engineiter = allEngines.begin();
        std::set<Engine*>::iterator end        = allEngines.end();
        for (; engineiter != end; engineiter++) (*engineiter)->SendSysex(pData, Size, this);
        SysexListenersReader.Unlock();
    }

    void MidiInputPort::DispatchProgramChange(uint8_t Program, uint MidiChannel) {
        if (Program > 127 || MidiChannel > 16) return;
        if (!pDevice || !pDevice->pSampler) {
            std::cerr << "MidiInputPort: ERROR, no sampler instance to handle program change."
                      << "This is a bug, please report it!\n" << std::flush;
            return;
        }

        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendProgramChange(Program);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            for (; engineiter != end; engineiter++) (*engineiter)->SendProgramChange(Program);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchBankSelectMsb(uint8_t BankMSB, uint MidiChannel) {
        if (BankMSB > 127 || MidiChannel > 16) return;
        if (!pDevice || !pDevice->pSampler) {
            std::cerr << "MidiInputPort: ERROR, no sampler instance to handle bank select MSB."
                      << "This is a bug, please report it!\n" << std::flush;
            return;
        }
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            // according to the MIDI specs, a bank select should not alter the patch
            for (; engineiter != end; engineiter++) (*engineiter)->SetMidiBankMsb(BankMSB);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            // according to the MIDI specs, a bank select should not alter the patch
            for (; engineiter != end; engineiter++) (*engineiter)->SetMidiBankMsb(BankMSB);
        }
        MidiChannelMapReader.Unlock();
    }

    void MidiInputPort::DispatchBankSelectLsb(uint8_t BankLSB, uint MidiChannel) {
        if (BankLSB > 127 || MidiChannel > 16) return;
        if (!pDevice || !pDevice->pSampler) {
            std::cerr << "MidiInputPort: ERROR, no sampler instance to handle bank select LSB."
                      << "This is a bug, please report it!\n" << std::flush;
            return;
        }
        const MidiChannelMap_t& midiChannelMap = MidiChannelMapReader.Lock();
        // dispatch event for engines listening to the same MIDI channel
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[MidiChannel].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[MidiChannel].end();
            // according to the MIDI specs, a bank select should not alter the patch
            for (; engineiter != end; engineiter++) (*engineiter)->SetMidiBankLsb(BankLSB);
        }
        // dispatch event for engines listening to ALL MIDI channels
        {
            std::set<EngineChannel*>::iterator engineiter = midiChannelMap[midi_chan_all].begin();
            std::set<EngineChannel*>::iterator end        = midiChannelMap[midi_chan_all].end();
            // according to the MIDI specs, a bank select should not alter the patch
            for (; engineiter != end; engineiter++) (*engineiter)->SetMidiBankLsb(BankLSB);
        }
        MidiChannelMapReader.Unlock();
    }
    
    /**
     * Handles the so called MIDI "running status" mode, which allows devices
     * to reduce bandwidth (data reduction).
     *
     * If the passed in MIDI data is regular MIDI data, this method will simply
     * return the original data pointer and just stores the status byte for
     * potential "running status" event eventually coming next.
     *
     * If the passed in MIDI data however seems to be in "running status" mode,
     * this method will return another buffer, which allows the MIDI parser
     * to handle the MIDI data as usually with "normal" MIDI data.
     */
    uint8_t* MidiInputPort::handleRunningStatus(uint8_t* pData) {
        if ((pData[0] & 0x80) || !runningStatusBuf[0]) {
            // store status byte for eventual "running status" in next event
            if (pData[0] & 0x80) {
                if (pData[0] < 0xf0) {
                    // "running status" is only allowed for channel messages
                    runningStatusBuf[0] = pData[0];
                } else if (pData[0] < 0xf8) {
                    // "system common" messages (0xf0..0xf7) shall reset any running
                    // status, however "realtime" messages (0xf8..0xff) shall be
                    // ignored here
                    runningStatusBuf[0] = 0;
                }
            }
            // it's either a regular status byte, or some invalid "running status"
            return pData;
        } else { // "running status" mode ...
            const uint8_t type = runningStatusBuf[0] & 0xf0;
            const int size = (type == 0xc0 || type == 0xd0) ? 1 : 2; // only program change & channel pressure have 1 data bytes
            memcpy(&runningStatusBuf[1], pData, size);
            return runningStatusBuf;
        }
    }

    void MidiInputPort::DispatchRaw(uint8_t* pData) {
        pData = handleRunningStatus(pData);
        
        uint8_t channel = pData[0] & 0x0f;
        switch (pData[0] & 0xf0) {
        case 0x80:
            DispatchNoteOff(pData[1], pData[2], channel);
            break;
        case 0x90:
            if (pData[2]) {
                DispatchNoteOn(pData[1], pData[2], channel);
            } else {
                DispatchNoteOff(pData[1], pData[2], channel);
            }
            break;
        case 0xA0:
            DispatchPolyphonicKeyPressure(pData[1], pData[2], channel);
            break;
        case 0xb0:
            if (pData[1] == 0) {
                DispatchBankSelectMsb(pData[2], channel);
            } else if (pData[1] == 32) {
                DispatchBankSelectLsb(pData[2], channel);
            }
            DispatchControlChange(pData[1], pData[2], channel);
            break;
        case 0xc0:
            DispatchProgramChange(pData[1], channel);
            break;
        case 0xd0:
            DispatchChannelPressure(pData[1], channel);
            break;
        case 0xe0:
            DispatchPitchbend((pData[1] | pData[2] << 7) - 8192, channel);
            break;
        }
    }

    void MidiInputPort::DispatchRaw(uint8_t* pData, int32_t FragmentPos) {
        pData = handleRunningStatus(pData);
        
        uint8_t channel = pData[0] & 0x0f;
        switch (pData[0] & 0xf0) {
        case 0x80:
            DispatchNoteOff(pData[1], pData[2], channel, FragmentPos);
            break;
        case 0x90:
            if (pData[2]) {
                DispatchNoteOn(pData[1], pData[2], channel, FragmentPos);
            } else {
                DispatchNoteOff(pData[1], pData[2], channel, FragmentPos);
            }
            break;
        case 0xA0:
            DispatchPolyphonicKeyPressure(pData[1], pData[2], channel, FragmentPos);
            break;
        case 0xb0:
            if (pData[1] == 0) {
                DispatchBankSelectMsb(pData[2], channel);
            } else if (pData[1] == 32) {
                DispatchBankSelectLsb(pData[2], channel);
            }
            DispatchControlChange(pData[1], pData[2], channel, FragmentPos);
            break;
        case 0xc0:
            DispatchProgramChange(pData[1], channel);
            break;
        case 0xd0:
            DispatchChannelPressure(pData[1], channel, FragmentPos);
            break;
        case 0xe0:
            DispatchPitchbend((pData[1] | pData[2] << 7) - 8192, channel, FragmentPos);
            break;
        }
    }
    
    void MidiInputPort::SetNoteOnVelocityFilter(const std::vector<uint8_t>& filter) {
        if (filter.size() != 128 && filter.size() != 0)
            throw MidiInputException("Note on velocity filter must be either of size 128 or 0");
        
        // check the value range of the filter
        if (!filter.empty())
            for (int i = 0; i < 128; i++)
                if (filter[i] > 127)
                    throw MidiInputException("Invalid note on velocity filter, values must be in range 0 .. 127");
        
        // apply new filter ...
        LockGuard lock(noteOnVelocityFilterMutex);
        // double buffer ... double work ...
        {
            std::vector<uint8_t>& config =
                noteOnVelocityFilter.GetConfigForUpdate();
            config = filter;
        }
        {
            std::vector<uint8_t>& config =
                noteOnVelocityFilter.SwitchConfig();
            config = filter;
        }
    }

    void MidiInputPort::Connect(EngineChannel* pEngineChannel, midi_chan_t MidiChannel) {
        if (MidiChannel < 0 || MidiChannel > 16)
            throw MidiInputException("MIDI channel index out of bounds");

        // first check if desired connection is already established
        {
            LockGuard lock(MidiChannelMapMutex);
            MidiChannelMap_t& midiChannelMap = MidiChannelMap.GetConfigForUpdate();
            if (midiChannelMap[MidiChannel].count(pEngineChannel)) return;
        }

        // remove all other connections of that engine channel (if any)
        Disconnect(pEngineChannel);

        // register engine channel on the desired MIDI channel
        {
            LockGuard lock(MidiChannelMapMutex);
            MidiChannelMap.GetConfigForUpdate()[MidiChannel].insert(pEngineChannel);
            MidiChannelMap.SwitchConfig()[MidiChannel].insert(pEngineChannel);
        }

        // inform engine channel about this connection
        pEngineChannel->Connect(this);
        if (pEngineChannel->MidiChannel() != MidiChannel)
            pEngineChannel->SetMidiChannel(MidiChannel);

        // mark engine channel as changed
        pEngineChannel->StatusChanged(true);
    }

    void MidiInputPort::Disconnect(EngineChannel* pEngineChannel) {
        if (!pEngineChannel) return;

        bool bChannelFound = false;

        // unregister engine channel from all MIDI channels
        try {
            LockGuard lock(MidiChannelMapMutex);
            {
                MidiChannelMap_t& midiChannelMap = MidiChannelMap.GetConfigForUpdate();
                for (int i = 0; i <= 16; i++) {
                    bChannelFound |= midiChannelMap[i].count(pEngineChannel);
                    midiChannelMap[i].erase(pEngineChannel);
                }
            }
            // do the same update again, after switching to the other config
            {
                MidiChannelMap_t& midiChannelMap = MidiChannelMap.SwitchConfig();
                for (int i = 0; i <= 16; i++) {
                    bChannelFound |= midiChannelMap[i].count(pEngineChannel);
                    midiChannelMap[i].erase(pEngineChannel);
                }
            }
        }
        catch(...) { /* NOOP */ }

        // inform engine channel about the disconnection (if there is one)
        if (bChannelFound) pEngineChannel->Disconnect(this);

        // mark engine channel as changed
        pEngineChannel->StatusChanged(true);
    }

    SynchronizedConfig<std::set<LinuxSampler::Engine*> > MidiInputPort::SysexListeners;

    void MidiInputPort::AddSysexListener(Engine* engine) {
        std::pair<std::set<Engine*>::iterator, bool> p = SysexListeners.GetConfigForUpdate().insert(engine);
        if (p.second) SysexListeners.SwitchConfig().insert(engine);
    }

    bool MidiInputPort::RemoveSysexListener(Engine* engine) {
        size_t count = SysexListeners.GetConfigForUpdate().erase(engine);
        if (count) SysexListeners.SwitchConfig().erase(engine);
        return count;
    }

    void MidiInputPort::Connect(VirtualMidiDevice* pDevice) {
        LockGuard lock(virtualMidiDevicesMutex);
        // double buffer ... double work ...
        {
            std::vector<VirtualMidiDevice*>& devices =
                virtualMidiDevices.GetConfigForUpdate();
            devices.push_back(pDevice);
        }
        {
            std::vector<VirtualMidiDevice*>& devices =
                virtualMidiDevices.SwitchConfig();
            devices.push_back(pDevice);
        }
    }

    void MidiInputPort::Disconnect(VirtualMidiDevice* pDevice) {
        LockGuard lock(virtualMidiDevicesMutex);
        // double buffer ... double work ...
        {
            std::vector<VirtualMidiDevice*>& devices =
                virtualMidiDevices.GetConfigForUpdate();
            devices.erase(std::find(devices.begin(), devices.end(), pDevice));
        }
        {
            std::vector<VirtualMidiDevice*>& devices =
                virtualMidiDevices.SwitchConfig();
            devices.erase(std::find(devices.begin(), devices.end(), pDevice));
        }
    }

    int MidiInputPort::expectedEventSize(unsigned char byte) {
        if (!(byte & 0x80) && runningStatusBuf[0])
            byte = runningStatusBuf[0]; // "running status" mode
        
        if (byte < 0x80) return -1; // not a valid status byte
        if (byte < 0xC0) return 3; // note on/off, note pressure, control change
        if (byte < 0xE0) return 2; // program change, channel pressure
        if (byte < 0xF0) return 3; // pitch wheel
        if (byte == 0xF0) return -1; // sysex message (variable size)
        if (byte == 0xF1) return 2; // time code per quarter frame
        if (byte == 0xF2) return 3; // sys. common song position pointer
        if (byte == 0xF3) return 2; // sys. common song select
        if (byte == 0xF4) return -1; // sys. common undefined / reserved
        if (byte == 0xF5) return -1; // sys. common undefined / reserved
        return 1; // tune request, end of SysEx, system real-time events
    }

} // namespace LinuxSampler
