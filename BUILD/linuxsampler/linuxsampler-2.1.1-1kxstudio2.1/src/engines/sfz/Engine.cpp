/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Grigor Iliev                                  *
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

#include "Engine.h"
#include "EngineChannel.h"
#include "InstrumentScriptVM.h"

namespace LinuxSampler { namespace sfz {
    Engine::Format Engine::GetEngineFormat() { return SFZ; }

    void Engine::CreateInstrumentScriptVM() {
        dmsg(2,("sfz::Engine created SFZ format scriptvm\n"));
        if (pScriptVM) return;
        pScriptVM = new InstrumentScriptVM; // sfz format specific extended script runner
    }
    
    Engine::Engine() {
        pCCPool = new Pool<CCSignalUnit::CC>(GLOBAL_MAX_VOICES * MaxCCPerVoice);
        pSmootherPool = new Pool<Smoother>(GLOBAL_MAX_VOICES * MaxCCPerVoice);
        for (VoiceIterator iterVoice = GetVoicePool()->allocAppend(); iterVoice == GetVoicePool()->last(); iterVoice = GetVoicePool()->allocAppend()) {
            (static_cast<SfzSignalUnitRack*>(iterVoice->pSignalUnitRack))->InitRTLists();
        }
        GetVoicePool()->clear();
    }
    
    Engine::~Engine() {
        if (pCCPool) {
            pCCPool->clear();
            delete pCCPool;
        }
        
        if (pSmootherPool) {
            pSmootherPool->clear();
            delete pSmootherPool;
        }
    }
    
    void Engine::PostSetMaxVoices(int iVoices) {
        pCCPool->resizePool(iVoices * MaxCCPerVoice);
        pSmootherPool->resizePool(iVoices * MaxCCPerVoice);
        
        for (VoiceIterator iterVoice = GetVoicePool()->allocAppend(); iterVoice == GetVoicePool()->last(); iterVoice = GetVoicePool()->allocAppend()) {
            (static_cast<SfzSignalUnitRack*>(iterVoice->pSignalUnitRack))->InitRTLists();
        }
        GetVoicePool()->clear();
    }

    /**
     *  Reacts on supported control change commands (e.g. pitch bend wheel,
     *  modulation wheel, aftertouch).
     *
     *  @param pEngineChannel - engine channel on which this event occured on
     *  @param itControlChangeEvent - controller, value and time stamp of the event
     */
    void Engine::ProcessControlChange (
        LinuxSampler::EngineChannel*  pEngineChannel,
        Pool<Event>::Iterator&        itControlChangeEvent
    ) {
        uint8_t cc = itControlChangeEvent->Param.CC.Controller;
        dmsg(4,("Engine::ContinuousController cc=%d v=%d\n", cc, itControlChangeEvent->Param.CC.Value));

        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);

        // update controller value in the engine channel's controller table
        pChannel->ControllerTable[cc] = itControlChangeEvent->Param.CC.Value;

        ProcessHardcodedControllers(pEngineChannel, itControlChangeEvent);

        // handle FX send controllers
        ProcessFxSendControllers(pChannel, itControlChangeEvent);

        // handle control triggered regions: a control change event
        // can trigger a new voice
        if (pChannel->pInstrument && cc < 128) {

            ::sfz::Query q;
            q.chan        = itControlChangeEvent->Param.CC.Channel + 1;
            q.key         = 60;
            q.vel         = 127;
            q.bend        = pChannel->Pitch;
            q.bpm         = 0;
            q.chanaft     = pChannel->ControllerTable[128];
            q.polyaft     = 0;
            q.prog        = 0;
            q.rand        = Random();
            q.cc          = pChannel->ControllerTable;
            q.timer       = 0;
            q.sw          = pChannel->PressedKeys;
            q.last_sw_key = pChannel->LastKeySwitch;
            q.prev_sw_key = pChannel->LastKey;
            q.trig        = TRIGGER_ATTACK | TRIGGER_FIRST;

            q.search(pChannel->pInstrument, cc);

            NoteIterator itNewNote;

            int i = 0;
            while (::sfz::Region* region = q.next()) {
                if (!RegionSuspended(region)) {
                    itControlChangeEvent->Param.Note.Key = 60;
                    itControlChangeEvent->Param.Note.Velocity = 127;
                    itControlChangeEvent->Param.Note.pRegion = region;
                    if (!itNewNote) {
                        const note_id_t noteID = LaunchNewNote(pEngineChannel, itControlChangeEvent);
                        itNewNote = GetNotePool()->fromID(noteID);
                        if (!itNewNote) {
                            dmsg(1,("sfz::Engine: Note pool empty!\n"));
                            return;
                        }
                    }
                    VoiceIterator itNewVoice =
                        LaunchVoice(pChannel, itControlChangeEvent, i, false, false, true);
                    if (itNewVoice)
                        itNewVoice.moveToEndOf(itNewNote->pActiveVoices);
                }
                i++;
            }
        }
    }

