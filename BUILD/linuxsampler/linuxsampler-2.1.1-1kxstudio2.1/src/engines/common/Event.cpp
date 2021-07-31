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

#include "Event.h"

namespace LinuxSampler {

    /**
     * Create an EventGenerator.
     *
     * @param SampleRate - sample rate of the sampler engine's audio output
     *                     signal (in Hz)
     */
    EventGenerator::EventGenerator(uint SampleRate) {
        uiSampleRate       = SampleRate;
        uiSamplesProcessed = 0;
        FragmentTime.end   = RTMath::CreateTimeStamp();
        uiTotalSamplesProcessed = 0;
    }

    void EventGenerator::SetSampleRate(uint SampleRate) {
        uiSampleRate = SampleRate;
    }

    /**
     * Updates the time stamps for the beginning and end of the current audio
     * fragment. This is needed to be able to calculate the respective sample
     * point later to which an event belongs to.
     *
     * @param SamplesToProcess - number of sample points to process in this
     *                           audio fragment cycle
     */
    void EventGenerator::UpdateFragmentTime(uint SamplesToProcess) {
        // update total amount of sample points being processed since this object was created
        uiTotalSamplesProcessed += uiSamplesProcessed;
        // update time stamp for this audio fragment cycle
        FragmentTime.begin = FragmentTime.end;
        FragmentTime.end   = RTMath::CreateTimeStamp();
        // recalculate sample ratio for this audio fragment
        time_stamp_t fragmentDuration = FragmentTime.end - FragmentTime.begin;
        FragmentTime.sample_ratio = (float) uiSamplesProcessed / (float) fragmentDuration;
        // store amount of samples to process for the next cycle
        uiSamplesProcessed = SamplesToProcess;
    }
    
    /**
     * Get the next scheduled MIDI event (the one with the lowest time value)
     * for the current audio fragment cycle and remove it from the queue. This
     * method will not return any event scheduled past the current audio
     * fragment boundary.
     *
     * @param queue - where the MIDI events are scheduled on
     * @param pool - used to allocate and deallocate ScheduledEvent objects
     * @param end - you @b MUST always pass EventGenerator::schedTimeAtCurrentFragmentEnd()
     *              here reflecting the current audio fragment's scheduler end time
     */
    RTList<ScheduledEvent>::Iterator EventGenerator::popNextScheduledEvent(RTAVLTree<ScheduledEvent>& queue, Pool<ScheduledEvent>& pool, sched_time_t end) {
        if (queue.isEmpty())
            return RTList<ScheduledEvent>::Iterator(); // return invalid iterator
        ScheduledEvent& e = queue.lowest();
        if (e.scheduleTime >= end)
            return RTList<ScheduledEvent>::Iterator(); // no event scheduled before 'end'
        RTList<ScheduledEvent>::Iterator itEvent = pool.fromPtr(&e);
        queue.erase(e);
        if (!itEvent || !itEvent->itEvent) {
            dmsg(1,("EventGenerator::popNextScheduledEvent(): !itEvent\n"));
            return itEvent; // should never happen at this point, but just to be sure
        }
        if (!itEvent->itEvent) {
            dmsg(1,("EventGenerator::popNextScheduledEvent(): !itEvent->itEvent\n"));
            return itEvent; // should never happen at this point, but just to be sure
        }

        // update position of this event in the current audio fragment
        // (since calling scheduleAheadMicroSec() will relate to this)
        itEvent->itEvent->iFragmentPos = int32_t( uiSamplesProcessed - (end - itEvent->scheduleTime) );
        // safety first: fragment boundary sanity checks
        if (itEvent->itEvent->iFragmentPos < 0)
            itEvent->itEvent->iFragmentPos = 0;
        if (itEvent->itEvent->iFragmentPos >= uiSamplesProcessed)
            itEvent->itEvent->iFragmentPos = uiSamplesProcessed - 1;

        return itEvent;
    }

    /**
     * Get the next instrument script event (the one with the lowest time value)
     * for the current audio fragment cycle and remove it from the queue. This
     * method will not return any event scheduled past the current audio
     * fragment boundary.
     *
     * @param queue - where the instrument script events are scheduled on
     * @param pool - used to allocate and deallocate ScriptEvent objects
     * @param end - you @b MUST always pass EventGenerator::schedTimeAtCurrentFragmentEnd()
     *              here reflecting the current audio fragment's scheduler end time
     */
    RTList<ScriptEvent>::Iterator EventGenerator::popNextScheduledScriptEvent(RTAVLTree<ScriptEvent>& queue, Pool<ScriptEvent>& pool, sched_time_t end) {
        if (queue.isEmpty())
            return RTList<ScriptEvent>::Iterator(); // return invalid iterator
        ScriptEvent& e = queue.lowest();
        if (e.scheduleTime >= end)
            return RTList<ScriptEvent>::Iterator(); // no event scheduled before 'end'
        RTList<ScriptEvent>::Iterator itEvent = pool.fromPtr(&e);
        queue.erase(e);
        if (!itEvent) { // should never happen at this point, but just to be sure
            dmsg(1,("EventGenerator::popNextScheduledScriptEvent(): !itEvent\n"));
            return itEvent;
        }

        // update position of this event in the current audio fragment
        // (since calling scheduleAheadMicroSec() will relate to this)
        itEvent->cause.iFragmentPos = int32_t( uiSamplesProcessed - (end - itEvent->scheduleTime) );
        // safety first: fragment boundary sanity checks
        if (itEvent->cause.iFragmentPos < 0)
            itEvent->cause.iFragmentPos = 0;
        if (itEvent->cause.iFragmentPos >= uiSamplesProcessed)
            itEvent->cause.iFragmentPos = uiSamplesProcessed - 1;

        return itEvent;
    }

