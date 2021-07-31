/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2010 Christian Schoenebeck and Grigor Iliev        *
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

#ifndef __LS_MIDIKEYBOARDMANAGER_H__
#define __LS_MIDIKEYBOARDMANAGER_H__

#include "Event.h"
#include "Stream.h"
#include "../../EventListeners.h"
#include "../../common/Pool.h"
#include "../../common/global_private.h"
#include "Note.h"

namespace LinuxSampler {

    /**
     * This class is used as a listener, which is notified
     * when MIDI keyboard events occur like note on, note off, etc.
     * Note that all events are triggered even when the channel is muted
     */
    class MidiKeyboardListener {
        public:
            /** Called before the engine start processing the note on event */
            virtual void PreProcessNoteOn(uint8_t key, uint8_t velocity) = 0;

            /** Called after the engine has processed the note on event */
            virtual void PostProcessNoteOn(uint8_t key, uint8_t velocity) = 0;

            /** Called before the engine start processing the note off event */
            virtual void PreProcessNoteOff(uint8_t key, uint8_t velocity) = 0;

            /** Called after the engine has processed the note off event */
            virtual void PostProcessNoteOff(uint8_t key, uint8_t velocity) = 0;

            /** Called before the engine start processing the sustain pedal up event */
            virtual void PreProcessSustainPedalUp() = 0;

            /** Called after the engine has processed the sustain pedal up event */
            virtual void PostProcessSustainPedalUp() = 0;

            /** Called before the engine start processing the sustain pedal down event */
            virtual void PreProcessSustainPedalDown() = 0;

            /** Called after the engine has processed the sustain pedal down event */
            virtual void PostProcessSustainPedalDown() = 0;

            /** Called before the engine start processing the sostenuto pedal up event */
            virtual void PreProcessSostenutoPedalUp() = 0;

            /** Called after the engine has processed the sostenuto pedal up event */
            virtual void PostProcessSostenutoPedalUp() = 0;

            /** Called before the engine start processing the sostenuto pedal down event */
            virtual void PreProcessSostenutoPedalDown() = 0;

            /** Called after the engine has processed the sostenuto pedal down event */
            virtual void PostProcessSostenutoPedalDown() = 0;
    };

    /**
     * This class exists as convenience for creating listener objects.
     * The methods in this class are empty.
     */
    class MidiKeyboardAdapter : public MidiKeyboardListener {
        public:
            virtual void PreProcessNoteOn(uint8_t key, uint8_t velocity) { }
            virtual void PostProcessNoteOn(uint8_t key, uint8_t velocity) { }
            virtual void PreProcessNoteOff(uint8_t key, uint8_t velocity) { }
            virtual void PostProcessNoteOff(uint8_t key, uint8_t velocity) { }
            virtual void PreProcessSustainPedalUp() { }
            virtual void PostProcessSustainPedalUp() { }
            virtual void PreProcessSustainPedalDown() { }
            virtual void PostProcessSustainPedalDown() { }
            virtual void PreProcessSostenutoPedalUp() { }
            virtual void PostProcessSostenutoPedalUp() { }
            virtual void PreProcessSostenutoPedalDown() { }
            virtual void PostProcessSostenutoPedalDown() { }
    };

