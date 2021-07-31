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

#ifndef __LS_ENGINECHANNELBASE_H__
#define	__LS_ENGINECHANNELBASE_H__

#include "AbstractEngineChannel.h"
#include "common/MidiKeyboardManager.h"
#include "common/Voice.h"
#include "../common/ResourceManager.h"

namespace LinuxSampler {
    /// Command used by the instrument loader thread to
    /// request an instrument change on a channel.
    template <class R /* Region */, class I /* Instrument */>
    class InstrumentChangeCmd {
        public:
            bool bChangeInstrument;       ///< Set to true by the loader when the channel should change instrument.
            I* pInstrument;               ///< The new instrument. Also used by the loader to read the previously loaded instrument.
            RTList<R*>* pRegionsInUse; ///< List of dimension regions in use by the currently loaded instrument. Continuously updated by the audio thread.
            InstrumentScript* pScript; ///< Instrument script to be executed for this instrument. This is never NULL, it is always a valid InstrumentScript pointer. Use InstrumentScript::bHasValidScript whether it reflects a valid instrument script to be executed.
    };

    template<class R>
    class RegionPools {
        public:
            virtual Pool<R*>* GetRegionPool(int index) = 0;
    };

    template<class V>
    class NotePool {
        public:
            /**
             * Pool from where Voice objects are allocated from (and freed back to).
             */
            virtual Pool<V>* GetVoicePool() = 0;

            /**
             * Pool from where new Note objects are allocated from (and freed back to).
             */
            virtual Pool< Note<V> >* GetNotePool() = 0;

            /**
             * Pool for saving already existing Note object IDs somewhere.
             *
             * @b IMPORTANT: This pool is @b NOT used for generating any IDs for
             * Note objects! The respective Note objective IDs are emitted by
             * the Note object pool (see GetNotePool() above).
             */
            virtual Pool<note_id_t>* GetNoteIDPool() = 0;
    };

    template <class V /* Voice */, class R /* Region */, class I /* Instrument */>
    class EngineChannelBase: public AbstractEngineChannel, public MidiKeyboardManager<V>, public ResourceConsumer<I> {
        public:
            typedef typename RTList< Note<V> >::Iterator NoteIterator;
            typedef typename RTList<R*>::Iterator RTListRegionIterator;
            typedef typename MidiKeyboardManager<V>::MidiKey MidiKey;

            virtual MidiKeyboardManagerBase* GetMidiKeyboardManager() OVERRIDE {
                return this;
            }

            virtual void HandBack(I* Instrument) {
                ResourceManager<InstrumentManager::instrument_id_t, I>* mgr =
                    dynamic_cast<ResourceManager<InstrumentManager::instrument_id_t, I>*>(pEngine->GetInstrumentManager());
                mgr->HandBack(Instrument, this);
            }

            virtual void ClearRegionsInUse() {
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    if (cmd.pRegionsInUse) cmd.pRegionsInUse->clear();
                    cmd.bChangeInstrument = false;
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    if (cmd.pRegionsInUse) cmd.pRegionsInUse->clear();
                    cmd.bChangeInstrument = false;
                }
            }

            virtual void ResetRegionsInUse(Pool<R*>* pRegionPool[]) {
                DeleteRegionsInUse();
                AllocateRegionsInUse(pRegionPool);
            }