    void Engine::ProcessChannelPressure(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itChannelPressureEvent) {
        // forward this to the CC routine, so it updates the current aftertouch value
        ProcessControlChange(pEngineChannel, itChannelPressureEvent);

        // if required: engine global aftertouch handling (apart from the per voice handling)
    }

    void Engine::ProcessPolyphonicKeyPressure(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNotePressureEvent) {
        // if required: engine global aftertouch handling (apart from the per voice handling)
    }

    DiskThread* Engine::CreateDiskThread() {
        return new DiskThread (
            iMaxDiskStreams,
            ((pAudioOutputDevice->MaxSamplesPerCycle() << CONFIG_MAX_PITCH) << 1) + 6, //FIXME: assuming stereo
            &instruments
        );
    }

    void Engine::TriggerNewVoices (
        LinuxSampler::EngineChannel* pEngineChannel,
        RTList<Event>::Iterator&     itNoteOnEvent,
        bool                         HandleKeyGroupConflicts
    ) {
        NoteIterator itNote = GetNotePool()->fromID(itNoteOnEvent->Param.Note.ID);
        if (!itNote) {
            dmsg(1,("sfz::Engine: No Note object for triggering new voices!\n"));
            return;
        }
        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);
        //MidiKey* pKey = &pChannel->pMIDIKeyInfo[itNoteOnEvent->Param.Note.Key];
        ::sfz::Query q;
        q.chan        = itNoteOnEvent->Param.Note.Channel + 1;
        q.key         = itNote->cause.Param.Note.Key; //itNoteOnEvent->Param.Note.Key; <- using note object instead, since note nr might been modified by script
        q.vel         = itNote->cause.Param.Note.Velocity; //itNoteOnEvent->Param.Note.Velocity; <- using note object instead, since velocity might been modified by script
        q.bend        = pChannel->Pitch;
        q.bpm         = 0;
        q.chanaft     = pChannel->ControllerTable[128];
        q.polyaft     = 0;
        q.prog        = 0;
        q.rand        = Random();
        q.cc          = pChannel->ControllerTable;
        q.timer       = 0;
        q.sw          = pChannel->PressedKeys;
        q.last_sw_key = pChannel->LastKeySwitch;
        q.prev_sw_key = pChannel->LastKey;
        q.trig        = TRIGGER_ATTACK |
            ((pChannel->LastKey != -1 &&
              pChannel->PressedKeys[pChannel->LastKey] &&
              pChannel->LastKey != q.key) ?
             TRIGGER_LEGATO : TRIGGER_FIRST);

        q.search(pChannel->pInstrument);

