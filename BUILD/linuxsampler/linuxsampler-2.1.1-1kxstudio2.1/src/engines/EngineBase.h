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

#ifndef __LS_ENGINEBASE_H__
#define	__LS_ENGINEBASE_H__

#include "AbstractEngine.h"
#include "EngineChannelBase.h"
#include "common/DiskThreadBase.h"
#include "common/MidiKeyboardManager.h"
#include "InstrumentManager.h"
#include "../common/global_private.h"

// a bit headroom over CONFIG_MAX_VOICES to avoid minor complications i.e. under voice stealing conditions
#define MAX_NOTES_HEADROOM  3
#define GLOBAL_MAX_NOTES    (GLOBAL_MAX_VOICES * MAX_NOTES_HEADROOM)

namespace LinuxSampler {

    class AbstractEngineChannel;

    template <
        class V   /* Voice */,
        class RR  /* Root Region */,
        class R   /* Region */,
        class D   /* Disk Thread */,
        class IM  /* Instrument Manager */,
        class I   /* Instrument */
    >
    class EngineBase: public AbstractEngine, public RegionPools<R>, public NotePool<V> {

        public:
            typedef typename RTList< Note<V> >::Iterator NoteIterator;
            typedef typename RTList<V>::Iterator VoiceIterator;
            typedef typename Pool<V>::Iterator PoolVoiceIterator;
            typedef typename RTList<RR*>::Iterator RootRegionIterator;
            typedef typename MidiKeyboardManager<V>::MidiKey MidiKey;
            
            EngineBase() : noteIDPool(GLOBAL_MAX_NOTES), SuspendedRegions(128) {
                pDiskThread          = NULL;
                pNotePool            = new Pool< Note<V> >(GLOBAL_MAX_NOTES);
                pNotePool->setPoolElementIDsReservedBits(INSTR_SCRIPT_EVENT_ID_RESERVED_BITS);
                pVoicePool           = new Pool<V>(GLOBAL_MAX_VOICES);
                pRegionPool[0]       = new Pool<R*>(GLOBAL_MAX_VOICES);
                pRegionPool[1]       = new Pool<R*>(GLOBAL_MAX_VOICES);
                pVoiceStealingQueue  = new RTList<Event>(pEventPool);
                iMaxDiskStreams      = GLOBAL_MAX_STREAMS;

                // init all Voice objects in voice pool
                for (VoiceIterator iterVoice = pVoicePool->allocAppend();
                     iterVoice; iterVoice = pVoicePool->allocAppend())
                {
                    iterVoice->SetEngine(this);
                }
                pVoicePool->clear();

                // init all Note objects in note pool
                for (NoteIterator itNote = pNotePool->allocAppend(); itNote;
                     itNote = pNotePool->allocAppend())
                {
                    itNote->init(pVoicePool, &noteIDPool);
                }
                pNotePool->clear();

                ResetInternal();
                ResetScaleTuning();
                ResetSuspendedRegions();
            }

            virtual ~EngineBase() {
                if (pDiskThread) {
                    dmsg(1,("Stopping disk thread..."));
                    pDiskThread->StopThread();
                    delete pDiskThread;
                    dmsg(1,("OK\n"));
                }

                if (pNotePool) {
                    pNotePool->clear();
                    delete pNotePool;
                }

                if (pVoicePool) {
                    pVoicePool->clear();
                    delete pVoicePool;
                }

                if (pVoiceStealingQueue) delete pVoiceStealingQueue;

                if (pRegionPool[0]) delete pRegionPool[0];
                if (pRegionPool[1]) delete pRegionPool[1];
                ResetSuspendedRegions();
            }

            // implementation of abstract methods derived from class 'LinuxSampler::Engine'

            /**
             * Let this engine proceed to render the given amount of sample points.
             * The engine will iterate through all engine channels and render audio
             * for each engine channel independently. The calculated audio data of
             * all voices of each engine channel will be placed into the audio sum
             * buffers of the respective audio output device, connected to the
             * respective engine channel.
             *
             *  @param Samples - number of sample points to be rendered
             *  @returns       0 on success
             */
            virtual int RenderAudio(uint Samples) OVERRIDE {
                dmsg(8,("RenderAudio(Samples=%d)\n", Samples));

                // return if engine disabled
                if (EngineDisabled.Pop()) {
                    dmsg(5,("EngineBase: engine disabled (val=%d)\n",EngineDisabled.GetUnsafe()));
                    EngineDisabled.RttDone();
                    return 0;
                }

                // process requests for suspending / resuming regions (i.e. to avoid
                // crashes while these regions are modified by an instrument editor)
                ProcessSuspensionsChanges();

                // update time of start and end of this audio fragment (as events' time stamps relate to this)
                pEventGenerator->UpdateFragmentTime(Samples);

                // We only allow the given maximum number of voices to be spawned
                // in each audio fragment. All subsequent request for spawning new
                // voices in the same audio fragment will be ignored.
                VoiceSpawnsLeft = MaxVoices();

                // get all events from the engine's global input event queue which belong to the current fragment
                // (these are usually just SysEx messages)
                ImportEvents(Samples);

                // process engine global events (these are currently only MIDI System Exclusive messages)
                {
                    RTList<Event>::Iterator itEvent = pGlobalEvents->first();
                    RTList<Event>::Iterator end     = pGlobalEvents->end();
                    for (; itEvent != end; ++itEvent) {
                        switch (itEvent->Type) {
                            case Event::type_sysex:
                                dmsg(5,("Engine: Sysex received\n"));
                                ProcessSysex(itEvent);
                                break;
                            default: ; // noop
                        }
                    }
                }
                
                // In case scale tuning has been changed, recalculate pitch for
                // all active voices.
                ProcessScaleTuningChange();

                // reset internal voice counter (just for statistic of active voices)
                ActiveVoiceCountTemp = 0;

                HandleInstrumentChanges();

                // handle events on all engine channels
                for (int i = 0; i < engineChannels.size(); i++) {
                    ProcessEvents(engineChannels[i], Samples);
                }

                // render all 'normal', active voices on all engine channels
                for (int i = 0; i < engineChannels.size(); i++) {
                    RenderActiveVoices(engineChannels[i], Samples);
                }

                // now that all ordinary voices on ALL engine channels are rendered, render new stolen voices
                RenderStolenVoices(Samples);

                // handle audio routing for engine channels with FX sends
                for (int i = 0; i < engineChannels.size(); i++) {
                    AbstractEngineChannel* pChannel = static_cast<AbstractEngineChannel*>(engineChannels[i]);
                    if (pChannel->fxSends.empty()) continue; // ignore if no FX sends
                    RouteAudio(engineChannels[i], Samples);
                }

                // handle cleanup on all engine channels for the next audio fragment
                for (int i = 0; i < engineChannels.size(); i++) {
                    PostProcess(engineChannels[i]);
                }

                // Just for debugging: dump the amount of free Note objects to
                // the terminal (note due to the static variables being used,
                // this is currently just intended for debugging with only one
                // engine channel).
                #if (CONFIG_DEBUG_LEVEL >= 3)
                {
                    static int slice = 0;
                    static int noteCount = -1;
                    if (slice++ % 10 == 0) {
                        int n = pNotePool->countFreeElements();
                        if (n != noteCount) {
                            noteCount = n;
                            dmsg(1,("[%d] free Note objects count = %d\n", slice / 10, n));
                        }
                    }
                }
                #endif

                // empty the engine's event list for the next audio fragment
                ClearEventLists();

                // reset voice stealing for the next audio fragment
                pVoiceStealingQueue->clear();

                // just some statistics about this engine instance
                SetVoiceCount(ActiveVoiceCountTemp);
                if (VoiceCount() > ActiveVoiceCountMax) ActiveVoiceCountMax = VoiceCount();

                // in case regions were previously suspended and we killed voices
                // with disk streams due to that, check if those streams have finally
                // been deleted by the disk thread
                if (iPendingStreamDeletions) ProcessPendingStreamDeletions();

                // Release the instrument change command. (This has to
                // be done after all voices have been rendered and not
                // in HandleInstrumentChanges, as the RegionsInUse
                // list has been built up by the voice renderers.)
                for (int i = 0; i < engineChannels.size(); i++) {
                    EngineChannelBase<V, R, I>* channel =
                        static_cast<EngineChannelBase<V, R, I>*>(engineChannels[i]);
                    channel->InstrumentChangeCommandReader.Unlock();
                }
                FrameTime += Samples;

                EngineDisabled.RttDone();
                return 0;
            }

            virtual int MaxVoices() OVERRIDE { return pVoicePool->poolSize(); }

            virtual void SetMaxVoices(int iVoices) throw (Exception) OVERRIDE {
                if (iVoices < 1)
                    throw Exception("Maximum voices for an engine cannot be set lower than 1");

                SuspendAll();

                // NOTE: we need to clear pRegionsInUse before deleting pDimRegionPool,
                // otherwise memory corruption will occur if there are active voices (see bug #118)
                for (int iChannel = 0; iChannel < engineChannels.size(); iChannel++) {
                    EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannel]);
                    pChannel->ClearRegionsInUse();
                }

                if (pRegionPool[0]) delete pRegionPool[0];
                if (pRegionPool[1]) delete pRegionPool[1];

                pRegionPool[0] = new Pool<R*>(iVoices);
                pRegionPool[1] = new Pool<R*>(iVoices);