    /**
     * This is the base class for class MidiKeyboardManager::MidiKey. It is
     * not intended to be instantiated directly. Instead it just defines
     * the part of class MidiKey which is not dependant on a C++ template
     * parameter.
     *
     * There are also ScriptEvent lists maintained for each key, which are not
     * stored here though, but on the InstrumentScript structure. Simply because
     * RTLists are tied to one Pool instance, and it would be error prone to
     * maintain @c Pool<ScriptEvent> and @c RTList<ScriptEvent> separately,
     * since one would need to be very careful to reallocate the lists when the
     * script was changed or when the Engine instance changed, etc.
     *
     * @see InstrumentScript::pKeyEvents
     */
    class MidiKeyBase {
        public:
            bool            KeyPressed;     ///< Is true if the respective MIDI key is currently pressed.
            bool            Active;         ///< If the key contains active voices.
            release_trigger_t ReleaseTrigger; ///< If we have to launch release triggered voice(s) when either the key or sustain pedal is released.
            Pool<uint>::Iterator itSelf;    ///< hack to allow fast deallocation of the key from the list of active keys
            RTList<Event>*  pEvents;        ///< Key specific events (only Note-on, Note-off and sustain pedal currently)
            int             VoiceTheftsQueued; ///< Amount of voices postponed due to shortage of voices.
            uint32_t*       pRoundRobinIndex; ///< For the round robin dimension: current articulation for this key, will be incremented for each note on
            uint8_t         Velocity;       ///< Latest Note-on velocity for this key
            unsigned long   NoteOnTime;     ///< Time for latest Note-on event for this key
            float           Volume;         ///< Individual volume level for this MIDI key (usually 1.0f unless Roland GS NRPN 0x1Ann was received, nn reflecting the note number, see EngineBase::ProcessHardcodedControllers())
            float           PanLeft;        ///< Individual volume balance (left channel coefficient) for this MIDI key (usually 1.0f unless Roland GS NRPN 0x1Cnn was received, nn reflecting the note number, see EngineBase::ProcessHardcodedControllers())
            float           PanRight;       ///< Individual volume balance (right channel coefficient) for this MIDI key (usually 1.0f unless Roland GS NRPN 0x1Cnn was received, nn reflecting the note number, see EngineBase::ProcessHardcodedControllers())
            optional<float> ReverbSend;     ///< Optional individual reverb send level for this MIDI key (usually not set, unless Roland GS NRPN 0x1Dnn was received, nn reflecting the note number, see EngineBase::ProcessHardcodedControllers())
            optional<float> ChorusSend;     ///< Optional individual chorus send level for this MIDI key (usually not set, unless Roland GS NRPN 0x1Enn was received, nn reflecting the note number, see EngineBase::ProcessHardcodedControllers())
    };

    class MidiKeyboardManagerBase {
    public:
        Pool<uint>*           pActiveKeys;  ///< Holds all keys in it's allocation list with active voices.
        bool                  SoloMode;                 ///< in Solo Mode we only play one voice (group) at a time
        int                   SoloKey;                  ///< Currently 'active' solo key, that is the key to which the currently sounding voice belongs to (only if SoloMode is enabled)
        bool                  SustainPedal;             ///< true if sustain pedal is down
        bool                  SostenutoPedal;           ///< true if sostenuto pedal is down
        int                   SostenutoKeys[128];
        int                   SostenutoKeyCount;
        uint32_t              RoundRobinIndexes[128];
        int8_t                KeyDown[128]; ///< True if the respective key is currently pressed down. Currently only used as built-in instrument script array variable %KEY_DOWN. It is currently not used by the sampler for any other purpose.

        virtual void ProcessReleaseTriggerBySustain(RTList<Event>::Iterator& itEvent) = 0;
    };

    template <class V>
    class MidiKeyboardManager : public MidiKeyboardManagerBase {
        public:
            /** @brief Voice Stealing Algorithms
             *
             * Enumeration of all possible voice stealing algorithms.
             */
            enum voice_steal_algo_t {
                voice_steal_algo_none,              ///< Voice stealing disabled.
                voice_steal_algo_oldestvoiceonkey,  ///< Try to kill the oldest voice from same key where the new voice should be spawned.
                voice_steal_algo_oldestkey          ///< Try to kill the oldest voice from the oldest active key.
            };


            /** @brief MIDI key runtime informations
             *
             * Reflects runtime informations for one MIDI key.
             */
            class MidiKey : public MidiKeyBase {
            public:
                RTList< Note<V> >* pActiveNotes; ///< Contains the active notes associated with the MIDI key.

