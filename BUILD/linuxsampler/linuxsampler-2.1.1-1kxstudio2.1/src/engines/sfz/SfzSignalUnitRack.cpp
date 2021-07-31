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

#include "SfzSignalUnitRack.h"
#include "Engine.h"

#define _200TH_ROOT_OF_10 1.011579454259899

namespace LinuxSampler { namespace sfz {
    
    double ToRatio(int Centibels) {
        if (Centibels == 0) return 1.0;
        return pow(_200TH_ROOT_OF_10, Centibels);
    }
    
    SfzSignalUnit::SfzSignalUnit(SfzSignalUnitRack* rack): SignalUnit(rack), pVoice(rack->pVoice) {
        
    }
    
    double SfzSignalUnit::GetSampleRate() {
        return pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
    }
    
    float SfzSignalUnit::GetInfluence(ArrayList< ::sfz::CC>& cc) {
        float f = 0;
        for (int i = 0; i < cc.size(); i++) {
            int val = pVoice->GetControllerValue(cc[i].Controller);
            f += (val / 127.0f) * cc[i].Influence;
        }
        return f;
    }
    
    void XFInCCUnit::SetCrossFadeCCs(::sfz::Array<int>& loCCs, ::sfz::Array<int>& hiCCs) {
        RemoveAllCCs();
        
        for (int cc = 0; cc < 128; cc++) {
            if (loCCs[cc] == 0 && hiCCs[cc] == 0) continue;
            int i = loCCs[cc];
            int j = hiCCs[cc];
            if (j == 0) j = 127;
            i += j << 8; // workaround to keep both values in the Influence parameter
            AddCC(cc, i);
        }
    }
    
    void XFInCCUnit::Calculate() {
        float l = 1;
                
        RTList<CC>::Iterator ctrl = pCtrls->first();
        RTList<CC>::Iterator end  = pCtrls->end();
        for(; ctrl != end; ++ctrl) {
            float c = 1;
            int influence = (*ctrl).Influence;
            int lo = influence & 0xff;
            int hi = influence >> 8;
            if ((*ctrl).Value <= lo) {
                c = 0;
            } else if ((*ctrl).Value >= hi) {
                c = 1;
            } else {
                float xfVelSize = hi - lo;
                float velPos = (*ctrl).Value - lo;
                c = velPos / xfVelSize;
                if (pVoice->pRegion->xf_cccurve == ::sfz::POWER) {
                    c = sin(c * M_PI / 2.0);
                }
            }
            
            l *= c;
        }
        
        if (Level != l) {
            Level = l;
            if (pListener != NULL) pListener->ValueChanged(this);
        }
    }
    
    
    void XFOutCCUnit::Calculate() {
        float l = 1;
                
        RTList<CC>::Iterator ctrl = pCtrls->first();
        RTList<CC>::Iterator end  = pCtrls->end();
        for(; ctrl != end; ++ctrl) {
            float c = 1;
            int influence = (*ctrl).Influence;
            int lo = influence & 0xff;
            int hi = influence >> 8;
            if ((*ctrl).Value >= hi) {
                c = 0;
            } else if ((*ctrl).Value <= lo) {
                c = 1;
            } else {
                float xfVelSize = hi - lo;
                float velPos = (*ctrl).Value - lo;
                c = 1.0f - velPos / xfVelSize;
                if (pVoice->pRegion->xf_cccurve == ::sfz::POWER) {
                    c = sin(c * M_PI / 2.0);
                }
            }
            
            l *= c;
        }
        
        if (Level != l) {
            Level = l;
            if (pListener != NULL) pListener->ValueChanged(this);
        }
    }
    
    
    EGv2Unit::EGv2Unit(SfzSignalUnitRack* rack)
        : EGUnit< ::LinuxSampler::sfz::EG>(rack), EqUnitSupport(rack), suAmpOnCC(rack), suVolOnCC(rack),
          suPitchOnCC(rack), suCutoffOnCC(rack), suResOnCC(rack), suPanOnCC(rack)
    { }
    
    void EGv2Unit::Trigger() {
        egInfo = *pEGInfo;
        for (int i = 0; i < egInfo.node.size(); i++) {
            float f = GetInfluence(egInfo.node[i].level_oncc);
            egInfo.node[i].level = std::min(egInfo.node[i].level + f, 1.0f);
            
            f = GetInfluence(egInfo.node[i].time_oncc);
            egInfo.node[i].time = std::min(egInfo.node[i].time + f, 100.0f);
        }
        EG.trigger(egInfo, GetSampleRate(), pVoice->MIDIVelocity());
    }
    
    
    void PitchEGUnit::Trigger() {
        ::sfz::Region* const pRegion = pVoice->pRegion;
        depth = pRegion->pitcheg_depth + GetInfluence(pRegion->pitcheg_depth_oncc);
        
        // the length of the decay and release curves are dependent on the velocity
        const double velrelease = 1 / pVoice->GetVelocityRelease(pVoice->MIDIVelocity());

        // set the delay trigger
        float delay = pRegion->pitcheg_delay + pRegion->pitcheg_vel2delay * velrelease;
        delay += GetInfluence(pRegion->pitcheg_delay_oncc);
        uiDelayTrigger = std::max(0.0f, delay) * GetSampleRate();
        
        float start = (pRegion->pitcheg_start + GetInfluence(pRegion->pitcheg_start_oncc)) * 10;
        
        float attack = pRegion->pitcheg_attack + pRegion->pitcheg_vel2attack * velrelease;
        attack = std::max(0.0f, attack + GetInfluence(pRegion->pitcheg_attack_oncc));
        
        float hold = pRegion->pitcheg_hold + pRegion->pitcheg_vel2hold * velrelease;
        hold = std::max(0.0f, hold + GetInfluence(pRegion->pitcheg_hold_oncc));
        
        float decay = pRegion->pitcheg_decay + pRegion->pitcheg_vel2decay * velrelease;
        decay = std::max(0.0f, decay + GetInfluence(pRegion->pitcheg_decay_oncc));
        
        float sustain = pRegion->pitcheg_sustain + pRegion->pitcheg_vel2sustain * velrelease;
        sustain = 10 * (sustain + GetInfluence(pRegion->pitcheg_sustain_oncc));
        
        float release = pRegion->pitcheg_release + pRegion->pitcheg_vel2release * velrelease;
        release = std::max(0.0f, release + GetInfluence(pRegion->pitcheg_release_oncc));
        
        EG.trigger (
            uint(std::min(std::max(0.0f, start), 1000.0f)), attack, hold, decay,
            uint(std::min(std::max(0.0f, sustain), 1000.0f)), release, GetSampleRate(), true
        );
    }
    
    
    void FilEGUnit::Trigger() {
        ::sfz::Region* const pRegion = pVoice->pRegion;
        depth = pRegion->fileg_depth + GetInfluence(pRegion->fileg_depth_oncc);
        
        // the length of the decay and release curves are dependent on the velocity
        const double velrelease = 1 / pVoice->GetVelocityRelease(pVoice->MIDIVelocity());

        // set the delay trigger
        float delay = pRegion->fileg_delay + pRegion->fileg_vel2delay * velrelease;
        delay += GetInfluence(pRegion->fileg_delay_oncc);
        uiDelayTrigger = std::max(0.0f, delay) * GetSampleRate();
        
        float start = (pRegion->fileg_start + GetInfluence(pRegion->fileg_start_oncc)) * 10;
        
        float attack = pRegion->fileg_attack + pRegion->fileg_vel2attack * velrelease;
        attack = std::max(0.0f, attack + GetInfluence(pRegion->fileg_attack_oncc));
        
        float hold = pRegion->fileg_hold + pRegion->fileg_vel2hold * velrelease;
        hold = std::max(0.0f, hold + GetInfluence(pRegion->fileg_hold_oncc));
        
        float decay = pRegion->fileg_decay + pRegion->fileg_vel2decay * velrelease;
        decay = std::max(0.0f, decay + GetInfluence(pRegion->fileg_decay_oncc));
        
        float sustain = pRegion->fileg_sustain + pRegion->fileg_vel2sustain * velrelease;
        sustain = 10 * (sustain + GetInfluence(pRegion->fileg_sustain_oncc));
        
        float release = pRegion->fileg_release + pRegion->fileg_vel2release * velrelease;
        release = std::max(0.0f, release + GetInfluence(pRegion->fileg_release_oncc));
        
        EG.trigger (
            uint(std::min(std::max(0.0f, start), 1000.0f)), attack, hold, decay,
            uint(std::min(std::max(0.0f, sustain), 1000.0f)), release, GetSampleRate(), true
        );
    }
    
    
    void AmpEGUnit::Trigger() {
        ::sfz::Region* const pRegion = pVoice->pRegion;
        
        // the length of the decay and release curves are dependent on the velocity
        const double velrelease = 1 / pVoice->GetVelocityRelease(pVoice->MIDIVelocity());

        // set the delay trigger
        float delay = pRegion->ampeg_delay + pRegion->ampeg_vel2delay * velrelease;
        delay += GetInfluence(pRegion->ampeg_delaycc);
        uiDelayTrigger = std::max(0.0f, delay) * GetSampleRate();
        
        float start = (pRegion->ampeg_start + GetInfluence(pRegion->ampeg_startcc)) * 10;
        
        float attack = pRegion->ampeg_attack + pRegion->ampeg_vel2attack * velrelease;
        attack = std::max(0.0f, attack + GetInfluence(pRegion->ampeg_attackcc));
        
        float hold = pRegion->ampeg_hold + pRegion->ampeg_vel2hold * velrelease;
        hold = std::max(0.0f, hold + GetInfluence(pRegion->ampeg_holdcc));
        
        float decay = pRegion->ampeg_decay + pRegion->ampeg_vel2decay * velrelease;
        decay = std::max(0.0f, decay + GetInfluence(pRegion->ampeg_decaycc));
        
        float sustain = pRegion->ampeg_sustain + pRegion->ampeg_vel2sustain * velrelease;
        sustain = 10 * (sustain + GetInfluence(pRegion->ampeg_sustaincc));
        if (pVoice->pNote)
            sustain *= pVoice->pNote->Override.Sustain;

        float release = pRegion->ampeg_release + pRegion->ampeg_vel2release * velrelease;
        release = std::max(0.0f, release + GetInfluence(pRegion->ampeg_releasecc));
        
        EG.trigger (
            uint(std::min(std::max(0.0f, start), 1000.0f)), attack, hold, decay,
            uint(std::min(std::max(0.0f, sustain), 1000.0f)), release, GetSampleRate(), false
        );
    }
    
    
    LFOUnit::LFOUnit(SfzSignalUnitRack* rack)
        : SfzSignalUnit(rack), pLfoInfo(NULL), pLFO(NULL),
          suFadeEG(rack), suDepthOnCC(rack), suFreqOnCC(rack, this)
    { }
    