                for (int iChannel = 0; iChannel < engineChannels.size(); iChannel++) {
                    EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannel]);
                    pChannel->ResetRegionsInUse(pRegionPool);
                }

                // FIXME: Shouldn't all those pool elements be freed before resizing the pools?
                try {
                    pVoicePool->resizePool(iVoices);
                    pNotePool->resizePool(iVoices * MAX_NOTES_HEADROOM);
                    noteIDPool.resizePool(iVoices * MAX_NOTES_HEADROOM);
                } catch (...) {
                    throw Exception("FATAL: Could not resize voice pool!");
                }

                for (VoiceIterator iterVoice = pVoicePool->allocAppend();
                     iterVoice; iterVoice = pVoicePool->allocAppend())
                {
                    iterVoice->SetEngine(this);
                    iterVoice->pDiskThread = this->pDiskThread;
                }
                pVoicePool->clear();

                for (NoteIterator itNote = pNotePool->allocAppend(); itNote;
                     itNote = pNotePool->allocAppend())
                {
                    itNote->init(pVoicePool, &noteIDPool);
                }
                pNotePool->clear();

                PostSetMaxVoices(iVoices);
                ResumeAll();
            }
            
            /** Called after the new max number of voices is set and before resuming the engine. */
            virtual void PostSetMaxVoices(int iVoices) { }

            virtual uint DiskStreamCount() OVERRIDE { return (pDiskThread) ? pDiskThread->GetActiveStreamCount() : 0; }
            virtual uint DiskStreamCountMax() OVERRIDE { return (pDiskThread) ? pDiskThread->ActiveStreamCountMax : 0; }
            virtual int  MaxDiskStreams() OVERRIDE { return iMaxDiskStreams; }

            virtual void SetMaxDiskStreams(int iStreams) throw (Exception) OVERRIDE {
                if (iStreams < 0)
                    throw Exception("Maximum disk streams for an engine cannot be set lower than 0");

                SuspendAll();

                iMaxDiskStreams = iStreams;

                // reconnect to audio output device, because that will automatically
                // recreate the disk thread with the required amount of streams
                if (pAudioOutputDevice) Connect(pAudioOutputDevice);

                ResumeAll();
            }

            virtual String DiskStreamBufferFillBytes() OVERRIDE { return (pDiskThread) ? pDiskThread->GetBufferFillBytes() : ""; }
            virtual String DiskStreamBufferFillPercentage() OVERRIDE { return (pDiskThread) ? pDiskThread->GetBufferFillPercentage() : ""; }
            virtual InstrumentManager* GetInstrumentManager() OVERRIDE { return &instruments; }

            /**
             * Connect this engine instance with the given audio output device.
             * This method will be called when an Engine instance is created.
             * All of the engine's data structures which are dependant to the used
             * audio output device / driver will be (re)allocated and / or
             * adjusted appropriately.
             *
             * @param pAudioOut - audio output device to connect to
             */
            virtual void Connect(AudioOutputDevice* pAudioOut) OVERRIDE {
                // caution: don't ignore if connecting to the same device here,
                // because otherwise SetMaxDiskStreams() implementation won't work anymore!

                pAudioOutputDevice = pAudioOut;

                ResetInternal();

                // inform audio driver for the need of two channels
                try {
                    pAudioOutputDevice->AcquireChannels(2); // default stereo
                }
                catch (AudioOutputException e) {
                    String msg = "Audio output device unable to provide 2 audio channels, cause: " + e.Message();
                    throw Exception(msg);
                }

                this->MaxSamplesPerCycle = pAudioOutputDevice->MaxSamplesPerCycle();
                this->SampleRate         = pAudioOutputDevice->SampleRate();

                MinFadeOutSamples = int(double(SampleRate) * CONFIG_EG_MIN_RELEASE_TIME) - 1;
                if (MaxSamplesPerCycle < MinFadeOutSamples) {
                    std::cerr << "EngineBase: WARNING, CONFIG_EG_MIN_RELEASE_TIME "
                              << "too big for current audio fragment size & sampling rate! "
                              << "May lead to click sounds if voice stealing chimes in!\n" << std::flush;
                    // force volume ramp downs at the beginning of each fragment
                    MinFadeOutSamples = MaxSamplesPerCycle;
                    // lower minimum release time
                    const float minReleaseTime = (float) MaxSamplesPerCycle / (float) SampleRate;
                    pVoicePool->clear();
                    for (VoiceIterator iterVoice = pVoicePool->allocAppend(); iterVoice == pVoicePool->last(); iterVoice = pVoicePool->allocAppend()) {
                        iterVoice->CalculateFadeOutCoeff(minReleaseTime, SampleRate);
                    }
                    pVoicePool->clear();
                }

                // (re)create disk thread
                if (this->pDiskThread) {
                    dmsg(1,("Stopping disk thread..."));
                    this->pDiskThread->StopThread();
                    delete this->pDiskThread;
                    dmsg(1,("OK\n"));
                }
                this->pDiskThread = CreateDiskThread();

                if (!pDiskThread) {
                    dmsg(0,("EngineBase  new diskthread = NULL\n"));
                    exit(EXIT_FAILURE);
                }

                pVoicePool->clear();
                for (VoiceIterator iterVoice = pVoicePool->allocAppend(); iterVoice == pVoicePool->last(); iterVoice = pVoicePool->allocAppend()) {
                    iterVoice->pDiskThread = this->pDiskThread;
                    dmsg(3,("d"));
                }
                pVoicePool->clear();

                // update event generator
                pEventGenerator->SetSampleRate(pAudioOut->SampleRate());

                dmsg(1,("Starting disk thread..."));
                pDiskThread->StartThread();
                dmsg(1,("OK\n"));

                bool printEqInfo = true;
                for (VoiceIterator iterVoice = pVoicePool->allocAppend(); iterVoice == pVoicePool->last(); iterVoice = pVoicePool->allocAppend()) {
                    if (!iterVoice->pDiskThread) {
                        dmsg(0,("Engine -> voice::trigger: !pDiskThread\n"));
                        exit(EXIT_FAILURE);
                    }
                    
                    iterVoice->CreateEq();
                    
                    if(printEqInfo) {
                        iterVoice->PrintEqInfo();
                        printEqInfo = false;
                    }
                }
                pVoicePool->clear();
                
                // (re)create dedicated voice audio buffers
                //TODO: we could optimize resource usage a bit by just allocating these dedicated voice buffers when there is at least one engine channel with FX sends, because only in this case those special buffers are used actually, but since it would usually only save couple bytes in total, its probably not worth it
                if (pDedicatedVoiceChannelLeft)  delete pDedicatedVoiceChannelLeft;
                if (pDedicatedVoiceChannelRight) delete pDedicatedVoiceChannelRight;
                pDedicatedVoiceChannelLeft  = new AudioChannel(0, MaxSamplesPerCycle);
                pDedicatedVoiceChannelRight = new AudioChannel(1, MaxSamplesPerCycle);
            }
        
            // Implementattion for abstract method derived from Engine.
            virtual void ReconnectAudioOutputDevice() OVERRIDE {
                SuspendAll();
                if (pAudioOutputDevice) Connect(pAudioOutputDevice);
                ResumeAll();
            }

            /**
             * Similar to @c Disable() but this method additionally kills all voices
             * and disk streams and blocks until all voices and disk streams are actually
             * killed / deleted.
             *
             * @e Note: only the original calling thread is able to re-enable the
             * engine afterwards by calling @c ResumeAll() later on!
             */
            virtual void SuspendAll() {
                dmsg(2,("Engine: Suspending all ...\n"));
                // stop the engine, so we can safely modify the engine's
                // data structures from this foreign thread
                DisableAndLock();
                // we could also use the respective class member variable here,
                // but this is probably safer and cleaner
                int iPendingStreamDeletions = 0;
                // kill all voices on all engine channels the *die hard* way
                for (int iChannel = 0; iChannel < engineChannels.size(); iChannel++) {
                    EngineChannelBase<V, R, I>* pEngineChannel =
                            static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannel]);

                    iPendingStreamDeletions += pEngineChannel->KillAllVoicesImmediately();
                }
                // wait until all streams were actually deleted by the disk thread
                while (iPendingStreamDeletions) {
                    while (
                        iPendingStreamDeletions &&
                        pDiskThread->AskForDeletedStream() != Stream::INVALID_HANDLE
                    ) iPendingStreamDeletions--;
                    if (!iPendingStreamDeletions) break;
                    usleep(10000); // sleep for 10ms
                }
                dmsg(2,("EngineBase: Everything suspended.\n"));
            }

            /**
             * At the moment same as calling @c Enable() directly, but this might
             * change in future, so better call this method as counterpart to
             * @c SuspendAll() instead of @c Enable() !
             */
            virtual void ResumeAll() { Enable(); }

            /**
             * Order the engine to stop rendering audio for the given region.
             * Additionally this method will block until all voices and their disk
             * streams associated with that region are actually killed / deleted, so
             * one can i.e. safely modify the region with an instrument editor after
             * returning from this method.
             *
             * @param pRegion - region the engine shall stop using
             */
            virtual void Suspend(RR* pRegion) {
                dmsg(2,("EngineBase: Suspending Region %p ...\n",(void*)pRegion));
                {
                    LockGuard lock(SuspendedRegionsMutex);
                    SuspensionChangeOngoing.Set(true);
                    pPendingRegionSuspension = pRegion;
                    SuspensionChangeOngoing.WaitAndUnlockIf(true);
                }
                dmsg(2,("EngineBase: Region %p suspended.",(void*)pRegion));
            }

            /**
             * Orders the engine to resume playing back the given region, previously
             * suspended with @c Suspend() .
             *
             * @param pRegion - region the engine shall be allowed to use again
             */
            virtual void Resume(RR* pRegion) {
                dmsg(2,("EngineBase: Resuming Region %p ...\n",(void*)pRegion));
                {
                    LockGuard lock(SuspendedRegionsMutex);
                    SuspensionChangeOngoing.Set(true);
                    pPendingRegionResumption = pRegion;
                    SuspensionChangeOngoing.WaitAndUnlockIf(true);
                }
                dmsg(2,("EngineBase: Region %p resumed.\n",(void*)pRegion));
            }

            virtual void ResetSuspendedRegions() {
                SuspendedRegions.clear();
                iPendingStreamDeletions = 0;
                pPendingRegionSuspension = pPendingRegionResumption = NULL;
                SuspensionChangeOngoing.Set(false);
            }

            /**
             * Called by the engine's (audio) thread once per cycle to process requests
             * from the outer world to suspend or resume a given @c gig::Region .
             */
            virtual void ProcessSuspensionsChanges() {
                // process request for suspending one region
                if (pPendingRegionSuspension) {
                    // kill all voices on all engine channels that use this region
                    for (int iChannel = 0; iChannel < engineChannels.size(); iChannel++) {
                        EngineChannelBase<V, R, I>* pEngineChannel =
                                static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannel]);
                        SuspensionVoiceHandler handler(pPendingRegionSuspension);
                        pEngineChannel->ProcessActiveVoices(&handler);
                        iPendingStreamDeletions += handler.PendingStreamDeletions;
                    }
                    // make sure the region is not yet on the list
                    bool bAlreadySuspended = false;
                    RootRegionIterator iter = SuspendedRegions.first();
                    RootRegionIterator end  = SuspendedRegions.end();
                    for (; iter != end; ++iter) { // iterate through all suspended regions
                        if (*iter == pPendingRegionSuspension) { // found
                            bAlreadySuspended = true;
                            dmsg(1,("EngineBase: attempt to suspend an already suspended region !!!\n"));
                            break;
                        }
                    }
                    if (!bAlreadySuspended) {
                        // put the region on the list of suspended regions
                        RootRegionIterator iter = SuspendedRegions.allocAppend();
                        if (iter) {
                            *iter = pPendingRegionSuspension;
                        } else std::cerr << "EngineBase: Could not suspend Region, list is full. This is a bug!!!\n" << std::flush;
                    }
                    // free request slot for next caller (and to make sure that
                    // we're not going to process the same request in the next cycle)
                    pPendingRegionSuspension = NULL;
                    // if no disk stream deletions are pending, awaken other side, as
                    // we're done in this case
                    if (!iPendingStreamDeletions) SuspensionChangeOngoing.Set(false);
                }

                // process request for resuming one region
                if (pPendingRegionResumption) {
                    // remove region from the list of suspended regions
                    RootRegionIterator iter = SuspendedRegions.first();
                    RootRegionIterator end  = SuspendedRegions.end();
                    for (; iter != end; ++iter) { // iterate through all suspended regions
                        if (*iter == pPendingRegionResumption) { // found
                            SuspendedRegions.free(iter);
                            break; // done
                        }
                    }
                    // free request slot for next caller
                    pPendingRegionResumption = NULL;
                    // awake other side as we're done
                    SuspensionChangeOngoing.Set(false);
                }
            }

            /**
             * Called by the engine's (audio) thread once per cycle to check if
             * streams of voices that were killed due to suspension request have
             * finally really been deleted by the disk thread.
             */
            virtual void ProcessPendingStreamDeletions() {
                if (!iPendingStreamDeletions) return;
                //TODO: or shall we better store a list with stream handles instead of a scalar amount of streams to be deleted? might be safer
                while (
                    iPendingStreamDeletions &&
                    pDiskThread->AskForDeletedStream() != Stream::INVALID_HANDLE
                ) iPendingStreamDeletions--;
                // just for safety ...
                while (pDiskThread->AskForDeletedStream() != Stream::INVALID_HANDLE);
                // now that all disk streams are deleted, awake other side as
                // we're finally done with suspending the requested region
                if (!iPendingStreamDeletions) SuspensionChangeOngoing.Set(false);
            }

            /**
             * Returns @c true if the given region is currently set to be suspended
             * from being used, @c false otherwise.
             */
            virtual bool RegionSuspended(RR* pRegion) {
                if (SuspendedRegions.isEmpty()) return false;
                //TODO: or shall we use a sorted container instead of the RTList? might be faster ... or trivial ;-)
                RootRegionIterator iter = SuspendedRegions.first();
                RootRegionIterator end  = SuspendedRegions.end();
                for (; iter != end; ++iter)  // iterate through all suspended regions
                    if (*iter == pRegion) return true;
                return false;
            }

            // implementation of abstract method derived from class 'LinuxSampler::RegionPools'
            virtual Pool<R*>* GetRegionPool(int index) OVERRIDE {
                if (index < 0 || index > 1) throw Exception("Index out of bounds");
                return pRegionPool[index];
            }

            // implementation of abstract methods derived from class 'LinuxSampler::NotePool'
            virtual Pool<V>* GetVoicePool() OVERRIDE { return pVoicePool; }
            virtual Pool< Note<V> >* GetNotePool() OVERRIDE { return pNotePool; }
            virtual Pool<note_id_t>* GetNoteIDPool() OVERRIDE { return &noteIDPool; }

            D* GetDiskThread() { return pDiskThread; }

            //friend class EngineChannelBase<V, R, I>;

            static IM instruments;

        protected:
            class SuspensionVoiceHandler : public MidiKeyboardManager<V>::VoiceHandler {
            public:
                int PendingStreamDeletions;
                RR* pPendingRegionSuspension;

                SuspensionVoiceHandler(RR* pPendingRegionSuspension) {
                    PendingStreamDeletions = 0;
                    this->pPendingRegionSuspension = pPendingRegionSuspension;
                }

                virtual bool Process(MidiKey* pMidiKey) OVERRIDE {
                    NoteIterator  itNote  = pMidiKey->pActiveNotes->first();
                    VoiceIterator itVoice = itNote->pActiveVoices->first();
                    // if current key is not associated with this region, skip this key
                    if (itVoice->GetRegion()->GetParent() != pPendingRegionSuspension) return false;

                    return true;
                }

                virtual void Process(VoiceIterator& itVoice) OVERRIDE {
                    // request a notification from disk thread side for stream deletion
                    const Stream::Handle hStream = itVoice->KillImmediately(true);
                    if (hStream != Stream::INVALID_HANDLE) { // voice actually used a stream
                        PendingStreamDeletions++;
                    }
                    //NOTE: maybe we should call FreeVoice() here, shouldn't cause a harm though I think, since the voices should be freed by RenderActiveVoices() in the render loop, they are probably just freed a bit later than they could/should be
                }
            };

            Pool<R*>* pRegionPool[2]; ///< Double buffered pool, used by the engine channels to keep track of regions in use.
            int       MinFadeOutSamples;     ///< The number of samples needed to make an instant fade out (e.g. for voice stealing) without leading to clicks.
            D*        pDiskThread;

            int                          ActiveVoiceCountTemp;  ///< number of currently active voices (for internal usage, will be used for incrementation)
            VoiceIterator                itLastStolenVoice;     ///< Only for voice stealing: points to the last voice which was theft in current audio fragment, NULL otherwise.
            NoteIterator                 itLastStolenNote;      ///< Only for voice stealing: points to the last note from which was theft in current audio fragment, NULL otherwise.
            RTList<uint>::Iterator       iuiLastStolenKey;      ///< Only for voice stealing: key number of last key on which the last voice was theft in current audio fragment, NULL otherwise.
            EngineChannelBase<V, R, I>*  pLastStolenChannel;    ///< Only for voice stealing: points to the engine channel on which the previous voice was stolen in this audio fragment.
            VoiceIterator                itLastStolenVoiceGlobally; ///< Same as itLastStolenVoice, but engine globally
            NoteIterator                 itLastStolenNoteGlobally; ///< Same as itLastStolenNote, but engine globally
            RTList<uint>::Iterator       iuiLastStolenKeyGlobally;  ///< Same as iuiLastStolenKey, but engine globally
            RTList<Event>*               pVoiceStealingQueue;   ///< All voice-launching events which had to be postponed due to free voice shortage.
            Mutex                        ResetInternalMutex;    ///< Mutex to protect the ResetInternal function for concurrent usage (e.g. by the lscp and instrument loader threads).
            int iMaxDiskStreams;

            NoteBase* NoteByID(note_id_t id) OVERRIDE {
                NoteIterator itNote = GetNotePool()->fromID(id);
                if (!itNote) return NULL;
                return &*itNote;
            }

            /**
             * Gets a new @c Note object from the note pool, initializes it
             * appropriately, links it with requested parent note (if
             * requested), moves it to the appropriate key's list of active
             * notes it, and sticks the new note's unique ID to the
             * passed @a pNoteOnEvent.
             *
             * @param pEngineChannel - engine channel on which this event happened
             * @param pNoteOnEvent - event which caused this
             * @returns new note's unique ID (or zero on error)
             */
            note_id_t LaunchNewNote(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNoteOnEvent) OVERRIDE {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);
                Pool< Note<V> >* pNotePool = GetNotePool();

                if (pNotePool->poolIsEmpty()) {
                    dmsg(1,("Engine: Could not launch new note; Note pool empty!\n"));
                    return 0; // error
                }

                // create a new note (for new voices to be assigned to)
                //NoteIterator itNewNote = pKey->pActiveNotes->allocAppend();
                NoteIterator itNewNote = pNotePool->allocAppend();
                const note_id_t newNoteID = pNotePool->getID(itNewNote);

                // remember the engine's time when this note was triggered exactly
                itNewNote->triggerSchedTime = itNoteOnEvent->SchedTime();

                // usually the new note (and its subsequent voices) will be
                // allocated on the key provided by the event's note number,
                // however if this new note is requested not to be a regular
                // note, but rather a child note, then this new note will be
                // allocated on the parent note's key instead in order to
                // release the child note simultaniously with its parent note
                itNewNote->hostKey = itNoteOnEvent->Param.Note.Key;

                // in case this new note was requested to be a child note,
                // then retrieve its parent note and link them with each other
                const note_id_t parentNoteID = itNoteOnEvent->Param.Note.ParentNoteID;
                if (parentNoteID) {
                    NoteIterator itParentNote = pNotePool->fromID(parentNoteID);                        
                    if (itParentNote) {
                        RTList<note_id_t>::Iterator itChildNoteID = itParentNote->pChildNotes->allocAppend();
                        if (itChildNoteID) {
                            // link parent and child note with each other
                            *itChildNoteID = newNoteID;
                            itNewNote->parentNoteID = parentNoteID;
                            itNewNote->hostKey = itParentNote->hostKey;
                        } else {    
                            dmsg(1,("Engine: Could not assign new note as child note; Note ID pool empty!\n"));
                            pNotePool->free(itNewNote);
                            return 0; // error
                        }
                    } else {
                        // the parent note was apparently released already, so
                        // free the new note again and inform caller that it
                        // should drop the event
                        dmsg(3,("Engine: Could not assign new note as child note; Parent note is gone!\n"));
                        pNotePool->free(itNewNote);
                        return 0; // error
                    }
                }

                dmsg(2,("Launched new note on host key %d\n", itNewNote->hostKey));

                // copy event which caused this note
                itNewNote->cause = *itNoteOnEvent;
                itNewNote->eventID = pEventPool->getID(itNoteOnEvent);
                if (!itNewNote->eventID) {
                    dmsg(0,("Engine: No valid event ID resolved for note. This is a bug!!!\n"));
                }

                // move new note to its host key
                MidiKey* pKey = &pChannel->pMIDIKeyInfo[itNewNote->hostKey];
                itNewNote.moveToEndOf(pKey->pActiveNotes);
                pChannel->markKeyAsActive(pKey);

                // assign unique note ID of this new note to the original note on event
                itNoteOnEvent->Param.Note.ID = newNoteID;

                return newNoteID; // success
            }

            /**
             * Dispatch and handle all events in this audio fragment for the given
             * engine channel.
             *
             * @param pEngineChannel - engine channel on which events should be
             *                         processed
             * @param Samples        - amount of sample points to be processed in
             *                         this audio fragment cycle
             */
            void ProcessEvents(EngineChannel* pEngineChannel, uint Samples) {
                // get all events from the engine channels's input event queue which belong to the current fragment
                // (these are the common events like NoteOn, NoteOff, ControlChange, etc.)
                AbstractEngineChannel* pChannel = static_cast<AbstractEngineChannel*>(pEngineChannel);
                pChannel->ImportEvents(Samples);

                // if a valid real-time instrument script is loaded, pre-process
                // the event list by running the script now, since the script
                // might filter events or add new ones for this cycle
                if (pChannel->pScript) {
                    const sched_time_t fragmentEndTime = pEventGenerator->schedTimeAtCurrentFragmentEnd();

                    // resume suspended script executions been scheduled for
                    // this audio fragment cycle (which were suspended in a
                    // previous audio fragment cycle)
                    ProcessSuspendedScriptEvents(pChannel, fragmentEndTime);

                    // spawn new script executions for the new MIDI events of
                    // this audio fragment cycle
                    //
                    // FIXME: it would probably be better to just schedule newly spawned script executions here and then execute them altogether with already suspended ones all at once in order of all their scheduled timing
                    for (RTList<Event>::Iterator itEvent = pChannel->pEvents->first(),
                        end = pChannel->pEvents->end(); itEvent != end; )
                    {
                        //HACK: avoids iterator invalidation which might happen below since an instrument script might drop an event by direct raw pointer access (it would be considerable to extend the Iterator class to detect and circumvent this case by checking the "reincarnation" member variable).
                        RTList<Event>::Iterator itNext = itEvent;
                        ++itNext;

                        switch (itEvent->Type) {
                            case Event::type_note_on:
                                if (pChannel->pScript->handlerNote)
                                    ProcessEventByScript(pChannel, itEvent, pChannel->pScript->handlerNote);
                                break;
                            case Event::type_note_off:
                                if (pChannel->pScript->handlerRelease)
                                    ProcessEventByScript(pChannel, itEvent, pChannel->pScript->handlerRelease);
                                break;
                            case Event::type_control_change:
                            case Event::type_channel_pressure:
                            case Event::type_pitchbend:
                                if (pChannel->pScript->handlerController)
                                    ProcessEventByScript(pChannel, itEvent, pChannel->pScript->handlerController);                            
                                break;
                            case Event::type_note_pressure:
                                //TODO: ...
                                break;

                            case Event::type_sysex:
                                //TODO: ...
                                break;

                            case Event::type_cancel_release_key:
                            case Event::type_release_key:
                            case Event::type_release_note:
                            case Event::type_play_note:
                            case Event::type_stop_note:
                            case Event::type_kill_note:
                            case Event::type_note_synth_param:
                                break; // noop
                        }

                        // see HACK comment above
                        itEvent = itNext;
                    }

                    // this has to be run again, since the newly spawned scripts
                    // above may have cause suspended scripts that must be
                    // resumed within this same audio fragment cycle
                    //
                    // FIXME: see FIXME comment above
                    ProcessSuspendedScriptEvents(pChannel, fragmentEndTime);
                }

                // if there are any delayed events scheduled for the current
                // audio fragment cycle, then move and sort them into the main
                // event list
                if (!pChannel->delayedEvents.queue.isEmpty()) {
                    dmsg(5,("Engine: There are delayed MIDI events (total queue size: %d) ...\n", pChannel->delayedEvents.queue.size()));
                    const sched_time_t fragmentEndTime = pEventGenerator->schedTimeAtCurrentFragmentEnd();
                    RTList<Event>::Iterator itEvent = pChannel->pEvents->first();
                    while (true) {
                        RTList<ScheduledEvent>::Iterator itDelayedEventNode =
                            pEventGenerator->popNextScheduledEvent(
                                pChannel->delayedEvents.queue,
                                pChannel->delayedEvents.schedulerNodes,
                                fragmentEndTime
                            );
                        if (!itDelayedEventNode) break;
                        // get the actual delayed event object and free the used scheduler node
                        RTList<Event>::Iterator itDelayedEvent = itDelayedEventNode->itEvent;
                        pChannel->delayedEvents.schedulerNodes.free(itDelayedEventNode);
                        if (!itDelayedEvent) { // should never happen, but just to be sure ...
                            dmsg(1,("Engine: Oops, invalid delayed event!\n"));
                            continue;
                        }
                        // skip all events on main event list which have a time
                        // before (or equal to) the delayed event to be inserted
                        for (; itEvent && itEvent->FragmentPos() <= itDelayedEvent->FragmentPos();
                             ++itEvent);
                        // now move delayed event from delayedEvents.pList to
                        // the current position on the main event list
                        itEvent = itDelayedEvent.moveBefore(itEvent);
                        dmsg(5,("Engine: Inserted event of type %d into main event list (queue size: %d).\n", itEvent->Type, pChannel->delayedEvents.queue.size()));
                    }
                    dmsg(5,("Engine: End of delayed events (total queue size: %d).\n", pChannel->delayedEvents.queue.size()));
                }

                // now process all events regularly
                {
                    RTList<Event>::Iterator itEvent = pChannel->pEvents->first();
                    RTList<Event>::Iterator end     = pChannel->pEvents->end();
                    for (; itEvent != end; ++itEvent) {
                        switch (itEvent->Type) {
                            case Event::type_note_on:
                                dmsg(5,("Engine: Note on received\n"));
                                ProcessNoteOn((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_play_note:
                                dmsg(5,("Engine: Play Note received\n"));
                                ProcessNoteOn((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_note_off:
                                dmsg(5,("Engine: Note off received\n"));
                                ProcessNoteOff((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_stop_note:
                                dmsg(5,("Engine: Stop Note received\n"));
                                ProcessNoteOff((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_kill_note:
                                dmsg(5,("Engine: Kill Note received\n"));
                                ProcessKillNote((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_control_change:
                                dmsg(5,("Engine: MIDI CC received\n"));
                                ProcessControlChange((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_channel_pressure:
                                dmsg(5,("Engine: MIDI Chan. Pressure received\n"));
                                ProcessChannelPressure((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_note_pressure:
                                dmsg(5,("Engine: MIDI Note Pressure received\n"));
                                ProcessPolyphonicKeyPressure((EngineChannel*)itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_pitchbend:
                                dmsg(5,("Engine: Pitchbend received\n"));
                                ProcessPitchbend(static_cast<AbstractEngineChannel*>(itEvent->pEngineChannel), itEvent);
                                break;
                            case Event::type_note_synth_param:
                                dmsg(5,("Engine: Note Synth Param received\n"));
                                ProcessNoteSynthParam(itEvent->pEngineChannel, itEvent);
                                break;
                            case Event::type_sysex:
                                break; // TODO ...

                            case Event::type_cancel_release_key:
                            case Event::type_release_key:
                            case Event::type_release_note:
                                break; // noop
                        }
                    }
                }

                // reset voice stealing for the next engine channel (or next audio fragment)
                itLastStolenVoice         = VoiceIterator();
                itLastStolenVoiceGlobally = VoiceIterator();
                itLastStolenNote          = NoteIterator();
                itLastStolenNoteGlobally  = NoteIterator();
                iuiLastStolenKey          = RTList<uint>::Iterator();
                iuiLastStolenKeyGlobally  = RTList<uint>::Iterator();
                pLastStolenChannel        = NULL;
            }

            /**
             * Run all suspended script execution instances which are scheduled
             * to be resumed for the current audio fragment cycle.
             * 
             * @param pChannel - engine channel on which suspended events occurred
             */
            void ProcessSuspendedScriptEvents(AbstractEngineChannel* pChannel, const sched_time_t fragmentEndTime) {
                while (true) {
                    RTList<ScriptEvent>::Iterator itEvent =
                        pEventGenerator->popNextScheduledScriptEvent(
                            pChannel->pScript->suspendedEvents,
                            *pChannel->pScript->pEvents, fragmentEndTime
                        );
                    if (!itEvent) break;
                    ResumeScriptEvent(pChannel, itEvent);
                }
            }

            /** @brief Call instrument script's event handler for this event.
             *
             * Causes a new execution instance of the currently loaded real-time
             * instrument script's event handler (callback) to be spawned for
             * the given MIDI event.
             *
             * @param pChannel - engine channel on which the MIDI event occurred
             * @param itEvent - MIDI event that causes this new script execution
             * @param pEventHandler - script's event handler to be executed
             */
            void ProcessEventByScript(AbstractEngineChannel* pChannel, RTList<Event>::Iterator& itEvent, VMEventHandler* pEventHandler) {
                const int key = itEvent->Param.Note.Key; // even if this is not a note on/off event, accessing it does not mean any harm
                // check if polyphonic data is passed from "note" to "release"
                // script event handlers
                if (pEventHandler == pChannel->pScript->handlerRelease &&
                    pChannel->pScript->handlerNote &&
                    pChannel->pScript->handlerNote->isPolyphonic() &&
                    pChannel->pScript->handlerRelease->isPolyphonic() &&
                    !pChannel->pScript->pKeyEvents[key]->isEmpty())
                {
                    // polyphonic variable data is used/passed from "note" to
                    // "release" script callback, so we have to recycle the
                    // original "note on" script event(s)
                    RTList<ScriptEvent>::Iterator it  = pChannel->pScript->pKeyEvents[key]->first();
                    RTList<ScriptEvent>::Iterator end = pChannel->pScript->pKeyEvents[key]->end();
                    for (; it != end; ++it) {
                        ProcessScriptEvent(
                            pChannel, itEvent, pEventHandler, it
                        );
                    }
                } else {
                    // no polyphonic data is used/passed from "note" to
                    // "release" script callback, so just use a new fresh
                    // script event object
                    RTList<ScriptEvent>::Iterator itScriptEvent =
                        pChannel->pScript->pEvents->allocAppend();
                    // if event handler uses polyphonic variables, reset them
                    // to zero values before starting to execute the handler
                    if (pEventHandler->isPolyphonic())
                        itScriptEvent->execCtx->resetPolyphonicData();
                    ProcessScriptEvent(
                        pChannel, itEvent, pEventHandler, itScriptEvent
                    );
                }
            }

            /** @brief Spawn new execution instance of an instrument script handler.
             * 
             * Will be called to initiate a new execution of a real-time
             * instrument script event right from the start of the script's
             * respective handler. If script execution did not complete after
             * calling this method, the respective script exeuction is then
             * suspended and a call to ResumeScriptEvent() will be used next
             * time to continue its execution.
             * 
             * @param pChannel - engine channel this script is running for
             * @param itEvent - event which caused execution of this script
             *                  event handler
             * @param pEventHandler - VM representation of event handler to be
             *                        executed
             * @param itScriptEvent - script event that shall be processed
             */
            void ProcessScriptEvent(AbstractEngineChannel* pChannel, RTList<Event>::Iterator& itEvent, VMEventHandler* pEventHandler, RTList<ScriptEvent>::Iterator& itScriptEvent) {
                if (!itScriptEvent) return; // not a valid script event (i.e. because no free script event was left in the script event pool)

                // fill the list of script handlers to be executed by this event
                int i = 0;
                itScriptEvent->handlers[i++] = pEventHandler; // actual event handler (i.e. note, controller)
                itScriptEvent->handlers[i] = NULL; // NULL termination of list

                // initialize/reset other members
                itScriptEvent->cause = *itEvent;
                itScriptEvent->scheduleTime = itEvent->SchedTime();
                itScriptEvent->currentHandler = 0;
                itScriptEvent->executionSlices = 0;
                itScriptEvent->ignoreAllWaitCalls = false;
                itScriptEvent->handlerType = pEventHandler->eventHandlerType();
                itScriptEvent->parentHandlerID = 0;
                itScriptEvent->childHandlerID[0] = 0;
                itScriptEvent->autoAbortByParent = false;
                itScriptEvent->forkIndex = 0;
                // this is the native representation of the $EVENT_ID script variable
                itScriptEvent->id =
                    (itEvent->Type == Event::type_note_on)
                        ? ScriptID::fromNoteID( itEvent->Param.Note.ID )
                        : ScriptID::fromEventID( pEventPool->getID(itEvent) );

                // run script handler(s)
                VMExecStatus_t res = pScriptVM->exec(
                    pChannel->pScript->parserContext, &*itScriptEvent
                );

                // was the script suspended?
                if (res & VM_EXEC_SUSPENDED) { // script was suspended ...
                    // in case the script was suspended, keep it on the allocated
                    // ScriptEvent list to be resume at the scheduled time in future,
                    // additionally insert it into a sorted time queue
                    pEventGenerator->scheduleAheadMicroSec(
                        pChannel->pScript->suspendedEvents, // scheduler queue
                        *itScriptEvent, // script event
                        itScriptEvent->cause.FragmentPos(), // current time of script event (basis for its next execution)
                        itScriptEvent->execCtx->suspensionTimeMicroseconds() // how long shall it be suspended
                    );
                } else { // script execution has finished without 'suspended' status ...
                    // if "polyphonic" variable data is passed from script's
                    // "note" event handler to its "release" event handler, then
                    // the script event must be kept and recycled for the later
                    // occuring "release" script event ...
                    if (pEventHandler == pChannel->pScript->handlerNote &&
                        pChannel->pScript->handlerRelease &&
                        pChannel->pScript->handlerNote->isPolyphonic() &&
                        pChannel->pScript->handlerRelease->isPolyphonic())
                    {
                        const int key = itEvent->Param.Note.Key;
                        itScriptEvent.moveToEndOf(pChannel->pScript->pKeyEvents[key & 127]);
                    } else {
                        // ... otherwise if no polyphonic data is passed and
                        // script's execution has finished without suspension
                        // status, then free the script event for a new future
                        // script event to be triggered from start
                        pChannel->pScript->pEvents->free(itScriptEvent);
                    }
                }
            }

            /** @brief Resume execution of instrument script.
             *
             * Will be called to resume execution of a real-time instrument
             * script event which has been suspended previously.
             *
             * Script execution might be suspended for various reasons. Usually
             * a script will be suspended if the script called the built-in
             * "wait()" function, but it might also be suspended automatically
             * if the script took too much execution time in an audio fragment
             * cycle. So in the latter case automatic suspension is performed in
             * order to avoid harm for the sampler's overall real-time
             * requirements.
             *
             * @param pChannel - engine channel this script is running for
             * @param itScriptEvent - script execution that shall be resumed
             */
            void ResumeScriptEvent(AbstractEngineChannel* pChannel, RTList<ScriptEvent>::Iterator& itScriptEvent) {
                VMEventHandler* handler = itScriptEvent->handlers[itScriptEvent->currentHandler];

                // run script
                VMExecStatus_t res = pScriptVM->exec(
                    pChannel->pScript->parserContext, &*itScriptEvent
                );

                // was the script suspended?
                if (res & VM_EXEC_SUSPENDED) {
                    // in case the script was suspended, keep it on the allocated
                    // ScriptEvent list to be resume at the scheduled time in future,
                    // additionally insert it into a sorted time queue
                    pEventGenerator->scheduleAheadMicroSec(
                        pChannel->pScript->suspendedEvents, // scheduler queue
                        *itScriptEvent, // script event
                        itScriptEvent->cause.FragmentPos(), // current time of script event (basis for its next execution)
                        itScriptEvent->execCtx->suspensionTimeMicroseconds() // how long shall it be suspended
                    );
                } else { // script execution has finished without 'suspended' status ...
                    // if "polyphonic" variable data is passed from script's
                    // "note" event handler to its "release" event handler, then
                    // the script event must be kept and recycled for the later
                    // occuring "release" script event ...
                    if (handler && handler == pChannel->pScript->handlerNote &&
                        pChannel->pScript->handlerRelease &&
                        pChannel->pScript->handlerNote->isPolyphonic() &&
                        pChannel->pScript->handlerRelease->isPolyphonic())
                    {
                        const int key = itScriptEvent->cause.Param.Note.Key;
                        itScriptEvent.moveToEndOf(pChannel->pScript->pKeyEvents[key & 127]);
                    } else {
                        // ... otherwise if no polyphonic data is passed and
                        // script's execution has finished without suspension
                        // status, then free the script event for a new future
                        // script event to be triggered from start
                        pChannel->pScript->pEvents->free(itScriptEvent);
                    }
                }
            }

            /**
             *  Will be called by LaunchVoice() method in case there are no free
             *  voices left. This method will select and kill one old voice for
             *  voice stealing and postpone the note-on event until the selected
             *  voice actually died.
             *
             *  @param pEngineChannel - engine channel on which this event occurred on
             *  @param itNoteOnEvent - key, velocity and time stamp of the event
             *  @returns 0 on success, a value < 0 if no active voice could be picked for voice stealing
             */
            int StealVoice(EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNoteOnEvent) {
                dmsg(3,("StealVoice()\n"));
                if (VoiceSpawnsLeft <= 0) {
                    dmsg(1,("Max. voice thefts per audio fragment reached (you may raise CONFIG_MAX_VOICES).\n"));
                    return -1;
                }

                EngineChannelBase<V, R, I>* pEngineChn = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                if (pEventPool->poolIsEmpty()) {
                    dmsg(1,("Event pool emtpy!\n"));
                    return -1;
                }

                if (!pEngineChn->StealVoice(itNoteOnEvent, &itLastStolenVoice, &itLastStolenNote, &iuiLastStolenKey)) {
                    --VoiceSpawnsLeft;
                    return 0;
                }

                // if we couldn't steal a voice from the same engine channel then
                // steal oldest voice on the oldest key from any other engine channel
                // (the smaller engine channel number, the higher priority)
                EngineChannelBase<V, R, I>*  pSelectedChannel;
                int                          iChannelIndex;
                VoiceIterator                itSelectedVoice;

                #if CONFIG_DEVMODE
                EngineChannel* pBegin = NULL; // to detect endless loop
                #endif

                // select engine channel
                if (pLastStolenChannel) {
                    pSelectedChannel = pLastStolenChannel;
                    iChannelIndex    = pSelectedChannel->iEngineIndexSelf;
                } else { // pick the engine channel followed by this engine channel
                    iChannelIndex    = (pEngineChn->iEngineIndexSelf + 1) % engineChannels.size();
                    pSelectedChannel = static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannelIndex]);
                }

                // if we already stole in this fragment, try to proceed on same note
                if (this->itLastStolenVoiceGlobally) {
                    itSelectedVoice = this->itLastStolenVoiceGlobally;
                    do {
                        ++itSelectedVoice;
                    } while (itSelectedVoice && !itSelectedVoice->IsStealable()); // proceed iterating if voice was created in this fragment cycle
                }
                // did we find a 'stealable' voice?
                if (itSelectedVoice && itSelectedVoice->IsStealable()) {
                    // remember which voice we stole, so we can simply proceed on next voice stealing
                    this->itLastStolenVoiceGlobally = itSelectedVoice;
                    // done
                    goto stealable_voice_found;
                }

                // get (next) oldest note
                if (this->itLastStolenNoteGlobally) {
                    for (NoteIterator itNote = ++this->itLastStolenNoteGlobally;
                         itNote; ++itNote)
                    {
                        for (itSelectedVoice = itNote->pActiveVoices->first(); itSelectedVoice; ++itSelectedVoice) {
                            // proceed iterating if voice was created in this audio fragment cycle
                            if (itSelectedVoice->IsStealable()) {
                                // remember which voice of which note we stole, so we can simply proceed on next voice stealing
                                this->itLastStolenNoteGlobally  = itNote;
                                this->itLastStolenVoiceGlobally = itSelectedVoice;
                                goto stealable_voice_found; // selection succeeded
                            }
                        }
                    }
                }

                #if CONFIG_DEVMODE
                pBegin = pSelectedChannel; // to detect endless loop
                #endif // CONFIG_DEVMODE

                while (true) { // iterate through engine channels                        
                    // get (next) oldest key
                    RTList<uint>::Iterator iuiSelectedKey = (this->iuiLastStolenKeyGlobally) ? ++this->iuiLastStolenKeyGlobally : pSelectedChannel->pActiveKeys->first();
                    this->iuiLastStolenKeyGlobally = RTList<uint>::Iterator(); // to prevent endless loop (see line above)
                    while (iuiSelectedKey) {
                        MidiKey* pSelectedKey = &pSelectedChannel->pMIDIKeyInfo[*iuiSelectedKey];

                        for (NoteIterator itNote = pSelectedKey->pActiveNotes->first(),
                             itNotesEnd = pSelectedKey->pActiveNotes->end();
                             itNote != itNotesEnd; ++itNote)
                        {
                            itSelectedVoice = itNote->pActiveVoices->first();
                            // proceed iterating if voice was created in this fragment cycle
                            while (itSelectedVoice && !itSelectedVoice->IsStealable()) ++itSelectedVoice;
                            // found a "stealable" voice ?
                            if (itSelectedVoice && itSelectedVoice->IsStealable()) {
                                // remember which voice of which note on which key on which engine channel we stole, so we can simply proceed on next voice stealing
                                this->iuiLastStolenKeyGlobally  = iuiSelectedKey;
                                this->itLastStolenNoteGlobally  = itNote;
                                this->itLastStolenVoiceGlobally = itSelectedVoice;
                                this->pLastStolenChannel        = pSelectedChannel;
                                goto stealable_voice_found; // selection succeeded
                            }
                        }
                        ++iuiSelectedKey; // get next key on current engine channel
                    }
                    // get next engine channel
                    iChannelIndex    = (iChannelIndex + 1) % engineChannels.size();
                    pSelectedChannel = static_cast<EngineChannelBase<V, R, I>*>(engineChannels[iChannelIndex]);

                    #if CONFIG_DEVMODE
                    if (pSelectedChannel == pBegin) {
                        dmsg(1,("FATAL ERROR: voice stealing endless loop!\n"));
                        dmsg(1,("VoiceSpawnsLeft=%d.\n", VoiceSpawnsLeft));
                        dmsg(1,("Exiting.\n"));
                        exit(-1);
                    }
                    #endif // CONFIG_DEVMODE
                }

                // jump point if a 'stealable' voice was found
                stealable_voice_found:

                #if CONFIG_DEVMODE
                if (!itSelectedVoice->IsActive()) {
                    dmsg(1,("EngineBase: ERROR, tried to steal a voice which was not active !!!\n"));
                    return -1;
                }
                #endif // CONFIG_DEVMODE

                // now kill the selected voice
                itSelectedVoice->Kill(itNoteOnEvent);

                --VoiceSpawnsLeft;

                return 0; // success
            }

            void HandleInstrumentChanges() {
                bool instrumentChanged = false;
                for (int i = 0; i < engineChannels.size(); i++) {
                    EngineChannelBase<V, R, I>* pEngineChannel =
                        static_cast<EngineChannelBase<V, R, I>*>(engineChannels[i]);

                    // as we're going to (carefully) write some status to the
                    // synchronized struct, we cast away the const
                    InstrumentChangeCmd<R, I>& cmd =
                        const_cast<InstrumentChangeCmd<R, I>&>(pEngineChannel->InstrumentChangeCommandReader.Lock());

                    pEngineChannel->pRegionsInUse = cmd.pRegionsInUse;
                    pEngineChannel->pRegionsInUse->clear();

                    if (cmd.bChangeInstrument) {
                        // change instrument
                        dmsg(5,("Engine: instrument change command received\n"));
                        cmd.bChangeInstrument = false;
                        pEngineChannel->pInstrument = cmd.pInstrument;
                        pEngineChannel->pScript =
                            cmd.pScript->bHasValidScript ? cmd.pScript : NULL;
                        instrumentChanged = true;

                        pEngineChannel->MarkAllActiveVoicesAsOrphans();

                        // the script's "init" event handler is only executed
                        // once (when the script is loaded or reloaded)
                        if (pEngineChannel->pScript && pEngineChannel->pScript->handlerInit) {
                            dmsg(5,("Engine: exec handlerInit %p\n", pEngineChannel->pScript->handlerInit));
                            RTList<ScriptEvent>::Iterator itScriptEvent =
                                pEngineChannel->pScript->pEvents->allocAppend();

                            itScriptEvent->cause = pEventGenerator->CreateEvent(0);
                            itScriptEvent->cause.Type = (Event::type_t) -1; // some invalid type to avoid random event processing
                            itScriptEvent->cause.pEngineChannel = pEngineChannel;
                            itScriptEvent->cause.pMidiInputPort = pEngineChannel->GetMidiInputPort();
                            itScriptEvent->id = 0;
                            itScriptEvent->handlers[0] = pEngineChannel->pScript->handlerInit;
                            itScriptEvent->handlers[1] = NULL;
                            itScriptEvent->currentHandler = 0;
                            itScriptEvent->executionSlices = 0;
                            itScriptEvent->ignoreAllWaitCalls = false;
                            itScriptEvent->handlerType = VM_EVENT_HANDLER_INIT;
                            itScriptEvent->parentHandlerID = 0;
                            itScriptEvent->childHandlerID[0] = 0;
                            itScriptEvent->autoAbortByParent = false;
                            itScriptEvent->forkIndex = 0;

                            VMExecStatus_t res;
                            size_t instructionsCount = 0;
                            const size_t maxInstructions = 200000; // aiming approx. 1 second max. (based on very roughly 5us / instruction)
                            bool bWarningShown = false;
                            do {
                                res = pScriptVM->exec(
                                    pEngineChannel->pScript->parserContext, &*itScriptEvent
                                );
                                instructionsCount += itScriptEvent->execCtx->instructionsPerformed();
                                if (instructionsCount > maxInstructions && !bWarningShown) {
                                    bWarningShown = true;
                                    dmsg(0,("[ScriptVM] WARNING: \"init\" event handler of instrument script executing for long time!\n"));
                                }
                            } while (res & VM_EXEC_SUSPENDED && !(res & VM_EXEC_ERROR));

                            pEngineChannel->pScript->pEvents->free(itScriptEvent);
                        }
                    }
                }

                if (instrumentChanged) {
                    //TODO: this is a lazy solution ATM and not safe in case somebody is currently editing the instrument we're currently switching to (we should store all suspended regions on instrument manager side and when switching to another instrument copy that list to the engine's local list of suspensions
                    ResetSuspendedRegions();
                }
            }

            /**
             * Render all 'normal' voices (that is voices which were not stolen in
             * this fragment) on the given engine channel.
             *
             * @param pEngineChannel - engine channel on which audio should be
             *                         rendered
             * @param Samples        - amount of sample points to be rendered in
             *                         this audio fragment cycle
             */
            void RenderActiveVoices(EngineChannel* pEngineChannel, uint Samples) {
                #if !CONFIG_PROCESS_MUTED_CHANNELS
                if (pEngineChannel->GetMute()) return; // skip if sampler channel is muted
                #endif

                EngineChannelBase<V, R, I>* pChannel =
                    static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);
                pChannel->RenderActiveVoices(Samples);

                ActiveVoiceCountTemp += pEngineChannel->GetVoiceCount();
            }

            /**
             * Render all stolen voices (only voices which were stolen in this
             * fragment) on the given engine channel. Stolen voices are rendered
             * after all normal voices have been rendered; this is needed to render
             * audio of those voices which were selected for voice stealing until
             * the point were the stealing (that is the take over of the voice)
             * actually happened.
             *
             * @param pEngineChannel - engine channel on which audio should be
             *                         rendered
             * @param Samples        - amount of sample points to be rendered in
             *                         this audio fragment cycle
             */
            void RenderStolenVoices(uint Samples) {
                RTList<Event>::Iterator itVoiceStealEvent = pVoiceStealingQueue->first();
                RTList<Event>::Iterator end               = pVoiceStealingQueue->end();
                for (; itVoiceStealEvent != end; ++itVoiceStealEvent) {
                    EngineChannelBase<V, R, I>* pEngineChannel =
                        static_cast<EngineChannelBase<V, R, I>*>(itVoiceStealEvent->pEngineChannel);;
                    if (!pEngineChannel->pInstrument) continue; // ignore if no instrument loaded
                    
                    PoolVoiceIterator itNewVoice =
                        LaunchVoice(pEngineChannel, itVoiceStealEvent, itVoiceStealEvent->Param.Note.Layer, itVoiceStealEvent->Param.Note.ReleaseTrigger, false, false);
                    if (itNewVoice) {
                        // usually there should already be a new Note object
                        NoteIterator itNote = GetNotePool()->fromID(itVoiceStealEvent->Param.Note.ID);
                        if (!itNote) { // should not happen, but just to be sure ...
                            dmsg(2,("Engine: No Note object for stolen voice!\n"));
                            const note_id_t noteID = LaunchNewNote(pEngineChannel, itVoiceStealEvent);
                            if (!noteID) {
                                dmsg(1,("Engine: Voice stealing failed; No Note object and Note pool empty!\n"));
                                continue;
                            }
                            itNote = GetNotePool()->fromID(noteID);
                        }
                        // move voice from whereever it was, to the new note's list of active voices
                        itNewVoice = itNewVoice.moveToEndOf(itNote->pActiveVoices);
                        // render audio of this new voice for the first time
                        itNewVoice->Render(Samples);
                        if (itNewVoice->IsActive()) { // still active
                            *(pEngineChannel->pRegionsInUse->allocAppend()) = itNewVoice->GetRegion();
                            ActiveVoiceCountTemp++;
                            pEngineChannel->SetVoiceCount(pEngineChannel->GetVoiceCount() + 1);

                            if (itNewVoice->PlaybackState == Voice::playback_state_disk) {
                                if (itNewVoice->DiskStreamRef.State != Stream::state_unused) {
                                    pEngineChannel->SetDiskStreamCount(pEngineChannel->GetDiskStreamCount() + 1);
                                }
                            }
                        } else { // voice reached end, is now inactive
                            pEngineChannel->FreeVoice(itNewVoice); // remove voice from the list of active voices
                        }
                    }
                    else dmsg(1,("EngineBase: ERROR, voice stealing didn't work out!\n"));

                    // we need to clear the key's event list explicitly here in case key was never active
                    MidiKey* pKey = &pEngineChannel->pMIDIKeyInfo[itVoiceStealEvent->Param.Note.Key];
                    pKey->VoiceTheftsQueued--;
                    if (!pKey->Active && !pKey->VoiceTheftsQueued) pKey->pEvents->clear();
                }
            }

            /**
             * Free all keys which have turned inactive in this audio fragment, from
             * the list of active keys and clear all event lists on that engine
             * channel.
             *
             * @param pEngineChannel - engine channel to cleanup
             */
            void PostProcess(EngineChannel* pEngineChannel) {
                EngineChannelBase<V, R, I>* pChannel =
                    static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);
                 pChannel->FreeAllInactiveKeys();

                // empty the engine channel's own event lists
                // (only events of the current audio fragment cycle)
                pChannel->ClearEventListsOfCurrentFragment();
            }

            /**
             * Process MIDI control change events with hard coded behavior,
             * that is controllers whose behavior is defined independently
             * of the actual sampler engine type and instrument.
             *
             * @param pEngineChannel - engine channel on which the MIDI CC event was received
             * @param itControlChangeEvent - the actual MIDI CC event
             */
            void ProcessHardcodedControllers (
                EngineChannel*          pEngineChannel,
                Pool<Event>::Iterator&  itControlChangeEvent
            ) {
                EngineChannelBase<V, R, I>* pChannel =
                    static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                switch (itControlChangeEvent->Param.CC.Controller) {
                    case 5: { // portamento time
                        pChannel->PortamentoTime = (float) itControlChangeEvent->Param.CC.Value / 127.0f * (float) CONFIG_PORTAMENTO_TIME_MAX + (float) CONFIG_PORTAMENTO_TIME_MIN;
                        break;
                    }
                    case 6: { // data entry (currently only used for RPN and NRPN controllers)
                        //dmsg(1,("DATA ENTRY %d\n", itControlChangeEvent->Param.CC.Value));
                        if (pChannel->GetMidiRpnController() >= 0) { // RPN controller number was sent previously ...
                            dmsg(4,("Guess it's an RPN ...\n"));
                            if (pChannel->GetMidiRpnController() == 2) { // coarse tuning in half tones
                                int transpose = (int) itControlChangeEvent->Param.CC.Value - 64;
                                // limit to +- two octaves for now
                                transpose = RTMath::Min(transpose,  24);
                                transpose = RTMath::Max(transpose, -24);
                                pChannel->GlobalTranspose = transpose;
                                // workaround, so we won't have hanging notes
                                pChannel->ReleaseAllVoices(itControlChangeEvent);
                            }
                            // to prevent other MIDI CC #6 messages to be misenterpreted as RPN controller data
                            pChannel->ResetMidiRpnController();
                        } else if (pChannel->GetMidiNrpnController() >= 0) { // NRPN controller number was sent previously ...
                            dmsg(4,("Guess it's an NRPN ...\n"));
                            const int NrpnCtrlMSB = pChannel->GetMidiNrpnController() >> 8; 
                            const int NrpnCtrlLSB = pChannel->GetMidiNrpnController() & 0xff;
                            dmsg(4,("NRPN MSB=%d LSB=%d Data=%d\n", NrpnCtrlMSB, NrpnCtrlLSB, itControlChangeEvent->Param.CC.Value));
                            switch (NrpnCtrlMSB) {
                                case 0x1a: { // volume level of note (Roland GS NRPN)
                                    const uint note = NrpnCtrlLSB;
                                    const uint vol  = itControlChangeEvent->Param.CC.Value;
                                    dmsg(4,("Note Volume NRPN received (note=%d,vol=%d).\n", note, vol));
                                    if (note < 128 && vol < 128)
                                        pChannel->pMIDIKeyInfo[note].Volume = VolumeCurve[vol];
                                    break;
                                }
                                case 0x1c: { // panpot of note (Roland GS NRPN)
                                    const uint note = NrpnCtrlLSB;
                                    const uint pan  = itControlChangeEvent->Param.CC.Value;
                                    dmsg(4,("Note Pan NRPN received (note=%d,pan=%d).\n", note, pan));
                                    if (note < 128 && pan < 128) {
                                        pChannel->pMIDIKeyInfo[note].PanLeft  = PanCurve[128 - pan];
                                        pChannel->pMIDIKeyInfo[note].PanRight = PanCurve[pan];
                                    }
                                    break;
                                }
                                case 0x1d: { // reverb send of note (Roland GS NRPN)
                                    const uint note = NrpnCtrlLSB;
                                    const float reverb = float(itControlChangeEvent->Param.CC.Value) / 127.0f;
                                    dmsg(4,("Note Reverb Send NRPN received (note=%d,send=%f).\n", note, reverb));
                                    if (note < 128)
                                        pChannel->pMIDIKeyInfo[note].ReverbSend = reverb;
                                    break;
                                }
                                case 0x1e: { // chorus send of note (Roland GS NRPN)
                                    const uint note = NrpnCtrlLSB;
                                    const float chorus = float(itControlChangeEvent->Param.CC.Value) / 127.0f;
                                    dmsg(4,("Note Chorus Send NRPN received (note=%d,send=%f).\n", note, chorus));
                                    if (note < 128)
                                        pChannel->pMIDIKeyInfo[note].ChorusSend = chorus;
                                    break;
                                }
                            }
                            // to prevent other MIDI CC #6 messages to be misenterpreted as NRPN controller data
                            pChannel->ResetMidiNrpnController();
                        }
                        break;
                    }
                    case 7: { // volume
                        //TODO: not sample accurate yet
                        pChannel->MidiVolume = VolumeCurve[itControlChangeEvent->Param.CC.Value];
                        pChannel->bStatusChanged = true; // engine channel status has changed, so set notify flag
                        break;
                    }
                    case 10: { // panpot
                        //TODO: not sample accurate yet
                        pChannel->iLastPanRequest = itControlChangeEvent->Param.CC.Value;
                        break;
                    }
                    case 64: { // sustain
                        if (itControlChangeEvent->Param.CC.Value >= 64 && !pChannel->SustainPedal) {
                            dmsg(4,("DAMPER (RIGHT) PEDAL DOWN\n"));
                            pChannel->SustainPedal = true;
                            pChannel->listeners.PreProcessSustainPedalDown();

                            #if !CONFIG_PROCESS_MUTED_CHANNELS
                            if (pEngineChannel->GetMute()) { // skip if sampler channel is muted
                                pChannel->listeners.PostProcessSustainPedalDown();
                                return;
                            }
                            #endif

                            pChannel->ProcessSustainPedalDown(itControlChangeEvent);
                            pChannel->listeners.PostProcessSustainPedalDown();
                        }
                        if (itControlChangeEvent->Param.CC.Value < 64 && pChannel->SustainPedal) {
                            dmsg(4,("DAMPER (RIGHT) PEDAL UP\n"));
                            pChannel->SustainPedal = false;
                            pChannel->listeners.PreProcessSustainPedalUp();

                            #if !CONFIG_PROCESS_MUTED_CHANNELS
                            if (pChannel->GetMute()) { // skip if sampler channel is muted
                                pChannel->listeners.PostProcessSustainPedalUp();
                                return;
                            }
                            #endif

                            pChannel->ProcessSustainPedalUp(itControlChangeEvent);
                            pChannel->listeners.PostProcessSustainPedalUp();
                        }
                        break;
                    }
                    case 65: { // portamento on / off
                        const bool bPortamento = itControlChangeEvent->Param.CC.Value >= 64;
                        if (bPortamento != pChannel->PortamentoMode)
                            KillAllVoices(pChannel, itControlChangeEvent);
                        pChannel->PortamentoMode = bPortamento;
                        break;
                    }
                    case 66: { // sostenuto
                        if (itControlChangeEvent->Param.CC.Value >= 64 && !pChannel->SostenutoPedal) {
                            dmsg(4,("SOSTENUTO (CENTER) PEDAL DOWN\n"));
                            pChannel->SostenutoPedal = true;
                            pChannel->listeners.PreProcessSostenutoPedalDown();

                            #if !CONFIG_PROCESS_MUTED_CHANNELS
                            if (pEngineChannel->GetMute()) { // skip if sampler channel is muted
                                pChannel->listeners.PostProcessSostenutoPedalDown();
                                return;
                            }
                            #endif

                            pChannel->ProcessSostenutoPedalDown();
                            pChannel->listeners.PostProcessSostenutoPedalDown();
                        }
                        if (itControlChangeEvent->Param.CC.Value < 64 && pChannel->SostenutoPedal) {
                            dmsg(4,("SOSTENUTO (CENTER) PEDAL UP\n"));
                            pChannel->SostenutoPedal = false;
                            pChannel->listeners.PreProcessSostenutoPedalUp();

                            #if !CONFIG_PROCESS_MUTED_CHANNELS
                            if (pEngineChannel->GetMute()) { // skip if sampler channel is muted
                                pChannel->listeners.PostProcessSostenutoPedalUp();
                                return;
                            }
                            #endif

                            pChannel->ProcessSostenutoPedalUp(itControlChangeEvent);
                            pChannel->listeners.PostProcessSostenutoPedalUp();
                        }
                        break;
                    }
                    case 98: { // NRPN controller LSB
                        dmsg(4,("NRPN LSB %d\n", itControlChangeEvent->Param.CC.Value));
                        pEngineChannel->SetMidiNrpnControllerLsb(itControlChangeEvent->Param.CC.Value);
                        break;
                    }
                    case 99: { // NRPN controller MSB
                        dmsg(4,("NRPN MSB %d\n", itControlChangeEvent->Param.CC.Value));
                        pEngineChannel->SetMidiNrpnControllerMsb(itControlChangeEvent->Param.CC.Value);
                        break;
                    }
                    case 100: { // RPN controller LSB
                        dmsg(4,("RPN LSB %d\n", itControlChangeEvent->Param.CC.Value));
                        pEngineChannel->SetMidiRpnControllerLsb(itControlChangeEvent->Param.CC.Value);
                        break;
                    }
                    case 101: { // RPN controller MSB
                        dmsg(4,("RPN MSB %d\n", itControlChangeEvent->Param.CC.Value));
                        pEngineChannel->SetMidiRpnControllerMsb(itControlChangeEvent->Param.CC.Value);
                        break;
                    }


                    // Channel Mode Messages

                    case 120: { // all sound off
                        KillAllVoices(pEngineChannel, itControlChangeEvent);
                        break;
                    }
                    case 121: { // reset all controllers
                        pChannel->ResetControllers();
                        break;
                    }
                    case 123: { // all notes off
                        #if CONFIG_PROCESS_ALL_NOTES_OFF
                        pChannel->ReleaseAllVoices(itControlChangeEvent);
                        #endif // CONFIG_PROCESS_ALL_NOTES_OFF
                        break;
                    }
                    case 126: { // mono mode on
                        if (!pChannel->SoloMode)
                            KillAllVoices(pEngineChannel, itControlChangeEvent);
                        pChannel->SoloMode = true;
                        break;
                    }
                    case 127: { // poly mode on
                        if (pChannel->SoloMode)
                            KillAllVoices(pEngineChannel, itControlChangeEvent);
                        pChannel->SoloMode = false;
                        break;
                    }
                }
            }

            virtual D* CreateDiskThread() = 0;

            /**
             *  Assigns and triggers a new voice for the respective MIDI key.
             *
             *  @param pEngineChannel - engine channel on which this event occurred on
             *  @param itNoteOnEvent - key, velocity and time stamp of the event
             */
            virtual void ProcessNoteOn(EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNoteOnEvent) OVERRIDE {
                EngineChannelBase<V, R, I>* pChannel =
                        static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                const int key = itNoteOnEvent->Param.Note.Key;
                const int vel = itNoteOnEvent->Param.Note.Velocity;
                if (key < 0 || key > 127) return; // ignore event, key outside allowed key range

                MidiKey* pKey = &pChannel->pMIDIKeyInfo[key];

                // There are real MIDI note-on events (Event::type_note_on) and
                // programmatically spawned notes (Event::type_play_note). We have
                // to distinguish between them, since certain processing below
                // must only be done on real MIDI note-on events (i.e. for
                // correctly updating which MIDI keys are currently pressed down).
                const bool isRealMIDINoteOnEvent = itNoteOnEvent->Type == Event::type_note_on;

                if (isRealMIDINoteOnEvent)
                    pChannel->listeners.PreProcessNoteOn(key, vel);

                #if !CONFIG_PROCESS_MUTED_CHANNELS
                if (pEngineChannel->GetMute()) { // skip if sampler channel is muted
                    if (isRealMIDINoteOnEvent)
                        pChannel->listeners.PostProcessNoteOn(key, vel);
                    return;
                }
                #endif

                if (!pChannel->pInstrument) {
                    if (isRealMIDINoteOnEvent)
                        pChannel->listeners.PostProcessNoteOn(key, vel);
                    return; // ignore if no instrument loaded
                }

                // move note on event to the key's own event list
                RTList<Event>::Iterator itNoteOnEventOnKeyList = itNoteOnEvent.moveToEndOf(pKey->pEvents);

                // if Solo Mode then kill all already active voices
                if (pChannel->SoloMode && isRealMIDINoteOnEvent) {
                    Pool<uint>::Iterator itYoungestKey = pChannel->pActiveKeys->last();
                    if (itYoungestKey) {
                        const int iYoungestKey = *itYoungestKey;
                        const MidiKey* pOtherKey = &pChannel->pMIDIKeyInfo[iYoungestKey];
                        if (pOtherKey->Active) {
                            // get final portamento position of currently active voice
                            if (pChannel->PortamentoMode) {
                                NoteIterator itNote = pOtherKey->pActiveNotes->last();
                                if (itNote) {
                                    VoiceIterator itVoice = itNote->pActiveVoices->last();
                                    if (itVoice) itVoice->UpdatePortamentoPos(itNoteOnEventOnKeyList);
                                }
                            }
                            // kill all voices on the (other) key
                            for (NoteIterator itNote = pOtherKey->pActiveNotes->first(); itNote; ++itNote) {
                                VoiceIterator itVoiceToBeKilled = itNote->pActiveVoices->first();
                                VoiceIterator end               = itNote->pActiveVoices->end();
                                for (; itVoiceToBeKilled != end; ++itVoiceToBeKilled) {
                                    if (!(itVoiceToBeKilled->Type & Voice::type_release_trigger))
                                        itVoiceToBeKilled->Kill(itNoteOnEventOnKeyList);
                                }
                            }
                        }
                    }
                    // set this key as 'currently active solo key'
                    pChannel->SoloKey = key;
                }

                if (isRealMIDINoteOnEvent) {
                    pChannel->ProcessKeySwitchChange(key);

                    pKey->KeyPressed = true; // the MIDI key was now pressed down
                    pChannel->KeyDown[key] = true; // just used as built-in %KEY_DOWN script variable
                    pKey->Velocity   = itNoteOnEventOnKeyList->Param.Note.Velocity;
                    pKey->NoteOnTime = FrameTime + itNoteOnEventOnKeyList->FragmentPos(); // will be used to calculate note length
                }

                // cancel release process of voices on this key if needed
                if (pKey->Active && !pChannel->SustainPedal && isRealMIDINoteOnEvent) {
                    RTList<Event>::Iterator itCancelReleaseEvent = pKey->pEvents->allocAppend();
                    if (itCancelReleaseEvent) {
                        *itCancelReleaseEvent = *itNoteOnEventOnKeyList;         // copy event
                        itCancelReleaseEvent->Type = Event::type_cancel_release_key; // transform event type
                    }
                    else dmsg(1,("Event pool emtpy!\n"));
                }

                TriggerNewVoices(pEngineChannel, itNoteOnEventOnKeyList);

                // if neither a voice was spawned or postponed then remove note on event from key again
                if (!pKey->Active && !pKey->VoiceTheftsQueued)
                    pKey->pEvents->free(itNoteOnEventOnKeyList);

                if (isRealMIDINoteOnEvent && (!pChannel->SoloMode || pChannel->PortamentoPos < 0.0f))
                    pChannel->PortamentoPos = (float) key;

                //NOTE: Hmm, I guess its a matter of taste whether round robin should be advanced only on real MIDI note-on events, isn't it?
                if (pKey->pRoundRobinIndex) {
                    (*pKey->pRoundRobinIndex)++; // counter specific for the key or region
                    pChannel->RoundRobinIndex++; // common counter for the channel
                }

                if (isRealMIDINoteOnEvent)
                    pChannel->listeners.PostProcessNoteOn(key, vel);
            }

            /**
             * Allocate and trigger new voice(s) for the key.
             */
            virtual void TriggerNewVoices (
                EngineChannel*            pEngineChannel,
                RTList<Event>::Iterator&  itNoteOnEvent,
                bool                      HandleKeyGroupConflicts = true
            ) = 0;

            /**
             * Allocate and trigger release voice(s) for the key.
             */
            virtual void TriggerReleaseVoices (
                EngineChannel*            pEngineChannel,
                RTList<Event>::Iterator&  itNoteOffEvent
            ) = 0;

            /**
             *  Releases the voices on the given key if sustain pedal is not pressed.
             *  If sustain is pressed, the release of the note will be postponed until
             *  sustain pedal will be released or voice turned inactive by itself (e.g.
             *  due to completion of sample playback).
             *
             *  @param pEngineChannel - engine channel on which this event occurred on
             *  @param itNoteOffEvent - key, velocity and time stamp of the event
             */
            virtual void ProcessNoteOff(EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNoteOffEvent) OVERRIDE {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                const int iKey = itNoteOffEvent->Param.Note.Key;
                const int vel  = itNoteOffEvent->Param.Note.Velocity;
                if (iKey < 0 || iKey > 127) return; // ignore event, key outside allowed key range

                MidiKey* pKey = &pChannel->pMIDIKeyInfo[iKey];

                // There are real MIDI note-off events (Event::type_note_off) and
                // programmatically spawned notes (Event::type_stop_note). We have
                // to distinguish between them, since certain processing below
                // must only be done on real MIDI note-off events (i.e. for
                // correctly updating which MIDI keys are currently pressed down),
                // plus a stop-note event just releases voices of one particular
                // note, whereas a note-off event releases all voices on a
                // particular MIDI key instead.
                const bool isRealMIDINoteOffEvent = itNoteOffEvent->Type == Event::type_note_off;

                if (isRealMIDINoteOffEvent)
                    pChannel->listeners.PreProcessNoteOff(iKey, vel);

                #if !CONFIG_PROCESS_MUTED_CHANNELS
                if (pEngineChannel->GetMute()) { // skip if sampler channel is muted
                    if (isRealMIDINoteOffEvent)
                        pChannel->listeners.PostProcessNoteOff(iKey, vel);
                    return;
                }
                #endif

                if (isRealMIDINoteOffEvent) {
                    pKey->KeyPressed = false; // the MIDI key was now released
                    pChannel->KeyDown[iKey] = false; // just used as built-in %KEY_DOWN script variable
                }

                // move event to the key's own event list
                RTList<Event>::Iterator itNoteOffEventOnKeyList = itNoteOffEvent.moveToEndOf(pKey->pEvents);

                if (isRealMIDINoteOffEvent) {
                    bool bShouldRelease = pKey->Active && pChannel->ShouldReleaseVoice(itNoteOffEventOnKeyList->Param.Note.Key);

                    // in case Solo Mode is enabled, kill all voices on this key and respawn a voice on the highest pressed key (if any)
                    if (pChannel->SoloMode && pChannel->pInstrument) { //TODO: this feels like too much code just for handling solo mode :P
                        bool bOtherKeysPressed = false;
                        if (iKey == pChannel->SoloKey) {
                            pChannel->SoloKey = -1;
                            // if there's still a key pressed down, respawn a voice (group) on the highest key
                            for (int i = 127; i > 0; i--) {
                                MidiKey* pOtherKey = &pChannel->pMIDIKeyInfo[i];
                                if (pOtherKey->KeyPressed) {
                                    bOtherKeysPressed = true;
                                    // make the other key the new 'currently active solo key'
                                    pChannel->SoloKey = i;
                                    // get final portamento position of currently active voice
                                    if (pChannel->PortamentoMode) {
                                        NoteIterator itNote = pKey->pActiveNotes->first();
                                        VoiceIterator itVoice = itNote->pActiveVoices->first();
                                        if (itVoice) itVoice->UpdatePortamentoPos(itNoteOffEventOnKeyList);
                                    }
                                    // create a pseudo note on event
                                    RTList<Event>::Iterator itPseudoNoteOnEvent = pOtherKey->pEvents->allocAppend();
                                    if (itPseudoNoteOnEvent) {
                                        // copy event
                                        *itPseudoNoteOnEvent = *itNoteOffEventOnKeyList;
                                        // transform event to a note on event
                                        itPseudoNoteOnEvent->Type                = Event::type_note_on; //FIXME: should probably use Event::type_play_note instead (to avoid i.e. hanging notes)
                                        itPseudoNoteOnEvent->Param.Note.Key      = i;
                                        itPseudoNoteOnEvent->Param.Note.Velocity = pOtherKey->Velocity;
                                        // assign a new note to this note-on event
                                        if (LaunchNewNote(pChannel, itPseudoNoteOnEvent)) {
                                            // allocate and trigger new voice(s) for the other key
                                            TriggerNewVoices(pChannel, itPseudoNoteOnEvent, false);
                                        }
                                        // if neither a voice was spawned or postponed then remove note on event from key again
                                        if (!pOtherKey->Active && !pOtherKey->VoiceTheftsQueued)
                                            pOtherKey->pEvents->free(itPseudoNoteOnEvent);

                                    } else dmsg(1,("Could not respawn voice, no free event left\n"));
                                    break; // done
                                }
                            }
                        }
                        if (bOtherKeysPressed) {
                            if (pKey->Active) { // kill all voices on this key
                                bShouldRelease = false; // no need to release, as we kill it here
                                for (NoteIterator itNote = pKey->pActiveNotes->first(); itNote; ++itNote) {
                                    VoiceIterator itVoiceToBeKilled = itNote->pActiveVoices->first();
                                    VoiceIterator end               = itNote->pActiveVoices->end();
                                    for (; itVoiceToBeKilled != end; ++itVoiceToBeKilled) {
                                        if (!(itVoiceToBeKilled->Type & Voice::type_release_trigger))
                                            itVoiceToBeKilled->Kill(itNoteOffEventOnKeyList);
                                    }
                                }
                            }
                        } else pChannel->PortamentoPos = -1.0f;
                    }

                    // if no solo mode (the usual case) or if solo mode and no other key pressed, then release voices on this key if needed
                    if (bShouldRelease) {
                        itNoteOffEventOnKeyList->Type = Event::type_release_key; // transform event type
                        // spawn release triggered voice(s) if needed
                        if (pKey->ReleaseTrigger & release_trigger_noteoff)
                            ProcessReleaseTrigger(pChannel, itNoteOffEventOnKeyList, pKey);
                    }
                } else if (itNoteOffEventOnKeyList->Type == Event::type_stop_note) {
                    // This programmatically caused event is caused by a call to
                    // the built-in instrument script function note_off(). In
                    // contrast to a real MIDI note-off event the stop-note
                    // event just intends to release voices of one particular note.
                    NoteBase* pNote = pChannel->pEngine->NoteByID( itNoteOffEventOnKeyList->Param.Note.ID );
                    if (pNote) { // the requested note is still alive ...
                        itNoteOffEventOnKeyList->Type = Event::type_release_note; // transform event type
                    } else { // note is dead and gone ..
                        pKey->pEvents->free(itNoteOffEventOnKeyList); // remove stop-note event from key again
                        return; // prevent event to be removed a 2nd time below
                    }
                }

                // if neither a voice was spawned or postponed on this key then remove note off event from key again
                if (!pKey->Active && !pKey->VoiceTheftsQueued)
                    pKey->pEvents->free(itNoteOffEventOnKeyList);

                if (isRealMIDINoteOffEvent)
                    pChannel->listeners.PostProcessNoteOff(iKey, vel);
            }

            /**
             * Called on sustain pedal up events to check and if required,
             * launch release trigger voices on the respective active key.
             *
             * @param pEngineChannel - engine channel on which this event occurred on
             * @param itEvent - release trigger event (contains note number)
             */
            virtual void ProcessReleaseTriggerBySustain(EngineChannel* pEngineChannel, RTList<Event>::Iterator& itEvent) OVERRIDE {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                const int iKey = itEvent->Param.Note.Key;
                if (iKey < 0 || iKey > 127) return; // ignore event, key outside allowed key range

                MidiKey* pKey = &pChannel->pMIDIKeyInfo[iKey];

                ProcessReleaseTrigger(pChannel, itEvent, pKey);
            }

            /**
             * Called on note-off and sustain pedal up events to check and if
             * required, launch release trigger voices on the respective active
             * key.
             *
             * @param pEngineChannel - engine channel on which this event occurred on
             * @param itEvent - note off event / release trigger event
             * @param pKey - key on which the release trigger voices shall be spawned
             */
            inline void ProcessReleaseTrigger(EngineChannelBase<V, R, I>* pChannel, RTList<Event>::Iterator& itEvent, MidiKey* pKey) {
                // spawn release triggered voice(s) if needed
                if (pKey->ReleaseTrigger && pChannel->pInstrument) {
                    // assign a new note to this release event
                    if (LaunchNewNote(pChannel, itEvent)) {
                        // allocate and trigger new release voice(s)
                        TriggerReleaseVoices(pChannel, itEvent);
                    }
                    pKey->ReleaseTrigger = release_trigger_none;
                }
            }

            /**
             * Called on "kill note" events, which currently only happens on
             * built-in real-time instrument script function fade_out(). This
             * method only fulfills one task: moving the even to the Note's own
             * event list so that its voices can process the kill event sample
             * accurately.
             */
            void ProcessKillNote(EngineChannel* pEngineChannel, RTList<Event>::Iterator& itEvent) {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                NoteBase* pNote = pChannel->pEngine->NoteByID( itEvent->Param.Note.ID );
                if (!pNote || pNote->hostKey < 0 || pNote->hostKey >= 128) return;

                // move note kill event to its MIDI key
                MidiKey* pKey = &pChannel->pMIDIKeyInfo[pNote->hostKey];
                itEvent.moveToEndOf(pKey->pEvents);
            }

            /**
             * Called on note synthesis parameter change events. These are
             * internal events caused by calling built-in real-time instrument
             * script functions like change_vol(), change_tune(), etc.
             *
             * This method performs two tasks:
             *
             * - It converts the event's relative values changes (Deltas) to
             *   the respective final new synthesis parameter value (AbsValue),
             *   for that particular moment of the event that is.
             *
             * - It moves the individual events to the Note's own event list
             *   (or actually to the event list of the MIDI key), so that
             *   voices can process those events sample accurately.
             *
             * @param pEngineChannel - engine channel on which this event occurred on
             * @param itEvent - note synthesis parameter change event
             */
            virtual void ProcessNoteSynthParam(EngineChannel* pEngineChannel, RTList<Event>::Iterator& itEvent) {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);

                NoteBase* pNote = pChannel->pEngine->NoteByID( itEvent->Param.NoteSynthParam.NoteID );
                if (!pNote || pNote->hostKey < 0 || pNote->hostKey >= 128) return;

                const bool& relative = itEvent->Param.NoteSynthParam.Relative;

                switch (itEvent->Param.NoteSynthParam.Type) {
                    case Event::synth_param_volume:
                        if (relative)
                            pNote->Override.Volume *= itEvent->Param.NoteSynthParam.Delta;
                        else
                            pNote->Override.Volume = itEvent->Param.NoteSynthParam.Delta;
                        itEvent->Param.NoteSynthParam.AbsValue = pNote->Override.Volume;
                        break;
                    case Event::synth_param_volume_time:
                        pNote->Override.VolumeTime = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_volume_curve:
                        itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        pNote->Override.VolumeCurve = (fade_curve_t) itEvent->Param.NoteSynthParam.AbsValue;
                        break;
                    case Event::synth_param_pitch:
                        if (relative)
                            pNote->Override.Pitch *= itEvent->Param.NoteSynthParam.Delta;
                        else
                            pNote->Override.Pitch = itEvent->Param.NoteSynthParam.Delta;
                        itEvent->Param.NoteSynthParam.AbsValue = pNote->Override.Pitch;
                        break;
                    case Event::synth_param_pitch_time:
                        pNote->Override.PitchTime = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_pitch_curve:
                        itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        pNote->Override.PitchCurve = (fade_curve_t) itEvent->Param.NoteSynthParam.AbsValue;
                        break;
                    case Event::synth_param_pan:
                        if (relative) {
                            pNote->Override.Pan = RTMath::RelativeSummedAvg(pNote->Override.Pan, itEvent->Param.NoteSynthParam.Delta, ++pNote->Override.PanSources);
                        } else {
                            pNote->Override.Pan = itEvent->Param.NoteSynthParam.Delta;
                            pNote->Override.PanSources = 1; // only relevant on subsequent change_pan() instrument script calls on same note with 'relative' argument being set
                        }
                        itEvent->Param.NoteSynthParam.AbsValue = pNote->Override.Pan;
                        break;
                    case Event::synth_param_pan_time:
                        pNote->Override.PanTime = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_pan_curve:
                        itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        pNote->Override.PanCurve = (fade_curve_t) itEvent->Param.NoteSynthParam.AbsValue;
                        break;
                    case Event::synth_param_cutoff:
                        pNote->Override.Cutoff = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_resonance:
                        pNote->Override.Resonance = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_attack:
                        pNote->Override.Attack = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_decay:
                        pNote->Override.Decay = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_sustain:
                        pNote->Override.Sustain = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_release:
                        pNote->Override.Release = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;

                    case Event::synth_param_cutoff_attack:
                        pNote->Override.CutoffAttack = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_cutoff_decay:
                        pNote->Override.CutoffDecay = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_cutoff_sustain:
                        pNote->Override.CutoffSustain = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_cutoff_release:
                        pNote->Override.CutoffRelease = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;

                    case Event::synth_param_amp_lfo_depth:
                        pNote->Override.AmpLFODepth = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_amp_lfo_freq:
                        pNote->Override.AmpLFOFreq = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_cutoff_lfo_depth:
                        pNote->Override.CutoffLFODepth = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_cutoff_lfo_freq:
                        pNote->Override.CutoffLFOFreq = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_pitch_lfo_depth:
                        pNote->Override.PitchLFODepth = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                    case Event::synth_param_pitch_lfo_freq:
                        pNote->Override.PitchLFOFreq = itEvent->Param.NoteSynthParam.AbsValue = itEvent->Param.NoteSynthParam.Delta;
                        break;
                }

                // move note parameter event to its MIDI key
                MidiKey* pKey = &pChannel->pMIDIKeyInfo[pNote->hostKey];
                itEvent.moveToEndOf(pKey->pEvents);
            }

            /**
             *  Reset all voices and disk thread and clear input event queue and all
             *  control and status variables. This method is protected by a mutex.
             */
            virtual void ResetInternal() OVERRIDE {
                LockGuard lock(ResetInternalMutex);

                // make sure that the engine does not get any sysex messages
                // while it's reseting
                bool sysexDisabled = MidiInputPort::RemoveSysexListener(this);
                SetVoiceCount(0);
                ActiveVoiceCountMax = 0;

                // reset voice stealing parameters
                pVoiceStealingQueue->clear();
                itLastStolenVoice          = VoiceIterator();
                itLastStolenVoiceGlobally  = VoiceIterator();
                itLastStolenNote           = NoteIterator();
                itLastStolenNoteGlobally   = NoteIterator();
                iuiLastStolenKey           = RTList<uint>::Iterator();
                iuiLastStolenKeyGlobally   = RTList<uint>::Iterator();
                pLastStolenChannel         = NULL;

                // reset all notes
                pNotePool->clear();
                for (NoteIterator itNote = pNotePool->allocAppend(); itNote;
                     itNote = pNotePool->allocAppend())
                {
                    itNote->reset();
                }
                pNotePool->clear();

                // reset all voices
                pVoicePool->clear();
                for (VoiceIterator iterVoice = pVoicePool->allocAppend(); iterVoice == pVoicePool->last(); iterVoice = pVoicePool->allocAppend()) {
                    iterVoice->Reset();
                }
                pVoicePool->clear();

                // reset all engine channels
                for (int i = 0; i < engineChannels.size(); i++) {
                    AbstractEngineChannel* pEngineChannel =
                        static_cast<AbstractEngineChannel*>(engineChannels[i]);
                    pEngineChannel->ResetInternal(false/*don't reset engine*/);
                }

                // reset disk thread
                if (pDiskThread) pDiskThread->Reset();

                // delete all input events
                pEventQueue->init();
                pSysexBuffer->init();
                if (sysexDisabled) MidiInputPort::AddSysexListener(this);
            }

            /**
             * Kills all voices on an engine channel as soon as possible. Voices
             * won't get into release state, their volume level will be ramped down
             * as fast as possible.
             *
             * @param pEngineChannel - engine channel on which all voices should be killed
             * @param itKillEvent    - event which caused this killing of all voices
             */
            virtual void KillAllVoices(EngineChannel* pEngineChannel, Pool<Event>::Iterator& itKillEvent) OVERRIDE {
                EngineChannelBase<V, R, I>* pChannel = static_cast<EngineChannelBase<V, R, I>*>(pEngineChannel);
                int count = pChannel->KillAllVoices(itKillEvent);
                VoiceSpawnsLeft -= count; //FIXME: just a temporary workaround, we should check the cause in StealVoice() instead
            }

            /**
             *  Allocates and triggers a new voice. This method will usually be
             *  called by the ProcessNoteOn() method and by the voices itself
             *  (e.g. to spawn further voices on the same key for layered sounds).
             *
             *  @param pEngineChannel      - engine channel on which this event occurred on
             *  @param itNoteOnEvent       - key, velocity and time stamp of the event
             *  @param iLayer              - layer index for the new voice (optional - only
             *                               in case of layered sounds of course)
             *  @param ReleaseTriggerVoice - if new voice is a release triggered voice
             *                               (optional, default = false)
             *  @param VoiceStealing       - if voice stealing should be performed
             *                               when there is no free voice
             *                               (optional, default = true)
             *  @param HandleKeyGroupConflicts - if voices should be killed due to a
             *                                   key group conflict
             *  @returns pointer to new voice or NULL if there was no free voice or
             *           if the voice wasn't triggered (for example when no region is
             *           defined for the given key).
             */
            virtual PoolVoiceIterator LaunchVoice (
                EngineChannel*          pEngineChannel,
                Pool<Event>::Iterator&  itNoteOnEvent,
                int                     iLayer,
                bool                    ReleaseTriggerVoice,
                bool                    VoiceStealing,
                bool                    HandleKeyGroupConflicts
            ) = 0;

            virtual int GetMinFadeOutSamples() OVERRIDE { return MinFadeOutSamples; }

            int InitNewVoice (
                EngineChannelBase<V, R, I>*  pChannel,
                R*                           pRegion,
                Pool<Event>::Iterator&       itNoteOnEvent,
                Voice::type_t                VoiceType,
                int                          iLayer,
                int                          iKeyGroup,
                bool                         ReleaseTriggerVoice,
                bool                         VoiceStealing,
                typename Pool<V>::Iterator&  itNewVoice
            ) {
                int key = itNoteOnEvent->Param.Note.Key;
                typename MidiKeyboardManager<V>::MidiKey* pKey = &pChannel->pMIDIKeyInfo[key];
                if (itNewVoice) {
                    // launch the new voice
                    if (itNewVoice->Trigger(pChannel, itNoteOnEvent, pChannel->Pitch, pRegion, VoiceType, iKeyGroup) < 0) {
                        dmsg(4,("Voice not triggered\n"));
                        GetVoicePool()->free(itNewVoice);
                    }
                    else { // on success
                        --VoiceSpawnsLeft;

                        // should actually be superfluous now, since this is
                        // already done in LaunchNewNote()
                        pChannel->markKeyAsActive(pKey);

                        if (itNewVoice->Type & Voice::type_release_trigger_required)
                            pKey->ReleaseTrigger |= itNewVoice->GetReleaseTriggerFlags(); // mark key for the need of release triggered voice(s)
                        return 0; // success
                    }
                }
                else if (VoiceStealing) {
                    // try to steal one voice
                    int result = StealVoice(pChannel, itNoteOnEvent);
                    if (!result) { // voice stolen successfully
                        // put note-on event into voice-stealing queue, so it will be reprocessed after killed voice died
                        RTList<Event>::Iterator itStealEvent = pVoiceStealingQueue->allocAppend();
                        if (itStealEvent) {
                            *itStealEvent = *itNoteOnEvent; // copy event
                            itStealEvent->Param.Note.Layer = iLayer;
                            itStealEvent->Param.Note.ReleaseTrigger = ReleaseTriggerVoice;
                            pKey->VoiceTheftsQueued++;
                        }
                        else dmsg(1,("Voice stealing queue full!\n"));
                    }
                }

                return -1;
            }
            
            /**
             * Checks whether scale tuning setting has been changed since last
             * time this method was called, if yes, it recalculates the pitch
             * for all active voices.
             */
            void ProcessScaleTuningChange() {
                const bool changed = ScaleTuningChanged.readAndReset();
                if (!changed) return;
                
                for (int i = 0; i < engineChannels.size(); i++) {
                    EngineChannelBase<V, R, I>* channel =
                        static_cast<EngineChannelBase<V, R, I>*>(engineChannels[i]);
                    channel->OnScaleTuningChanged();
                }
            }

        private:
            Pool< Note<V> >* pNotePool;
            Pool<note_id_t> noteIDPool;
            Pool<V>*    pVoicePool;            ///< Contains all voices that can be activated.
            Pool<RR*>   SuspendedRegions;
            Mutex       SuspendedRegionsMutex;
            Condition   SuspensionChangeOngoing;
            RR*         pPendingRegionSuspension;
            RR*         pPendingRegionResumption;
            int         iPendingStreamDeletions;
    };

    template <class V, class RR, class R, class D, class IM, class I>
    IM EngineBase<V, RR, R, D, IM, I>::instruments;

} // namespace LinuxSampler

#endif	/* __LS_ENGINEBASE_H__ */