                MidiKey() {
                    pActiveNotes   = NULL;
                    KeyPressed     = false;
                    Active         = false;
                    ReleaseTrigger = release_trigger_none;
                    pEvents        = NULL;
                    VoiceTheftsQueued = 0;
                    Volume = 1.0f;
                    PanLeft = 1.0f;
                    PanRight = 1.0f;
                }

                void Reset() {
                    if (pActiveNotes) {
                        RTListNoteIterator itNote = pActiveNotes->first();
                        RTListNoteIterator itNotesEnd = pActiveNotes->end();
                        for (; itNote != itNotesEnd; ++itNote) { // iterate through all active notes on this key
                            itNote->reset();
                        }
                        pActiveNotes->clear();
                    }
                    if (pEvents) pEvents->clear();
                    KeyPressed        = false;
                    Active            = false;
                    ReleaseTrigger    = release_trigger_none;
                    itSelf            = Pool<uint>::Iterator();
                    VoiceTheftsQueued = 0;
                    Volume = 1.0f;
                    PanLeft = 1.0f;
                    PanRight = 1.0f;
                    ReverbSend = optional<float>::nothing;
                    ChorusSend = optional<float>::nothing;
                }
            };

            typedef typename RTList< Note<V> >::Iterator RTListNoteIterator;
            typedef typename RTList<V>::Iterator RTListVoiceIterator;
            typedef typename Pool<V>::Iterator PoolVoiceIterator;

            /**
             * Override this class to iterate through all active keys/voices
             * using ProcessActiveVoices() method.
             */
            class VoiceHandler {
                public:
                    /**
                     * @returns true if the voices on the specified key should be processed
                     * adn false to cancel the processing of the active voices for the
                     * specified key
                     */
                    virtual bool Process(MidiKey* pMidiKey) = 0;

                    virtual void Process(RTListVoiceIterator& itVoice) = 0;
            };

            class VoiceHandlerBase : public VoiceHandler {
                public:
                    virtual bool Process(MidiKey* pMidiKey) { return true; }
                    virtual void Process(RTListVoiceIterator& itVoice) { }
            };

            MidiKey*              pMIDIKeyInfo; ///< Contains all active voices sorted by MIDI key number and other informations to the respective MIDI key

            MidiKeyboardManager(AbstractEngineChannel* pEngineChannel) {
                pMIDIKeyInfo = new MidiKey[128];
                pActiveKeys  = new Pool<uint>(128);
                SoloMode     = false;
                SustainPedal   = false;
                SostenutoPedal = false;
                for (int i = 0 ; i < 128 ; i++) {
                    RoundRobinIndexes[i] = 0;
                    KeyDown[i] = false;

                    // by default use one counter for each key (the
                    // gig engine will change this to one counter per
                    // region)
                    pMIDIKeyInfo[i].pRoundRobinIndex = &RoundRobinIndexes[i];
                }
                m_engineChannel = pEngineChannel;
                m_voicePool = NULL;
            }

            virtual ~MidiKeyboardManager() {
                listeners.RemoveAllListeners();
                if (pActiveKeys) delete pActiveKeys;
                if (pMIDIKeyInfo) delete[] pMIDIKeyInfo;
            }

            void Reset() {
                SoloKey = -1;    // no solo key active yet

                // reset key info
                for (uint i = 0; i < 128; i++) {
                    pMIDIKeyInfo[i].Reset();
                    KeyDown[i] = false;
                    if (m_engineChannel->pScript)
                        m_engineChannel->pScript->pKeyEvents[i]->clear();
                }

                // free all active keys
                pActiveKeys->clear();
            }

            void AllocateActiveNotesLists(Pool< Note<V> >* pNotePool, Pool<V>* pVoicePool) {
                DeleteActiveNotesLists();

                m_voicePool = pVoicePool;

                for (uint i = 0; i < 128; i++) {
                    pMIDIKeyInfo[i].pActiveNotes = new RTList< Note<V> >(pNotePool);
                }
            }