    LFOUnit::LFOUnit(const LFOUnit& Unit)
        : SfzSignalUnit(Unit), suFadeEG(static_cast<SfzSignalUnitRack*>(Unit.pRack)),
          suDepthOnCC(static_cast<SfzSignalUnitRack*>(Unit.pRack)),
          suFreqOnCC(static_cast<SfzSignalUnitRack*>(Unit.pRack), this)
    {
        Copy(Unit);
    }

    void LFOUnit::Increment() {
        if (DelayStage()) return;
        
        SignalUnit::Increment();
        
        Level = pLFO->Render();
        if (suFadeEG.Active()) Level *= suFadeEG.GetLevel();
    }
    
    void LFOUnit::Trigger() {
        //reset
        Level = 0;
        
        // set the delay trigger
        uiDelayTrigger = (pLfoInfo->delay + GetInfluence(pLfoInfo->delay_oncc)) * GetSampleRate();
        if(pLfoInfo->fade != 0 || !pLfoInfo->fade_oncc.empty()) {
            float f = pLfoInfo->fade;
            f += GetInfluence(pLfoInfo->fade_oncc);
            
            if (f != 0) {
                suFadeEG.uiDelayTrigger = pLfoInfo->delay * GetSampleRate();
                suFadeEG.EG.trigger(0, f, 0, 0, 1000, 0, GetSampleRate(), false);
            }
        }
    }
    
    void LFOUnit::ValueChanged(CCSignalUnit* pUnit) {
        if (pLFO == NULL) return;
        pLFO->SetFrequency(std::max(0.0f, suFreqOnCC.GetLevel() + pLfoInfo->freq), GetSampleRate());
    }
    
    
    void LFOv1Unit::Trigger() {
        LFOUnit::Trigger();
        
        lfo.trigger (
            pLfoInfo->freq + suFreqOnCC.GetLevel(),
            start_level_mid,
            1, 0, false, GetSampleRate()
        );
        lfo.updateByMIDICtrlValue(0);
    }
    
    
    LFOv2Unit::LFOv2Unit(SfzSignalUnitRack* rack)
        : LFOUnit(rack), EqUnitSupport(rack), lfos(8), lfo0(1200.0f), lfo1(1200.0f), lfo2(1200.0f),
          lfo3(1200.0f), lfo4(1200.0f), lfo5(1200.0f), lfo6(1200.0f), lfo7(1200.0f),
          suVolOnCC(rack), suPitchOnCC(rack), suPanOnCC(rack), suCutoffOnCC(rack), suResOnCC(rack)
    {
        lfos.add(&lfo0);
        lfos.add(&lfo1);
        lfos.add(&lfo2);
        lfos.add(&lfo3);
        lfos.add(&lfo4);
        lfos.add(&lfo5);
        lfos.add(&lfo6);
        lfos.add(&lfo7);
    }
    
    void LFOv2Unit::Trigger() {
        LFOUnit::Trigger();
        
        if (/*pLfoInfo->wave < 0 ||*/ pLfoInfo->wave >= lfos.size()) pLFO = &lfo0;
        else pLFO = lfos[pLfoInfo->wave];
        
        pLFO->Trigger (
            pLfoInfo->freq + suFreqOnCC.GetLevel(),
            start_level_mid,
            1, 0, false, GetSampleRate()
        );
        pLFO->Update(0);
        
        float phase = pLfoInfo->phase + GetInfluence(pLfoInfo->phase_oncc);
        if (phase != 0) pLFO->SetPhase(phase);
    }
    
    void AmpLFOUnit::Trigger() {
        bActive = true;
        ::sfz::Region* const pRegion = pVoice->pRegion;
        pLfoInfo->delay  = pRegion->amplfo_delay + GetInfluence(pRegion->amplfo_delay_oncc);
        pLfoInfo->freq   = pRegion->amplfo_freq;
        pLfoInfo->fade   = pRegion->amplfo_fade + GetInfluence(pRegion->amplfo_fade_oncc);
        pLfoInfo->volume = pRegion->amplfo_depth;
        
        if (pLfoInfo->freq <= 0) {
            if (!pRegion->amplfo_freqcc.empty()) pLfoInfo->freq = 0;
            else bActive = false;
        }
        
        LFOv1Unit::Trigger();
    }
    
    void PitchLFOUnit::Trigger() {
        bActive = true;
        ::sfz::Region* const pRegion = pVoice->pRegion;
        pLfoInfo->delay = pRegion->pitchlfo_delay + GetInfluence(pRegion->pitchlfo_delay_oncc);
        pLfoInfo->freq  = pRegion->pitchlfo_freq;
        pLfoInfo->fade  = pRegion->pitchlfo_fade + GetInfluence(pRegion->pitchlfo_fade_oncc);
        pLfoInfo->pitch = pRegion->pitchlfo_depth;
        
        if (pLfoInfo->freq <= 0) {
            if (!pRegion->pitchlfo_freqcc.empty()) pLfoInfo->freq = 0;
            else bActive = false;
        }
        
        LFOv1Unit::Trigger();
    }
    
    void FilLFOUnit::Trigger() {
        bActive = true;
        ::sfz::Region* const pRegion = pVoice->pRegion;
        pLfoInfo->delay  = pRegion->fillfo_delay + GetInfluence(pRegion->fillfo_delay_oncc);
        pLfoInfo->freq   = pRegion->fillfo_freq;
        pLfoInfo->fade   = pRegion->fillfo_fade + GetInfluence(pRegion->fillfo_fade_oncc);
        pLfoInfo->cutoff = pRegion->fillfo_depth;
        
        if (pLfoInfo->freq <= 0) {
            if (!pRegion->fillfo_freqcc.empty()) pLfoInfo->freq = 0;
            else bActive = false;
        }
        
        LFOv1Unit::Trigger();
    }
    
    CCUnit::CCUnit(SfzSignalUnitRack* rack, Listener* l): CCSignalUnit(rack, l) {
        pVoice = NULL;
    }
    
