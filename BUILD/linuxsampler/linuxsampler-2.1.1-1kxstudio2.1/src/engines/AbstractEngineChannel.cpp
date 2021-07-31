/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Christian Schoenebeck and Grigor Iliev        *
 *   Copyright (C) 2012-2017 Christian Schoenebeck and Andreas Persson     *
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

#include "AbstractEngineChannel.h"
#include "../common/global_private.h"
#include "../Sampler.h"

namespace LinuxSampler {

    AbstractEngineChannel::AbstractEngineChannel() :
        virtualMidiDevicesReader_AudioThread(virtualMidiDevices),
        virtualMidiDevicesReader_MidiThread(virtualMidiDevices)
    {
        pEngine      = NULL;
        pEvents      = NULL; // we allocate when we retrieve the right Engine object
        delayedEvents.pList = NULL;
        pEventQueue  = new RingBuffer<Event,false>(CONFIG_MAX_EVENTS_PER_FRAGMENT, 0);
        InstrumentIdx  = -1;
        InstrumentStat = -1;
        pChannelLeft  = NULL;
        pChannelRight = NULL;
        AudioDeviceChannelLeft  = -1;
        AudioDeviceChannelRight = -1;
        midiChannel = midi_chan_all;
        ResetControllers();
        PortamentoMode = false;
        PortamentoTime = CONFIG_PORTAMENTO_TIME_DEFAULT;
        pScript = NULL;
    }

    AbstractEngineChannel::~AbstractEngineChannel() {
        delete pEventQueue;
        DeleteGroupEventLists();
        RemoveAllFxSends();
    }

    Engine* AbstractEngineChannel::GetEngine() {
        return pEngine;
    }

    uint AbstractEngineChannel::Channels() {
        return 2;
    }

    /**
     * More or less a workaround to set the instrument name, index and load
     * status variable to zero percent immediately, that is without blocking
     * the calling thread. It might be used in future for other preparations
     * as well though.
     *
     * @param FileName   - file name of the instrument file
     * @param Instrument - index of the instrument in the file
     * @see LoadInstrument()
     */
    void AbstractEngineChannel::PrepareLoadInstrument(const char* FileName, uint Instrument) {
        InstrumentFile = FileName;
        InstrumentIdx  = Instrument;
        InstrumentStat = 0;
    }

    String AbstractEngineChannel::InstrumentFileName() {
        return InstrumentFile;
    }

    String AbstractEngineChannel::InstrumentName() {
        return InstrumentIdxName;
    }

    int AbstractEngineChannel::InstrumentIndex() {
        return InstrumentIdx;
    }

    int AbstractEngineChannel::InstrumentStatus() {
        return InstrumentStat;
    }

    String AbstractEngineChannel::EngineName() {
        return AbstractEngine::GetFormatString(GetEngineFormat());
    }

    void AbstractEngineChannel::Reset() {
        if (pEngine) pEngine->DisableAndLock();
        ResetInternal(false/*don't reset engine*/);
        ResetControllers();
        if (pEngine) {
            pEngine->Enable();
            pEngine->Reset();
        }
    }

    void AbstractEngineChannel::ResetControllers() {
        Pitch          = 0;
        GlobalVolume   = 1.0f;
        MidiVolume     = 1.0;
        iLastPanRequest = 64;
        GlobalTranspose = 0;
        // set all MIDI controller values to zero
        memset(ControllerTable, 0x00, 129);
        // reset all FX Send levels
        for (
            std::vector<FxSend*>::iterator iter = fxSends.begin();
            iter != fxSends.end(); iter++
        ) {
            (*iter)->Reset();
        }
    }

    /**
     * This method is not thread safe!
     */
    void AbstractEngineChannel::ResetInternal(bool bResetEngine) {
        CurrentKeyDimension = 0;
        PortamentoPos = -1.0f; // no portamento active yet

        // delete all active instrument script events
        if (pScript) pScript->resetEvents();

        // free all delayed MIDI events
        delayedEvents.clear();

        // delete all input events
        pEventQueue->init();

        if (bResetEngine && pEngine) pEngine->ResetInternal();

        // status of engine channel has changed, so set notify flag
        bStatusChanged = true;
    }

    /**
     * Implementation of virtual method from abstract EngineChannel interface.
     * This method will periodically be polled (e.g. by the LSCP server) to
     * check if some engine channel parameter has changed since the last
     * StatusChanged() call.
     *
     * This method can also be used to mark the engine channel as changed
     * from outside, e.g. by a MIDI input device. The optional argument
     * \a nNewStatus can be used for this.
     *
     * TODO: This "poll method" is just a lazy solution and might be
     *       replaced in future.
     * @param bNewStatus - (optional, default: false) sets the new status flag
     * @returns true if engine channel status has changed since last
     *          StatusChanged() call
     */
    bool AbstractEngineChannel::StatusChanged(bool bNewStatus) {
        bool b = bStatusChanged;
        bStatusChanged = bNewStatus;
        return b;
    }

    float AbstractEngineChannel::Volume() {
        return GlobalVolume;
    }

    void AbstractEngineChannel::Volume(float f) {
        GlobalVolume = f;
        bStatusChanged = true; // status of engine channel has changed, so set notify flag
    }

