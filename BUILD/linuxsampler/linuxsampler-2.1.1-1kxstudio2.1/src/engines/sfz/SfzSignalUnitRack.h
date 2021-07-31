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

#ifndef __LS_SFZSIGNALUNITRACK_H__
#define	__LS_SFZSIGNALUNITRACK_H__

#include "../common/SignalUnitRack.h"
#include "EG.h"
#include "EGADSR.h"
#include "../common/AbstractVoice.h"
#include "../common/PulseLFO.h"
#include "../common/SawLFO.h"
#include "../common/SineLFO.h"

namespace LinuxSampler { namespace sfz {
    const int MaxUnitCount = 200;
    const int maxEgCount = 30; // Maximum number of v2 envelope generators
    const int maxLfoCount = 30; // Maximum number of v2 LFOs
    
    class Voice;
    class SfzSignalUnitRack;
    
    class SfzSignalUnit: public SignalUnit {
        public:
            Voice* pVoice;

            SfzSignalUnit(SfzSignalUnitRack* rack);
            SfzSignalUnit(const SfzSignalUnit& Unit): SignalUnit(Unit.pRack) { Copy(Unit); }
            
            void Copy(const SfzSignalUnit& Unit) {
                pVoice = Unit.pVoice;
                
                SignalUnit::Copy(Unit);
            }
            
            double GetSampleRate();
            float  GetInfluence(ArrayList< ::sfz::CC>& cc);
    };
    
    
    class CCUnit: public CCSignalUnit {
        public:
            Voice* pVoice;

            CCUnit(SfzSignalUnitRack* rack, Listener* l = NULL);
            
            virtual void Trigger();
            
            void SetCCs(::sfz::Array<int>& pCC);
            void SetCCs(::sfz::Array<float>& pCC);
            void SetCCs(ArrayList< ::sfz::CC>& cc);
            
            virtual void AddSmoothCC(uint8_t Controller, float Influence, short int Curve, float Smooth, float Step);
            
            int GetCurveCount();
            ::sfz::Curve* GetCurve(int idx);
            
            double GetSampleRate();
    };
    
    class CurveCCUnit: public CCUnit {
        public:
            CurveCCUnit(SfzSignalUnitRack* rack, Listener* l = NULL): CCUnit(rack, l) { }
            
            virtual float Normalize(uint8_t val, short int curve = -1) {
                if (curve == -1) return val / 127.0f;
                return GetCurve(curve)->v[val];
            }
    };
    
    
    
    class SmoothCCUnit: public CurveCCUnit {
        protected:
            RTList<Smoother>* pSmoothers;
        public:
            SmoothCCUnit(SfzSignalUnitRack* rack, Listener* l = NULL): CurveCCUnit(rack, l), pSmoothers(NULL) { }
            virtual ~SmoothCCUnit();
            
            virtual void AddSmoothCC(uint8_t Controller, float Influence, short int Curve, float Smooth, float Step);
            virtual void RemoveAllCCs() { CurveCCUnit::RemoveAllCCs(); pSmoothers->clear(); }
            virtual void InitCCList(Pool<CC>* pCCPool, Pool<Smoother>* pSmootherPool);
            
            void InitSmoothers(Pool<Smoother>* pSmootherPool);
    };
    
    
    class EqUnitSupport {
        public:
            EqUnitSupport(SfzSignalUnitRack* pRack, Voice* pVoice = NULL);
            
            SmoothCCUnit suEq1GainOnCC;
            SmoothCCUnit suEq2GainOnCC;
            SmoothCCUnit suEq3GainOnCC;
            
            SmoothCCUnit suEq1FreqOnCC;
            SmoothCCUnit suEq2FreqOnCC;
            SmoothCCUnit suEq3FreqOnCC;
            
            SmoothCCUnit suEq1BwOnCC;
            SmoothCCUnit suEq2BwOnCC;
            SmoothCCUnit suEq3BwOnCC;
            
            void SetVoice(Voice* pVoice);
            void ImportUnits(SfzSignalUnitRack* pRack);
            void ResetUnits();
            void InitCCLists(Pool<CCSignalUnit::CC>* pCCPool, Pool<Smoother>* pSmootherPool);
    };
    
    
    class XFInCCUnit: public CCUnit {
        public:
            XFInCCUnit(SfzSignalUnitRack* rack, Listener* l = NULL): CCUnit(rack, l) { }
            
