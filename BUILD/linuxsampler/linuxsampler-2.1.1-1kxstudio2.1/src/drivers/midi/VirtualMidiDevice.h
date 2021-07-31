/*
    Copyright (C) 2008 - 2016 Christian Schoenebeck
 */

#ifndef LS_VIRTUALMIDIDEVICE_H
#define LS_VIRTUALMIDIDEVICE_H

#include "../../common/global.h"

namespace LinuxSampler {

/**
 * Light-weight MIDI interface (for MIDI in & out) intended to be used by
 * pure software MIDI "devices", that is e.g. a graphical virtual MIDI
 * keyboard in an instrument editor or in a sampler frontend. This class
 * should not be used for regular MIDI input device drivers for the sampler.
 * This primitive interface by design doesn't care about jitter, fast event
 * delivery or masses and masses of events in a short time!
 */
class VirtualMidiDevice {
public:
    enum event_type_t {
        EVENT_TYPE_NOTEON  = 1,
        EVENT_TYPE_NOTEOFF = 2,
        EVENT_TYPE_CC      = 3,
        EVENT_TYPE_PITCHBEND,
        EVENT_TYPE_PROGRAM,
        EVENT_TYPE_CHPRESSURE,
    };

    struct event_t {
        event_type_t Type;
        uint8_t      Arg1; ///< Depends on @c Type (e.g. key number for note on/off events).
        uint8_t      Arg2; ///< Depends on @c Type (e.g. velocity for note on/off events).
    };

    /////////////////////////////////////////////////////////////////
    // Device methods
    //     (called by the VirtualMidiDevice implementation)

    /**
     * Sends a MIDI @e note @e on event to the sampler.
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided values invalid)
     */
    bool SendNoteOnToSampler(uint8_t Key, uint8_t Velocity);

    /**
     * Sends a MIDI @e note @e off event to the sampler.
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided values invalid)
     */
    bool SendNoteOffToSampler(uint8_t Key, uint8_t Velocity);

    /**
     * Sends a MIDI @e Control @e Change event to the sampler.
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided values invalid)
     */
    bool SendCCToSampler(uint8_t Controller, uint8_t Value);

    /**
     * Sends a MIDI @e Channel @e Pressure (aftertouch) event to the sampler.
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided value invalid)
     */
    bool SendChannelPressureToSampler(uint8_t Pressure);

    /**
     * Sends a MIDI @e Pitch @e Bend event to the sampler.
     *
     * @param Pitch - MIDI pitch value (-8192 ... +8191)
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided pitch value out of valid range)
     */
    bool SendPitchBendToSampler(int Pitch);

    /**
     * Sends a MIDI @e Program @e Change event to the sampler.
     *
     * If you want to change the sound bank, call SendCCToSampler() (with
     * controller = 0 for bank select MSB and/or controller = 32 for bank select
     * LSB) before calling this method.
     *
     * @param Program - MIDI program number
     *
     * @returns true on success, false if internal FIFO full
     *          (or provided value invalid)
     */
    bool SendProgramChangeToSampler(uint8_t Program);

    /**
     * Can be called by the virtual MIDI device to check whether a new note
     * on or note off MIDI event arrived to the sampler during the last
     * call to this method. So this is a asynchronously, "polling" based
     * communication mechanism, which works in conjunction with the
     * NoteIsActive() method call.
     */
    bool NotesChanged();

    /**
     * Can be called by the virtual MIDI device to check whether a new note
     * on or note off MIDI event arrived to the sampler for @a Key during
     * the last call to this method. So this is a asynchronously,
     * "polling" based communication mechanism, which works in
     * conjunction with the NoteIsActive() method call.
     */
    bool NoteChanged(uint8_t Key);

    /**
     * Can be called by the virtual MIDI device to check which key / note
     * is currently active by the sampler, e.g. to highlight the
     * respective keys on a graphical virtual keyboard.
     *
     * @see NotesChanged(), NoteChanged()
     */
    bool NoteIsActive(uint8_t Key);

    /**
     * Returns the velocity of the @e last note on event. No FIFO is used!
     */
    uint8_t NoteOnVelocity(uint8_t Key);

    /**
     * Returns the velocity of the @e last note off event. No FIFO is used!
     */
    uint8_t NoteOffVelocity(uint8_t Key);

    /**
     * Can be called by the virtual MIDI device to check whether a Control
     * Change MIDI event arrived to the sampler during the last
     * call to this method. So this is a asynchronously, "polling" based
     * communication mechanism, which works in conjunction with the
     * ControllerValue() method call.
     */
    bool ControllersChanged();

    /**
     * Can be called by the virtual MIDI device to check whether a Control
     * Change MIDI event arrived to the sampler for @a Controller during
     * the last call to this method. So this is a asynchronously,
     * "polling" based communication mechanism, which works in
     * conjunction with the ControllerValue() method call.
     */
    bool ControllerChanged(uint8_t Controller);

    /**
     * Returns the value of the @e last Control Change event. No FIFO is used!
     */
    uint8_t ControllerValue(uint8_t Controller);

    /////////////////////////////////////////////////////////////////
    // Sampler methods
    //     (usually only called by the Sampler)

    /**
     * Informs the virtual MIDI device that a @e note @e on event occured
     * (e.g. caused by a MIDI keyboard connected to the sampler).
     * Communication acts asynchronously, that is this method call doesn't
     * lock in any way and returns immediately. It is thus realtime safe.
     *
     * @e Note: this method is usually only called by the sampler.
     *
     * @see ActiveNotesChanged(), NoteIsActive()
     */
    void SendNoteOnToDevice(uint8_t Key, uint8_t Velocity);

    /**
     * Informs the virtual MIDI device that a @e note @e off event occured
     * (e.g. caused by a MIDI keyboard connected to the sampler).
     * Communication acts asynchronously, that is this method call doesn't
     * lock in any way and returns immediately. It is thus realtime safe.
     *
     * @e Note: this method is usually only called by the sampler.
     *
     * @see ActiveNotesChanged(), NoteIsActive()
     */
    void SendNoteOffToDevice(uint8_t Key, uint8_t Velocity);

    /**
     * Informs the virtual MIDI device that a @e Control @e Change event
     * occured (e.g. caused by a MIDI keyboard connected to the sampler).
     * Communication acts asynchronously, that is this method call doesn't
     * lock in any way and returns immediately. It is thus realtime safe.
     *
     * @e Note: this method is usually only called by the sampler.
     *
     * @see ControllersChanged(), ControllerValue()
     */
    void SendCCToDevice(uint8_t Controller, uint8_t Value);

    /**
     * Gets the next pending MIDI event from the virtual MIDI device by
     * using a lockfree FIFO.
     *
     * @e Note: this method is usually only called by the sampler.
     *
     * @param Event - destination for writing the next event to
     * @returns true on success, false if no event pending
     */
    bool GetMidiEventFromDevice(event_t& Event);
    
    /////////////////////////////////////////////////////////////////
    // General Purpose Methods
    
    /**
     * Adjusts the internal event buffer to cover at least the given
     * amount of MIDI events. This might be useful, since the internal
     * event buffer is by default quite small (i.e. just 12 events).
     *
     * This method is not thread safe! Any operations upon this device
     * have to be stopped before calling this method!
     */
    void SetMaxEvents(int n);

    /**
     * Constructor
     */
    VirtualMidiDevice();

    /**
     * Destructor
     */
    virtual ~VirtualMidiDevice();

private:
    struct private_data_t;
    private_data_t* const p;
};

} // namespace LinuxSampler

#endif // LS_VIRTUALMIDIDEVICE_H