        int i = 0;
        while (::sfz::Region* region = q.next()) {
            if (!RegionSuspended(region)) {
                itNoteOnEvent->Param.Note.pRegion = region;
                VoiceIterator itNewVoice =
                    LaunchVoice(pChannel, itNoteOnEvent, i, false, true, HandleKeyGroupConflicts);
                if (itNewVoice)
                    itNewVoice.moveToEndOf(itNote->pActiveVoices);
            }
            i++;
        }
    }

    void Engine::TriggerReleaseVoices (
        LinuxSampler::EngineChannel*  pEngineChannel,
        RTList<Event>::Iterator&      itNoteOffEvent
    ) {
        NoteIterator itNote = GetNotePool()->fromID(itNoteOffEvent->Param.Note.ID);
        if (!itNote) {
            dmsg(1,("sfz::Engine: No Note object for triggering new release voices!\n"));
            return;
        }
        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);
        ::sfz::Query q;
        q.chan        = itNoteOffEvent->Param.Note.Channel + 1;
        q.key         = itNote->cause.Param.Note.Key; //itNoteOffEvent->Param.Note.Key; <- using note object instead, since note nr might been modified by script

        // MIDI note-on velocity is used instead of note-off velocity
        q.vel         = itNote->cause.Param.Note.Velocity; //pChannel->pMIDIKeyInfo[q.key].Velocity; <- using note object instead, since velocity might been modified by script
        itNoteOffEvent->Param.Note.Velocity = q.vel;

        q.bend        = pChannel->Pitch;
        q.bpm         = 0;
        q.chanaft     = pChannel->ControllerTable[128];
        q.polyaft     = 0;
        q.prog        = 0;
        q.rand        = Random();
        q.cc          = pChannel->ControllerTable;
        q.timer       = 0;
        q.sw          = pChannel->PressedKeys;
        q.last_sw_key = pChannel->LastKeySwitch;
        q.prev_sw_key = pChannel->LastKey;
        q.trig        = TRIGGER_RELEASE;

        q.search(pChannel->pInstrument);

        // now launch the required amount of voices
        int i = 0;
        while (::sfz::Region* region = q.next()) {
            itNoteOffEvent->Param.Note.pRegion = region;
            VoiceIterator itNewVoice =
                LaunchVoice(pChannel, itNoteOffEvent, i, true, false, true); //FIXME: for the moment we don't perform voice stealing for release triggered samples
            if (itNewVoice)
                itNewVoice.moveToEndOf(itNote->pActiveVoices);
            i++;
        }
    }

    Pool<Voice>::Iterator Engine::LaunchVoice (
        LinuxSampler::EngineChannel*  pEngineChannel,
        Pool<Event>::Iterator&        itNoteOnEvent,
        int                           iLayer,
        bool                          ReleaseTriggerVoice,
        bool                          VoiceStealing,
        bool                          HandleKeyGroupConflicts
    ) {
        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);
        //int key = itNoteOnEvent->Param.Note.Key;
        //EngineChannel::MidiKey* pKey  = &pChannel->pMIDIKeyInfo[key];
        ::sfz::Region* pRgn = static_cast< ::sfz::Region*>(itNoteOnEvent->Param.Note.pRegion);

        Voice::type_t VoiceType =
            itNoteOnEvent->Type == Event::type_control_change ? Voice::type_controller_triggered :
            ReleaseTriggerVoice ? Voice::type_release_trigger :
            iLayer == 0 ? Voice::type_release_trigger_required :
            Voice::type_normal;
        if (pRgn->loop_mode == ::sfz::ONE_SHOT) VoiceType |= Voice::type_one_shot;

        Pool<Voice>::Iterator itNewVoice;

        if (HandleKeyGroupConflicts) pChannel->HandleKeyGroupConflicts(pRgn->group, itNoteOnEvent);

        // no need to process if sample is silent
        if (!pRgn->GetSample(false) || !pRgn->GetSample()->GetTotalFrameCount()) return Pool<Voice>::Iterator();

        // allocate a new voice for the key
        itNewVoice = GetVoicePool()->allocAppend();
        int res = InitNewVoice (
                pChannel, pRgn, itNoteOnEvent, VoiceType, iLayer,
                pRgn->off_by, ReleaseTriggerVoice, VoiceStealing, itNewVoice
        );
        if (!res) return itNewVoice;

        // return if this is a release triggered voice and there is no
        // releasetrigger dimension (could happen if an instrument
        // change has occured between note on and off)
        //if (ReleaseTriggerVoice && VoiceType != Voice::type_release_trigger) return Pool<Voice>::Iterator();

        return Pool<Voice>::Iterator(); // no free voice or error
    }

    bool Engine::DiskStreamSupported() {
        return true;
    }

    String Engine::Description() {
        return "SFZ Format Engine";
    }

    String Engine::Version() {
        String s = "$Revision: 3219 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

}} // namespace LinuxSampler::sfz
