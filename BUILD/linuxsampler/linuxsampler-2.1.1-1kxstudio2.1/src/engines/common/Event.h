/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2017 Christian Schoenebeck                       *
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

#ifndef __LS_EVENT_H__
#define __LS_EVENT_H__

#include "../../common/global.h"
#include "../../common/RTMath.h"
#include "../../common/RTAVLTree.h"
#include "../../common/Pool.h"
#include "../EngineChannel.h"
#include "../../scriptvm/common.h"

namespace LinuxSampler {

    // just symbol prototyping
    class Event;
    class SchedulerNode;
    class ScriptEvent;
    class ScheduledEvent;

    /**
     * Data type used to schedule events sample point accurately both within, as
     * well as beyond the scope of the current audio fragment cycle. The timing
     * reflected by this data type is consecutively running for a very long
     * time. Even with a sample rate of 96 kHz a scheduler time of this data
     * type will not wrap before 6 million years. So in practice such time
     * stamps are unique and will not repeat (unless the EventGenerator is
     * reset).
     */
    typedef uint64_t sched_time_t;

    /**
     * Generates Event objects and is responsible for resolving the position
     * in the current audio fragment each Event actually belongs to.
     */
    class EventGenerator {
        public:
            EventGenerator(uint SampleRate);
            void UpdateFragmentTime(uint SamplesToProcess);
            void SetSampleRate(uint SampleRate);
            Event CreateEvent();
            Event CreateEvent(int32_t FragmentPos);

            template<typename T>
            void scheduleAheadMicroSec(RTAVLTree<T>& queue, T& node, int32_t fragmentPosBase, uint64_t microseconds);

            RTList<ScheduledEvent>::Iterator popNextScheduledEvent(RTAVLTree<ScheduledEvent>& queue, Pool<ScheduledEvent>& pool, sched_time_t end);
            RTList<ScriptEvent>::Iterator popNextScheduledScriptEvent(RTAVLTree<ScriptEvent>& queue, Pool<ScriptEvent>& pool, sched_time_t end);

            /**
             * Returns the scheduler time for the first sample point of the
             * current audio fragment cycle.
             */
            sched_time_t schedTimeAtCurrentFragmentStart() const {
                return uiTotalSamplesProcessed;
            }

            /**
             * Returns the scheduler time for the first sample point of the next
             * audio fragment cycle.
             */
            sched_time_t schedTimeAtCurrentFragmentEnd() const {
                return uiTotalSamplesProcessed + uiSamplesProcessed;
            }

        protected:
            typedef RTMath::time_stamp_t time_stamp_t;
            inline int32_t ToFragmentPos(time_stamp_t TimeStamp) {
                return int32_t (int32_t(TimeStamp - FragmentTime.begin) * FragmentTime.sample_ratio);
            }
            friend class Event;
        private:
            uint uiSampleRate;
            uint uiSamplesProcessed;
            struct __FragmentTime__ {
                time_stamp_t begin;        ///< Real time stamp of the beginning of this audio fragment cycle.
                time_stamp_t end;          ///< Real time stamp of the end of this audio fragment cycle.
                float        sample_ratio; ///< (Samples per cycle) / (Real time duration of cycle)
            } FragmentTime;
            sched_time_t uiTotalSamplesProcessed; ///< Total amount of sample points that have been processed since this EventGenerator object has been created. This is used to schedule instrument script events long time ahead in future (that is beyond the scope of the current audio fragment).
    };

    /**
     * Unique numeric ID of an event which can be used to retrieve access to
     * the actual @c Event object. Once the event associated with a certain ID
     * was released (back to its event pool), this numeric ID becomes invalid
     * and Pool< Event >::fromID() will detect this circumstance and will
     * return an invalid Iterator, and thus will prevent you from misusing an
     * event which no longer "exists".
     *
     * Note that an @c Event object usually just "exists" for exactly on audio
     * fragment cycle: that is it exists right from the beginning of the audio
     * fragment cycle where it was caused (i.e. where its MIDI data was
     * received by the respective engine channel) and will disappear
     * automatically at the end of that audio fragment cycle.
     */
    typedef pool_element_id_t event_id_t;

    /**
     * Unique numeric ID of a note which can be used to retrieve access to the
     * actual @c Note object. Once the note associated with a certain ID was
     * released (back to its note pool), this numeric ID becomes invalid and
     * Pool< Note >::fromID() will detect this circumstance and will return
     * an invalid Iterator, and thus will prevent you from misusing a note
     * which no longer is "alive".
     *
     * A @c Note object exists right when the respective MIDI note-on event
     * was received by the respective engine channel, and remains existent
     * until the caused note and all its voices were finally freed (which might
     * even be long time after the respective note-off event was received,
     * depending on the duration of the voice's release stages etc.).
     */
    typedef pool_element_id_t note_id_t;

