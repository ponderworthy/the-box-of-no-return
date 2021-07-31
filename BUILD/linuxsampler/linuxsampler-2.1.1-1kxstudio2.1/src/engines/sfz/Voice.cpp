/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2015 Christian Schoenebeck and Grigor Iliev      *
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

#define LN_10_DIV_20 0.115129254649702

namespace LinuxSampler { namespace sfz {

    typedef LinuxSampler::VoiceBase<EngineChannel, ::sfz::Region, Sample, DiskThread> SfzVoiceBase;

    Voice::Voice(): SfzVoiceBase(&SignalRack), SignalRack(this) {
        pEngine     = NULL;
        bEqSupport = true;
    }

    Voice::~Voice() {

    }

    EngineChannel* Voice::GetSfzEngineChannel() {
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
        si.SampleRate       = pSample->GetSampleRate();
        si.ChannelCount     = pSample->GetChannelCount();
        si.FrameSize        = pSample->GetFrameSize();
        si.BitDepth         = (pSample->GetFrameSize() / pSample->GetChannelCount()) * 8;
        si.TotalFrameCount  = (uint)pSample->GetTotalFrameCount();

        si.HasLoops       = pRegion->HasLoop();
        si.LoopStart      = pRegion->GetLoopStart();
        si.LoopLength     = pRegion->GetLoopEnd() - pRegion->GetLoopStart();
        si.LoopPlayCount  = pRegion->GetLoopCount();
        si.Unpitched      = pRegion->pitch_keytrack == 0;
        return si;
    }

    Voice::RegionInfo Voice::GetRegionInfo() {
        RegionInfo ri;
        ri.UnityNote = pRegion->pitch_keycenter;
        ri.FineTune  = pRegion->tune + pRegion->transpose * 100;
        ri.Pan       = int(pRegion->pan * 0.63); // convert from -100..100 to -64..63
        ri.SampleStartOffset = pRegion->offset ? *(pRegion->offset) : 0;

        ri.VCFEnabled    = pRegion->cutoff;
        switch (pRegion->fil_type) {
        case ::sfz::LPF_1P:
            ri.VCFType = Filter::vcf_type_1p_lowpass;
            break;
        case ::sfz::LPF_2P:
            ri.VCFType = Filter::vcf_type_2p_lowpass;
            break;
        case ::sfz::LPF_4P:
            ri.VCFType = Filter::vcf_type_4p_lowpass;
            break;
        case ::sfz::LPF_6P:
            ri.VCFType = Filter::vcf_type_6p_lowpass;
            break;
        case ::sfz::HPF_1P:
            ri.VCFType = Filter::vcf_type_1p_highpass;
            break;
        case ::sfz::HPF_2P:
            ri.VCFType = Filter::vcf_type_2p_highpass;
            break;
        case ::sfz::HPF_4P:
            ri.VCFType = Filter::vcf_type_4p_highpass;
            break;
        case ::sfz::HPF_6P:
            ri.VCFType = Filter::vcf_type_6p_highpass;
            break;
        case ::sfz::BPF_1P:
        case ::sfz::BPF_2P:
            ri.VCFType = Filter::vcf_type_2p_bandpass;
            break;
        case ::sfz::BRF_1P:
        case ::sfz::BRF_2P:
            ri.VCFType = Filter::vcf_type_2p_bandreject;
            break;
        case ::sfz::APF_1P:
        case ::sfz::PKF_2P:
        default:
            ri.VCFEnabled = false;
            break;
        }

        ri.VCFResonance  = pRegion->resonance;

        // rt_decay is in dB. Precalculate a suitable value for exp in
        // GetReleaseTriggerAttenuation: -ln(10) / 20 * rt_decay
        ri.ReleaseTriggerDecay = -LN_10_DIV_20 * pRegion->rt_decay;

        return ri;
    }

    Voice::InstrumentInfo Voice::GetInstrumentInfo() {
        InstrumentInfo ii;
        ii.FineTune = 0; // TODO: 
        ii.PitchbendRange = 2; // TODO: 

        return ii;
    }

    double Voice::GetSampleAttenuation() {
        return exp(LN_10_DIV_20 * pRegion->volume) * pRegion->amplitude / 100;
    }

