/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2011 - 2012 Grigor Iliev                                *
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

#include "SF2SignalUnitRack.h"
#include "Voice.h"

namespace LinuxSampler { namespace sf2 {
    
    SFSignalUnit::SFSignalUnit(SF2SignalUnitRack* rack): SignalUnit(rack), pVoice(NULL) {
        
    }
    
    void EGUnit::EnterReleaseStage() {
        update(EG::event_release, pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
    }
    
    void EGUnit::CancelRelease() {
        update(EG::event_cancel_release, pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
    }

    
    void VolEGUnit::Trigger() {
        // set the delay trigger
        double d = pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        uiDelayTrigger = pVoice->pRegion->GetEG1PreAttackDelay(pVoice->pPresetRegion) * d;
        ////////////

        // GetEG1Sustain gets the decrease in level in centibels
        uint sustain = ::sf2::ToRatio(-1 * pVoice->pRegion->GetEG1Sustain(pVoice->pPresetRegion)) * 1000; // in permille
        if (pVoice->pNote)
            sustain *= pVoice->pNote->Override.Sustain;

        trigger (
            0, // should be in permille
            pVoice->pRegion->GetEG1Attack(pVoice->pPresetRegion),
            pVoice->pRegion->GetEG1Hold(pVoice->pPresetRegion),
            pVoice->pRegion->GetEG1Decay(pVoice->pPresetRegion),
            sustain,
            pVoice->pRegion->GetEG1Release(pVoice->pPresetRegion),
            pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE,
            false
        );
    }
    
    void VolEGUnit::Increment() {
        if (DelayStage()) return;

        EGUnit::Increment();
        if (!active()) return;
        
        switch (getSegmentType()) {
            case EG::segment_lin:
                processLin();
                break;
            case EG::segment_exp:
                processExp();
                break;
            case EG::segment_end:
                getLevel();
                break; // noop
            case EG::segment_pow:
                processPow();
                break;
        }
        
        if (active()) {

            // if sample has a loop and loop start has been reached in this subfragment, send a special event to EG1 to let it finish the attack hold stage
            /*if (pVoice->SmplInfo.HasLoops && pVoice->Pos <= pVoice->SmplInfo.LoopStart && pVoice->SmplInfo.LoopStart < newPos) {
                update(EG::event_hold_end, pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
            }*/
            // TODO: ^^^

            increment(1);
            if (!toStageEndLeft()) update(EG::event_stage_end, pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
         }
    }
    
    void ModEGUnit::Trigger() {
        double d = pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        uiDelayTrigger = pVoice->pRegion->GetEG2PreAttackDelay(pVoice->pPresetRegion) * d;

        trigger (
            0, // should be in permille
            pVoice->pRegion->GetEG2Attack(pVoice->pPresetRegion),
            pVoice->pRegion->GetEG2Hold(pVoice->pPresetRegion),
            pVoice->pRegion->GetEG2Decay(pVoice->pPresetRegion),
            uint(1000 - pVoice->pRegion->GetEG2Sustain(pVoice->pPresetRegion)),
            pVoice->pRegion->GetEG2Release(pVoice->pPresetRegion),
            pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE,
            true
        );
    }
    
    void ModEGUnit::Increment() {
        if (DelayStage()) return;

        EGUnit::Increment();
        if (!active()) return;
        
        switch (getSegmentType()) {
            case EG::segment_lin:
                processLin();
                break;
            case EG::segment_exp:
                processExp();
                break;
            case EG::segment_end:
                getLevel();
                break; // noop
            case EG::segment_pow:
                processPow();
                break;
        }
        
        if (active()) {
            increment(1);
            if (!toStageEndLeft()) update(EG::event_stage_end, pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
        }
    }


    void ModLfoUnit::Trigger() {
        //reset
        Level = 0;
        
        // set the delay trigger
        double samplerate = pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        uiDelayTrigger = pVoice->pRegion->GetDelayModLfo(pVoice->pPresetRegion) * samplerate;
        ////////////
            
        trigger (
            pVoice->pRegion->GetFreqModLfo(pVoice->pPresetRegion),
            start_level_min,
            1, 0, false, samplerate
        );
        updateByMIDICtrlValue(0);
    }

    void ModLfoUnit::Increment() {
        if (DelayStage()) return;
        
        SignalUnit::Increment();
        
        Level = render();
    }


    void VibLfoUnit::Trigger() {
        //reset
        Level = 0;

        // set the delay trigger
        double samplerate = pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        uiDelayTrigger = pVoice->pRegion->GetDelayVibLfo(pVoice->pPresetRegion) * samplerate;
        ////////////
            
        trigger (
            pVoice->pRegion->GetFreqVibLfo(pVoice->pPresetRegion),
            start_level_min,
            pVoice->pRegion->GetVibLfoToPitch(pVoice->pPresetRegion),
            0, false, samplerate
        );
        updateByMIDICtrlValue(0);
    }

    void VibLfoUnit::Increment() {
        if (DelayStage()) return;
        
        SignalUnit::Increment();
        
        Level = render();
    }


    EndpointUnit::EndpointUnit(SF2SignalUnitRack* rack): EndpointSignalUnit(rack) {
        
    }

    void EndpointUnit::Trigger() {
        prmModEgPitch->Coeff = pVoice->pRegion->GetModEnvToPitch(pVoice->pPresetRegion);
        if (prmModEgPitch->Coeff == ::sf2::NONE) prmModEgPitch->Coeff = 0;

        prmModEgCutoff->Coeff = pVoice->pRegion->GetModEnvToFilterFc(pVoice->pPresetRegion); // cents
        if (prmModEgCutoff->Coeff == ::sf2::NONE) prmModEgCutoff->Coeff = 0;

        prmModLfoVol->Coeff = pVoice->pRegion->GetModLfoToVolume(pVoice->pPresetRegion); // centibels
        if (prmModLfoVol->Coeff == ::sf2::NONE) prmModLfoVol->Coeff = 0;

        prmModLfoCutoff->Coeff = pVoice->pRegion->GetModLfoToFilterFc(pVoice->pPresetRegion);
        if (prmModLfoCutoff->Coeff == ::sf2::NONE) prmModLfoCutoff->Coeff = 0;

        prmModLfoPitch->Coeff = pVoice->pRegion->GetModLfoToPitch(pVoice->pPresetRegion);
        if (prmModLfoPitch->Coeff == ::sf2::NONE) prmModLfoPitch->Coeff = 0;
    }
    
    bool EndpointUnit::Active() {
        return prmVolEg->pUnit->Active(); // volEGUnit
    }
    
    float EndpointUnit::GetVolume() {
        if (!prmVolEg->pUnit->Active()) return 0;
        return prmVolEg->GetValue() * 
               ::sf2::ToRatio(prmModLfoVol->GetValue() /*logarithmically modified */);
    }
    
    float EndpointUnit::GetFilterCutoff() {
        double modEg, modLfo;
        modEg = prmModEgCutoff->pUnit->Active() ? prmModEgCutoff->GetValue() : 0;
        modEg = RTMath::CentsToFreqRatioUnlimited(modEg);
        
        modLfo = prmModLfoCutoff->pUnit->Active() ? prmModLfoCutoff->GetValue() : 0;
        modLfo = RTMath::CentsToFreqRatioUnlimited(modLfo);
        
        return modEg * modLfo;
    }
    
    float EndpointUnit::GetPitch() {
        double modEg, modLfo, vibLfo;
        modEg  = prmModEgPitch->pUnit->Active() ? RTMath::CentsToFreqRatioUnlimited(prmModEgPitch->GetValue()) : 1;
        modLfo = prmModLfoPitch->pUnit->Active() ? RTMath::CentsToFreqRatioUnlimited(prmModLfoPitch->GetValue()) : 1;
        vibLfo = prmVibLfo->pUnit->Active() ? RTMath::CentsToFreqRatioUnlimited(prmVibLfo->GetValue()) : 1;
        
        return modEg * modLfo * vibLfo;
    }
    
    float EndpointUnit::GetResonance() {
        return 1;
    }
    
    SF2SignalUnitRack::SF2SignalUnitRack(Voice* voice)
        : SignalUnitRack(MaxUnitCount), suVolEG(this), suModEG(this), suModLfo(this), suVibLfo(this), suEndpoint(this), pVoice(voice) {

        suVolEG.pVoice = suModEG.pVoice = suModLfo.pVoice = suVibLfo.pVoice = suEndpoint.pVoice = voice;
        Units.add(&suVolEG);
        Units.add(&suModEG);
        Units.add(&suModLfo);
        Units.add(&suVibLfo);
        Units.add(&suEndpoint);
        
        // Volume envelope
        suEndpoint.Params.add(SignalUnit::Parameter(&suVolEG)); // Volume
        // Modulation envelope
        suEndpoint.Params.add(SignalUnit::Parameter(&suModEG)); // Pitch
        suEndpoint.Params.add(SignalUnit::Parameter(&suModEG)); // Filter cutoff
        // Modulation LFO
        suEndpoint.Params.add(SignalUnit::Parameter(&suModLfo)); // Volume
        suEndpoint.Params.add(SignalUnit::Parameter(&suModLfo)); // Pitch
        suEndpoint.Params.add(SignalUnit::Parameter(&suModLfo)); // Filter cutoff
        // Vibrato LFO
        suEndpoint.Params.add(SignalUnit::Parameter(&suVibLfo)); // Pitch

        /* This should be done at the end because when a parameter is added to
           ArrayList a new copy is made for all parameters */
        suEndpoint.prmVolEg = &suEndpoint.Params[0];
        suEndpoint.prmModEgPitch = &suEndpoint.Params[1];
        suEndpoint.prmModEgCutoff = &suEndpoint.Params[2];
        suEndpoint.prmModLfoVol = &suEndpoint.Params[3];
        suEndpoint.prmModLfoPitch = &suEndpoint.Params[4];
        suEndpoint.prmModLfoCutoff = &suEndpoint.Params[5];
        suEndpoint.prmVibLfo = &suEndpoint.Params[6];
    }
    
    EndpointSignalUnit* SF2SignalUnitRack::GetEndpointUnit() {
        return static_cast<EndpointSignalUnit*> (&suEndpoint);
    }
    
    void SF2SignalUnitRack::EnterFadeOutStage() {
        suVolEG.enterFadeOutStage();
    }

    void SF2SignalUnitRack::EnterFadeOutStage(int maxFadeOutSteps) {
        suVolEG.enterFadeOutStage(maxFadeOutSteps);
    }

    void SF2SignalUnitRack::CalculateFadeOutCoeff(float FadeOutTime, float SampleRate) {
        suVolEG.CalculateFadeOutCoeff(FadeOutTime, SampleRate);
    }
}} // namespace LinuxSampler::sf2