            virtual bool Active() { return !pCtrls->isEmpty(); }
            virtual void Calculate();
            virtual void SetCrossFadeCCs(::sfz::Array<int>& loCCs, ::sfz::Array<int>& hiCCs);
    };
    
    
    class XFOutCCUnit: public XFInCCUnit {
        public:
            XFOutCCUnit(SfzSignalUnitRack* rack, Listener* l = NULL): XFInCCUnit(rack, l) { }
            
            virtual void Calculate();
    };
    
    
    template <class T>
    class EGUnit: public SfzSignalUnit {
        public:
            ::sfz::EG* pEGInfo;
            T EG;

            EGUnit(SfzSignalUnitRack* rack): SfzSignalUnit(rack), pEGInfo(NULL) { }
            EGUnit(const EGUnit& Unit): SfzSignalUnit(Unit) { Copy(Unit); }
            void operator=(const EGUnit& Unit) { Copy(Unit); }
            
            void Copy(const EGUnit& Unit) {
                pEGInfo = Unit.pEGInfo;
                
                SfzSignalUnit::Copy(Unit);
            }

            virtual bool  Active() { return EG.active(); }
            virtual float GetLevel() { return DelayStage() ? 0 : EG.getLevel(); }
            
            virtual void EnterReleaseStage() { EG.update(EG::event_release, GetSampleRate()); }
            virtual void CancelRelease() { EG.update(EG::event_cancel_release, GetSampleRate()); }
            
            virtual void Increment() {
                if (DelayStage()) return;

                SfzSignalUnit::Increment();
                if (!EG.active()) return;
        
                switch (EG.getSegmentType()) {
                    case EG::segment_lin:
                        EG.processLin();
                        break;
                    case EG::segment_exp:
                        EG.processExp();
                        break;
                    case EG::segment_end:
                        EG.getLevel();
                        break; // noop
                    case EG::segment_pow:
                        EG.processPow();
                        break;
                }
        
                if (EG.active()) {
                    EG.increment(1);
                    if (!EG.toStageEndLeft()) EG.update(EG::event_stage_end, GetSampleRate());
                }
            }
    };
    
    class EGv1Unit: public EGUnit<EGADSR> {
        public:
            int depth;
            EGv1Unit(SfzSignalUnitRack* rack): EGUnit<EGADSR>(rack), depth(0) { }
    };
    
    class EGv2Unit: public EGUnit< ::LinuxSampler::sfz::EG>, public EqUnitSupport {
        protected:
            ::sfz::EG egInfo;
        public:
            CCUnit suAmpOnCC;
            CCUnit suVolOnCC;
            CCUnit suPitchOnCC;
            CCUnit suCutoffOnCC;
            CCUnit suResOnCC;
            CurveCCUnit suPanOnCC;
            
            EGv2Unit(SfzSignalUnitRack* rack);
            virtual void Trigger();
    };
    
    class PitchEGUnit: public EGv1Unit {
        public:
            PitchEGUnit(SfzSignalUnitRack* rack): EGv1Unit(rack) { }
            virtual void Trigger();
    };
    
    class FilEGUnit: public EGv1Unit {
        public:
            FilEGUnit(SfzSignalUnitRack* rack): EGv1Unit(rack) { }
            virtual void Trigger();
    };
    
    class AmpEGUnit: public EGv1Unit {
        public:
            AmpEGUnit(SfzSignalUnitRack* rack): EGv1Unit(rack) { }
            virtual void Trigger();
    };
    
    class AbstractLfo {
        public:
            virtual float Render() = 0;
            virtual void Update(const uint16_t& ExtControlValue) = 0;
            virtual void Trigger(float Frequency, start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) = 0;
            virtual void SetPhase(float phase) = 0;
            virtual void SetFrequency(float Frequency, unsigned int SampleRate) = 0;
    };
    
    template <class T>
    class LfoBase: public AbstractLfo, public T {
        public:
            LfoBase(float Max): T(Max) { }
            virtual float Render() { return T::render(); }
            
            virtual void Update(const uint16_t& ExtControlValue) { T::updateByMIDICtrlValue(ExtControlValue); }
            