    void CCUnit::Trigger() {
        RTList<CC>::Iterator ctrl = pCtrls->first();
        RTList<CC>::Iterator end  = pCtrls->end();
        for(; ctrl != end; ++ctrl) {
            (*ctrl).Value = pVoice->GetControllerValue((*ctrl).Controller);
            if ((*ctrl).pSmoother != NULL) {
                if ((*ctrl).Step > 0) {
                    float val = Normalize((*ctrl).Value, (*ctrl).Curve) * (*ctrl).Influence;
                    (*ctrl).pSmoother->setValue( ((int) (val / (*ctrl).Step)) * (*ctrl).Step );
                } else {
                    (*ctrl).pSmoother->setValue((*ctrl).Value);
                }
            }
        }
        CCSignalUnit::Trigger();
    }
    
    void CCUnit::SetCCs(::sfz::Array<int>& cc) {
        RemoveAllCCs();
        for (int i = 0; i < 128; i++) {
            if (cc[i] != 0) AddCC(i, cc[i]);
        }
    }
    
    void CCUnit::SetCCs(::sfz::Array<float>& cc) {
        RemoveAllCCs();
        for (int i = 0; i < 128; i++) {
            if (cc[i] != 0) AddCC(i, cc[i]);
        }
    }
    
    void CCUnit::SetCCs(ArrayList< ::sfz::CC>& cc) {
        RemoveAllCCs();
        for (int i = 0; i < cc.size(); i++) {
            if (cc[i].Influence != 0) {
                short int curve = cc[i].Curve;
                if (curve >= GetCurveCount()) curve = -1;
                AddSmoothCC(cc[i].Controller, cc[i].Influence, curve, cc[i].Smooth, cc[i].Step);
            }
        }
    }
     
    void CCUnit::AddSmoothCC(uint8_t Controller, float Influence, short int Curve, float Smooth, float Step) {
        AddCC(Controller, Influence, Curve, NULL, Step);
    }
     
    int CCUnit::GetCurveCount() {
        return pVoice->pRegion->GetInstrument()->curves.size();
    }
     
    ::sfz::Curve* CCUnit::GetCurve(int idx) { 
        return &pVoice->pRegion->GetInstrument()->curves[idx];
    }
     
    double CCUnit::GetSampleRate() {
        return pVoice->GetSampleRate() / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
    }
    
    
    SmoothCCUnit::~SmoothCCUnit() {
        if (pSmoothers != NULL) delete pSmoothers;
    }
     
    void SmoothCCUnit::AddSmoothCC(uint8_t Controller, float Influence, short int Curve, float Smooth, float Step) {
        if (Smooth > 0) {
            if (pSmoothers->poolIsEmpty()) {
                std::cerr << "Maximum number of smoothers reached" << std::endl;
                return;
            }
            Smoother* smoother = &(*(pSmoothers->allocAppend()));
            smoother->trigger(Smooth / 1000.0f, GetSampleRate());
            AddCC(Controller, Influence, Curve, smoother, Step);
        } else {
            AddCC(Controller, Influence, Curve, NULL, Step);
        }
    }
     
    void SmoothCCUnit::InitSmoothers(Pool<Smoother>* pSmootherPool) {
        if (pSmoothers != NULL) delete pSmoothers;
        pSmoothers = new RTList<Smoother>(pSmootherPool);
    }
    
    void SmoothCCUnit::InitCCList(Pool<CC>* pCCPool, Pool<Smoother>* pSmootherPool) {
        CurveCCUnit::InitCCList(pCCPool, pSmootherPool);
        InitSmoothers(pSmootherPool);
    }


    EndpointUnit::EndpointUnit(SfzSignalUnitRack* rack)
        : EndpointSignalUnit(rack), pitchVeltrackRatio(0), suXFInCC(rack), suXFOutCC(rack), suPanOnCC(rack)
    {
        
    }
    
    float EndpointUnit::GetInfluence(::sfz::Array< optional<float> >& cc) {
        float f = 0;
        for (int i = 0; i < 128; i++) {
            if (cc[i]) {
                f += (pVoice->GetControllerValue(i) / 127.0f) * (*cc[i]);
            }
        }
        return f;
    }
    
    float EndpointUnit::GetInfluence(::sfz::Array< optional<int> >& cc) {
        float f = 0;
        for (int i = 0; i < 128; i++) {
            if (cc[i]) {
                f += (pVoice->GetControllerValue(i) / 127.0f) * (*cc[i]);
            }
        }
        return f;
    }
    
    SfzSignalUnitRack* const EndpointUnit::GetRack() {
        return static_cast<SfzSignalUnitRack* const>(pRack);
    }
    
    void EndpointUnit::Trigger() {
        uiDelayTrigger = (uint)GetInfluence(pVoice->pRegion->delay_samples_oncc);
        if (pVoice->pRegion->delay_samples) uiDelayTrigger += *pVoice->pRegion->delay_samples;
        
        if (pVoice->pRegion->delay) {
            /* here we use the device sample rate */
            uiDelayTrigger += (uint)( (*pVoice->pRegion->delay) * pVoice->GetSampleRate() );
        }
        
        if (pVoice->pRegion->delay_random) {
            float r = pVoice->GetEngine()->Random();
            uiDelayTrigger += (uint)( r * (*pVoice->pRegion->delay_random) * pVoice->GetSampleRate() );
        }
        
        uiDelayTrigger += (uint)(GetInfluence(pVoice->pRegion->delay_oncc) * pVoice->GetSampleRate());
        
        float xfInVelCoeff = 1;
        
        if (pVoice->MIDIVelocity() <= pVoice->pRegion->xfin_lovel) {
            xfInVelCoeff = 0;
        } else if (pVoice->MIDIVelocity() >= pVoice->pRegion->xfin_hivel) {
            xfInVelCoeff = 1;
        } else {
            float xfVelSize = pVoice->pRegion->xfin_hivel - pVoice->pRegion->xfin_lovel;
            float velPos = pVoice->MIDIVelocity() - pVoice->pRegion->xfin_lovel;
            xfInVelCoeff = velPos / xfVelSize;
            if (pVoice->pRegion->xf_velcurve == ::sfz::POWER) {
                xfInVelCoeff = sin(xfInVelCoeff * M_PI / 2.0);
            }
        }
        
        float xfOutVelCoeff = 1;
        
        if (pVoice->MIDIVelocity() >= pVoice->pRegion->xfout_hivel) {
            if (pVoice->pRegion->xfout_lovel < 127 /* is set */) xfOutVelCoeff = 0;
        } else if (pVoice->MIDIVelocity() <= pVoice->pRegion->xfout_lovel) {
            xfOutVelCoeff = 1;
        } else {
            float xfVelSize = pVoice->pRegion->xfout_hivel - pVoice->pRegion->xfout_lovel;
            float velPos = pVoice->MIDIVelocity() - pVoice->pRegion->xfout_lovel;
            xfOutVelCoeff = 1.0f - velPos / xfVelSize;
            if (pVoice->pRegion->xf_velcurve == ::sfz::POWER) {
                xfOutVelCoeff = sin(xfOutVelCoeff * M_PI / 2.0);
            }
        }
        
        float xfInKeyCoeff = 1;
        
        if (pVoice->MIDIKey() <= pVoice->pRegion->xfin_lokey) {
            if (pVoice->pRegion->xfin_hikey > 0 /* is set */) xfInKeyCoeff = 0;
        } else if (pVoice->MIDIKey() >= pVoice->pRegion->xfin_hikey) {
            xfInKeyCoeff = 1;
        } else {
            float xfKeySize = pVoice->pRegion->xfin_hikey - pVoice->pRegion->xfin_lokey;
            float keyPos = pVoice->MIDIKey() - pVoice->pRegion->xfin_lokey;
            xfInKeyCoeff = keyPos / xfKeySize;
            if (pVoice->pRegion->xf_keycurve == ::sfz::POWER) {
                xfInKeyCoeff = sin(xfInKeyCoeff * M_PI / 2.0);
            }
        }
        
        float xfOutKeyCoeff = 1;
        
        if (pVoice->MIDIKey() >= pVoice->pRegion->xfout_hikey) {
            if (pVoice->pRegion->xfout_lokey < 127 /* is set */) xfOutKeyCoeff = 0;
        } else if (pVoice->MIDIKey() <= pVoice->pRegion->xfout_lokey) {
            xfOutKeyCoeff = 1;
        } else {
            float xfKeySize = pVoice->pRegion->xfout_hikey - pVoice->pRegion->xfout_lokey;
            float keyPos = pVoice->MIDIKey() - pVoice->pRegion->xfout_lokey;
            xfOutKeyCoeff = 1.0f - keyPos / xfKeySize;
            if (pVoice->pRegion->xf_keycurve == ::sfz::POWER) {
                xfOutKeyCoeff = sin(xfOutKeyCoeff * M_PI / 2.0);
            }
        }
        
        xfCoeff = xfInVelCoeff * xfOutVelCoeff * xfInKeyCoeff * xfOutKeyCoeff;
        
        suXFInCC.SetCrossFadeCCs(pVoice->pRegion->xfin_locc, pVoice->pRegion->xfin_hicc);
        suXFOutCC.SetCrossFadeCCs(pVoice->pRegion->xfout_locc, pVoice->pRegion->xfout_hicc);
        
        suPanOnCC.SetCCs(pVoice->pRegion->pan_oncc);
        
        pitchVeltrackRatio = RTMath::CentsToFreqRatioUnlimited((pVoice->MIDIVelocity() / 127.0f) * pVoice->pRegion->pitch_veltrack);
    }
    