    /**
     * Unique numeric ID of a script callback ID instance which can be used to
     * retrieve access to the actual @c ScriptEvent object. Once the script
     * callback instance associated with a certain ID stopped its execution
     * (that is completely stopped, not just suspended) then this numeric ID
     * becomes invalid and Pool< ScriptEvent >::fromID() will detect this
     * circumstance and will return an invalid Iterator, and thus will prevent
     * you from misusing a script callback instance which no longer "exists".
     */
    typedef pool_element_id_t script_callback_id_t;

    /**
     * Events are usually caused by a MIDI source or an internal modulation
     * controller like LFO or EG. An event should only be created by an
     * EventGenerator!
     *
     * @see EventGenerator, ScriptEvent
     */
    class Event {
        public:
            Event(){}
            enum type_t {
                type_note_on, ///< (real) MIDI note-on event
                type_note_off, ///< (real) MIDI note-off event
                type_pitchbend, ///< MIDI pitch bend wheel change event
                type_control_change, ///< MIDI CC event
                type_sysex,           ///< MIDI system exclusive message
                type_cancel_release_key, ///< transformed either from a (real) MIDI note-on or sustain-pedal-down event
                type_release_key,     ///< transformed either from a (real) MIDI note-off or sustain-pedal-up event
                type_release_note,    ///< transformed from a type_stop_note event
                type_channel_pressure, ///< a.k.a. aftertouch
                type_note_pressure, ///< polyphonic key pressure (aftertouch)
                type_play_note, ///< caused by a call to built-in instrument script function play_note()
                type_stop_note, ///< caused by a call to built-in instrument script function note_off()
                type_kill_note, ///< caused by a call to built-in instrument script function fade_out()
                type_note_synth_param, ///< change a note's synthesis parameters (upon real-time instrument script function calls, i.e. change_vol(), change_tune(), change_pan(), etc.)
            } Type;
            enum synth_param_t {
                synth_param_volume,
                synth_param_volume_time,
                synth_param_volume_curve,
                synth_param_pitch,
                synth_param_pitch_time,
                synth_param_pitch_curve,
                synth_param_pan,
                synth_param_pan_time,
                synth_param_pan_curve,
                synth_param_cutoff,
                synth_param_resonance,
                synth_param_attack,
                synth_param_decay,
                synth_param_sustain,
                synth_param_release,
                synth_param_cutoff_attack,
                synth_param_cutoff_decay,
                synth_param_cutoff_sustain,
                synth_param_cutoff_release,
                synth_param_amp_lfo_depth,
                synth_param_amp_lfo_freq,
                synth_param_cutoff_lfo_depth,
                synth_param_cutoff_lfo_freq,
                synth_param_pitch_lfo_depth,
                synth_param_pitch_lfo_freq,
            };
            union {
                /// Note-on and note-off event specifics
                struct _Note {
                    uint8_t Channel;     ///< MIDI channel (0..15)
                    uint8_t Key;         ///< MIDI key number of note-on / note-off event.
                    uint8_t Velocity;    ///< Trigger or release velocity of note-on / note-off event.
                    int8_t  Layer;       ///< Layer index (usually only used if a note-on event has to be postponed, e.g. due to shortage of free voices).
                    int8_t  ReleaseTrigger; ///< If new voice should be a release triggered voice (actually boolean field and usually only used if a note-on event has to be postponed, e.g. due to shortage of free voices).
                    note_id_t ID;        ///< Unique numeric ID of the @c Note object associated with this note event.
                    note_id_t ParentNoteID; ///< If not zero: Unique numeric ID of the parent @c Note object that shall become parent of resulting new Note object of this Event. So this is used to associate a new note with a previous note, i.e. to release the new note once the parent note was released.
                    void*   pRegion;     ///< Engine specific pointer to instrument region
                } Note;
                /// Control change event specifics
                struct _CC {
                    uint8_t Channel;     ///< MIDI channel (0..15)
                    uint8_t Controller;  ///< MIDI controller number of control change event.
                    uint8_t Value;       ///< Controller Value of control change event.
                } CC;
                /// Pitchbend event specifics
                struct _Pitch {
                    uint8_t Channel;     ///< MIDI channel (0..15)
                    int16_t Pitch;       ///< Pitch value of pitchbend event.
                } Pitch;
                /// MIDI system exclusive event specifics
                struct _Sysex {
                    uint Size;           ///< Data length (in bytes) of MIDI system exclusive message.
                } Sysex;
                /// Channel Pressure (aftertouch) event specifics
                struct _ChannelPressure {
                    uint8_t Channel; ///< MIDI channel (0..15)
                    uint8_t Controller; ///< Should always be assigned to CTRL_TABLE_IDX_AFTERTOUCH.
                    uint8_t Value;   ///< New aftertouch / pressure value for keys on that channel.
                } ChannelPressure;
                /// Polyphonic Note Pressure (aftertouch) event specifics
                struct _NotePressure {
                    uint8_t Channel; ///< MIDI channel (0..15)
                    uint8_t Key;     ///< MIDI note number where key pressure (polyphonic aftertouch) changed.
                    uint8_t Value;   ///< New pressure value for note.
                } NotePressure;
                ///< Note synthesis parameter change event's specifics (used for real-time instrument script built-in functions which may alter synthesis parameters on note level).
                struct _NoteSynthParam {
                    note_id_t     NoteID;   ///< ID of Note whose voices shall be modified.
                    synth_param_t Type;     ///< Synthesis parameter which is to be changed.
                    float         Delta;    ///< The value change that should be applied against the note's current synthesis parameter value.
                    bool          Relative; ///< Whether @c Delta should be applied relatively against the note's current synthesis parameter value (false means the paramter's current value is simply replaced by Delta).
                    float         AbsValue; ///< New current absolute value of synthesis parameter (that is after @c Delta being applied).
                } NoteSynthParam;
            } Param;
            EngineChannel* pEngineChannel; ///< Pointer to the EngineChannel where this event occured on, NULL means Engine global event (e.g. SysEx message).
            MidiInputPort* pMidiInputPort; ///< Pointer to the MIDI input port on which this event occured (NOTE: currently only for global events, that is SysEx messages)