            virtual void Trigger (
                float Frequency, start_level_t StartLevel, uint16_t InternalDepth,
                uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate
            ) {
                T::trigger(Frequency, StartLevel, InternalDepth, ExtControlDepth, FlipPhase, SampleRate);
            }
            
            virtual void SetPhase(float phase) { T::setPhase(phase); }
            
            virtual void SetFrequency(float Frequency, unsigned int SampleRate) {
                T::setFrequency(Frequency, SampleRate);
            }
    };
    
    class LFOUnit;
    
    class FadeEGUnit: public EGUnit<EGADSR> {
        public:
            FadeEGUnit(SfzSignalUnitRack* rack): EGUnit<EGADSR>(rack) { }
            virtual void Trigger() { }
            virtual void EnterReleaseStage() { }
            virtual void CancelRelease() { }
            
            friend class LFOUnit;
    };
    
    class LFOUnit: public SfzSignalUnit, public CCSignalUnit::Listener {
        public:
            ::sfz::LFO*  pLfoInfo;
            AbstractLfo* pLFO;
            FadeEGUnit   suFadeEG;
            SmoothCCUnit suDepthOnCC;
            SmoothCCUnit suFreqOnCC;
            
            LFOUnit(SfzSignalUnitRack* rack);
            LFOUnit(const LFOUnit& Unit);
            void operator=(const LFOUnit& Unit) { Copy(Unit); }
            
            void Copy(const LFOUnit& Unit) {
                pLfoInfo   = Unit.pLfoInfo;
                suFadeEG   = Unit.suFadeEG;
                
                SfzSignalUnit::Copy(Unit);
            }
            
            virtual void  Trigger();
            virtual void  Increment();
            virtual float GetLevel() { return Level; }
            
            // CCSignalUnit::Listener interface implementation
            virtual void ValueChanged(CCSignalUnit* pUnit);
    };
    
    class LFOv1Unit: public LFOUnit {
        public:
            ::sfz::LFO lfoInfo;
            LfoBase<LFOSigned> lfo;
            
            LFOv1Unit(SfzSignalUnitRack* rack): LFOUnit(rack), lfo(1200.0f) {
                pLfoInfo = &lfoInfo; pLFO = &lfo;
            }
            
            virtual void Trigger();
    };
    
    class LFOv2Unit: public LFOUnit, public EqUnitSupport {
        protected:
            FixedArray<AbstractLfo*> lfos;
            LfoBase<LFOSigned>                       lfo0; // triangle
            LfoBase<SineLFO<range_signed> >          lfo1; // sine
            LfoBase<PulseLFO<range_unsigned, 750> >  lfo2; // pulse 75%
            LfoBase<SquareLFO<range_signed> >        lfo3; // square
            LfoBase<PulseLFO<range_unsigned, 250> >  lfo4; // pulse 25%
            LfoBase<PulseLFO<range_unsigned, 125> >  lfo5; // pulse 12,5%
            LfoBase<SawLFO<range_unsigned, true> >   lfo6; // saw up
            LfoBase<SawLFO<range_unsigned, false> >  lfo7; // saw down
            
            
        public:
            SmoothCCUnit suVolOnCC;
            SmoothCCUnit suPitchOnCC;
            SmoothCCUnit suPanOnCC;
            SmoothCCUnit suCutoffOnCC;
            SmoothCCUnit suResOnCC;
            
            LFOv2Unit(SfzSignalUnitRack* rack);
            
            virtual void Trigger();
            virtual bool  Active() { return true; }
    };
    
    class AmpLFOUnit: public LFOv1Unit {
        public:
            AmpLFOUnit(SfzSignalUnitRack* rack): LFOv1Unit(rack) { }
            
            virtual void Trigger();
    };
    
    class PitchLFOUnit: public LFOv1Unit {
        public:
            PitchLFOUnit(SfzSignalUnitRack* rack): LFOv1Unit(rack) { }
            
            virtual void Trigger();
    };
    
    class FilLFOUnit: public LFOv1Unit {
        public:
            FilLFOUnit(SfzSignalUnitRack* rack): LFOv1Unit(rack) { }
            
