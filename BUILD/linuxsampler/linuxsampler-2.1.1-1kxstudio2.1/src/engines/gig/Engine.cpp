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

#include "Engine.h"
#include "EngineChannel.h"
#include "InstrumentScriptVM.h"

namespace LinuxSampler { namespace gig {
    Engine::Format Engine::GetEngineFormat() { return GIG; }

    void Engine::CreateInstrumentScriptVM() {
        dmsg(2,("gig::Engine created Giga format scriptvm\n"));
        if (pScriptVM) return;
        pScriptVM = new InstrumentScriptVM; // gig format specific extended script runner
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
        dmsg(4,("Engine::ContinuousController cc=%d v=%d\n", itControlChangeEvent->Param.CC.Controller, itControlChangeEvent->Param.CC.Value));

        EngineChannel* pChannel = dynamic_cast<EngineChannel*>(pEngineChannel);
        // handle the "control triggered" MIDI rule: a control change
        // event can trigger a new note on or note off event
        if (pChannel->pInstrument) {

            ::gig::MidiRule* rule;
            for (int i = 0 ; (rule = pChannel->pInstrument->GetMidiRule(i)) ; i++) {

                if (::gig::MidiRuleCtrlTrigger* ctrlTrigger =
                    dynamic_cast< ::gig::MidiRuleCtrlTrigger*>(rule)) {
                    if (itControlChangeEvent->Param.CC.Controller ==
                        ctrlTrigger->ControllerNumber) {

                        uint8_t oldCCValue = pChannel->ControllerTable[
                            itControlChangeEvent->Param.CC.Controller];
                        uint8_t newCCValue = itControlChangeEvent->Param.CC.Value;

                        for (int i = 0 ; i < ctrlTrigger->Triggers ; i++) {
                            ::gig::MidiRuleCtrlTrigger::trigger_t* pTrigger =
                                  &ctrlTrigger->pTriggers[i];

                            // check if the controller has passed the
                            // trigger point in the right direction
                            if ((pTrigger->Descending &&
                                 oldCCValue > pTrigger->TriggerPoint &&
                                 newCCValue <= pTrigger->TriggerPoint) ||
                                (!pTrigger->Descending &&
                                 oldCCValue < pTrigger->TriggerPoint &&
                                 newCCValue >= pTrigger->TriggerPoint)) {

                                RTList<Event>::Iterator itNewEvent = pGlobalEvents->allocAppend();
                                if (itNewEvent) {
                                    *itNewEvent = *itControlChangeEvent;
                                    itNewEvent->Param.Note.Key = pTrigger->Key;

                                    if (pTrigger->NoteOff || pTrigger->Velocity == 0) {
                                        itNewEvent->Type = Event::type_note_off;
                                        itNewEvent->Param.Note.Velocity = 100;

                                        ProcessNoteOff(pEngineChannel, itNewEvent);
                                    } else {
                                        itNewEvent->Type = Event::type_note_on;
                                        //TODO: if Velocity is 255, the triggered velocity should
                                        // depend on how fast the controller is moving
                                        itNewEvent->Param.Note.Velocity =
                                            pTrigger->Velocity == 255 ? 100 :
                                            pTrigger->Velocity;

                                        ProcessNoteOn(pEngineChannel, itNewEvent);
                                    }
                                }
                                else dmsg(1,("Event pool emtpy!\n"));
                            }
                        }
                    }
                }
            }
        }

        // update controller value in the engine channel's controller table
        pChannel->ControllerTable[itControlChangeEvent->Param.CC.Controller] = itControlChangeEvent->Param.CC.Value;

        ProcessHardcodedControllers(pEngineChannel, itControlChangeEvent);

        // handle FX send controllers
        ProcessFxSendControllers(pChannel, itControlChangeEvent);
    }

    void Engine::ProcessChannelPressure(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itChannelPressureEvent) {
        // forward this to the CC routine, so it updates the current aftertouch value and may handle aftertouch trigger rules
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
            dmsg(1,("gig::Engine: No Note object for triggering new voices!\n"));
            return;
        }

        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);

        // first, get total amount of required voices (dependant on amount of layers)
        // (using the note's MIDI note number instead of the MIDI event's one,
        //  because an instrument script might have modified the note number)
        ::gig::Region* pRegion = pChannel->pInstrument->GetRegion(itNote->cause.Param.Note.Key);
        if (!pRegion || RegionSuspended(pRegion))
            return;
        const int voicesRequired = pRegion->Layers;
        if (voicesRequired <= 0)
            return;