            inline void Init() {
                Param.Note.ID = 0;
                Param.Note.ParentNoteID = 0;
                Param.NoteSynthParam.NoteID = 0;
            }
            inline int32_t FragmentPos() {
                if (iFragmentPos >= 0) return iFragmentPos;
                iFragmentPos = pEventGenerator->ToFragmentPos(TimeStamp);
                if (iFragmentPos < 0) iFragmentPos = 0; // if event arrived shortly before the beginning of current fragment
                return iFragmentPos;
            }
            inline void ResetFragmentPos() {
                iFragmentPos = -1;
            }
            inline void CopyTimeFrom(const Event& other) {
                TimeStamp = other.TimeStamp;
                iFragmentPos = other.iFragmentPos;
            }
            inline sched_time_t SchedTime() {
                return pEventGenerator->schedTimeAtCurrentFragmentStart() + FragmentPos();
            }
        protected:
            typedef EventGenerator::time_stamp_t time_stamp_t;
            Event(EventGenerator* pGenerator, EventGenerator::time_stamp_t Time);
            Event(EventGenerator* pGenerator, int32_t FragmentPos);
            friend class EventGenerator;
        private:
            EventGenerator* pEventGenerator; ///< Creator of the event.
            time_stamp_t    TimeStamp;       ///< Time stamp of the event's occurence.
            int32_t         iFragmentPos;    ///< Position in the current fragment this event refers to.
    };

    /**
     * Used to sort timing relevant objects (i.e. events) into timing/scheduler
     * queue. This class is just intended as base class and should be derived
     * for its actual purpose (for the precise data type being scheduled).
     */
    class SchedulerNode : public RTAVLNode {
    public:
        using RTAVLNode::reset; // make reset() method public

        sched_time_t scheduleTime; ///< Time ahead in future (in sample points) when this object shall be processed. This value is compared with EventGenerator's uiTotalSamplesProcessed member variable.

        /// Required operator implementation for RTAVLTree class.
        inline bool operator==(const SchedulerNode& other) const {
            return this->scheduleTime == other.scheduleTime;
        }

        /// Required operator implementation for RTAVLTree class.
        inline bool operator<(const SchedulerNode& other) const {
            return this->scheduleTime < other.scheduleTime;
        }

        /// This is actually just for code readability.
        inline RTAVLTreeBase* currentSchedulerQueue() const { return rtavlTree(); }
    };

    /**
     * Used to sort delayed MIDI events into a timing/scheduler queue. This
     * object just contains the timing informations, the actual MIDI event is
     * pointed by member variable @c itEvent.
     */
    class ScheduledEvent : public SchedulerNode {
    public:
        Pool<Event>::Iterator itEvent; ///< Points to the actual Event object being scheduled.
    };