    float AbstractEngineChannel::Pan() {
        return float(iLastPanRequest - 64) / 64.0f;
    }

    void AbstractEngineChannel::Pan(float f) {
        int iMidiPan = int(f * 64.0f) + 64;
        if (iMidiPan > 127) iMidiPan = 127;
        else if (iMidiPan < 0) iMidiPan = 0;
        iLastPanRequest = iMidiPan;
    }

    AudioOutputDevice* AbstractEngineChannel::GetAudioOutputDevice() {
        return (pEngine) ? pEngine->pAudioOutputDevice : NULL;
    }

    /**
     * Gets thread safe access to the currently connected audio output
     * device from other threads than the lscp thread.
     */
    AudioOutputDevice* AbstractEngineChannel::GetAudioOutputDeviceSafe() {
        LockGuard lock(EngineMutex);
        return GetAudioOutputDevice();
    }

    void AbstractEngineChannel::SetOutputChannel(uint EngineAudioChannel, uint AudioDeviceChannel) {
        if (!pEngine || !pEngine->pAudioOutputDevice) throw AudioOutputException("No audio output device connected yet.");

        AudioChannel* pChannel = pEngine->pAudioOutputDevice->Channel(AudioDeviceChannel);
        if (!pChannel) throw AudioOutputException("Invalid audio output device channel " + ToString(AudioDeviceChannel));
        switch (EngineAudioChannel) {
            case 0: // left output channel
                if (fxSends.empty()) pChannelLeft = pChannel;
                AudioDeviceChannelLeft = AudioDeviceChannel;
                break;
            case 1: // right output channel
                if (fxSends.empty()) pChannelRight = pChannel;
                AudioDeviceChannelRight = AudioDeviceChannel;
                break;
            default:
                throw AudioOutputException("Invalid engine audio channel " + ToString(EngineAudioChannel));
        }

        bStatusChanged = true;
    }

    int AbstractEngineChannel::OutputChannel(uint EngineAudioChannel) {
        switch (EngineAudioChannel) {
            case 0: // left channel
                return AudioDeviceChannelLeft;
            case 1: // right channel
                return AudioDeviceChannelRight;
            default:
                throw AudioOutputException("Invalid engine audio channel " + ToString(EngineAudioChannel));
        }
    }

    void AbstractEngineChannel::Connect(MidiInputPort* pMidiPort) {
        if (!pMidiPort) return;

        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();

        // check if connection already exists
        for (int i = 0; i < connections->size(); ++i)
            if ((*connections)[i] == pMidiPort)
                return; // to avoid endless recursion

        connections->add(pMidiPort);

        // inform MIDI port about this new connection
        pMidiPort->Connect(this, MidiChannel());
    }

    void AbstractEngineChannel::Disconnect(MidiInputPort* pMidiPort) {
        if (!pMidiPort) return;

        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();

        for (int i = 0; i < connections->size(); ++i) {
            if ((*connections)[i] == pMidiPort) {
                connections->remove(i);
                // inform MIDI port about this disconnection
                pMidiPort->Disconnect(this);
                return;
            }
        }
    }

    void AbstractEngineChannel::DisconnectAllMidiInputPorts() {
        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();
        ArrayList<MidiInputPort*> clonedList = *connections;
        connections->clear();
        for (int i = 0; i < clonedList.size(); ++i) clonedList[i]->Disconnect(this);
    }

    uint AbstractEngineChannel::GetMidiInputPortCount() {
        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();
        return connections->size();
    }

    MidiInputPort* AbstractEngineChannel::GetMidiInputPort(uint index) {
        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();
        return (index < connections->size()) ? (*connections)[index] : NULL;
    }

    // deprecated (just for API backward compatibility) - may be removed in future
    void AbstractEngineChannel::Connect(MidiInputPort* pMidiPort, midi_chan_t MidiChannel) {
        if (!pMidiPort) return;

        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();

        // check against endless recursion
        if (connections->size() == 1 && (*connections)[0] == pMidiPort && this->midiChannel == MidiChannel)
            return;
        
        if (!isValidMidiChan(MidiChannel))
            throw MidiInputException("Invalid MIDI channel (" + ToString(int(MidiChannel)) + ")");

        this->midiChannel = MidiChannel;

        // disconnect all currently connected MIDI ports
        ArrayList<MidiInputPort*> clonedList = *connections;
        connections->clear();
        for (int i = 0; i < clonedList.size(); ++i)
            clonedList[i]->Disconnect(this);

        // connect the new port
        connections->add(pMidiPort);
        pMidiPort->Connect(this, MidiChannel);
    }

    // deprecated (just for API backward compatibility) - may be removed in future
    void AbstractEngineChannel::DisconnectMidiInputPort() {
        DisconnectAllMidiInputPorts();
    }

    // deprecated (just for API backward compatibility) - may be removed in future
    MidiInputPort* AbstractEngineChannel::GetMidiInputPort() {
        return GetMidiInputPort(0);
    }

    midi_chan_t AbstractEngineChannel::MidiChannel() {
        return midiChannel;
    }