            virtual void DeleteRegionsInUse() {
                RTList<R*>* previous = NULL; // prevent double free
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    if (cmd.pRegionsInUse) {
                        previous = cmd.pRegionsInUse;
                        delete cmd.pRegionsInUse;
                        cmd.pRegionsInUse = NULL;
                    }
                    cmd.bChangeInstrument = false;
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    if (cmd.pRegionsInUse) {
                        if (cmd.pRegionsInUse != previous)
                            delete cmd.pRegionsInUse;
                        cmd.pRegionsInUse = NULL;
                    }
                    cmd.bChangeInstrument = false;
                }
            }

            virtual void AllocateRegionsInUse(Pool<R*>* pRegionPool[]) {
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    cmd.pRegionsInUse = new RTList<R*>(pRegionPool[0]);
                    cmd.bChangeInstrument = false;
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    cmd.pRegionsInUse = new RTList<R*>(pRegionPool[1]);
                    cmd.bChangeInstrument = false;
                }
            }

            virtual void Connect(AudioOutputDevice* pAudioOut) OVERRIDE {
                if (pEngine) {
                    if (pEngine->pAudioOutputDevice == pAudioOut) return;
                    DisconnectAudioOutputDevice();
                }
                AbstractEngine* newEngine = AbstractEngine::AcquireEngine(this, pAudioOut);
                {
                    LockGuard lock(EngineMutex);
                    pEngine = newEngine;
                }
                ResetInternal(false/*don't reset engine*/); // 'false' is error prone here, but the danger of recursion with 'true' would be worse, there could be a better solution though
                pEvents = new RTList<Event>(pEngine->pEventPool);
                delayedEvents.pList = new RTList<Event>(pEngine->pEventPool);

                RegionPools<R>* pRegionPool = dynamic_cast<RegionPools<R>*>(pEngine);
                // reset the instrument change command struct (need to be done
                // twice, as it is double buffered)
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    cmd.pRegionsInUse = new RTList<R*>(pRegionPool->GetRegionPool(0));
                    cmd.pInstrument = 0;
                    cmd.bChangeInstrument = false;
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    cmd.pRegionsInUse = new RTList<R*>(pRegionPool->GetRegionPool(1));
                    cmd.pInstrument = 0;
                    cmd.bChangeInstrument = false;
                }

                if (pInstrument != NULL) {
                    pInstrument = NULL;
                    InstrumentStat = -1;
                    InstrumentIdx  = -1;
                    InstrumentIdxName = "";
                    InstrumentFile = "";
                    bStatusChanged = true;
                }

                NotePool<V>* pNotePool = dynamic_cast<NotePool<V>*>(pEngine);
                MidiKeyboardManager<V>::AllocateActiveNotesLists(
                    pNotePool->GetNotePool(),
                    pNotePool->GetVoicePool()
                );
                MidiKeyboardManager<V>::AllocateEventsLists(pEngine->pEventPool);

                AudioDeviceChannelLeft  = 0;
                AudioDeviceChannelRight = 1;
                if (fxSends.empty()) { // render directly into the AudioDevice's output buffers
                    pChannelLeft  = pAudioOut->Channel(AudioDeviceChannelLeft);
                    pChannelRight = pAudioOut->Channel(AudioDeviceChannelRight);
                } else { // use local buffers for rendering and copy later
                    // ensure the local buffers have the correct size
                    if (pChannelLeft)  delete pChannelLeft;
                    if (pChannelRight) delete pChannelRight;
                    pChannelLeft  = new AudioChannel(0, pAudioOut->MaxSamplesPerCycle());
                    pChannelRight = new AudioChannel(1, pAudioOut->MaxSamplesPerCycle());
                }
                if (pEngine->EngineDisabled.GetUnsafe()) pEngine->Enable();
                MidiInputPort::AddSysexListener(pEngine);
            }

            virtual void DisconnectAudioOutputDevice() OVERRIDE {
                if (pEngine) { // if clause to prevent disconnect loops

                    ResetInternal(false/*don't reset engine*/); // 'false' is error prone here, but the danger of recursion with 'true' would be worse, there could be a better solution though

                    DeleteRegionsInUse();
                    UnloadScriptInUse();

                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    if (cmd.pInstrument) {
                        // release the currently loaded instrument
                        HandBack(cmd.pInstrument);
                    }

                    if (pEvents) {
                        delete pEvents;
                        pEvents = NULL;
                    }
                    if (delayedEvents.pList) {
                        delete delayedEvents.pList;
                        delayedEvents.pList = NULL;
                    }

                    MidiKeyboardManager<V>::DeleteActiveNotesLists();
                    MidiKeyboardManager<V>::DeleteEventsLists();
                    DeleteGroupEventLists();

                    AudioOutputDevice* oldAudioDevice = pEngine->pAudioOutputDevice;
                    {
                        LockGuard lock(EngineMutex);
                        pEngine = NULL;
                    }
                    AbstractEngine::FreeEngine(this, oldAudioDevice);
                    AudioDeviceChannelLeft  = -1;
                    AudioDeviceChannelRight = -1;
                    if (!fxSends.empty()) { // free the local rendering buffers
                        if (pChannelLeft)  delete pChannelLeft;
                        if (pChannelRight) delete pChannelRight;
                    }
                    pChannelLeft  = NULL;
                    pChannelRight = NULL;
                }
            }

            class ClearEventListsHandler : public MidiKeyboardManager<V>::VoiceHandlerBase {
                public:
                    virtual bool Process(MidiKey* pMidiKey) { pMidiKey->pEvents->clear(); return false; }
            };

            /**
             * Free all events of the current audio fragment cycle. Calling
             * this method will @b NOT free events scheduled past the current
             * fragment's boundary! (@see AbstractEngineChannel::delayedEvents).
             */
            void ClearEventListsOfCurrentFragment() {
                pEvents->clear();
                // empty MIDI key specific event lists
                ClearEventListsHandler handler;
                this->ProcessActiveVoices(&handler);

                // empty exclusive group specific event lists
                // (pInstrument == 0 could mean that LoadInstrument is
                // building new group event lists, so we must check
                // for that)
                if (pInstrument) ClearGroupEventLists();
            }

            // implementation of abstract methods derived from interface class 'InstrumentConsumer'

            /**
             * Will be called by the InstrumentResourceManager when the instrument
             * we are currently using on this EngineChannel is going to be updated,
             * so we can stop playback before that happens.
             */
            virtual void ResourceToBeUpdated(I* pResource, void*& pUpdateArg) OVERRIDE {
                dmsg(3,("EngineChannelBase: Received instrument update message.\n"));
                if (pEngine) pEngine->DisableAndLock();
                ResetInternal(false/*don't reset engine*/);
                this->pInstrument = NULL;
            }

            /**
             * Will be called by the InstrumentResourceManager when the instrument
             * update process was completed, so we can continue with playback.
             */
            virtual void ResourceUpdated(I* pOldResource, I* pNewResource, void* pUpdateArg) OVERRIDE {
                this->pInstrument = pNewResource; //TODO: there are couple of engine parameters we should update here as well if the instrument was updated (see LoadInstrument())
                if (pEngine) pEngine->Enable();
                bStatusChanged = true; // status of engine has changed, so set notify flag
            }

            /**
             * Will be called by the InstrumentResourceManager on progress changes
             * while loading or realoading an instrument for this EngineChannel.
             *
             * @param fProgress - current progress as value between 0.0 and 1.0
             */
            virtual void OnResourceProgress(float fProgress) OVERRIDE {
                this->InstrumentStat = int(fProgress * 100.0f);
                dmsg(7,("EngineChannelBase: progress %d%%", InstrumentStat));
                bStatusChanged = true; // status of engine has changed, so set notify flag
            }

            /**
             * Called on sustain pedal up events to check and if required,
             * launch release trigger voices on the respective active key.
             *
             * @param pEngineChannel - engine channel on which this event occurred on
             * @param itEvent - release trigger event (contains note number)
             */
            virtual void ProcessReleaseTriggerBySustain(RTList<Event>::Iterator& itEvent) OVERRIDE {
                if (!pEngine) return;
                pEngine->ProcessReleaseTriggerBySustain(this, itEvent);
            }

            void RenderActiveVoices(uint Samples) {
                RenderVoicesHandler handler(this, Samples);
                this->ProcessActiveVoices(&handler);

                SetVoiceCount(handler.VoiceCount);
                SetDiskStreamCount(handler.StreamCount);
            }

            /**
             * Called by real-time instrument script functions to schedule a
             * new note (new note-on event and a new @c Note object linked to it)
             * @a delay microseconds in future.
             * 
             * @b IMPORTANT: for the supplied @a delay to be scheduled
             * correctly, the passed @a pEvent must be assigned a valid
             * fragment time within the current audio fragment boundaries. That
             * fragment time will be used by this method as basis for
             * interpreting what "now" acutally is, and thus it will be used as
             * basis for calculating the precise scheduling time for @a delay.
             * The easiest way to achieve this is by copying a recent event
             * which happened within the current audio fragment cycle: i.e. the
             * original event which caused calling this method here, or by using
             * Event::copyTimefrom() method to only copy the time, without any
             * other event data.
             *
             * @param pEvent - note-on event to be scheduled in future (event
             *                 data will be copied)
             * @param delay - amount of microseconds in future (from now) when
             *                event shall be processed
             * @returns unique note ID of scheduled new note, or NULL on error
             */
            note_id_t ScheduleNoteMicroSec(const Event* pEvent, int delay) OVERRIDE {
                // add (copied) note-on event into scheduler queue
                const event_id_t noteOnEventID = ScheduleEventMicroSec(pEvent, delay);
                if (!noteOnEventID) return 0; // error
                // get access to (copied) event on the scheduler queue
                RTList<Event>::Iterator itEvent = pEvents->fromID(noteOnEventID);
                // stick a new note to the (copied) event on the queue
                const note_id_t noteID = pEngine->LaunchNewNote(this, itEvent);
                return noteID;
            }

            /**
             * Called by real-time instrument script functions to ignore the note
             * reflected by given note ID. The note's event will be freed immediately
             * to its event pool and this will prevent voices to be launched for the
             * note.
             *
             * NOTE: preventing a note by calling this method works only if the note
             * was launched within the current audio fragment cycle.
             *
             * @param id - unique ID of note to be dropped
             */
            void IgnoreNote(note_id_t id) OVERRIDE {
                Pool< Note<V> >* pNotePool =
                    dynamic_cast<NotePool<V>*>(pEngine)->GetNotePool();

                NoteIterator itNote = pNotePool->fromID(id);
                if (!itNote) return; // note probably already released

                // if the note already got active voices, then it is too late to drop it
                if (!itNote->pActiveVoices->isEmpty()) return;

                // if the original (note-on) event is not available anymore, then it is too late to drop it
                RTList<Event>::Iterator itEvent = pEvents->fromID(itNote->eventID);
                if (!itEvent) return;

                // drop the note
                pNotePool->free(itNote);

                // drop the original event
                pEvents->free(itEvent);
            }

            /**
             * Copies the note IDs of all currently active notes on this engine
             * channel to the note ID buffer @a dstBuf, and returns the amount
             * of note IDs that have been copied to the destination buffer.
             *
             * @param dstBuf  - destination buffer for note IDs
             * @param bufSize - size of destination buffer (as amount of max.
             *                  note IDs, not as amount of bytes)
             * @returns amount of note IDs that have been copied to buffer
             */
            uint AllNoteIDs(note_id_t* dstBuf, uint bufSize) OVERRIDE {
                uint n = 0;

                Pool< Note<V> >* pNotePool =
                    dynamic_cast<NotePool<V>*>(pEngine)->GetNotePool();

                RTList<uint>::Iterator iuiKey = this->pActiveKeys->first();
                RTList<uint>::Iterator end    = this->pActiveKeys->end();
                for(; iuiKey != end; ++iuiKey) {
                    MidiKey* pKey = &this->pMIDIKeyInfo[*iuiKey];
                    NoteIterator itNote = pKey->pActiveNotes->first();
                    for (; itNote; ++itNote) {
                        if (n >= bufSize) goto done;
                        dstBuf[n++] = pNotePool->getID(itNote);
                    }
                }
                done:
                return n;
            }

            RTList<R*>* pRegionsInUse;     ///< temporary pointer into the instrument change command, used by the audio thread
            I* pInstrument;

            template<class TV, class TRR, class TR, class TD, class TIM, class TI> friend class EngineBase;

        protected:
            EngineChannelBase() :
                MidiKeyboardManager<V>(this),
                InstrumentChangeCommandReader(InstrumentChangeCommand)
            {
                pInstrument = NULL;

                // reset the instrument change command struct (need to be done
                // twice, as it is double buffered)
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    cmd.pRegionsInUse = NULL;
                    cmd.pInstrument = NULL;
                    cmd.pScript = new InstrumentScript(this);
                    cmd.bChangeInstrument = false;
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    cmd.pRegionsInUse = NULL;
                    cmd.pInstrument = NULL;
                    cmd.pScript = new InstrumentScript(this);
                    cmd.bChangeInstrument = false;
                }
            }

            virtual ~EngineChannelBase() {
                InstrumentScript* previous = NULL; // prevent double free
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    if (cmd.pScript) {
                        previous = cmd.pScript;
                        delete cmd.pScript;
                        cmd.pScript = NULL;
                    }
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    if (cmd.pScript) {
                        if (previous != cmd.pScript)
                            delete cmd.pScript;
                        cmd.pScript = NULL;
                    }
                }
            }

            typedef typename RTList<V>::Iterator RTListVoiceIterator;

            class RenderVoicesHandler : public MidiKeyboardManager<V>::VoiceHandlerBase {
                public:
                    uint Samples;
                    uint VoiceCount;
                    uint StreamCount;
                    EngineChannelBase<V, R, I>* pChannel;

                    RenderVoicesHandler(EngineChannelBase<V, R, I>* channel, uint samples) :
                        Samples(samples), VoiceCount(0), StreamCount(0), pChannel(channel) { }

                    virtual void Process(RTListVoiceIterator& itVoice) {
                        // now render current voice
                        itVoice->Render(Samples);
                        if (itVoice->IsActive()) { // still active
                            if (!itVoice->Orphan) {
                                *(pChannel->pRegionsInUse->allocAppend()) = itVoice->GetRegion();
                            }
                            VoiceCount++;

                            if (itVoice->PlaybackState == Voice::playback_state_disk) {
                                if ((itVoice->DiskStreamRef).State != Stream::state_unused) StreamCount++;
                            }
                        }  else { // voice reached end, is now inactive
                            itVoice->VoiceFreed();
                            pChannel->FreeVoice(itVoice); // remove voice from the list of active voices
                        }
                    }
            };

            typedef typename SynchronizedConfig<InstrumentChangeCmd<R, I> >::Reader SyncConfInstrChangeCmdReader;

            SynchronizedConfig<InstrumentChangeCmd<R, I> > InstrumentChangeCommand;
            SyncConfInstrChangeCmdReader InstrumentChangeCommandReader;

            /** This method is not thread safe! */
            virtual void ResetInternal(bool bResetEngine) OVERRIDE {
                AbstractEngineChannel::ResetInternal(bResetEngine);

                MidiKeyboardManager<V>::Reset();
            }

            virtual void ResetControllers() OVERRIDE {
                AbstractEngineChannel::ResetControllers();

                MidiKeyboardManager<V>::SustainPedal   = false;
                MidiKeyboardManager<V>::SostenutoPedal = false;
            }

            /**
             * Unload the currently used and loaded real-time instrument script.
             * The source code of the script is retained, so that it can still
             * be reloaded.
             */
            void UnloadScriptInUse() {
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                    if (cmd.pScript) cmd.pScript->unload();
                }
                {
                    InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.SwitchConfig();
                    if (cmd.pScript) cmd.pScript->unload();
                }
                InstrumentChangeCommand.SwitchConfig(); // switch back to original one
            }

            /**
             * Load real-time instrument script and all its resources required
             * for the upcoming instrument change.
             *
             * @param text - source code of script
             */
            void LoadInstrumentScript(const String& text) {
                InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                // load the new script
                cmd.pScript->load(text);
            }

            /**
             * Changes the instrument for an engine channel.
             *
             * @param pInstrument - new instrument
             * @returns the resulting instrument change command after the
             *          command switch, containing the old instrument and
             *          the dimregions it is using
             */
            InstrumentChangeCmd<R, I>& ChangeInstrument(I* pInstrument) {
                InstrumentChangeCmd<R, I>& cmd = InstrumentChangeCommand.GetConfigForUpdate();
                cmd.pInstrument = pInstrument;
                cmd.bChangeInstrument = true;

                return InstrumentChangeCommand.SwitchConfig();
            }

            virtual void ProcessKeySwitchChange(int key) = 0;
    };

} // namespace LinuxSampler

#endif	/* __LS_ENGINECHANNELBASE_H__ */