        // now launch the required amount of voices
        for (int i = 0; i < voicesRequired; i++) {
            VoiceIterator itNewVoice =
                LaunchVoice(pChannel, itNoteOnEvent, i, false, true, HandleKeyGroupConflicts);
            if (!itNewVoice) continue;
            itNewVoice.moveToEndOf(itNote->pActiveVoices);
        }
    }

    void Engine::TriggerReleaseVoices (
        LinuxSampler::EngineChannel*  pEngineChannel,
        RTList<Event>::Iterator&      itNoteOffEvent
    ) {
        NoteIterator itNote = GetNotePool()->fromID(itNoteOffEvent->Param.Note.ID);
        if (!itNote) {
            dmsg(1,("gig::Engine: No Note object for triggering new release voices!\n"));
            return;
        }

        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);
        MidiKey* pKey = &pChannel->pMIDIKeyInfo[itNoteOffEvent->Param.Note.Key];
        // first, get total amount of required voices (dependant on amount of layers)
        // (using the note's MIDI note number instead of the MIDI event's one,
        //  because an instrument script might have modified the note number)
        ::gig::Region* pRegion = pChannel->pInstrument->GetRegion(itNote->cause.Param.Note.Key);
        if (!pRegion)
            return;
        const int voicesRequired = pRegion->Layers;
        if (voicesRequired <= 0)
            return;

        // MIDI note-on velocity is used instead of note-off velocity
        itNoteOffEvent->Param.Note.Velocity = pKey->Velocity;

        // now launch the required amount of voices
        for (int i = 0; i < voicesRequired; i++) {
            VoiceIterator itNewVoice =
                LaunchVoice(pChannel, itNoteOffEvent, i, true, false, false); //FIXME: for the moment we don't perform voice stealing for release triggered samples
            if (!itNewVoice) continue;
            itNewVoice.moveToEndOf(itNote->pActiveVoices);
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
        NoteIterator itNote = GetNotePool()->fromID(itNoteOnEvent->Param.Note.ID);
        if (!itNote) {
            dmsg(1,("gig::Engine: No Note object for launching voices!\n"));
            return Pool<Voice>::Iterator();
        }

        EngineChannel* pChannel = static_cast<EngineChannel*>(pEngineChannel);
        // the note's MIDI note number might differ from the event's note number
        // because a script might have modified the note's note number
        int MIDIKey = itNoteOnEvent->Param.Note.Key;
        int NoteKey = itNote->cause.Param.Note.Key;
        //EngineChannel::MidiKey* pKey  = &pChannel->pMIDIKeyInfo[MIDIKey];
        ::gig::Region* pRegion = pChannel->pInstrument->GetRegion(NoteKey);

        // if nothing defined for this key
        if (!pRegion) return Pool<Voice>::Iterator(); // nothing to do

        int iKeyGroup = pRegion->KeyGroup;
        // only need to send a group event from the first voice in a layered region,
        // as all layers in a region always belongs to the same key group
        if (HandleKeyGroupConflicts && iLayer == 0) pChannel->HandleKeyGroupConflicts(iKeyGroup, itNoteOnEvent);

        Voice::type_t VoiceType = Voice::type_normal;

        // get current dimension values to select the right dimension region
        //TODO: for stolen voices this dimension region selection block is processed twice, this should be changed
        //FIXME: controller values for selecting the dimension region here are currently not sample accurate
        uint DimValues[8] = { 0 };
        for (int i = pRegion->Dimensions - 1; i >= 0; i--) {
            switch (pRegion->pDimensionDefinitions[i].dimension) {
                case ::gig::dimension_samplechannel:
                    DimValues[i] = 0; //TODO: we currently ignore this dimension
                    break;
                case ::gig::dimension_layer:
                    DimValues[i] = iLayer;
                    break;
                case ::gig::dimension_velocity:
                    DimValues[i] = itNote->cause.Param.Note.Velocity;
                    break;
                case ::gig::dimension_channelaftertouch:
                    DimValues[i] = pChannel->ControllerTable[128];
                    break;
                case ::gig::dimension_releasetrigger:
                    VoiceType = (ReleaseTriggerVoice) ? Voice::type_release_trigger : (!iLayer) ? Voice::type_release_trigger_required : Voice::type_normal;
                    DimValues[i] = (uint) ReleaseTriggerVoice;
                    break;
                case ::gig::dimension_keyboard:
                    DimValues[i] = (uint) (pChannel->CurrentKeyDimension * pRegion->pDimensionDefinitions[i].zones);
                    break;
                case ::gig::dimension_roundrobin:
                    DimValues[i] = uint(*pChannel->pMIDIKeyInfo[MIDIKey].pRoundRobinIndex % pRegion->pDimensionDefinitions[i].zones); // RoundRobinIndex is incremented for each note on in this Region
                    break;
                case ::gig::dimension_roundrobinkeyboard:
                    DimValues[i] = uint(pChannel->RoundRobinIndex % pRegion->pDimensionDefinitions[i].zones); // RoundRobinIndex is incremented for each note on
                    break;
                case ::gig::dimension_random:
                    DimValues[i] = uint(Random() * pRegion->pDimensionDefinitions[i].zones);
                    break;
                case ::gig::dimension_smartmidi:
                    DimValues[i] = 0;
                    break;
                case ::gig::dimension_modwheel:
                    DimValues[i] = pChannel->ControllerTable[1];
                    break;
                case ::gig::dimension_breath:
                    DimValues[i] = pChannel->ControllerTable[2];
                    break;
                case ::gig::dimension_foot:
                    DimValues[i] = pChannel->ControllerTable[4];
                    break;
                case ::gig::dimension_portamentotime:
                    DimValues[i] = pChannel->ControllerTable[5];
                    break;
                case ::gig::dimension_effect1:
                    DimValues[i] = pChannel->ControllerTable[12];
                    break;
                case ::gig::dimension_effect2:
                    DimValues[i] = pChannel->ControllerTable[13];
                    break;
                case ::gig::dimension_genpurpose1:
                    DimValues[i] = pChannel->ControllerTable[16];
                    break;
                case ::gig::dimension_genpurpose2:
                    DimValues[i] = pChannel->ControllerTable[17];
                    break;
                case ::gig::dimension_genpurpose3:
                    DimValues[i] = pChannel->ControllerTable[18];
                    break;
                case ::gig::dimension_genpurpose4:
                    DimValues[i] = pChannel->ControllerTable[19];
                    break;
                case ::gig::dimension_sustainpedal:
                    DimValues[i] = pChannel->ControllerTable[64];
                    break;
                case ::gig::dimension_portamento:
                    DimValues[i] = pChannel->ControllerTable[65];
                    break;
                case ::gig::dimension_sostenutopedal:
                    DimValues[i] = pChannel->ControllerTable[66];
                    break;
                case ::gig::dimension_softpedal:
                    DimValues[i] = pChannel->ControllerTable[67];
                    break;
                case ::gig::dimension_genpurpose5:
                    DimValues[i] = pChannel->ControllerTable[80];
                    break;
                case ::gig::dimension_genpurpose6:
                    DimValues[i] = pChannel->ControllerTable[81];
                    break;
                case ::gig::dimension_genpurpose7:
                    DimValues[i] = pChannel->ControllerTable[82];
                    break;
                case ::gig::dimension_genpurpose8:
                    DimValues[i] = pChannel->ControllerTable[83];
                    break;
                case ::gig::dimension_effect1depth:
                    DimValues[i] = pChannel->ControllerTable[91];
                    break;
                case ::gig::dimension_effect2depth:
                    DimValues[i] = pChannel->ControllerTable[92];
                    break;
                case ::gig::dimension_effect3depth:
                    DimValues[i] = pChannel->ControllerTable[93];
                    break;
                case ::gig::dimension_effect4depth:
                    DimValues[i] = pChannel->ControllerTable[94];
                    break;
                case ::gig::dimension_effect5depth:
                    DimValues[i] = pChannel->ControllerTable[95];
                    break;
                case ::gig::dimension_none:
                    std::cerr << "gig::Engine::LaunchVoice() Error: dimension=none\n" << std::flush;
                    break;
                default:
                    std::cerr << "gig::Engine::LaunchVoice() Error: Unknown dimension\n" << std::flush;
            }
        }

        // return if this is a release triggered voice and there is no
        // releasetrigger dimension (could happen if an instrument
        // change has occured between note on and off)
        if (ReleaseTriggerVoice && !(VoiceType & Voice::type_release_trigger)) return Pool<Voice>::Iterator();

        ::gig::DimensionRegion* pDimRgn;
        if (!itNote->Format.Gig.DimMask) { // normal case ...
            pDimRgn = pRegion->GetDimensionRegionByValue(DimValues);
        } else { // some dimension zones were overridden (i.e. by instrument script) ...
            dmsg(3,("trigger with dim mask=%d val=%d\n", itNote->Format.Gig.DimMask, itNote->Format.Gig.DimBits));
            int index = pRegion->GetDimensionRegionIndexByValue(DimValues);
            index &= ~itNote->Format.Gig.DimMask;
            index |=  itNote->Format.Gig.DimBits & itNote->Format.Gig.DimMask;
            pDimRgn = pRegion->pDimensionRegions[index & 255];
        }
        if (!pDimRgn) return Pool<Voice>::Iterator(); // error (could not resolve dimension region)

        // no need to continue if sample is silent
        if (!pDimRgn->pSample || !pDimRgn->pSample->SamplesTotal) return Pool<Voice>::Iterator();
        
        dmsg(2,("sample -> \"%s\"\n", pDimRgn->pSample->pInfo->Name.c_str()));

        // allocate a new voice for the key
        Pool<Voice>::Iterator itNewVoice = GetVoicePool()->allocAppend();

        int res = InitNewVoice (
                pChannel, pDimRgn, itNoteOnEvent, VoiceType, iLayer,
                iKeyGroup, ReleaseTriggerVoice, VoiceStealing, itNewVoice
        );
        if (!res) return itNewVoice;

        return Pool<Voice>::Iterator(); // no free voice or error
    }

    bool Engine::DiskStreamSupported() {
        return true;
    }

    String Engine::Description() {
        return "GigaSampler Format Engine";
    }

    String Engine::Version() {
        String s = "$Revision: 3219 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

}} // namespace LinuxSampler::gig