            void DeleteActiveNotesLists() {
                for (uint i = 0; i < 128; i++) {
                    if (pMIDIKeyInfo[i].pActiveNotes) {
                        delete pMIDIKeyInfo[i].pActiveNotes;
                        pMIDIKeyInfo[i].pActiveNotes = NULL;
                    }
                }
                m_voicePool = NULL;
            }

            void AllocateEventsLists(Pool<Event>* pEventPool) {
                DeleteEventsLists();

                for (uint i = 0; i < 128; i++) {
                    pMIDIKeyInfo[i].pEvents = new RTList<Event>(pEventPool);
                }
            }

            void DeleteEventsLists() {
                for (uint i = 0; i < 128; i++) {
                    if (pMIDIKeyInfo[i].pEvents) {
                        delete pMIDIKeyInfo[i].pEvents;
                        pMIDIKeyInfo[i].pEvents = NULL;
                    }
                }
            }

            /*void ClearAllActiveKeyEvents() {
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end    = pActiveKeys->end();
                for(; iuiKey != end; ++iuiKey) {
                    pMIDIKeyInfo[*iuiKey].pEvents->clear(); // free all events on the key
                }
            }*/

            /**
             * Make sure the passed MIDI key is part of the list of active keys,
             * if it is not already, then add it to that list. Accordingly it is
             * safe to call this method even if the requested key is already
             * marked as active.
             */
            void markKeyAsActive(MidiKey* pKey) {
                if (!pKey->Active) { // mark as active key
                    pKey->Active = true;
                    pKey->itSelf = pActiveKeys->allocAppend();
                    const int iKey = pKey - &pMIDIKeyInfo[0];
                    *pKey->itSelf = iKey;
                }
            }

            /**
             *  Removes the given voice from the MIDI key's list of active voices.
             *  This method will be called when a voice went inactive, e.g. because
             *  it finished to playback its sample, finished its release stage or
             *  just was killed.
             *
             *  @param itVoice - points to the voice to be freed
             */
            void FreeVoice(PoolVoiceIterator& itVoice) {
                if (itVoice) {
                    //MidiKey* pKey = &pMIDIKeyInfo[itVoice->MIDIKey];

                    // if the sample and dimension region belong to an
                    // instrument that is unloaded, tell the disk thread to
                    // release them
                    if (itVoice->Orphan) {
                        if(itVoice->pDiskThread != NULL) {
                            itVoice->pDiskThread->OrderDeletionOfRegion(itVoice->GetRegion());
                        }
                    }

                    // free the voice object
                    m_voicePool->free(itVoice);
                }
                else std::cerr << "Couldn't release voice! (!itVoice)\n" << std::flush;
            }

            /**
             *  Called when there's no more voice left on a key, this call will
             *  update the key info respectively.
             *
             *  @param pEngineChannel - engine channel on which this event occured on
             *  @param pKey - key which is now inactive
             */
            void FreeKey(MidiKey* pKey) {
                if (pKey->pActiveNotes->isEmpty()) {
                    if (m_engineChannel->pScript)
                        m_engineChannel->pScript->pKeyEvents[pKey->itSelf]->clear();
                    pKey->Active = false;
                    pActiveKeys->free(pKey->itSelf); // remove key from list of active keys
                    pKey->itSelf = RTList<uint>::Iterator();
                    pKey->ReleaseTrigger = release_trigger_none;
                    pKey->pEvents->clear();
                    dmsg(3,("Key has no more voices now\n"));
                }
                else dmsg(1,("MidiKeyboardManager: Oops, tried to free a key which contains voices.\n"));
            }

