/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2016 Christian Schoenebeck and Grigor Iliev      *
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

#include "Voice.h"

#include "Engine.h"
#include "EngineChannel.h"

namespace LinuxSampler { namespace sf2 {

    typedef LinuxSampler::VoiceBase<EngineChannel, ::sf2::Region, ::sf2::Sample, DiskThread> SF2VoiceBase;

    Voice::Voice(): SF2VoiceBase(&SignalRack), SignalRack(this) {
        pEngine = NULL;
        pEG1 = NULL;
        pEG2 = NULL;
    }

    Voice::~Voice() {

    }

    void Voice::AboutToTrigger() {
        
    }

    EngineChannel* Voice::GetSf2EngineChannel() {
        return static_cast<EngineChannel*>(pEngineChannel);
    }

    void Voice::SetEngine(LinuxSampler::Engine* pEngine) {
        Engine* engine = static_cast<Engine*>(pEngine);
        this->pEngine     = engine;
        this->pDiskThread = engine->pDiskThread;
        dmsg(6,("Voice::SetEngine()\n"));
    }

    Voice::SampleInfo Voice::GetSampleInfo() {
        SampleInfo si;
        si.SampleRate       = pSample->SampleRate;
        si.ChannelCount     = pSample->GetChannelCount();
        si.FrameSize        = pSample->GetFrameSize();
        si.BitDepth         = (pSample->GetFrameSize() / pSample->GetChannelCount()) * 8;
        si.TotalFrameCount  = (uint)pSample->GetTotalFrameCount();

        si.HasLoops       = pRegion->HasLoop;
        si.LoopStart      = (si.HasLoops) ? pRegion->LoopStart : 0;
        si.LoopLength     = (si.HasLoops) ? ((pRegion->LoopEnd) - pRegion->LoopStart): 0;
        si.LoopPlayCount  = 0; // TODO:
        si.Unpitched      = pSample->IsUnpitched();

        return si;
    }

    Voice::RegionInfo Voice::GetRegionInfo() {
        ::sf2::Region* reg = NULL;
        ::sf2::Preset* preset = GetSf2EngineChannel()->pInstrument;
        for (int i = 0; i < preset->GetRegionCount(); i++) { // TODO: some optimization?
            if (preset->GetRegion(i)->pInstrument == pRegion->GetParentInstrument()) {
                reg = preset->GetRegion(i); // TODO: Can the instrument belong to more than one preset region?
                break;
            }
        }
        pPresetRegion = reg;

        RegionInfo ri;
        ri.UnityNote = pRegion->GetUnityNote();
        ri.FineTune  = pRegion->GetFineTune(reg) + (pRegion->GetCoarseTune(reg) * 100);
        ri.Pan       = pRegion->GetPan(reg);
        ri.SampleStartOffset = pRegion->startAddrsOffset + pRegion->startAddrsCoarseOffset;

        // sample pitch
        ri.VCFEnabled    = true; // TODO:
        ri.VCFType       = Filter::vcf_type_2p_lowpass; // TODO:
        ri.VCFResonance  = 0; // TODO:

        ri.ReleaseTriggerDecay = 0; // TODO:

        return ri;
    }

    Voice::InstrumentInfo Voice::GetInstrumentInfo() {
        InstrumentInfo ii;
        ii.FineTune = 0; // TODO: 
        ii.PitchbendRange = 2; // TODO: 

        return ii;
    }

    double Voice::GetSampleAttenuation() {
        return 1.0; // TODO: 
    }

    double Voice::GetVelocityAttenuation(uint8_t MIDIKeyVelocity) {
        return double(MIDIKeyVelocity) / 127.0f; // TODO: 
    }

    double Voice::GetVelocityRelease(uint8_t MIDIKeyVelocity) {
        return 0.9; // TODO:
    }

    void Voice::ProcessCCEvent(RTList<Event>::Iterator& itEvent) {
        /*if (itEvent->Type == Event::type_control_change && itEvent->Param.CC.Controller) { // if (valid) MIDI control change event
            if (pRegion->AttenuationController.type == ::gig::attenuation_ctrl_t::type_controlchange &&
                itEvent->Param.CC.Controller == pRegion->AttenuationController.controller_number) {
                CrossfadeSmoother.update(AbstractEngine::CrossfadeCurve[CrossfadeAttenuation(itEvent->Param.CC.Value)]);
            }
        }*/ // TODO: ^^^
    }
    
    void Voice::ProcessChannelPressureEvent(RTList<Event>::Iterator& itEvent) {
        //TODO: ...
    }
    
    void Voice::ProcessPolyphonicKeyPressureEvent(RTList<Event>::Iterator& itEvent) {
        //TODO: ...
    }