    bool EndpointUnit::Active() {
        if (pRack->isReleaseStageEntered() && uiDelayTrigger) {
            return false; // The key was released before the delay end, so the voice won't play at all.
        }
        
        if (GetRack()->suVolEG.Active()) return true;
        
        bool b = false;
        for (int i = 0; i < GetRack()->volEGs.size(); i++) {
            if (GetRack()->volEGs[i]->Active()) { b = true; break; }
        }
        
        return b;
    }
    
    float EndpointUnit::GetVolume() {
        float vol = GetRack()->suVolEG.Active() ? GetRack()->suVolEG.GetLevel() : 0;
        
        for (int i = 0; i < GetRack()->volEGs.size(); i++) {
            EGv2Unit* eg = GetRack()->volEGs[i];
            if (!eg->Active()) continue;
            
            float dB = eg->suVolOnCC.Active() ? eg->suVolOnCC.GetLevel() : -200;
            if (dB < -144) dB = eg->pEGInfo->volume;
            else if (eg->pEGInfo->volume >= -144) dB += eg->pEGInfo->volume;
            
            float amp = eg->suAmpOnCC.Active() ? eg->suAmpOnCC.GetLevel() : 0;
            amp = (amp + eg->pEGInfo->amplitude) / 100.0f;
            
            if (dB >= -144) {
                if (amp == 0 && eg->suAmpOnCC.GetCCCount() == 0) amp = 1.0f;
                amp *= ToRatio(dB * 10.0);
            }
            
            vol += amp * eg->GetLevel();
        }
        
        AmpLFOUnit* u = &(GetRack()->suAmpLFO);
        CCSignalUnit* u2 = &(GetRack()->suAmpLFO.suDepthOnCC);
        float f = u2->Active() ? u2->GetLevel() : 0;
        vol *= u->Active() ? ToRatio((u->GetLevel() * (u->pLfoInfo->volume + f) * 10.0)) : 1;
        
        vol *= ToRatio(GetRack()->suVolOnCC.GetLevel() * 10.0);
        
        for (int i = 0; i < GetRack()->volLFOs.size(); i++) {
            LFOv2Unit* lfo = GetRack()->volLFOs[i];
            if (!lfo->Active()) continue;
            
            float f = lfo->suVolOnCC.Active() ? lfo->suVolOnCC.GetLevel() : 0;
            vol *= ToRatio(lfo->GetLevel() * (lfo->pLfoInfo->volume + f) * 10.0);
        }
        
        if (suXFInCC.Active())  vol *= suXFInCC.GetLevel();
        if (suXFOutCC.Active()) vol *= suXFOutCC.GetLevel();
        return vol * xfCoeff;
    }
    
    float EndpointUnit::GetFilterCutoff() {
        float val = GetRack()->suCutoffOnCC.Active() ? RTMath::CentsToFreqRatioUnlimited(GetRack()->suCutoffOnCC.GetLevel()) : 1;
        
        FilLFOUnit* u = &(GetRack()->suFilLFO);
        CCSignalUnit* u1 = &(GetRack()->suFilLFO.suDepthOnCC);
        float f = u1->Active() ? u1->GetLevel() : 0;
        val *= u->Active() ? RTMath::CentsToFreqRatioUnlimited(u->GetLevel() * (u->pLfoInfo->cutoff + f)) : 1;
        
        FilEGUnit* u2 = &(GetRack()->suFilEG);
        val *= u2->Active() ? RTMath::CentsToFreqRatioUnlimited(u2->GetLevel() * u2->depth) : 1;
        
        for (int i = 0; i < GetRack()->filEGs.size(); i++) {
            EGv2Unit* eg = GetRack()->filEGs[i];
            if (!eg->Active()) continue;
            
            float f = eg->suCutoffOnCC.Active() ? eg->suCutoffOnCC.GetLevel() : 0;
            f = eg->GetLevel() * (eg->pEGInfo->cutoff + f);
            val *= RTMath::CentsToFreqRatioUnlimited(f);
        }
        
        for (int i = 0; i < GetRack()->filLFOs.size(); i++) {
            LFOv2Unit* lfo = GetRack()->filLFOs[i];
            if (!lfo->Active()) continue;
            
            float f = lfo->suCutoffOnCC.Active() ? lfo->suCutoffOnCC.GetLevel() : 0;
            f = lfo->GetLevel() * (lfo->pLfoInfo->cutoff + f);
            val *= RTMath::CentsToFreqRatioUnlimited(f);
        }
        
        return val;
    }
    
    float EndpointUnit::CalculateFilterCutoff(float cutoff) {
        cutoff *= GetFilterCutoff();
        float maxCutoff = 0.49 * pVoice->GetSampleRate();
        return cutoff > maxCutoff ? maxCutoff : cutoff;
    }
    
    float EndpointUnit::GetPitch() {
        double p = GetRack()->suPitchOnCC.Active() ? RTMath::CentsToFreqRatioUnlimited(GetRack()->suPitchOnCC.GetLevel()) : 1;
        
        EGv1Unit* u = &(GetRack()->suPitchEG);
        p *= u->Active() ? RTMath::CentsToFreqRatioUnlimited(u->GetLevel() * u->depth) : 1;
        
        for (int i = 0; i < GetRack()->pitchEGs.size(); i++) {
            EGv2Unit* eg = GetRack()->pitchEGs[i];
            if (!eg->Active()) continue;
            
            float f = eg->suPitchOnCC.Active() ? eg->suPitchOnCC.GetLevel() : 0;
            p *= RTMath::CentsToFreqRatioUnlimited(eg->GetLevel() * (eg->pEGInfo->pitch + f));
        }
        
        PitchLFOUnit* u2 = &(GetRack()->suPitchLFO);
        CCSignalUnit* u3 = &(GetRack()->suPitchLFO.suDepthOnCC);
        float f = u3->Active() ? u3->GetLevel() : 0;
        p *= u2->Active() ? RTMath::CentsToFreqRatioUnlimited(u2->GetLevel() * (u2->pLfoInfo->pitch + f)) : 1;
        
        for (int i = 0; i < GetRack()->pitchLFOs.size(); i++) {
            LFOv2Unit* lfo = GetRack()->pitchLFOs[i];
            if (!lfo->Active()) continue;
            
            float f = lfo->suPitchOnCC.Active() ? lfo->suPitchOnCC.GetLevel() : 0;
            p *= RTMath::CentsToFreqRatioUnlimited(lfo->GetLevel() * (lfo->pLfoInfo->pitch + f));
        }
        
        return p * pitchVeltrackRatio;
    }
    
    float EndpointUnit::GetResonance() {
         float val = GetRack()->suResOnCC.Active() ? GetRack()->suResOnCC.GetLevel() : 0;
        
        for (int i = 0; i < GetRack()->resEGs.size(); i++) {
            EGv2Unit* eg = GetRack()->resEGs[i];
            if (!eg->Active()) continue;
            
            float f = eg->suResOnCC.Active() ? eg->suResOnCC.GetLevel() : 0;
            val += eg->GetLevel() * (eg->pEGInfo->resonance + f);
        }
        
        for (int i = 0; i < GetRack()->resLFOs.size(); i++) {
            LFOv2Unit* lfo = GetRack()->resLFOs[i];
            if (!lfo->Active()) continue;
            
            float f = lfo->suResOnCC.Active() ? lfo->suResOnCC.GetLevel() : 0;
            val += lfo->GetLevel() * (lfo->pLfoInfo->resonance + f);
        }
        
        return val;
    }
    