            /**
             * Free all keys which have no active voices left
             */
            void FreeAllInactiveKeys() {
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end    = pActiveKeys->end();
                while (iuiKey != end) { // iterate through all active keys
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    ++iuiKey;
                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    { // iterate over all active notes on that key ...
                        if (itNote->pActiveVoices->isEmpty()) { // free note ...
                            itNote->reset();
                            pKey->pActiveNotes->free(itNote);
                        }
                        #if CONFIG_DEVMODE
                        else { // just a sanity check for debugging
                            RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                            RTListVoiceIterator itVoicesEnd = itNote->pActiveVoices->end();
                            for (; itVoice != itVoicesEnd; ++itVoice) { // iterate through all voices on this key
                                if (itVoice->itKillEvent) {
                                    dmsg(1,("gig::Engine: ERROR, killed voice survived !!!\n"));
                                }
                            }
                        }
                        #endif // CONFIG_DEVMODE
                    }
                    if (pKey->pActiveNotes->isEmpty()) FreeKey(pKey);
                }
            }

            int StealVoice (
                Pool<Event>::Iterator&   itNoteOnEvent,
                RTListVoiceIterator*     LastStolenVoice,
                RTListNoteIterator*      LastStolenNote,
                RTList<uint>::Iterator*  LastStolenKey
            ) {
                RTListVoiceIterator itSelectedVoice;

                // Select one voice for voice stealing
                switch (CONFIG_VOICE_STEAL_ALGO) {

                    // try to pick the oldest voice on the key where the new
                    // voice should be spawned, if there is no voice on that
                    // key, or no voice left to kill, then procceed with
                    // 'oldestkey' algorithm
                    case voice_steal_algo_oldestvoiceonkey: {
                        MidiKey* pSelectedKey = &pMIDIKeyInfo[itNoteOnEvent->Param.Note.Key];
                        for (RTListNoteIterator itNote = pSelectedKey->pActiveNotes->first(),
                             itNotesEnd = pSelectedKey->pActiveNotes->end();
                             itNote != itNotesEnd; ++itNote)
                        {
                            for (itSelectedVoice = itNote->pActiveVoices->first(); itSelectedVoice; ++itSelectedVoice)
                                if (itSelectedVoice->IsStealable()) // proceed iterating if voice was created in this audio fragment cycle
                                    goto voiceFound; // selection succeeded
                        }
                        // if we haven't found a voice then proceed with algorithm 'oldestkey' ...
                    } // no break - intentional !

                    // try to pick the oldest voice on the oldest active key
                    // from the same engine channel
                    // (caution: must stay after 'oldestvoiceonkey' algorithm !)
                    case voice_steal_algo_oldestkey: {
                        // if we already stole in this fragment, try to proceed to steal on same note
                        if (*LastStolenVoice) {
                            itSelectedVoice = *LastStolenVoice;
                            do {
                                ++itSelectedVoice;
                            } while (itSelectedVoice && !itSelectedVoice->IsStealable()); // proceed iterating if voice was created in this fragment cycle
                            // found a "stealable" voice ?
                            if (itSelectedVoice && itSelectedVoice->IsStealable()) {
                                // remember which voice we stole, so we can simply proceed on next voice stealing
                                *LastStolenVoice = itSelectedVoice;
                                break; // selection succeeded
                            }
                        }

                        // get (next) oldest note
                        if (*LastStolenNote) {
                            for (RTListNoteIterator itNote = ++(*LastStolenNote);
                                 itNote; ++itNote)
                            {
                                for (itSelectedVoice = itNote->pActiveVoices->first(); itSelectedVoice; ++itSelectedVoice) {
                                    // proceed iterating if voice was created in this audio fragment cycle
                                    if (itSelectedVoice->IsStealable()) {
                                        // remember which voice on which note we stole, so we can simply proceed on next voice stealing
                                        *LastStolenNote  = itNote;
                                        *LastStolenVoice = itSelectedVoice;
                                        goto voiceFound; // selection succeeded
                                    }
                                }
                            }
                        }

                        // get (next) oldest key
                        RTList<uint>::Iterator iuiSelectedKey = (*LastStolenKey) ? ++(*LastStolenKey) : pActiveKeys->first();
                        for (; iuiSelectedKey; ++iuiSelectedKey) {
                            MidiKey* pSelectedKey = &pMIDIKeyInfo[*iuiSelectedKey];

                            for (RTListNoteIterator itNote = pSelectedKey->pActiveNotes->first(),
                                 itNotesEnd = pSelectedKey->pActiveNotes->end();
                                 itNote != itNotesEnd; ++itNote)
                            {
                                for (itSelectedVoice = itNote->pActiveVoices->first(); itSelectedVoice; ++itSelectedVoice) {
                                    // proceed iterating if voice was created in this audio fragment cycle
                                    if (itSelectedVoice->IsStealable()) {
                                        // remember which voice on which key we stole, so we can simply proceed on next voice stealing
                                        *LastStolenKey  = iuiSelectedKey;
                                        *LastStolenNote = itNote;
                                        *LastStolenVoice = itSelectedVoice;
                                        goto voiceFound; // selection succeeded
                                    }
                                }
                            }
                        }
                        break;
                    }

                    // don't steal anything
                    case voice_steal_algo_none:
                    default: {
                        dmsg(1,("No free voice (voice stealing disabled)!\n"));
                        return -1;
                    }
                }
                
                voiceFound:

                if (!itSelectedVoice || !itSelectedVoice->IsStealable()) return -1;

                #if CONFIG_DEVMODE
                if (!itSelectedVoice->IsActive()) {
                    dmsg(1,("gig::Engine: ERROR, tried to steal a voice which was not active !!!\n"));
                    return -1;
                }
                #endif // CONFIG_DEVMODE

                // now kill the selected voice
                itSelectedVoice->Kill(itNoteOnEvent);

                return 0;
            }

