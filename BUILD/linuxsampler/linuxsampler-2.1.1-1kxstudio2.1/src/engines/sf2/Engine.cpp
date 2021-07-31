/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2010 Grigor Iliev                                  *
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

namespace LinuxSampler { namespace sf2 {
    Engine::Format Engine::GetEngineFormat() { return SF2; }

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
        dmsg(4,("Engine::ContinuousController cc=%d v=%d\n", itControlChangeEvent->Param.CC.Controller, itControlChangeEvent->Param.CC.Value));

        EngineChannel* pChannel = dynamic_cast<EngineChannel*>(pEngineChannel);
        // handle the "control triggered" MIDI rule: a control change
        // event can trigger a new note on or note off event
        if (pChannel->pInstrument) {

            // TODO:
        }

        // update controller value in the engine channel's controller table
        pChannel->ControllerTable[itControlChangeEvent->Param.CC.Controller] = itControlChangeEvent->Param.CC.Value;

        ProcessHardcodedControllers(pEngineChannel, itControlChangeEvent);

        // handle FX send controllers
        ProcessFxSendControllers(pChannel, itControlChangeEvent);
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
            dmsg(1,("sf2::Engine: No Note object for triggering new voices!\n"));
            return;
        }
        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);

        //uint8_t  chan     = pChannel->MidiChannel();
        int      key      = itNote->cause.Param.Note.Key; //itNoteOnEvent->Param.Note.Key; <- using note object instead, since note nr might been modified by script
        uint8_t  vel      = itNote->cause.Param.Note.Velocity; //itNoteOnEvent->Param.Note.Velocity; <- using note object instead, since velocity might been modified by script
        //int      bend     = pChannel->Pitch;
        //uint8_t  chanaft  = pChannel->ControllerTable[128];
        //uint8_t* cc       = pChannel->ControllerTable;

        int layer = 0;
        ::sf2::Query query(*pChannel->pInstrument);
        query.key = key;
        query.vel = vel;
        while (::sf2::Region* region = query.next()) {
            // TODO: Generators in the PGEN sub-chunk are applied relative to generators in the IGEN sub-chunk in an additive manner.  In
            // other words, PGEN generators increase or decrease the value of an IGEN generator.
            ::sf2::Query subQuery(*region->pInstrument);
            subQuery.key = key;
            subQuery.vel = vel;
            while (::sf2::Region* r = subQuery.next()) {
                //std::cout << r->GetSample()->GetName();
                //std::cout << " loKey: " << r->loKey << " hiKey: " << r->hiKey << " minVel: " << r->minVel << " maxVel: " << r->maxVel << " Vel: " << ((int)vel) << std::endl << std::endl;
                if (!RegionSuspended(r)) {
                    itNoteOnEvent->Param.Note.pRegion = r;
                    VoiceIterator itNewVoice =
                        LaunchVoice(pChannel, itNoteOnEvent, layer, false, true, HandleKeyGroupConflicts);
                    if (itNewVoice)
                        itNewVoice.moveToEndOf(itNote->pActiveVoices);
                }
                layer++;
            }
        }
    }

    void Engine::TriggerReleaseVoices (
        LinuxSampler::EngineChannel*  pEngineChannel,
        RTList<Event>::Iterator&      itNoteOffEvent
    ) {

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
        //EngineChannel::MidiKey* pKey = &pChannel->pMIDIKeyInfo[key];

        Voice::type_t VoiceType = Voice::type_normal;

        Pool<Voice>::Iterator itNewVoice;
        ::sf2::Region* pRgn = static_cast< ::sf2::Region*>(itNoteOnEvent->Param.Note.pRegion);

        // no need to process if sample is silent
        if (!pRgn->GetSample() || !pRgn->GetSample()->GetTotalFrameCount()) return Pool<Voice>::Iterator();

        int iKeyGroup = pRgn->exclusiveClass;
        if (HandleKeyGroupConflicts) pChannel->HandleKeyGroupConflicts(iKeyGroup, itNoteOnEvent);

        // allocate a new voice for the key
        itNewVoice = GetVoicePool()->allocAppend();
        int res = InitNewVoice (
                pChannel, pRgn, itNoteOnEvent, VoiceType, iLayer,
                iKeyGroup, ReleaseTriggerVoice, VoiceStealing, itNewVoice
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
        return "SoundFont Format Engine";
    }

    String Engine::Version() {
        String s = "$Revision: 3219 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

}} // namespace LinuxSampler::sf2