    /**
     * Create a new event with the current time as time stamp.
     */
    Event EventGenerator::CreateEvent() {
        return Event(this, RTMath::CreateTimeStamp());
    }

    /**
     * Create a new event for the given sample point position in the current
     * audio fragment.
     *
     * @param FragmentPos - actual sample point position in the current
     *                      audio fragment to which the new event belongs to
     */
    Event EventGenerator::CreateEvent(int32_t FragmentPos) {
        return Event(this, FragmentPos);
    }

    /**
     * Will be called by an EventGenerator to create a new Event.
     * This Constructor expects a time stamp. The actual sample point
     * position to which this event belongs to will be calculated later
     * when FragmentPos() was called the first time.
     *
     * @param pGenerator - creator of this event
     * @param Time       - time stamp on which this event occured
     */
    Event::Event(EventGenerator* pGenerator, time_stamp_t Time) {
        Init();
        pEventGenerator = pGenerator;
        TimeStamp       = Time;
        iFragmentPos    = -1;
    }

    /**
     * Will be called by an EventGenerator to create a new Event.
     * This constructor expects the final sample point position to which
     * this event belongs to.
     *
     * @param pGenerator  - creator of this event
     * @param FragmentPos - actual sample point position in the current
     *                      audio fragment to which this event belongs to
     */
    Event::Event(EventGenerator* pGenerator, int32_t FragmentPos) {
        Init();
        pEventGenerator = pGenerator;
        iFragmentPos    = FragmentPos;
    }

    /**
     * Implements fork() behavior, that is it copies the current state of this
     * script event handler to the new event handler @a e with entire execution
     * state and polyphonic data.
     *
     * After calling this method, addChildHandler() should be called as well.
     */
    void ScriptEvent::forkTo(ScriptEvent* e, bool bAutoAbort) const {
        e->scheduleTime = scheduleTime;
        e->cause = cause;
        e->id = id;
        // forked script shall only run the current event handler of parent,
        // no other potentially chained handlers
        e->handlers[0] = handlers[currentHandler];
        e->handlers[1] = NULL; // NULL termination of list
        e->currentHandler = 0;
        e->executionSlices = 0;
        e->ignoreAllWaitCalls = ignoreAllWaitCalls;
        e->handlerType = handlerType;
        e->parentHandlerID = 0; // just an arbitrary init value, must still be set later!
        e->childHandlerID[0] = 0;
        e->autoAbortByParent = bAutoAbort;
        e->forkIndex = 1; // just an arbitrary init value, must still be set later!

        execCtx->forkTo(e->execCtx);
    }

    /**
     * Returns amount of child event handlers that have been created so far
     * during the entire life time of this event handler instance by calls of
     * this event handler instace to the built-in script function fork().
     *
     * To make this clear: this number does never decrease during the entire
     * life time of an event handler instance! It may only increase, i.e. by
     * additional calls to fork(). Consequently even if child event handler
     * instances already terminated, this will not decrease this count.
     */
    int ScriptEvent::countChildHandlers() const {
        int n = 0;
        for (int i = 0; i < MAX_FORK_PER_SCRIPT_HANDLER && childHandlerID[i]; ++i)
            ++n;
        return n;
    }

    /**
     * This must be called after calling forkTo() to stick the script callback
     * ID of the newly forked child execution instance to the parent execution
     * instance.
     *
     * @param childID - script callback ID of the newly forked child handler
     */
    void ScriptEvent::addChildHandlerID(script_callback_id_t childID) {
        const int n = countChildHandlers();
        if (n >= MAX_FORK_PER_SCRIPT_HANDLER) {
            dmsg(1,("ScriptEvent::addChildHandlerID(): MAX_FORK_PER_SCRIPT_HANDLER exceeded, this is a bug!\n"));
            return;
        }
        childHandlerID[n]   = childID;
        childHandlerID[n+1] = 0; // zero termination of list
    }

} // namespace LinuxSampler