            /**
             * Releases all voices. All voices will go into
             * the release stage and thus it might take some time (e.g. dependant to
             * their envelope release time) until they actually die.
             *
             * @param itReleaseEvent - event which caused this releasing of all voices
             */
            void ReleaseAllVoices(Pool<Event>::Iterator& itReleaseEvent) {
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                while (iuiKey) {
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    ++iuiKey;
                    // append a 'release' event to the key's own event list
                    RTList<Event>::Iterator itNewEvent = pKey->pEvents->allocAppend();
                    if (itNewEvent) {
                        *itNewEvent = *itReleaseEvent; // copy original event (to the key's event list)
                        itNewEvent->Type = Event::type_release_key; // transform event type
                    }
                    else dmsg(1,("Event pool emtpy!\n"));
                }
            }

            /**
             * Kill all active voices.
             * @returns The number of voices.
             */
            int KillAllVoices(Pool<Event>::Iterator& itKillEvent) {
                int count = 0;

                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end = pActiveKeys->end();
                for (; iuiKey != end; ++iuiKey) { // iterate through all active keys
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];

                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    {
                        RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                        RTListVoiceIterator itVoicesEnd = itNote->pActiveVoices->end();
                        for (; itVoice != itVoicesEnd; ++itVoice) { // iterate through all voices on this key
                            itVoice->Kill(itKillEvent);
                            count++;
                        }
                    }
                }

                return count;
            }

            /**
             * Kill all voices the *die hard* way.
             * @returns The number of pending stream deletions
             */
            int KillAllVoicesImmediately() {
                int iPendingStreamDeletions = 0;

                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end = pActiveKeys->end();
                for (; iuiKey != end; ++iuiKey) { // iterate through all active keys
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];

                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    {
                        RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                        RTListVoiceIterator itVoicesEnd = itNote->pActiveVoices->end();
                        for (; itVoice != itVoicesEnd; ++itVoice) { // iterate through all voices on this key
                            // request a notification from disk thread side for stream deletion
                            const Stream::Handle hStream = itVoice->KillImmediately(true);
                            if (hStream != Stream::INVALID_HANDLE) { // voice actually used a stream
                                iPendingStreamDeletions++;
                            }
                            // free the voice to the voice pool and update key info
                            itVoice->VoiceFreed();
                            FreeVoice(itVoice);
                        }
                    }
                }