    class VMEventHandler;
    class VMExecContext;

    /**
     * Maximum amount of child script handler instances one script handler is
     * allowed to create by calling built-in script function fork().
     */
    #define MAX_FORK_PER_SCRIPT_HANDLER 8

    /** @brief Real-time instrument script event.
     *
     * Encapsulates one execution instance of a real-time instrument script for
     * exactly one script event handler (script event callback).
     *
     * This class derives from SchedulerNode for being able to be sorted efficiently
     * by the script scheduler if the script was either a) calling the wait()
     * script function or b) the script was auto suspended by the ScriptVM
     * because the script was executing for too long. In both cases the
     * scheduler has to sort the ScriptEvents in its execution queue according
     * to the precise time the respective script execution instance needs to be
     * resumed.
     */
    class ScriptEvent : public SchedulerNode {
    public:
        Event cause; ///< Copy of original external @c Event that triggered this script event (i.e. MIDI note on event, MIDI CC event, etc.).
        pool_element_id_t id; ///< Native representation of built-in script variable $EVENT_ID. For scripts' "note" event handler this will reflect the unique ID of the @c Note object, for all other event handlers the unique ID of the original external @c Event object that triggered this script event.
        VMEventHandler** handlers; ///< The script's event handlers (callbacks) to be processed (NULL terminated list).
        VMExecContext* execCtx; ///< Script's current execution state (polyphonic variables and execution stack).
        int currentHandler; ///< Current index in 'handlers' list above.
        int executionSlices; ///< Amount of times this script event has been executed by the ScriptVM runner class.
        bool ignoreAllWaitCalls; ///< If true: calling any built-in wait*() script function should be ignored (this variable may be set with the 2nd argument of built-in script function stop_wait()).
        VMEventHandlerType_t handlerType; ///< Native representation of built-in script variable $NI_CALLBACK_TYPE, reflecting the script event type of this script event.
        script_callback_id_t parentHandlerID; ///< Only in case this script handler instance was created by calling built-in script function fork(): callback ID of the parent event handler instance which created this child. For regular event handler instances which were not created by fork(), this variable reflects 0 (which is always considered an invalid handler ID).
        script_callback_id_t childHandlerID[MAX_FORK_PER_SCRIPT_HANDLER+1]; ///< In case built-in script function fork() was called by this script handler instance: A zero terminated ID list of all child event handler instances (note: children will not vanish from this list after they terminated).
        bool autoAbortByParent; ///< Only if this is a child event handler created by calling fork(): if this is true then this child will automatically aborted if the parent event handler terminates.
        int forkIndex; ///< Only for fork() calls: distinguishment feature which is 0 for parent, 1 for 1st child, 2 for 2nd child, etc.

        void forkTo(ScriptEvent* e, bool bAutoAbort) const;
        int countChildHandlers() const;
        void addChildHandlerID(script_callback_id_t childID);
    };

    /**
     * Insert given @a node into the supplied timing @a queue with a scheduled
     * timing position given by @a fragmentPosBase and @a microseconds, where
     * @a microseconds reflects the amount of microseconds in future from "now"
     * where the node shall be scheduled, and @a fragmentPos identifies the
     * sample point within the current audio fragment cycle which shall be
     * interpreted by this method to be "now".
     *
     * The meaning of @a fragmentPosBase becomes more important the larger
     * the audio fragment size, and vice versa it becomes less important the
     * smaller the audio fragment size.
     *
     * @param queue - destination scheduler queue
     * @param node - node (i.e. event) to be inserted into the queue
     * @param fragmentPosBase - sample point in current audio fragment to be "now"
     * @param microseconds - timing of node from "now" (in microseconds)
     */
    template<typename T>
    void EventGenerator::scheduleAheadMicroSec(RTAVLTree<T>& queue, T& node, int32_t fragmentPosBase, uint64_t microseconds) {
        // round up (+1) if microseconds is not zero (i.e. because 44.1 kHz and
        // 1 us would yield in < 1 and thus would be offset == 0)
        const sched_time_t offset =
            (microseconds != 0LL) ?
                1.f + (float(uiSampleRate) * (float(microseconds) / 1000000.f))
                : 0.f;
        node.scheduleTime = uiTotalSamplesProcessed + fragmentPosBase + offset;
        queue.insert(node);
    }

} // namespace LinuxSampler

#endif // __LS_EVENT_H__