    float EndpointUnit::GetPan() {
        float pan = suPanOnCC.Active() ? suPanOnCC.GetLevel() : 0;
        
        for (int i = 0; i < GetRack()->panEGs.size(); i++) {
            EGv2Unit* eg = GetRack()->panEGs[i];
            if (!eg->Active()) continue;
            
            float f = eg->suPanOnCC.Active() ? eg->suPanOnCC.GetLevel() : 0;
            
            if (eg->pEGInfo->pan_curve >= 0 && eg->pEGInfo->pan_curve < suPanOnCC.GetCurveCount()) {
                uint8_t val = eg->GetLevel() * 127;
                if (val > 127) val = 127;
                pan += eg->pEGInfo->pan * suPanOnCC.GetCurve(eg->pEGInfo->pan_curve)->v[val] +  eg->GetLevel() * f;
            } else {
                pan += eg->GetLevel() * (eg->pEGInfo->pan + f);
            }
        }
        
        for (int i = 0; i < GetRack()->panLFOs.size(); i++) {
            LFOv2Unit* lfo = GetRack()->panLFOs[i];
            if (!lfo->Active()) continue;
            
            float f = lfo->suPanOnCC.Active() ? lfo->suPanOnCC.GetLevel() : 0;
            pan += lfo->GetLevel() * (lfo->pLfoInfo->pan + f);
        }
        
        return pan;
    }
    
    
    SfzSignalUnitRack::SfzSignalUnitRack(Voice* voice)
        : SignalUnitRack(MaxUnitCount), EqUnitSupport(this, voice),
        suEndpoint(this), suVolEG(this), suFilEG(this), suPitchEG(this),
        suAmpLFO(this), suPitchLFO(this), suFilLFO(this),
        suVolOnCC(this), suPitchOnCC(this), suCutoffOnCC(this), suResOnCC(this),
        EGs(maxEgCount), volEGs(maxEgCount), pitchEGs(maxEgCount), filEGs(maxEgCount),
        resEGs(maxEgCount), panEGs(maxEgCount), eqEGs(maxEgCount),
        LFOs(maxLfoCount), volLFOs(maxLfoCount), pitchLFOs(maxLfoCount),
        filLFOs(maxLfoCount), resLFOs(maxLfoCount), panLFOs(maxLfoCount), eqLFOs(maxLfoCount),
        pVoice(voice)
    {
        suEndpoint.pVoice = suEndpoint.suXFInCC.pVoice = suEndpoint.suXFOutCC.pVoice = suEndpoint.suPanOnCC.pVoice = voice;
        suVolEG.pVoice = suFilEG.pVoice = suPitchEG.pVoice = voice;
        suAmpLFO.pVoice = suPitchLFO.pVoice = suFilLFO.pVoice = voice;
        
        suVolOnCC.pVoice = suPitchOnCC.pVoice = suCutoffOnCC.pVoice = suResOnCC.pVoice = voice;
        suPitchLFO.suDepthOnCC.pVoice = suPitchLFO.suFadeEG.pVoice = suPitchLFO.suFreqOnCC.pVoice = voice;
        suFilLFO.suFadeEG.pVoice = suFilLFO.suDepthOnCC.pVoice = suFilLFO.suFreqOnCC.pVoice = voice;
        suAmpLFO.suFadeEG.pVoice = suAmpLFO.suDepthOnCC.pVoice = suAmpLFO.suFreqOnCC.pVoice = voice;
        
        for (int i = 0; i < EGs.capacity(); i++) {
            EGs[i] = new EGv2Unit(this);
            EGs[i]->pVoice = voice;
            EGs[i]->suAmpOnCC.pVoice = voice;
            EGs[i]->suVolOnCC.pVoice = voice;
            EGs[i]->suPitchOnCC.pVoice = voice;
            EGs[i]->suCutoffOnCC.pVoice = voice;
            EGs[i]->suResOnCC.pVoice = voice;
            EGs[i]->suPanOnCC.pVoice = voice;
            EGs[i]->SetVoice(voice); // class EqUnitSupport
        }
        
        for (int i = 0; i < LFOs.capacity(); i++) {
            LFOs[i] = new LFOv2Unit(this);
            LFOs[i]->pVoice = voice;
            LFOs[i]->suDepthOnCC.pVoice = voice;
            LFOs[i]->suFreqOnCC.pVoice = voice;
            LFOs[i]->suFadeEG.pVoice = voice;
            LFOs[i]->suVolOnCC.pVoice = voice;
            LFOs[i]->suPitchOnCC.pVoice = voice;
            LFOs[i]->suFreqOnCC.pVoice = voice;
            LFOs[i]->suPanOnCC.pVoice = voice;
            LFOs[i]->suCutoffOnCC.pVoice = voice;
            LFOs[i]->suResOnCC.pVoice = voice;
            LFOs[i]->SetVoice(voice); // class EqUnitSupport
        }
    }
    
    SfzSignalUnitRack::~SfzSignalUnitRack() {
        for (int i = 0; i < EGs.capacity(); i++) {
            delete EGs[i]; EGs[i] = NULL;
        }
        
        for (int i = 0; i < LFOs.capacity(); i++) {
            delete LFOs[i]; LFOs[i] = NULL;
        }
    }
    
    void SfzSignalUnitRack::InitRTLists() {
        Pool<CCSignalUnit::CC>* pCCPool = pVoice->pEngine->pCCPool;
        Pool<Smoother>* pSmootherPool = pVoice->pEngine->pSmootherPool;
        
        EqUnitSupport::InitCCLists(pCCPool, pSmootherPool);
        
        suVolOnCC.InitCCList(pCCPool, pSmootherPool);
        suPitchOnCC.InitCCList(pCCPool, pSmootherPool);
        suCutoffOnCC.InitCCList(pCCPool, pSmootherPool);
        suResOnCC.InitCCList(pCCPool, pSmootherPool);
        suEndpoint.suXFInCC.InitCCList(pCCPool, pSmootherPool);
        suEndpoint.suXFOutCC.InitCCList(pCCPool, pSmootherPool);
        suEndpoint.suPanOnCC.InitCCList(pCCPool, pSmootherPool);
        suPitchLFO.suDepthOnCC.InitCCList(pCCPool, pSmootherPool);
        suPitchLFO.suFreqOnCC.InitCCList(pCCPool, pSmootherPool);
        suFilLFO.suDepthOnCC.InitCCList(pCCPool, pSmootherPool);
        suFilLFO.suFreqOnCC.InitCCList(pCCPool, pSmootherPool);
        suAmpLFO.suDepthOnCC.InitCCList(pCCPool, pSmootherPool);
        suAmpLFO.suFreqOnCC.InitCCList(pCCPool, pSmootherPool);
        
        for (int i = 0; i < EGs.capacity(); i++) {
            EGs[i]->suAmpOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->suVolOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->suPitchOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->suCutoffOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->suResOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->suPanOnCC.InitCCList(pCCPool, pSmootherPool);
            EGs[i]->InitCCLists(pCCPool, pSmootherPool); // class EqUnitSupport
        }
        
        for (int i = 0; i < LFOs.capacity(); i++) {
            LFOs[i]->suDepthOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suFreqOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suVolOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suPitchOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suFreqOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suPanOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suCutoffOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->suResOnCC.InitCCList(pCCPool, pSmootherPool);
            LFOs[i]->InitCCLists(pCCPool, pSmootherPool); // class EqUnitSupport
        }
    }
    