                return iPendingStreamDeletions;
            }

            /**
             * Mark all currently active voices as "orphans", which means that the regions and
             * samples they use should be released to the instrument manager when the voices die.
             */
            void MarkAllActiveVoicesAsOrphans() {
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end = pActiveKeys->end();
                for (; iuiKey != end; ++iuiKey) { // iterate through all active keys
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];

                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    {
                        RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                        RTListVoiceIterator itVoicesEnd = itNote->pActiveVoices->end();
                        for (; itVoice != itVoicesEnd; ++itVoice) { // iterate through all voices on this key
                            itVoice->Orphan = true;
                        }
                    }
                }
            }

            void ProcessActiveVoices(VoiceHandler* pVoiceHandler) {
                if (pVoiceHandler == NULL) return;

                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                RTList<uint>::Iterator end = pActiveKeys->end();
                for (; iuiKey != end; ++iuiKey) { // iterate through all active keys
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    if (!pVoiceHandler->Process(pKey)) continue;

                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    {
                        RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                        RTListVoiceIterator itVoicesEnd = itNote->pActiveVoices->end();
                        for (; itVoice != itVoicesEnd; ++itVoice) { // iterate through all voices on this key
                            pVoiceHandler->Process(itVoice);
                        }
                    }
                }
            }
            
            /**
             * Recalculate the pitch of all active voices.
             */
            void OnScaleTuningChanged() {
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                for (; iuiKey; ++iuiKey) {
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];

                    for (RTListNoteIterator itNote = pKey->pActiveNotes->first(),
                         itNotesEnd = pKey->pActiveNotes->end();
                         itNote != itNotesEnd; ++itNote)
                    {
                        RTListVoiceIterator itVoice = itNote->pActiveVoices->first();
                        for (; itVoice; ++itVoice) {
                            itVoice->onScaleTuningChanged();
                        }
                    }
                }
            }
            
            void ProcessSustainPedalDown(Pool<Event>::Iterator& itEvent) {
                // Cancel release process of all voices
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                for (; iuiKey; ++iuiKey) {
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    if (!pKey->KeyPressed) {
                        RTList<Event>::Iterator itNewEvent = pKey->pEvents->allocAppend();
                        if (itNewEvent) {
                            *itNewEvent = *itEvent; // copy event to the key's own event list
                            itNewEvent->Type = Event::type_cancel_release_key; // transform event type
                        }
                        else dmsg(1,("Event pool emtpy!\n"));
                    }
                }
            }

            void ProcessSustainPedalUp(Pool<Event>::Iterator& itEvent) {
                // release voices if their respective key is not pressed
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                for (; iuiKey; ++iuiKey) {
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    if (!pKey->KeyPressed && ShouldReleaseVoice(*iuiKey)) {
                        RTList<Event>::Iterator itNewEvent = pKey->pEvents->allocAppend();
                        if (itNewEvent) {
                            *itNewEvent = *itEvent; // copy event to the key's own event list
                            itNewEvent->Type = Event::type_release_key; // transform event type
                            itNewEvent->Param.Note.Key = *iuiKey;
                            itNewEvent->Param.Note.Velocity = 127;

                            // process release trigger (if requested)
                            if (pKey->ReleaseTrigger & release_trigger_sustain) {
                                if (pKey->ReleaseTrigger & release_trigger_sustain_keyvelocity)
                                    itNewEvent->Param.Note.Velocity = pKey->Velocity;

                                //HACK: set sustain CC (64) as "pressed down" for a short moment, so that release trigger voices can distinguish between note off and sustain pedal up cases
                                AbstractEngineChannel* pChannel = (AbstractEngineChannel*) itEvent->pEngineChannel;
                                const int8_t CC64Value = pChannel->ControllerTable[64];
                                pChannel->ControllerTable[64] = 127;

                                // now spawn release trigger voices (if required)
                                ProcessReleaseTriggerBySustain(itNewEvent);

                                //HACK: reset sustain pedal CC value to old one (see comment above)
                                pChannel->ControllerTable[64] = CC64Value;   
                            }
                        }
                        else dmsg(1,("Event pool emtpy!\n"));
                    }
                }
            }

            /**
             * Whether @a key is still kept active due to sostenuto pedal usage.
             *
             * @param key - note number of key
             */
            inline bool SostenutoActiveOnKey(int key) const {
                if (SostenutoPedal) {
                    for (int i = 0; i < SostenutoKeyCount; i++)
                        if (key == SostenutoKeys[i]) return true;
                }
                return false;
            }

            /**
             * Determines whether the specified voice should be released.
             *
             * @param pEngineChannel - The engine channel on which the voice should be checked
             * @param Key - The key number
             * @returns true if the specified voice should be released, false otherwise.
             */
            bool ShouldReleaseVoice(int Key) {
                if (SustainPedal) return false;
                if (SostenutoActiveOnKey(Key)) return false;
                return true;
            }

            void ProcessSostenutoPedalDown() {
                SostenutoKeyCount = 0;
                // Remeber the pressed keys
                RTList<uint>::Iterator iuiKey = pActiveKeys->first();
                for (; iuiKey; ++iuiKey) {
                    MidiKey* pKey = &pMIDIKeyInfo[*iuiKey];
                    if (pKey->KeyPressed && SostenutoKeyCount < 128) SostenutoKeys[SostenutoKeyCount++] = *iuiKey;
                }
            }

            void ProcessSostenutoPedalUp(Pool<Event>::Iterator& itEvent) {
                // release voices if the damper pedal is up and their respective key is not pressed
                for (int i = 0; i < SostenutoKeyCount; i++) {
                    MidiKey* pKey = &pMIDIKeyInfo[SostenutoKeys[i]];
                    if (!pKey->KeyPressed && !SustainPedal) {
                        RTList<Event>::Iterator itNewEvent = pKey->pEvents->allocAppend();
                        if (itNewEvent) {
                            *itNewEvent = *itEvent; // copy event to the key's own event list
                            itNewEvent->Type = Event::type_release_key; // transform event type
                        }
                        else dmsg(1,("Event pool emtpy!\n"));
                    }
                }
            }

            void AddMidiKeyboardListener(MidiKeyboardListener* l) { listeners.AddListener(l); }

            void RemoveMidiKeyboardListener(MidiKeyboardListener* l) { listeners.RemoveListener(l); }

        protected:
            AbstractEngineChannel* m_engineChannel;
            Pool<V>* m_voicePool;

            class Listeners : public MidiKeyboardListener, public ListenerList<MidiKeyboardListener*> {
            public:
                REGISTER_FIRE_EVENT_METHOD_ARG2(PreProcessNoteOn, uint8_t, uint8_t)
                REGISTER_FIRE_EVENT_METHOD_ARG2(PostProcessNoteOn, uint8_t, uint8_t)
                REGISTER_FIRE_EVENT_METHOD_ARG2(PreProcessNoteOff, uint8_t, uint8_t)
                REGISTER_FIRE_EVENT_METHOD_ARG2(PostProcessNoteOff, uint8_t, uint8_t)
                REGISTER_FIRE_EVENT_METHOD(PreProcessSustainPedalUp)
                REGISTER_FIRE_EVENT_METHOD(PostProcessSustainPedalUp)
                REGISTER_FIRE_EVENT_METHOD(PreProcessSustainPedalDown)
                REGISTER_FIRE_EVENT_METHOD(PostProcessSustainPedalDown)
                REGISTER_FIRE_EVENT_METHOD(PreProcessSostenutoPedalUp)
                REGISTER_FIRE_EVENT_METHOD(PostProcessSostenutoPedalUp)
                REGISTER_FIRE_EVENT_METHOD(PreProcessSostenutoPedalDown)
                REGISTER_FIRE_EVENT_METHOD(PostProcessSostenutoPedalDown)
            } listeners;
    };
} // namespace LinuxSampler

#endif  /* __LS_MIDIKEYBOARDMANAGER_H__ */