    void Voice::ProcessCutoffEvent(RTList<Event>::Iterator& itEvent) {
        /*int ccvalue = itEvent->Param.CC.Value;
        if (VCFCutoffCtrl.value == ccvalue) return;
        VCFCutoffCtrl.value == ccvalue;
        if (pRegion->VCFCutoffControllerInvert)  ccvalue = 127 - ccvalue;
        if (ccvalue < pRegion->VCFVelocityScale) ccvalue = pRegion->VCFVelocityScale;
        float cutoff = CutoffBase * float(ccvalue);
        if (cutoff > 127.0f) cutoff = 127.0f;

        VCFCutoffCtrl.fvalue = cutoff; // needed for initialization of fFinalCutoff next time
        fFinalCutoff = cutoff;*/ // TODO: ^^^
    }

    double Voice::CalculateCrossfadeVolume(uint8_t MIDIKeyVelocity) {
        /*float crossfadeVolume;
        switch (pRegion->AttenuationController.type) {
            case ::gig::attenuation_ctrl_t::type_channelaftertouch:
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(GetSf2EngineChannel()->ControllerTable[128])];
                break;
            case ::gig::attenuation_ctrl_t::type_velocity:
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(MIDIKeyVelocity)];
                break;
            case ::gig::attenuation_ctrl_t::type_controlchange: //FIXME: currently not sample accurate
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(GetSf2EngineChannel()->ControllerTable[pRegion->AttenuationController.controller_number])];
                break;
            case ::gig::attenuation_ctrl_t::type_none: // no crossfade defined
            default:
                crossfadeVolume = 1.0f;
        }

        return crossfadeVolume;*/ // TODO: ^^^
        return 1.0f;
    }

    double Voice::GetEG1ControllerValue(uint8_t MIDIKeyVelocity) {
        /*double eg1controllervalue = 0;
        switch (pRegion->EG1Controller.type) {
            case ::gig::eg1_ctrl_t::type_none: // no controller defined
                eg1controllervalue = 0;
                break;
            case ::gig::eg1_ctrl_t::type_channelaftertouch:
                eg1controllervalue = GetSf2EngineChannel()->ControllerTable[128];
                break;
            case ::gig::eg1_ctrl_t::type_velocity:
                eg1controllervalue = MIDIKeyVelocity;
                break;
            case ::gig::eg1_ctrl_t::type_controlchange: // MIDI control change controller
                eg1controllervalue = GetSf2EngineChannel()->ControllerTable[pRegion->EG1Controller.controller_number];
                break;
        }
        if (pRegion->EG1ControllerInvert) eg1controllervalue = 127 - eg1controllervalue;

        return eg1controllervalue;*/ // TODO: ^^^
        return 0;
    }

    Voice::EGInfo Voice::CalculateEG1ControllerInfluence(double eg1ControllerValue) {
        /*EGInfo eg;
        // (eg1attack is different from the others)
        eg.Attack  = (pRegion->EG1ControllerAttackInfluence)  ?
            1 + 0.031 * (double) (pRegion->EG1ControllerAttackInfluence == 1 ?
                                  1 : 1 << pRegion->EG1ControllerAttackInfluence) * eg1ControllerValue : 1.0;
        eg.Decay   = (pRegion->EG1ControllerDecayInfluence)   ? 1 + 0.00775 * (double) (1 << pRegion->EG1ControllerDecayInfluence)   * eg1ControllerValue : 1.0;
        eg.Release = (pRegion->EG1ControllerReleaseInfluence) ? 1 + 0.00775 * (double) (1 << pRegion->EG1ControllerReleaseInfluence) * eg1ControllerValue : 1.0;

        return eg;*/ // TODO: ^^^
        EGInfo eg;
        eg.Attack = 1.0;
        eg.Decay = 1.0;
        eg.Release = 1.0;
        return eg;
    }

    double Voice::GetEG2ControllerValue(uint8_t MIDIKeyVelocity) {
        /*double eg2controllervalue = 0;
        switch (pRegion->EG2Controller.type) {
            case ::gig::eg2_ctrl_t::type_none: // no controller defined
                eg2controllervalue = 0;
                break;
            case ::gig::eg2_ctrl_t::type_channelaftertouch:
                eg2controllervalue = GetSf2EngineChannel()->ControllerTable[128];
                break;
            case ::gig::eg2_ctrl_t::type_velocity:
                eg2controllervalue = MIDIKeyVelocity;
                break;
            case ::gig::eg2_ctrl_t::type_controlchange: // MIDI control change controller
                eg2controllervalue = GetSf2EngineChannel()->ControllerTable[pRegion->EG2Controller.controller_number];
                break;
        }
        if (pRegion->EG2ControllerInvert) eg2controllervalue = 127 - eg2controllervalue;

        return eg2controllervalue;*/ // TODO: ^^^
        return 0;
    }

