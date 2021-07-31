/*
    Copyright (C) 2008 - 2016 Christian Schoenebeck
 */

#include "VirtualMidiDevice.h"

#include "../../common/global_private.h"
#include "../../common/atomic.h"
#include "../../common/RingBuffer.h"

#define MIDI_KEYS		128
#define MIDI_CONTROLLERS	128

// assuming VirtualMidiDevice implementation is only controlled
// by mouse (and the user not being Billy the Kid)
#define MAX_EVENTS  12

namespace LinuxSampler {

    struct VirtualMidiDevice::private_data_t {
        atomic_t notesChanged; // whether some key changed at all
        atomic_t pNoteChanged[MIDI_KEYS]; // which key(s) changed
        atomic_t pNoteIsActive[MIDI_KEYS]; // status of each key (either active or inactive)
        atomic_t pNoteOnVelocity[MIDI_KEYS];
        atomic_t pNoteOffVelocity[MIDI_KEYS];
        atomic_t ccsChanged; // whether some controller changed at all
        atomic_t pCCChanged[MIDI_CONTROLLERS]; // which controller(s) changed
        atomic_t pCCValue[MIDI_CONTROLLERS]; // current value of each controller
        RingBuffer<VirtualMidiDevice::event_t,false> events;

        private_data_t() : events(MAX_EVENTS, 0) {}
    };

    VirtualMidiDevice::VirtualMidiDevice() : p(new private_data_t) {
        atomic_t zero = ATOMIC_INIT(0);
        atomic_t defaultVelocity = ATOMIC_INIT(127);
        atomic_t defaultCCValue = ATOMIC_INIT(0);
        atomic_set( &p->notesChanged, zero );
        atomic_set( &p->ccsChanged, zero );
        for (int i = 0; i < MIDI_KEYS; i++) {
            atomic_set( &p->pNoteChanged[i], zero );
            atomic_set( &p->pNoteIsActive[i], zero );
            atomic_set( &p->pNoteOnVelocity[i], defaultVelocity );
            atomic_set( &p->pNoteOffVelocity[i], defaultVelocity );
            atomic_set( &p->pCCChanged[i], zero );
            atomic_set( &p->pCCValue[i], defaultCCValue );
        }
    }

    VirtualMidiDevice::~VirtualMidiDevice() {
        delete p;
    }
    
    void VirtualMidiDevice::SetMaxEvents(int n) {
        p->events.resize(n);
    }

    bool VirtualMidiDevice::SendNoteOnToSampler(uint8_t Key, uint8_t Velocity) {
        if (Key >= MIDI_KEYS || Velocity > 127) return false;
        if (Velocity == 0) {
            return SendNoteOffToSampler(Key, Velocity);
        }
        event_t ev = { EVENT_TYPE_NOTEON, Key, Velocity };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }

    bool VirtualMidiDevice::SendNoteOffToSampler(uint8_t Key, uint8_t Velocity) {
        if (Key >= MIDI_KEYS || Velocity > 127) return false;
        event_t ev = { EVENT_TYPE_NOTEOFF, Key, Velocity };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }
    
    bool VirtualMidiDevice::SendCCToSampler(uint8_t Controller, uint8_t Value) {
        if (Controller >= MIDI_CONTROLLERS || Value > 127) return false;
        event_t ev = { EVENT_TYPE_CC, Controller, Value };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }

    bool VirtualMidiDevice::SendChannelPressureToSampler(uint8_t Pressure) {
        if (Pressure > 127) return false;
        event_t ev = { EVENT_TYPE_CHPRESSURE, 128 /*actually ignored by engine*/, Pressure };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }

    bool VirtualMidiDevice::SendPitchBendToSampler(int Pitch) {
        if (Pitch < -8192 || Pitch > 8191) return false;
        Pitch += 8192;
        // order: LSB, MSB like it would be in a regular pitch bend MIDI message
        event_t ev = {
            EVENT_TYPE_PITCHBEND,
            static_cast<uint8_t>(Pitch & 0x7f),
            static_cast<uint8_t>((Pitch >> 7) & 0x7f)
        };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }

    bool VirtualMidiDevice::SendProgramChangeToSampler(uint8_t Program) {
        if (Program > 127) return false;
        event_t ev = { EVENT_TYPE_PROGRAM, Program, 0 };
        if (p->events.write_space() <= 0) return false;
        p->events.push(&ev);
        return true;
    }

    bool VirtualMidiDevice::GetMidiEventFromDevice(event_t& Event) {
        return (p->events.pop(&Event) > 0);
    }

    bool VirtualMidiDevice::NotesChanged() {
        int c = atomic_read( &p->notesChanged );
        atomic_sub(c, &p->notesChanged );
        return c;
    }
    
    bool VirtualMidiDevice::ControllersChanged() {
        int c = atomic_read( &p->ccsChanged );
        atomic_sub(c, &p->ccsChanged );
        return c;
    }

    bool VirtualMidiDevice::NoteChanged(uint8_t Key) {
        int c = atomic_read( &(p->pNoteChanged)[Key] );
        atomic_sub(c, &(p->pNoteChanged)[Key] );
        return c;
    }
    
    bool VirtualMidiDevice::ControllerChanged(uint8_t Controller) {
        int c = atomic_read( &(p->pCCChanged)[Controller] );
        atomic_sub(c, &(p->pCCChanged)[Controller] );
        return c;
    }

    bool VirtualMidiDevice::NoteIsActive(uint8_t Key) {
        return atomic_read( &(p->pNoteIsActive)[Key] );
    }

    uint8_t VirtualMidiDevice::NoteOnVelocity(uint8_t Key) {
        return atomic_read( &(p->pNoteOnVelocity)[Key] );
    }

    uint8_t VirtualMidiDevice::NoteOffVelocity(uint8_t Key) {
        return atomic_read( &(p->pNoteOffVelocity)[Key] );
    }
    
    uint8_t VirtualMidiDevice::ControllerValue(uint8_t Controller) {
        return atomic_read( &(p->pCCValue)[Controller] );
    }

    void VirtualMidiDevice::SendNoteOnToDevice(uint8_t Key, uint8_t Velocity) {
        if (Key >= MIDI_KEYS) return;
        if (Velocity == 0) {
            SendNoteOffToDevice(Key, Velocity);
            return;
        }
        atomic_set( &(p->pNoteOnVelocity)[Key], Velocity );
        atomic_inc( &(p->pNoteIsActive)[Key] );
        atomic_inc( &(p->pNoteChanged)[Key] );
        atomic_inc( &p->notesChanged );
    }

    void VirtualMidiDevice::SendNoteOffToDevice(uint8_t Key, uint8_t Velocity) {
        if (Key >= MIDI_KEYS) return;
        atomic_set( &(p->pNoteOffVelocity)[Key], Velocity );
        if (atomic_read( &(p->pNoteIsActive)[Key] )) // only decrement if not zero
            atomic_dec( &(p->pNoteIsActive)[Key] );
        atomic_inc( &(p->pNoteChanged)[Key] );
        atomic_inc( &p->notesChanged );
    }
    
    void VirtualMidiDevice::SendCCToDevice(uint8_t Controller, uint8_t Value) {
        if (Controller >= MIDI_CONTROLLERS) return;
        atomic_set( &(p->pCCValue)[Controller], Value );
        atomic_inc( &(p->pCCChanged)[Controller] );
        atomic_inc( &p->ccsChanged );
    }

} // namespace LinuxSampler