    void AbstractEngineChannel::SetMidiChannel(midi_chan_t MidiChannel) {
        if (this->midiChannel == MidiChannel) return;
        if (!isValidMidiChan(MidiChannel))
            throw MidiInputException("Invalid MIDI channel (" + ToString(int(MidiChannel)) + ")");

        this->midiChannel = MidiChannel;
        
        Sync< ArrayList<MidiInputPort*> > connections = midiInputs.back();
        ArrayList<MidiInputPort*> clonedList = *connections;

        DisconnectAllMidiInputPorts();

        for (int i = 0; i < clonedList.size(); ++i) Connect(clonedList[i]);
    }

    void AbstractEngineChannel::Connect(VirtualMidiDevice* pDevice) {
        // double buffer ... double work ...
        {
            ArrayList<VirtualMidiDevice*>& devices = virtualMidiDevices.GetConfigForUpdate();
            devices.add(pDevice);
        }
        {
            ArrayList<VirtualMidiDevice*>& devices = virtualMidiDevices.SwitchConfig();
            devices.add(pDevice);
        }
    }

    void AbstractEngineChannel::Disconnect(VirtualMidiDevice* pDevice) {
        // double buffer ... double work ...
        {
            ArrayList<VirtualMidiDevice*>& devices = virtualMidiDevices.GetConfigForUpdate();
            devices.remove(pDevice);
        }
        {
            ArrayList<VirtualMidiDevice*>& devices = virtualMidiDevices.SwitchConfig();
            devices.remove(pDevice);
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to let the audio thread trigger a new
     *  voice for the given key. This method is meant for real time rendering,
     *  that is an event will immediately be created with the current system
     *  time as time stamp.
     *
     *  @param Key      - MIDI key number of the triggered key
     *  @param Velocity - MIDI velocity value of the triggered key
     */
    void AbstractEngineChannel::SendNoteOn(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent();
            event.Type                = Event::type_note_on;
            event.Param.Note.Key      = Key;
            event.Param.Note.Velocity = Velocity;
            event.Param.Note.Channel  = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("EngineChannel: Input event queue full!"));
            // inform connected virtual MIDI devices if any ...
            // (e.g. virtual MIDI keyboard in instrument editor(s))
            ArrayList<VirtualMidiDevice*>& devices =
                const_cast<ArrayList<VirtualMidiDevice*>&>(
                    virtualMidiDevicesReader_MidiThread.Lock()
                );
            for (int i = 0; i < devices.size(); i++) {
                devices[i]->SendNoteOnToDevice(Key, Velocity);
            }
            virtualMidiDevicesReader_MidiThread.Unlock();
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to let the audio thread trigger a new
     *  voice for the given key. This method is meant for offline rendering
     *  and / or for cases where the exact position of the event in the current
     *  audio fragment is already known.
     *
     *  @param Key         - MIDI key number of the triggered key
     *  @param Velocity    - MIDI velocity value of the triggered key
     *  @param FragmentPos - sample point position in the current audio
     *                       fragment to which this event belongs to
     */
    void AbstractEngineChannel::SendNoteOn(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel, int32_t FragmentPos) {
        if (FragmentPos < 0) {
            dmsg(1,("EngineChannel::SendNoteOn(): negative FragmentPos! Seems MIDI driver is buggy!"));
        }
        else if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type                = Event::type_note_on;
            event.Param.Note.Key      = Key;
            event.Param.Note.Velocity = Velocity;
            event.Param.Note.Channel  = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("EngineChannel: Input event queue full!"));
            // inform connected virtual MIDI devices if any ...
            // (e.g. virtual MIDI keyboard in instrument editor(s))
            ArrayList<VirtualMidiDevice*>& devices =
                const_cast<ArrayList<VirtualMidiDevice*>&>(
                    virtualMidiDevicesReader_MidiThread.Lock()
                );
            for (int i = 0; i < devices.size(); i++) {
                devices[i]->SendNoteOnToDevice(Key, Velocity);
            }
            virtualMidiDevicesReader_MidiThread.Unlock();
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread to release
     *  voice(s) on the given key. This method is meant for real time rendering,
     *  that is an event will immediately be created with the current system
     *  time as time stamp.
     *
     *  @param Key      - MIDI key number of the released key
     *  @param Velocity - MIDI release velocity value of the released key
     */
    void AbstractEngineChannel::SendNoteOff(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent();
            event.Type                = Event::type_note_off;
            event.Param.Note.Key      = Key;
            event.Param.Note.Velocity = Velocity;
            event.Param.Note.Channel  = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("EngineChannel: Input event queue full!"));
            // inform connected virtual MIDI devices if any ...
            // (e.g. virtual MIDI keyboard in instrument editor(s))
            ArrayList<VirtualMidiDevice*>& devices =
                const_cast<ArrayList<VirtualMidiDevice*>&>(
                    virtualMidiDevicesReader_MidiThread.Lock()
                );
            for (int i = 0; i < devices.size(); i++) {
                devices[i]->SendNoteOffToDevice(Key, Velocity);
            }
            virtualMidiDevicesReader_MidiThread.Unlock();
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread to release
     *  voice(s) on the given key. This method is meant for offline rendering
     *  and / or for cases where the exact position of the event in the current
     *  audio fragment is already known.
     *
     *  @param Key         - MIDI key number of the released key
     *  @param Velocity    - MIDI release velocity value of the released key
     *  @param FragmentPos - sample point position in the current audio
     *                       fragment to which this event belongs to
     */
    void AbstractEngineChannel::SendNoteOff(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel, int32_t FragmentPos) {
        if (FragmentPos < 0) {
            dmsg(1,("EngineChannel::SendNoteOff(): negative FragmentPos! Seems MIDI driver is buggy!"));
        }
        else if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type                = Event::type_note_off;
            event.Param.Note.Key      = Key;
            event.Param.Note.Velocity = Velocity;
            event.Param.Note.Channel  = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("EngineChannel: Input event queue full!"));
            // inform connected virtual MIDI devices if any ...
            // (e.g. virtual MIDI keyboard in instrument editor(s))
            ArrayList<VirtualMidiDevice*>& devices =
                const_cast<ArrayList<VirtualMidiDevice*>&>(
                    virtualMidiDevicesReader_MidiThread.Lock()
                );
            for (int i = 0; i < devices.size(); i++) {
                devices[i]->SendNoteOffToDevice(Key, Velocity);
            }
            virtualMidiDevicesReader_MidiThread.Unlock();
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread to change
     *  the pitch value for all voices. This method is meant for real time
     *  rendering, that is an event will immediately be created with the
     *  current system time as time stamp.
     *
     *  @param Pitch - MIDI pitch value (-8192 ... +8191)
     */
    void AbstractEngineChannel::SendPitchbend(int Pitch, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event             = pEngine->pEventGenerator->CreateEvent();
            event.Type              = Event::type_pitchbend;
            event.Param.Pitch.Pitch = Pitch;
            event.Param.Pitch.Channel = MidiChannel;
            event.pEngineChannel    = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("EngineChannel: Input event queue full!"));
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread to change
     *  the pitch value for all voices. This method is meant for offline
     *  rendering and / or for cases where the exact position of the event in
     *  the current audio fragment is already known.
     *
     *  @param Pitch       - MIDI pitch value (-8192 ... +8191)
     *  @param FragmentPos - sample point position in the current audio
     *                       fragment to which this event belongs to
     */
    void AbstractEngineChannel::SendPitchbend(int Pitch, uint8_t MidiChannel, int32_t FragmentPos) {
        if (FragmentPos < 0) {
            dmsg(1,("AbstractEngineChannel::SendPitchBend(): negative FragmentPos! Seems MIDI driver is buggy!"));
        }
        else if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event             = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type              = Event::type_pitchbend;
            event.Param.Pitch.Pitch = Pitch;
            event.Param.Pitch.Channel = MidiChannel;
            event.pEngineChannel    = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread that a
     *  continuous controller value has changed. This method is meant for real
     *  time rendering, that is an event will immediately be created with the
     *  current system time as time stamp.
     *
     *  @param Controller - MIDI controller number of the occured control change
     *  @param Value      - value of the control change
     */
    void AbstractEngineChannel::SendControlChange(uint8_t Controller, uint8_t Value, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent();
            event.Type                = Event::type_control_change;
            event.Param.CC.Controller = Controller;
            event.Param.CC.Value      = Value;
            event.Param.CC.Channel    = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    /**
     *  Will be called by the MIDIIn Thread to signal the audio thread that a
     *  continuous controller value has changed. This method is meant for
     *  offline rendering and / or for cases where the exact position of the
     *  event in the current audio fragment is already known.
     *
     *  @param Controller  - MIDI controller number of the occured control change
     *  @param Value       - value of the control change
     *  @param FragmentPos - sample point position in the current audio
     *                       fragment to which this event belongs to
     */
    void AbstractEngineChannel::SendControlChange(uint8_t Controller, uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) {
        if (FragmentPos < 0) {
            dmsg(1,("AbstractEngineChannel::SendControlChange(): negative FragmentPos! Seems MIDI driver is buggy!"));
        }
        else if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event               = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type                = Event::type_control_change;
            event.Param.CC.Controller = Controller;
            event.Param.CC.Value      = Value;
            event.Param.CC.Channel    = MidiChannel;
            event.pEngineChannel      = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    void AbstractEngineChannel::SendChannelPressure(uint8_t Value, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event = pEngine->pEventGenerator->CreateEvent();
            event.Type                          = Event::type_channel_pressure;
            event.Param.ChannelPressure.Controller = CTRL_TABLE_IDX_AFTERTOUCH; // required for instrument scripts
            event.Param.ChannelPressure.Value   = Value;
            event.Param.ChannelPressure.Channel = MidiChannel;
            event.pEngineChannel                = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    void AbstractEngineChannel::SendChannelPressure(uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type                          = Event::type_channel_pressure;
            event.Param.ChannelPressure.Controller = CTRL_TABLE_IDX_AFTERTOUCH; // required for instrument scripts
            event.Param.ChannelPressure.Value   = Value;
            event.Param.ChannelPressure.Channel = MidiChannel;
            event.pEngineChannel                = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    void AbstractEngineChannel::SendPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint8_t MidiChannel) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event = pEngine->pEventGenerator->CreateEvent();
            event.Type                       = Event::type_note_pressure;
            event.Param.NotePressure.Key     = Key;
            event.Param.NotePressure.Value   = Value;
            event.Param.NotePressure.Channel = MidiChannel;
            event.pEngineChannel             = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    void AbstractEngineChannel::SendPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) {
        if (pEngine) {
            // protection in case there are more than 1 MIDI input threads sending MIDI events to this EngineChannel
            LockGuard g;
            if (hasMultipleMIDIInputs()) g = LockGuard(MidiInputMutex);

            Event event = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            event.Type                       = Event::type_note_pressure;
            event.Param.NotePressure.Key     = Key;
            event.Param.NotePressure.Value   = Value;
            event.Param.NotePressure.Channel = MidiChannel;
            event.pEngineChannel             = this;
            if (this->pEventQueue->write_space() > 0) this->pEventQueue->push(&event);
            else dmsg(1,("AbstractEngineChannel: Input event queue full!"));
        }
    }

    bool AbstractEngineChannel::applyTranspose(Event* event) {
        if (event->Type != Event::type_note_on && event->Type != Event::type_note_off)
            return true; // event OK (not a note event, nothing to do with it here)

        //HACK: we should better add the transpose value only to the most mandatory places (like for retrieving the region and calculating the tuning), because otherwise voices will unintendedly survive when changing transpose while playing
        const int k = event->Param.Note.Key + GlobalTranspose;
        if (k < 0 || k > 127)
            return false; // bad event, drop it

        event->Param.Note.Key = k;

        return true; // event OK
    }

    /**
     * Copy all events from the engine channel's input event queue buffer to
     * the internal event list. This will be done at the beginning of each
     * audio cycle (that is each RenderAudio() call) to distinguish all
     * events which have to be processed in the current audio cycle. Each
     * EngineChannel has it's own input event queue for the common channel
     * specific events (like NoteOn, NoteOff and ControlChange events).
     * Beside that, the engine also has a input event queue for global
     * events (usually SysEx messages).
     *
     * @param Samples - number of sample points to be processed in the
     *                  current audio cycle
     */
    void AbstractEngineChannel::ImportEvents(uint Samples) {
        // import events from pure software MIDI "devices"
        // (e.g. virtual keyboard in instrument editor)
        {
            const uint8_t channel = MidiChannel() == midi_chan_all ? 0 : MidiChannel();
            const int FragmentPos = 0; // randomly chosen, we don't care about jitter for virtual MIDI devices
            const Event event = pEngine->pEventGenerator->CreateEvent(FragmentPos);
            VirtualMidiDevice::event_t devEvent; // the event format we get from the virtual MIDI device
            // as we're going to (carefully) write some status to the
            // synchronized struct, we cast away the const
            ArrayList<VirtualMidiDevice*>& devices =
                const_cast<ArrayList<VirtualMidiDevice*>&>(virtualMidiDevicesReader_AudioThread.Lock());
            // iterate through all virtual MIDI devices
            for (int i = 0; i < devices.size(); i++) {
                VirtualMidiDevice* pDev = devices[i];
                // I think we can simply flush the whole FIFO(s), the user shouldn't be so fast ;-)
                while (pDev->GetMidiEventFromDevice(devEvent)) {
                    if (pEvents->poolIsEmpty()) {
                        dmsg(1,("Event pool emtpy!\n"));
                        goto exitVirtualDevicesLoop;
                    }

                    // copy event to internal event list (this is already
                    // required here, because the LaunchNewNote() call below
                    // requires the event to be from the internal event pool for
                    // being able to generate a valid event ID)
                    RTList<Event>::Iterator itEvent = pEvents->allocAppend();
                    *itEvent = event;

                    itEvent->pEngineChannel = this;

                    switch (devEvent.Type) {
                        case VirtualMidiDevice::EVENT_TYPE_NOTEON:
                            itEvent->Type = Event::type_note_on;
                            itEvent->Param.Note.Key      = devEvent.Arg1;
                            itEvent->Param.Note.Velocity = devEvent.Arg2;
                            itEvent->Param.Note.Channel  = channel;
                            // apply transpose setting to (note on/off) event
                            if (!applyTranspose(&*itEvent)) {
                                // note value is out of range, so drop this event
                                pEvents->free(itEvent);
                                continue;
                            }
                            // assign a new note to this note-on event
                            if (!pEngine->LaunchNewNote(this, itEvent)) {
                                // failed launching new note, so drop this event
                                pEvents->free(itEvent);
                                continue;
                            }
                            break;
                        case VirtualMidiDevice::EVENT_TYPE_NOTEOFF:
                            itEvent->Type = Event::type_note_off;
                            itEvent->Param.Note.Key      = devEvent.Arg1;
                            itEvent->Param.Note.Velocity = devEvent.Arg2;
                            itEvent->Param.Note.Channel  = channel;
                            if (!applyTranspose(&*itEvent)) {
                                // note value is out of range, so drop this event
                                pEvents->free(itEvent);
                                continue;
                            }
                            break;
                        case VirtualMidiDevice::EVENT_TYPE_CC:
                            switch (devEvent.Arg1) {
                                case 0: // bank select MSB ...
                                    SetMidiBankMsb(devEvent.Arg2);
                                    // don't push this event into FIFO
                                    pEvents->free(itEvent);
                                    continue;
                                case 32: // bank select LSB ...
                                    SetMidiBankLsb(devEvent.Arg2);
                                    // don't push this event into FIFO
                                    pEvents->free(itEvent);
                                    continue;
                                default: // regular MIDI CC ...
                                    itEvent->Type = Event::type_control_change;
                                    itEvent->Param.CC.Controller = devEvent.Arg1;
                                    itEvent->Param.CC.Value      = devEvent.Arg2;
                                    itEvent->Param.CC.Channel    = channel;
                            }
                            break;
                        case VirtualMidiDevice::EVENT_TYPE_PITCHBEND:
                            itEvent->Type = Event::type_pitchbend;
                            itEvent->Param.Pitch.Pitch = int(devEvent.Arg2 << 7 | devEvent.Arg1) - 8192;
                            itEvent->Param.Pitch.Channel = channel;
                            break;
                        case VirtualMidiDevice::EVENT_TYPE_PROGRAM:
                            SendProgramChange(devEvent.Arg1);
                            // don't push this event into FIFO
                            pEvents->free(itEvent);
                            continue;
                        case VirtualMidiDevice::EVENT_TYPE_CHPRESSURE:
                            itEvent->Type = Event::type_channel_pressure;
                            itEvent->Param.ChannelPressure.Controller = CTRL_TABLE_IDX_AFTERTOUCH;
                            itEvent->Param.ChannelPressure.Value   = devEvent.Arg2;
                            itEvent->Param.ChannelPressure.Channel = channel;
                            break;
                        default:
                            std::cerr << "AbstractEngineChannel::ImportEvents() ERROR: unknown event type ("
                                      << devEvent.Type << "). This is a bug!";
                            pEvents->free(itEvent); // drop event
                            continue;
                    }
                }
            }
        }
        exitVirtualDevicesLoop:
        virtualMidiDevicesReader_AudioThread.Unlock();

        // import events from the regular MIDI devices
        RingBuffer<Event,false>::NonVolatileReader eventQueueReader = pEventQueue->get_non_volatile_reader();
        Event* pEvent;
        while (true) {
            // get next event from input event queue
            if (!(pEvent = eventQueueReader.pop())) break;
            // if younger event reached, ignore that and all subsequent ones for now
            if (pEvent->FragmentPos() >= Samples) {
                eventQueueReader--;
                dmsg(2,("Younger Event, pos=%d ,Samples=%d!\n",pEvent->FragmentPos(),Samples));
                pEvent->ResetFragmentPos();
                break;
            }
            if (pEvents->poolIsEmpty()) {
                dmsg(1,("Event pool emtpy!\n"));
                break;
            }

            // copy event to internal event list
            // (required already because LaunchNewNote() relies on it, see
            // comment about it above)
            RTList<Event>::Iterator itEvent = pEvents->allocAppend();
            *itEvent = *pEvent;

            // apply transpose setting to (note on/off) event
            if (!applyTranspose(&*itEvent)) {
                // it's a note event which has a note value out of range, so drop this event
                pEvents->free(itEvent);
                continue;
            }
            // assign a new note to this event (if its a note-on event)
            if (itEvent->Type == Event::type_note_on) {
                if (!pEngine->LaunchNewNote(this, itEvent)) {
                    // failed launching new note, so drop this event
                    pEvents->free(itEvent);
                    continue;
                }
            }

        }
        eventQueueReader.free(); // free all copied events from input queue
    }

    /**
     * Called by real-time instrument script functions to schedule a new event
     * @a delay microseconds in future.
     * 
     * @b IMPORTANT: for the supplied @a delay to be scheduled correctly, the
     * passed @a pEvent must be assigned a valid fragment time within the
     * current audio fragment boundaries. That fragment time will be used by
     * this method as basis for interpreting what "now" acutally is, and thus
     * it will be used as basis for calculating the precise scheduling time
     * for @a delay. The easiest way to achieve this is by copying a recent
     * event which happened within the current audio fragment cycle: i.e. the
     * original event which caused calling this method here.
     *
     * @param pEvent - event to be scheduled in future (event data will be copied)
     * @param delay - amount of microseconds in future (from now) when event shall be processed
     * @returns unique event ID of scheduled new event, or NULL on error
     */
    event_id_t AbstractEngineChannel::ScheduleEventMicroSec(const Event* pEvent, int delay) {
        dmsg(3,("AbstractEngineChannel::ScheduleEventMicroSec(Event.Type=%d,delay=%d)\n", pEvent->Type, delay));
        RTList<Event>::Iterator itEvent = pEvents->allocAppend();
        if (!itEvent) {
            dmsg(1,("AbstractEngineChannel::ScheduleEventMicroSec(): Event pool emtpy!\n"));
            return 0;
        }
        RTList<ScheduledEvent>::Iterator itNode = delayedEvents.schedulerNodes.allocAppend();
        if (!itNode) { // scheduler node pool empty ...
            dmsg(1,("AbstractEngineChannel::ScheduleEventMicroSec(): ScheduledEvent pool empty!\n"));
            pEvents->free(itEvent);
            return 0;
        }
        // copy passed event
        *itEvent = *pEvent;
        // move copied event to list of delayed events
        itEvent = itEvent.moveToEndOf(delayedEvents.pList);
        // connect scheduler node with the copied event
        itNode->itEvent = itEvent;
        // add entry to time sorted scheduler queue for copied event
        pEngine->pEventGenerator->scheduleAheadMicroSec(
            delayedEvents.queue, *itNode, itEvent->FragmentPos(), delay
        );
        //dmsg(5,("ScheduledEvent queue size: %d\n", delayedEvents.queue.size()));
        return pEvents->getID(itEvent);
    }

    /**
     * Called by real-time instrument script functions to ignore the event
     * reflected by given event ID. The event will be freed immediately to its
     * pool and cannot be dereferenced by its old ID anymore. Even if its
     * allocated back from the Pool later on, it will have a different ID.
     *
     * @param id - unique ID of event to be dropped
     */
    void AbstractEngineChannel::IgnoreEvent(event_id_t id) {
        RTList<Event>::Iterator it = pEvents->fromID(id);
        if (it) pEvents->free(it);
    }

    /** @brief Drop the requested event.
     *
     * Called by real-time instrument script functions to ignore the event
     * reflected by the given event @a id. This method detects whether the
     * passed ID is actually a @c Note ID or a regular @c Event ID and act
     * accordingly.
     *
     * @param id - event id (from script scope)
     * @see ScriptID
     */
    void AbstractEngineChannel::IgnoreEventByScriptID(const ScriptID& id) {
        switch (id.type()) {
            case ScriptID::EVENT:
                IgnoreEvent( id.eventID() );
                break;
            case ScriptID::NOTE:
                IgnoreNote( id.noteID() );
                break;
        }
    }

    /** @brief Order resuming of script execution instance "now".
     *
     * Called by real-time instrument script function stop_wait() to resume a
     * script callback currently being suspended (i.e. due to a wait() script
     * function call).
     *
     * @param itCallback - suspended script callback to be resumed
     * @param now - current scheduler time to be "now"
     * @param forever - whether this particulare script callback should ignore
     *                  all subsequent wait*() script function calls
     */
    void AbstractEngineChannel::ScheduleResumeOfScriptCallback(RTList<ScriptEvent>::Iterator& itCallback, sched_time_t now, bool forever) {
        // ignore if invalid iterator was passed
        if (!itCallback) return;

        ScriptEvent* pCallback = &*itCallback;

        // mark this callback to ignore all subsequent built-in wait*() script function calls
        if (forever) pCallback->ignoreAllWaitCalls = true;

        // ignore if callback is not in the scheduler queue
        if (pCallback->currentSchedulerQueue() != &pScript->suspendedEvents) return;

        // ignore if callback is already scheduled to be resumed "now"
        if (pCallback->scheduleTime <= now) return;

        // take it out from the scheduler queue and re-insert callback
        // to schedule the script callback for resuming execution "now"
        pScript->suspendedEvents.erase(*pCallback);
        pCallback->scheduleTime = now + 1;
        pScript->suspendedEvents.insert(*pCallback);
    }

    /** @brief Fork the given script execution instance.
     *
     * Called by real-time instrument script function fork() to create a new
     * script execution instance (child) of the script execution instance
     * (parent) that was calling fork(). This is essentially like creating a
     * new thread for a script handler being executing. The entire execution
     * state of parent is copied to the "forked" child.
     *
     * @param parent - original active script callback instance from which the
     *                 new child shall be forked from
     * @param bAutoAbort - whether the forked child shall automatically be
     *                     terminated as soon as parent terminates
     * @returns forked new child execution instance
     */
    RTList<ScriptEvent>::Iterator AbstractEngineChannel::forkScriptCallback(ScriptEvent* parent, bool bAutoAbort) {
        // check if the max. amount of child forks for this parent event handler
        // instance have not been exceeded yet
        if (parent->countChildHandlers() >= MAX_FORK_PER_SCRIPT_HANDLER)
            return RTList<ScriptEvent>::Iterator();

        // allocate a new script callback instance for child to be forked
        RTList<ScriptEvent>::Iterator itChild = pScript->pEvents->allocAppend();
        if (!itChild) return itChild;

        // copy entire script handler state from parent to forked child
        parent->forkTo(&*itChild, bAutoAbort);

        // stick the parent ID and child ID respectively to each other
        itChild->parentHandlerID = GetScriptCallbackID(parent);
        parent->addChildHandlerID( GetScriptCallbackID(&*itChild) );

        // insert newly created (forked) child event handler instance to the
        // scheduler queue for being executed soon
        pEngine->pEventGenerator->scheduleAheadMicroSec(
            pScript->suspendedEvents, // scheduler queue
            *itChild, // script event
            parent->cause.FragmentPos(), // current time of script event (basis for its next execution)
            0 // "resume" new child script callback instance ASAP
        );

        return itChild;
    }

    FxSend* AbstractEngineChannel::AddFxSend(uint8_t MidiCtrl, String Name) throw (Exception) {
        if (pEngine) pEngine->DisableAndLock();
        FxSend* pFxSend = new FxSend(this, MidiCtrl, Name);
        if (fxSends.empty()) {
            if (pEngine && pEngine->pAudioOutputDevice) {
                AudioOutputDevice* pDevice = pEngine->pAudioOutputDevice;
                // create local render buffers
                pChannelLeft  = new AudioChannel(0, pDevice->MaxSamplesPerCycle());
                pChannelRight = new AudioChannel(1, pDevice->MaxSamplesPerCycle());
            } else {
                // postpone local render buffer creation until audio device is assigned
                pChannelLeft  = NULL;
                pChannelRight = NULL;
            }
        }
        fxSends.push_back(pFxSend);
        if (pEngine) pEngine->Enable();
        fireFxSendCountChanged(GetSamplerChannel()->Index(), GetFxSendCount());

        return pFxSend;
    }

    FxSend* AbstractEngineChannel::GetFxSend(uint FxSendIndex) {
        return (FxSendIndex < fxSends.size()) ? fxSends[FxSendIndex] : NULL;
    }

    uint AbstractEngineChannel::GetFxSendCount() {
        return (uint)fxSends.size();
    }

    void AbstractEngineChannel::RemoveFxSend(FxSend* pFxSend) {
        if (pEngine) pEngine->DisableAndLock();
        for (
            std::vector<FxSend*>::iterator iter = fxSends.begin();
            iter != fxSends.end(); iter++
        ) {
            if (*iter == pFxSend) {
                delete pFxSend;
                fxSends.erase(iter);
                if (fxSends.empty()) {
                    // destroy local render buffers
                    if (pChannelLeft)  delete pChannelLeft;
                    if (pChannelRight) delete pChannelRight;
                    // fallback to render directly into AudioOutputDevice's buffers
                    if (pEngine && pEngine->pAudioOutputDevice) {
                        pChannelLeft  = pEngine->pAudioOutputDevice->Channel(AudioDeviceChannelLeft);
                        pChannelRight = pEngine->pAudioOutputDevice->Channel(AudioDeviceChannelRight);
                    } else { // we update the pointers later
                        pChannelLeft  = NULL;
                        pChannelRight = NULL;
                    }
                }
                break;
            }
        }
        if (pEngine) pEngine->Enable();
        fireFxSendCountChanged(GetSamplerChannel()->Index(), GetFxSendCount());
    }

    void AbstractEngineChannel::RemoveAllFxSends() {
        if (pEngine) pEngine->DisableAndLock();
        if (!fxSends.empty()) { // free local render buffers
            if (pChannelLeft) {
                delete pChannelLeft;
                if (pEngine && pEngine->pAudioOutputDevice) {
                    // fallback to render directly to the AudioOutputDevice's buffer
                    pChannelLeft = pEngine->pAudioOutputDevice->Channel(AudioDeviceChannelLeft);
                } else pChannelLeft = NULL;
            }
            if (pChannelRight) {
                delete pChannelRight;
                if (pEngine && pEngine->pAudioOutputDevice) {
                    // fallback to render directly to the AudioOutputDevice's buffer
                    pChannelRight = pEngine->pAudioOutputDevice->Channel(AudioDeviceChannelRight);
                } else pChannelRight = NULL;
            }
        }
        for (int i = 0; i < fxSends.size(); i++) delete fxSends[i];
        fxSends.clear();
        if (pEngine) pEngine->Enable();
    }

    /**
     * Add a group number to the set of key groups. Should be called
     * when an instrument is loaded to make sure there are event lists
     * for all key groups.
     */ 
    void AbstractEngineChannel::AddGroup(uint group) {
        if (group) {
            std::pair<ActiveKeyGroupMap::iterator, bool> p =
                ActiveKeyGroups.insert(ActiveKeyGroupMap::value_type(group, 0));
            if (p.second) {
                // If the engine channel is pending deletion (see bug
                // #113), pEngine will be null, so we can't use
                // pEngine->pEventPool here. Instead we're using a
                // specialized RTList that allows specifying the pool
                // later.
                (*p.first).second = new LazyList<Event>;
            }
        }
    }

    /**
     * Handle key group (a.k.a. exclusive group) conflicts.
     */
    void AbstractEngineChannel::HandleKeyGroupConflicts(uint KeyGroup, Pool<Event>::Iterator& itNoteOnEvent) {
        dmsg(4,("HandelKeyGroupConflicts KeyGroup=%d\n", KeyGroup));
        if (KeyGroup) {
            // send a release event to all active voices in the group
            RTList<Event>::Iterator itEvent = ActiveKeyGroups[KeyGroup]->allocAppend(pEngine->pEventPool);
            *itEvent = *itNoteOnEvent;
        }
    }

    /**
     * Empty the lists of group events. Should be called from the
     * audio thread, after all voices have been rendered.
     */
    void AbstractEngineChannel::ClearGroupEventLists() {
        for (ActiveKeyGroupMap::iterator iter = ActiveKeyGroups.begin();
             iter != ActiveKeyGroups.end(); iter++) {
            if (iter->second) {
                iter->second->clear();
            } else {
                dmsg(1,("EngineChannel: group event list was NULL"));
            }
        }
    }

    /**
     * Remove all lists with group events.
     */
    void AbstractEngineChannel::DeleteGroupEventLists() {
        for (ActiveKeyGroupMap::iterator iter = ActiveKeyGroups.begin();
             iter != ActiveKeyGroups.end(); iter++) {
            delete iter->second;
        }
        ActiveKeyGroups.clear();
    }

} // namespace LinuxSampler