    Voice::EGInfo Voice::CalculateEG2ControllerInfluence(double eg2ControllerValue) {
        /*EGInfo eg;
        eg.Attack  = (pRegion->EG2ControllerAttackInfluence)  ? 1 + 0.00775 * (double) (1 << pRegion->EG2ControllerAttackInfluence)  * eg2ControllerValue : 1.0;
        eg.Decay   = (pRegion->EG2ControllerDecayInfluence)   ? 1 + 0.00775 * (double) (1 << pRegion->EG2ControllerDecayInfluence)   * eg2ControllerValue : 1.0;
        eg.Release = (pRegion->EG2ControllerReleaseInfluence) ? 1 + 0.00775 * (double) (1 << pRegion->EG2ControllerReleaseInfluence) * eg2ControllerValue : 1.0;

        return eg;*/ // TODO: ^^^
        EGInfo eg;
        eg.Attack = 1.0;
        eg.Decay = 1.0;
        eg.Release = 1.0;
        return eg;
    }

    float Voice::CalculateCutoffBase(uint8_t MIDIKeyVelocity) {
        float cutoff = pRegion->GetInitialFilterFc(pPresetRegion);
        if (MIDIKeyVelocity == 0) return cutoff;

        cutoff *= RTMath::CentsToFreqRatioUnlimited (
            ((127 - MIDIKeyVelocity) / 127.0) * -2400 // 8.4.2 MIDI Note-On Velocity to Filter Cutoff
        );

        return cutoff;
    }

    float Voice::CalculateFinalCutoff(float cutoffBase) {
        /*int cvalue;
        if (VCFCutoffCtrl.controller) {
            cvalue = GetSf2EngineChannel()->ControllerTable[VCFCutoffCtrl.controller];
            if (pRegion->VCFCutoffControllerInvert) cvalue = 127 - cvalue;
            // VCFVelocityScale in this case means Minimum cutoff
            if (cvalue < pRegion->VCFVelocityScale) cvalue = pRegion->VCFVelocityScale;
        }
        else {
            cvalue = pRegion->VCFCutoff;
        }
        float fco = cutoffBase * float(cvalue);
        if (fco > 127.0f) fco = 127.0f;

        return fco;*/ // TODO: ^^^
        return cutoffBase;
    }

    uint8_t Voice::GetVCFCutoffCtrl() {
        /*uint8_t ctrl;
        switch (pRegion->VCFCutoffController) {
            case ::gig::vcf_cutoff_ctrl_modwheel:
                ctrl = 1;
                break;
            case ::gig::vcf_cutoff_ctrl_effect1:
                ctrl = 12;
                break;
            case ::gig::vcf_cutoff_ctrl_effect2:
                ctrl = 13;
                break;
            case ::gig::vcf_cutoff_ctrl_breath:
                ctrl = 2;
                break;
            case ::gig::vcf_cutoff_ctrl_foot:
                ctrl = 4;
                break;
            case ::gig::vcf_cutoff_ctrl_sustainpedal:
                ctrl = 64;
                break;
            case ::gig::vcf_cutoff_ctrl_softpedal:
                ctrl = 67;
                break;
            case ::gig::vcf_cutoff_ctrl_genpurpose7:
                ctrl = 82;
                break;
            case ::gig::vcf_cutoff_ctrl_genpurpose8:
                ctrl = 83;
                break;
            case ::gig::vcf_cutoff_ctrl_aftertouch:
                ctrl = CTRL_TABLE_IDX_AFTERTOUCH;
                break;
            case ::gig::vcf_cutoff_ctrl_none:
            default:
                ctrl = 0;
                break;
        }

        return ctrl;*/ // TODO: ^^^
        return 0;
    }

    uint8_t Voice::GetVCFResonanceCtrl() {
        /*uint8_t ctrl;
        switch (pRegion->VCFResonanceController) {
            case ::gig::vcf_res_ctrl_genpurpose3:
                ctrl = 18;
                break;
            case ::gig::vcf_res_ctrl_genpurpose4:
                ctrl = 19;
                break;
            case ::gig::vcf_res_ctrl_genpurpose5:
                ctrl = 80;
                break;
            case ::gig::vcf_res_ctrl_genpurpose6:
                ctrl = 81;
                break;
            case ::gig::vcf_res_ctrl_none:
            default:
                ctrl = 0;
        }

        return ctrl;*/ // TODO: ^^^
        return 0;
    }

    void Voice::ProcessGroupEvent(RTList<Event>::Iterator& itEvent) {
        if (itEvent->Param.Note.Key != HostKey()) {
            // kill the voice fast
            SignalRack.EnterFadeOutStage();
        }
    }

    void Voice::CalculateFadeOutCoeff(float FadeOutTime, float SampleRate) {
        SignalRack.CalculateFadeOutCoeff(FadeOutTime, SampleRate);
    }

    int Voice::CalculatePan(uint8_t pan) {
        int p = pan + RgnInfo.Pan;

        if (p < 0) return 0;
        if (p > 127) return 127;
        return p;
    }

    release_trigger_t Voice::GetReleaseTriggerFlags() {
        return release_trigger_none;
    }

}} // namespace LinuxSampler::sf2