    double Voice::GetVelocityAttenuation(uint8_t MIDIKeyVelocity) {
        float offset = -pRegion->amp_veltrack;
        if (offset <= 0) offset += 100;
        return (offset + pRegion->amp_veltrack * pRegion->amp_velcurve[MIDIKeyVelocity]) / 100;
    }

    double Voice::GetVelocityRelease(uint8_t MIDIKeyVelocity) {
        return 127.0 / MIDIKeyVelocity;
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

    double Voice::CalculateCrossfadeVolume(uint8_t MIDIKeyVelocity) {
        /*float crossfadeVolume;
        switch (pRegion->AttenuationController.type) {
            case ::gig::attenuation_ctrl_t::type_channelaftertouch:
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(GetSfzEngineChannel()->ControllerTable[128])];
                break;
            case ::gig::attenuation_ctrl_t::type_velocity:
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(MIDIKeyVelocity)];
                break;
            case ::gig::attenuation_ctrl_t::type_controlchange: //FIXME: currently not sample accurate
                crossfadeVolume = Engine::CrossfadeCurve[CrossfadeAttenuation(GetSfzEngineChannel()->ControllerTable[pRegion->AttenuationController.controller_number])];
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
                eg1controllervalue = GetSfzEngineChannel()->ControllerTable[128];
                break;
            case ::gig::eg1_ctrl_t::type_velocity:
                eg1controllervalue = MIDIKeyVelocity;
                break;
            case ::gig::eg1_ctrl_t::type_controlchange: // MIDI control change controller
                eg1controllervalue = GetSfzEngineChannel()->ControllerTable[pRegion->EG1Controller.controller_number];
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
                eg2controllervalue = GetSfzEngineChannel()->ControllerTable[128];
                break;
            case ::gig::eg2_ctrl_t::type_velocity:
                eg2controllervalue = MIDIKeyVelocity;
                break;
            case ::gig::eg2_ctrl_t::type_controlchange: // MIDI control change controller
                eg2controllervalue = GetSfzEngineChannel()->ControllerTable[pRegion->EG2Controller.controller_number];
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
        float cutoff = *pRegion->cutoff;
        cutoff *= RTMath::CentsToFreqRatioUnlimited(
            MIDIKeyVelocity / 127.0f * pRegion->fil_veltrack +
            (MIDIKey() - pRegion->fil_keycenter) * pRegion->fil_keytrack);
        return cutoff;
    }

    float Voice::CalculateFinalCutoff(float cutoffBase) {
        float cutoff = cutoffBase;
        if (cutoff > 0.49 * pEngine->SampleRate) cutoff = 0.49 * pEngine->SampleRate;
        return cutoff;
    }

    float Voice::GetReleaseTriggerAttenuation(float noteLength) {
        // pow(10, -rt_decay * noteLength / 20):
        return expf(RgnInfo.ReleaseTriggerDecay * noteLength);
    }

    release_trigger_t Voice::GetReleaseTriggerFlags() {
        return release_trigger_noteoff;
    }

    void Voice::ProcessGroupEvent(RTList<Event>::Iterator& itEvent) {
        dmsg(4,("Voice %p processGroupEvents event type=%d", (void*)this, itEvent->Type));
        if (itEvent->Type == Event::type_control_change ||
            (Type & Voice::type_controller_triggered) ||
            itEvent->Param.Note.Key != HostKey()) {
            dmsg(4,("Voice %p - kill", (void*)this));
            if (pRegion->off_mode == ::sfz::OFF_NORMAL) {
                // turn off the voice by entering release envelope stage
                EnterReleaseStage();
            } else {
                // kill the voice fast
                SignalRack.EnterFadeOutStage();
            }
        }
    }

    void Voice::SetSampleStartOffset() {
        AbstractVoice::SetSampleStartOffset();

        if (DiskVoice && Pos > pSample->MaxOffset) {
            // The offset is applied to the RAM buffer
            finalSynthesisParameters.dPos = 0;
            Pos = 0;
        }
    }

    void Voice::CalculateFadeOutCoeff(float FadeOutTime, float SampleRate) {
        SignalRack.CalculateFadeOutCoeff(FadeOutTime, SampleRate);
    }

    int Voice::CalculatePan(uint8_t pan) {
        // the value isn't limited to [0, 127] here, as this is done
        // later in SignalUnit.CalculatePan
        return pan + RgnInfo.Pan;
    }

}} // namespace LinuxSampler::sfz