            virtual void Trigger();
    };
    
    
    class EndpointUnit: public EndpointSignalUnit {
        private:
            float xfCoeff; // crossfade coefficient
            float pitchVeltrackRatio;
            
        public:
            Voice* pVoice;
            XFInCCUnit   suXFInCC;
            XFOutCCUnit  suXFOutCC;
            SmoothCCUnit suPanOnCC;

            EndpointUnit(SfzSignalUnitRack* rack);

            virtual void Trigger();

            /**
             * The endpoint should be active until the volume EG is active.
             * This method determines the end of the voice playback.
             */
            virtual bool Active();
            
            virtual float GetVolume();
            virtual float GetFilterCutoff();
            virtual float GetPitch();
            virtual float GetResonance();
            virtual float GetPan();
            
            SfzSignalUnitRack* const GetRack();
            
            virtual float CalculateResonance(float res) {
                return GetResonance() + res;
            }
            
            virtual float CalculateFilterCutoff(float cutoff);
            
            float  GetInfluence(::sfz::Array< optional<float> >& cc);
            float  GetInfluence(::sfz::Array< optional<int> >& cc);
    };
    
    
    class SfzSignalUnitRack : public SignalUnitRack, public EqUnitSupport {
        private:
            EndpointUnit  suEndpoint;
            AmpEGUnit     suVolEG;
            FilEGUnit     suFilEG;
            PitchEGUnit   suPitchEG;
            
            AmpLFOUnit   suAmpLFO;
            PitchLFOUnit suPitchLFO;
            FilLFOUnit   suFilLFO;
            
            // SFZ v2
            
            SmoothCCUnit suVolOnCC;
            SmoothCCUnit suPitchOnCC;
            SmoothCCUnit suCutoffOnCC;
            SmoothCCUnit suResOnCC;
            
            FixedArray<EGv2Unit*> EGs;
            
            // used for optimization - contains only the ones that are modulating volume
            FixedArray<EGv2Unit*> volEGs;
            
            // used for optimization - contains only the ones that are modulating pitch
            FixedArray<EGv2Unit*> pitchEGs;
            
            // used for optimization - contains only the ones that are modulating filter cutoff
            FixedArray<EGv2Unit*> filEGs;
            
            // used for optimization - contains only the ones that are modulating resonance
            FixedArray<EGv2Unit*> resEGs;
            
            // used for optimization - contains only the ones that are modulating pan
            FixedArray<EGv2Unit*> panEGs;
            
            // used for optimization - contains only the ones that are modulating EQ
            FixedArray<EGv2Unit*> eqEGs;
            
            
            FixedArray<LFOv2Unit*> LFOs;
            
            // used for optimization - contains only the ones that are modulating volume
            FixedArray<LFOv2Unit*> volLFOs;
            
            // used for optimization - contains only the ones that are modulating pitch
            FixedArray<LFOv2Unit*> pitchLFOs;
            
            // used for optimization - contains only the ones that are modulating filter cutoff
            FixedArray<LFOv2Unit*> filLFOs;
            
            // used for optimization - contains only the ones that are modulating resonance
            FixedArray<LFOv2Unit*> resLFOs;
            
            // used for optimization - contains only the ones that are modulating pan
            FixedArray<LFOv2Unit*> panLFOs;
            
            // used for optimization - contains only the ones that are modulating EQ
            FixedArray<LFOv2Unit*> eqLFOs;
            

        public:
            Voice* const pVoice;
        
            /**
             * @param Voice The voice to which this rack belongs.
             */
            SfzSignalUnitRack(Voice* voice);
            ~SfzSignalUnitRack();

            virtual EndpointSignalUnit* GetEndpointUnit();
            
            virtual void Trigger();
            virtual void EnterFadeOutStage();
            virtual void EnterFadeOutStage(int maxFadeOutSteps);
            
            /** Called when the engine is set and the engine's pools are ready to use. */
            void InitRTLists();
            
            /** Invoked when the voice gone inactive. */
            void Reset();

            void CalculateFadeOutCoeff(float FadeOutTime, float SampleRate);
            
            virtual void UpdateEqSettings(EqSupport* pEqSupport);
            
            friend class EndpointUnit;
    };
}} // namespace LinuxSampler::sfz

#endif	/* __LS_SFZSIGNALUNITRACK_H__ */