    void SfzSignalUnitRack::Trigger() {
        EGs.clear();
        volEGs.clear();
        pitchEGs.clear();
        filEGs.clear();
        resEGs.clear();
        panEGs.clear();
        eqEGs.clear();
        
        LFOs.clear();
        volLFOs.clear();
        pitchLFOs.clear();
        filLFOs.clear();
        resLFOs.clear();
        panLFOs.clear();
        eqLFOs.clear();
        
        ::sfz::Region* const pRegion = pVoice->pRegion;
        
        suVolOnCC.SetCCs(pRegion->volume_oncc);
        suPitchOnCC.SetCCs(pRegion->pitch_oncc);
        suCutoffOnCC.SetCCs(pRegion->cutoff_oncc);
        suResOnCC.SetCCs(pRegion->resonance_oncc);
        
        for (int i = 0; i < pRegion->eg.size(); i++) {
            if (pRegion->eg[i].node.size() == 0) continue;
            
            if(EGs.size() < EGs.capacity()) {
                EGv2Unit eg(this);
                eg.pEGInfo = &(pRegion->eg[i]);
                EGs.increment()->Copy(eg);
                EGs[EGs.size() - 1]->suAmpOnCC.SetCCs(pRegion->eg[i].amplitude_oncc);
                EGs[EGs.size() - 1]->suVolOnCC.SetCCs(pRegion->eg[i].volume_oncc);
                EGs[EGs.size() - 1]->suPitchOnCC.SetCCs(pRegion->eg[i].pitch_oncc);
                EGs[EGs.size() - 1]->suCutoffOnCC.SetCCs(pRegion->eg[i].cutoff_oncc);
                EGs[EGs.size() - 1]->suResOnCC.SetCCs(pRegion->eg[i].resonance_oncc);
                EGs[EGs.size() - 1]->suPanOnCC.SetCCs(pRegion->eg[i].pan_oncc);
                if (pVoice->bEqSupport) {
                    EGs[EGs.size() - 1]->suEq1FreqOnCC.SetCCs(pRegion->eg[i].eq1freq_oncc);
                    EGs[EGs.size() - 1]->suEq2FreqOnCC.SetCCs(pRegion->eg[i].eq2freq_oncc);
                    EGs[EGs.size() - 1]->suEq3FreqOnCC.SetCCs(pRegion->eg[i].eq3freq_oncc);
                    EGs[EGs.size() - 1]->suEq1GainOnCC.SetCCs(pRegion->eg[i].eq1gain_oncc);
                    EGs[EGs.size() - 1]->suEq2GainOnCC.SetCCs(pRegion->eg[i].eq2gain_oncc);
                    EGs[EGs.size() - 1]->suEq3GainOnCC.SetCCs(pRegion->eg[i].eq3gain_oncc);
                    EGs[EGs.size() - 1]->suEq1BwOnCC.SetCCs(pRegion->eg[i].eq1bw_oncc);
                    EGs[EGs.size() - 1]->suEq2BwOnCC.SetCCs(pRegion->eg[i].eq2bw_oncc);
                    EGs[EGs.size() - 1]->suEq3BwOnCC.SetCCs(pRegion->eg[i].eq3bw_oncc);
                }
            } else { std::cerr << "Maximum number of EGs reached!" << std::endl; break; }
            
            if ( pRegion->eg[i].amplitude > 0 || !pRegion->eg[i].amplitude_oncc.empty() ||
                 pRegion->eg[i].volume > -145 || !pRegion->eg[i].volume_oncc.empty()
            ) {
                if(volEGs.size() < volEGs.capacity()) volEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
            
            if (pRegion->eg[i].cutoff != 0 || !pRegion->eg[i].cutoff_oncc.empty()) {
                if(filEGs.size() < filEGs.capacity()) filEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
            
            if (pRegion->eg[i].resonance != 0 || !pRegion->eg[i].resonance_oncc.empty()) {
                if(resEGs.size() < resEGs.capacity()) resEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
            
            if (pRegion->eg[i].pitch != 0 || !pRegion->eg[i].pitch_oncc.empty()) {
                if(pitchEGs.size() < pitchEGs.capacity()) pitchEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
            
            if (pRegion->eg[i].pan != 0 || !pRegion->eg[i].pan_oncc.empty()) {
                if(panEGs.size() < panEGs.capacity()) panEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
            
            if (pRegion->eg[i].HasEq()) {
                if(eqEGs.size() < eqEGs.capacity()) eqEGs.add(EGs[EGs.size() - 1]);
                else std::cerr << "Maximum number of EGs reached!" << std::endl;
            }
        }
        
        if (pRegion->ampeg_sustain == -1) {
            if (volEGs.size() > 0) pRegion->ampeg_sustain = 0;
            else pRegion->ampeg_sustain = 100;
        }
        
        // LFO
        for (int i = 0; i < pRegion->lfos.size(); i++) {
            if (pRegion->lfos[i].freq <= 0) {
                if (pRegion->lfos[i].freq_oncc.empty()) continue; // Not initialized
                else pRegion->lfos[i].freq = 0;
            }
            
            if(LFOs.size() < LFOs.capacity()) {
                LFOv2Unit lfo(this);
                lfo.pLfoInfo = &(pRegion->lfos[i]);
                LFOs.increment()->Copy(lfo);
                LFOs[LFOs.size() - 1]->suVolOnCC.SetCCs(pRegion->lfos[i].volume_oncc);
                LFOs[LFOs.size() - 1]->suPitchOnCC.SetCCs(pRegion->lfos[i].pitch_oncc);
                LFOs[LFOs.size() - 1]->suFreqOnCC.SetCCs(pRegion->lfos[i].freq_oncc);
                LFOs[LFOs.size() - 1]->suPanOnCC.SetCCs(pRegion->lfos[i].pan_oncc);
                LFOs[LFOs.size() - 1]->suCutoffOnCC.SetCCs(pRegion->lfos[i].cutoff_oncc);
                LFOs[LFOs.size() - 1]->suResOnCC.SetCCs(pRegion->lfos[i].resonance_oncc);
                if (pVoice->bEqSupport) {
                    LFOs[LFOs.size() - 1]->suEq1FreqOnCC.SetCCs(pRegion->lfos[i].eq1freq_oncc);
                    LFOs[LFOs.size() - 1]->suEq2FreqOnCC.SetCCs(pRegion->lfos[i].eq2freq_oncc);
                    LFOs[LFOs.size() - 1]->suEq3FreqOnCC.SetCCs(pRegion->lfos[i].eq3freq_oncc);
                    LFOs[LFOs.size() - 1]->suEq1GainOnCC.SetCCs(pRegion->lfos[i].eq1gain_oncc);
                    LFOs[LFOs.size() - 1]->suEq2GainOnCC.SetCCs(pRegion->lfos[i].eq2gain_oncc);
                    LFOs[LFOs.size() - 1]->suEq3GainOnCC.SetCCs(pRegion->lfos[i].eq3gain_oncc);
                    LFOs[LFOs.size() - 1]->suEq1BwOnCC.SetCCs(pRegion->lfos[i].eq1bw_oncc);
                    LFOs[LFOs.size() - 1]->suEq2BwOnCC.SetCCs(pRegion->lfos[i].eq2bw_oncc);
                    LFOs[LFOs.size() - 1]->suEq3BwOnCC.SetCCs(pRegion->lfos[i].eq3bw_oncc);
                }
            } else { std::cerr << "Maximum number of LFOs reached!" << std::endl; break; }
            
            if (pRegion->lfos[i].volume != 0 || !pRegion->lfos[i].volume_oncc.empty()) {
                if(volLFOs.size() < volLFOs.capacity()) volLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
            
            if (pRegion->lfos[i].pitch != 0 || !pRegion->lfos[i].pitch_oncc.empty()) {
                if(pitchLFOs.size() < pitchLFOs.capacity()) pitchLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
            
            if (pRegion->lfos[i].cutoff != 0 || !pRegion->lfos[i].cutoff_oncc.empty()) {
                if(filLFOs.size() < filLFOs.capacity()) filLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
            
            if (pRegion->lfos[i].resonance != 0 || !pRegion->lfos[i].resonance_oncc.empty()) {
                if(resLFOs.size() < resLFOs.capacity()) resLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
            
            if (pRegion->lfos[i].pan != 0 || !pRegion->lfos[i].pan_oncc.empty()) {
                if(panLFOs.size() < panLFOs.capacity()) panLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
            
            if (pRegion->lfos[i].HasEq()) {
                if(eqLFOs.size() < eqLFOs.capacity()) eqLFOs.add(LFOs[LFOs.size() - 1]);
                else std::cerr << "Maximum number of LFOs reached!" << std::endl;
            }
        }
        
        if (!pVoice->bEqSupport) {
            bHasEq = false;
        } else {
            suEq1GainOnCC.SetCCs(pRegion->eq1_gain_oncc);
            suEq2GainOnCC.SetCCs(pRegion->eq2_gain_oncc);
            suEq3GainOnCC.SetCCs(pRegion->eq3_gain_oncc);
            suEq1FreqOnCC.SetCCs(pRegion->eq1_freq_oncc);
            suEq2FreqOnCC.SetCCs(pRegion->eq2_freq_oncc);
            suEq3FreqOnCC.SetCCs(pRegion->eq3_freq_oncc);
            suEq1BwOnCC.SetCCs(pRegion->eq1_bw_oncc);
            suEq2BwOnCC.SetCCs(pRegion->eq2_bw_oncc);
            suEq3BwOnCC.SetCCs(pRegion->eq3_bw_oncc);
        
            bHasEq = pRegion->eq1_gain || pRegion->eq2_gain || pRegion->eq3_gain ||
                     pRegion->eq1_vel2gain || pRegion->eq2_vel2gain || pRegion->eq3_vel2gain ||
                     suEq1GainOnCC.HasCCs() || suEq2GainOnCC.HasCCs() || suEq3GainOnCC.HasCCs() ||
                     eqEGs.size() > 0 || eqLFOs.size() > 0;
        }
        
        suPitchLFO.suDepthOnCC.SetCCs(pRegion->pitchlfo_depthcc);
        suPitchLFO.suFreqOnCC.SetCCs(pRegion->pitchlfo_freqcc);
        
        suFilLFO.suDepthOnCC.SetCCs(pRegion->fillfo_depthcc);
        suFilLFO.suFreqOnCC.SetCCs(pRegion->fillfo_freqcc);
        
        suAmpLFO.suDepthOnCC.SetCCs(pRegion->amplfo_depthcc);
        suAmpLFO.suFreqOnCC.SetCCs(pRegion->amplfo_freqcc);
        
        Units.clear();
        
        EqUnitSupport::ImportUnits(this);
        
        Units.add(&suVolOnCC);
        Units.add(&suPitchOnCC);
        Units.add(&suCutoffOnCC);
        Units.add(&suResOnCC);
        
        Units.add(&suVolEG);
        Units.add(&suFilEG);
        Units.add(&suPitchEG);
        
        Units.add(&suPitchLFO.suFreqOnCC); // Don't change order! (should be triggered before the LFO)
        Units.add(&suPitchLFO);
        Units.add(&suPitchLFO.suDepthOnCC);
        Units.add(&suPitchLFO.suFadeEG);
        
        Units.add(&suAmpLFO.suFreqOnCC); // Don't change order! (should be triggered before the LFO)
        Units.add(&suAmpLFO.suDepthOnCC);
        Units.add(&suAmpLFO);
        Units.add(&suAmpLFO.suFadeEG);
        
        Units.add(&suFilLFO.suFreqOnCC); // Don't change order! (should be triggered before the LFO)
        Units.add(&suFilLFO.suDepthOnCC);
        Units.add(&suFilLFO);
        Units.add(&suFilLFO.suFadeEG);
        
        for (int i = 0; i < EGs.size(); i++) {
            Units.add(EGs[i]);
            Units.add(&(EGs[i]->suAmpOnCC));
            Units.add(&(EGs[i]->suVolOnCC));
            Units.add(&(EGs[i]->suPitchOnCC));
            Units.add(&(EGs[i]->suCutoffOnCC));
            Units.add(&(EGs[i]->suResOnCC));
            Units.add(&(EGs[i]->suPanOnCC));
            EGs[i]->ImportUnits(this); // class EqUnitSupport
        }
        
        for (int i = 0; i < LFOs.size(); i++) {
            Units.add(&(LFOs[i]->suFreqOnCC)); // Don't change order! (should be triggered before the LFO)
            Units.add(LFOs[i]);
            Units.add(&(LFOs[i]->suFadeEG));
            Units.add(&(LFOs[i]->suVolOnCC));
            Units.add(&(LFOs[i]->suPitchOnCC));
            Units.add(&(LFOs[i]->suPanOnCC));
            Units.add(&(LFOs[i]->suCutoffOnCC));
            Units.add(&(LFOs[i]->suResOnCC));
            LFOs[i]->ImportUnits(this); // class EqUnitSupport
        }
        
        Units.add(&suEndpoint);
        Units.add(&suEndpoint.suXFInCC);
        Units.add(&suEndpoint.suXFOutCC);
        Units.add(&suEndpoint.suPanOnCC);
        
        SignalUnitRack::Trigger();
    }
    
    EndpointSignalUnit* SfzSignalUnitRack::GetEndpointUnit() {
        return &suEndpoint;
    }
    
    void SfzSignalUnitRack::EnterFadeOutStage() {
        suVolEG.EG.enterFadeOutStage();
        
        for (int i = 0; i < volEGs.size(); i++) {
            volEGs[i]->EG.enterFadeOutStage();
        }
    }

    void SfzSignalUnitRack::EnterFadeOutStage(int maxFadeOutSteps) {
        suVolEG.EG.enterFadeOutStage(maxFadeOutSteps);
        for (int i = 0; i < volEGs.size(); i++) {
            volEGs[i]->EG.enterFadeOutStage(maxFadeOutSteps);
        }
    }

    void SfzSignalUnitRack::Reset() {
        EqUnitSupport::ResetUnits();
        
        suVolOnCC.RemoveAllCCs();
        suPitchOnCC.RemoveAllCCs();
        suCutoffOnCC.RemoveAllCCs();
        suResOnCC.RemoveAllCCs();
        suEndpoint.suXFInCC.RemoveAllCCs();
        suEndpoint.suXFOutCC.RemoveAllCCs();
        suEndpoint.suPanOnCC.RemoveAllCCs();
        suPitchLFO.suDepthOnCC.RemoveAllCCs();
        suPitchLFO.suFreqOnCC.RemoveAllCCs();
        suFilLFO.suDepthOnCC.RemoveAllCCs();
        suFilLFO.suFreqOnCC.RemoveAllCCs();
        suAmpLFO.suDepthOnCC.RemoveAllCCs();
        suAmpLFO.suFreqOnCC.RemoveAllCCs();
        
        for (int i = 0; i < EGs.capacity(); i++) {
            EGs[i]->suAmpOnCC.RemoveAllCCs();
            EGs[i]->suVolOnCC.RemoveAllCCs();
            EGs[i]->suPitchOnCC.RemoveAllCCs();
            EGs[i]->suCutoffOnCC.RemoveAllCCs();
            EGs[i]->suResOnCC.RemoveAllCCs();
            EGs[i]->suPanOnCC.RemoveAllCCs();
            EGs[i]->ResetUnits(); // class EqUnitSupport
        }
        
        for (int i = 0; i < LFOs.capacity(); i++) {
            LFOs[i]->suDepthOnCC.RemoveAllCCs();
            LFOs[i]->suFreqOnCC.RemoveAllCCs();
            LFOs[i]->suVolOnCC.RemoveAllCCs();
            LFOs[i]->suPitchOnCC.RemoveAllCCs();
            LFOs[i]->suFreqOnCC.RemoveAllCCs();
            LFOs[i]->suPanOnCC.RemoveAllCCs();
            LFOs[i]->suCutoffOnCC.RemoveAllCCs();
            LFOs[i]->suResOnCC.RemoveAllCCs();
            LFOs[i]->ResetUnits(); // class EqUnitSupport
        }
    }

    void SfzSignalUnitRack::CalculateFadeOutCoeff(float FadeOutTime, float SampleRate) {
        suVolEG.EG.CalculateFadeOutCoeff(FadeOutTime, SampleRate);
        for (int i = 0; i < EGs.capacity(); i++) {
            EGs[i]->EG.CalculateFadeOutCoeff(FadeOutTime, SampleRate);
        }
    }
    
    void SfzSignalUnitRack::UpdateEqSettings(EqSupport* pEqSupport) {
        if (!pEqSupport->HasSupport()) return;
        if (pEqSupport->GetBandCount() < 3) {
            std::cerr << "SfzSignalUnitRack::UpdateEqSettings: EQ should have at least 3 bands\n";
            return;
        }
        
        ::sfz::Region* const pRegion = pVoice->pRegion;
        
        float dB1 = (suEq1GainOnCC.Active() ? suEq1GainOnCC.GetLevel() : 0) + pRegion->eq1_gain;
        float dB2 = (suEq2GainOnCC.Active() ? suEq2GainOnCC.GetLevel() : 0) + pRegion->eq2_gain;
        float dB3 = (suEq3GainOnCC.Active() ? suEq3GainOnCC.GetLevel() : 0) + pRegion->eq3_gain;
        
        float freq1 = (suEq1FreqOnCC.Active() ? suEq1FreqOnCC.GetLevel() : 0) + pRegion->eq1_freq;
        float freq2 = (suEq2FreqOnCC.Active() ? suEq2FreqOnCC.GetLevel() : 0) + pRegion->eq2_freq;
        float freq3 = (suEq3FreqOnCC.Active() ? suEq3FreqOnCC.GetLevel() : 0) + pRegion->eq3_freq;
        
        float bw1 = (suEq1BwOnCC.Active() ? suEq1BwOnCC.GetLevel() : 0) + pRegion->eq1_bw;
        float bw2 = (suEq2BwOnCC.Active() ? suEq2BwOnCC.GetLevel() : 0) + pRegion->eq2_bw;
        float bw3 = (suEq3BwOnCC.Active() ? suEq3BwOnCC.GetLevel() : 0) + pRegion->eq3_bw;
        
        const float vel = pVoice->MIDIVelocity() / 127.0f;
        
        dB1 += pRegion->eq1_vel2gain * vel;
        dB2 += pRegion->eq2_vel2gain * vel;
        dB3 += pRegion->eq3_vel2gain * vel;
        
        freq1 += pRegion->eq1_vel2freq * vel;
        freq2 += pRegion->eq2_vel2freq * vel;
        freq3 += pRegion->eq3_vel2freq * vel;
        
        for (int i = 0; i < eqEGs.size(); i++) {
            EGv2Unit* eg = eqEGs[i];
            if (!eg->Active()) continue;
            
            float l = eg->GetLevel();
            dB1 += ((eg->suEq1GainOnCC.Active() ? eg->suEq1GainOnCC.GetLevel() : 0) + eg->pEGInfo->eq1gain) * l;
            dB2 += ((eg->suEq2GainOnCC.Active() ? eg->suEq2GainOnCC.GetLevel() : 0) + eg->pEGInfo->eq2gain) * l;
            dB3 += ((eg->suEq3GainOnCC.Active() ? eg->suEq3GainOnCC.GetLevel() : 0) + eg->pEGInfo->eq3gain) * l;
            
            freq1 += ((eg->suEq1FreqOnCC.Active() ? eg->suEq1FreqOnCC.GetLevel() : 0) + eg->pEGInfo->eq1freq) * l;
            freq2 += ((eg->suEq2FreqOnCC.Active() ? eg->suEq2FreqOnCC.GetLevel() : 0) + eg->pEGInfo->eq2freq) * l;
            freq3 += ((eg->suEq3FreqOnCC.Active() ? eg->suEq3FreqOnCC.GetLevel() : 0) + eg->pEGInfo->eq3freq) * l;
            
            bw1 += ((eg->suEq1BwOnCC.Active() ? eg->suEq1BwOnCC.GetLevel() : 0) + eg->pEGInfo->eq1bw) * l;
            bw2 += ((eg->suEq2BwOnCC.Active() ? eg->suEq2BwOnCC.GetLevel() : 0) + eg->pEGInfo->eq2bw) * l;
            bw3 += ((eg->suEq3BwOnCC.Active() ? eg->suEq3BwOnCC.GetLevel() : 0) + eg->pEGInfo->eq3bw) * l;
        }
        
        for (int i = 0; i < eqLFOs.size(); i++) {
            LFOv2Unit* lfo = eqLFOs[i];
            if (!lfo->Active()) continue;
            
            float l = lfo->GetLevel();
            dB1 += ((lfo->suEq1GainOnCC.Active() ? lfo->suEq1GainOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq1gain) * l;
            dB2 += ((lfo->suEq2GainOnCC.Active() ? lfo->suEq2GainOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq2gain) * l;
            dB3 += ((lfo->suEq3GainOnCC.Active() ? lfo->suEq3GainOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq3gain) * l;
            
            freq1 += ((lfo->suEq1FreqOnCC.Active() ? lfo->suEq1FreqOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq1freq) * l;
            freq2 += ((lfo->suEq2FreqOnCC.Active() ? lfo->suEq2FreqOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq2freq) * l;
            freq3 += ((lfo->suEq3FreqOnCC.Active() ? lfo->suEq3FreqOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq3freq) * l;
            
            bw1 += ((lfo->suEq1BwOnCC.Active() ? lfo->suEq1BwOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq1bw) * l;
            bw2 += ((lfo->suEq2BwOnCC.Active() ? lfo->suEq2BwOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq2bw) * l;
            bw3 += ((lfo->suEq3BwOnCC.Active() ? lfo->suEq3BwOnCC.GetLevel() : 0) + lfo->pLfoInfo->eq3bw) * l;
        }
        
        pEqSupport->SetGain(0, dB1);
        pEqSupport->SetGain(1, dB2);
        pEqSupport->SetGain(2, dB3);
        
        pEqSupport->SetFreq(0, freq1);
        pEqSupport->SetFreq(1, freq2);
        pEqSupport->SetFreq(2, freq3);
        
        pEqSupport->SetBandwidth(0, bw1);
        pEqSupport->SetBandwidth(1, bw2);
        pEqSupport->SetBandwidth(2, bw3);
    }
    
    EqUnitSupport::EqUnitSupport(SfzSignalUnitRack* pRack, Voice* pVoice)
        : suEq1GainOnCC(pRack), suEq2GainOnCC(pRack), suEq3GainOnCC(pRack),
          suEq1FreqOnCC(pRack), suEq2FreqOnCC(pRack), suEq3FreqOnCC(pRack),
          suEq1BwOnCC(pRack), suEq2BwOnCC(pRack), suEq3BwOnCC(pRack)
    {
        SetVoice(pVoice);
    }
    
    void EqUnitSupport::SetVoice(Voice* pVoice) {
        suEq1GainOnCC.pVoice = suEq2GainOnCC.pVoice = suEq3GainOnCC.pVoice = pVoice;
        suEq1FreqOnCC.pVoice = suEq2FreqOnCC.pVoice = suEq3FreqOnCC.pVoice = pVoice;
        suEq1BwOnCC.pVoice = suEq2BwOnCC.pVoice = suEq3BwOnCC.pVoice = pVoice;
    }
    
    void EqUnitSupport::ImportUnits(SfzSignalUnitRack* pRack) {
        if (suEq1GainOnCC.HasCCs()) pRack->Units.add(&suEq1GainOnCC);
        if (suEq2GainOnCC.HasCCs()) pRack->Units.add(&suEq2GainOnCC);
        if (suEq3GainOnCC.HasCCs()) pRack->Units.add(&suEq3GainOnCC);
        if (suEq1FreqOnCC.HasCCs()) pRack->Units.add(&suEq1FreqOnCC);
        if (suEq2FreqOnCC.HasCCs()) pRack->Units.add(&suEq2FreqOnCC);
        if (suEq3FreqOnCC.HasCCs()) pRack->Units.add(&suEq3FreqOnCC);
        if (suEq1BwOnCC.HasCCs()) pRack->Units.add(&suEq1BwOnCC);
        if (suEq2BwOnCC.HasCCs()) pRack->Units.add(&suEq2BwOnCC);
        if (suEq3BwOnCC.HasCCs()) pRack->Units.add(&suEq3BwOnCC);
    }
    
    void EqUnitSupport::ResetUnits() {
        suEq1GainOnCC.RemoveAllCCs();
        suEq2GainOnCC.RemoveAllCCs();
        suEq3GainOnCC.RemoveAllCCs();
        suEq1FreqOnCC.RemoveAllCCs();
        suEq2FreqOnCC.RemoveAllCCs();
        suEq3FreqOnCC.RemoveAllCCs();
        suEq1BwOnCC.RemoveAllCCs();
        suEq2BwOnCC.RemoveAllCCs();
        suEq3BwOnCC.RemoveAllCCs();
    }
    
    void EqUnitSupport::InitCCLists(Pool<CCSignalUnit::CC>* pCCPool, Pool<Smoother>* pSmootherPool) {
        suEq1GainOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq2GainOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq3GainOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq1FreqOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq2FreqOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq3FreqOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq1BwOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq2BwOnCC.InitCCList(pCCPool, pSmootherPool);
        suEq3BwOnCC.InitCCList(pCCPool, pSmootherPool);
    }
    
}} // namespace LinuxSampler::sfz
