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

#ifndef __LS_SF2SIGNALUNITRACK_H__
#define	__LS_SF2SIGNALUNITRACK_H__

#include "../common/SignalUnitRack.h"
#include "../sfz/EGADSR.h"
#include "../common/AbstractVoice.h"

namespace LinuxSampler { namespace sf2 {
    const int MaxUnitCount = 5;
    
    class Voice;
    class SF2SignalUnitRack;
    
    class SFSignalUnit: public SignalUnit {
        public:
            Voice* pVoice;

            SFSignalUnit(SF2SignalUnitRack* rack);
    };
    
    class EGUnit : public SFSignalUnit, public ::LinuxSampler::sfz::EGADSR {
        public:
            EGUnit(SF2SignalUnitRack* rack): SFSignalUnit(rack) { }

            virtual bool  Active() OVERRIDE { return active(); }
            virtual float GetLevel() OVERRIDE { return getLevel(); }
            virtual void  EnterReleaseStage() OVERRIDE;
            virtual void  CancelRelease() OVERRIDE;
    };

    class VolEGUnit : public EGUnit {
        public:
            VolEGUnit(SF2SignalUnitRack* rack): EGUnit(rack) { }

            virtual void  Trigger() OVERRIDE;
            virtual void  Increment() OVERRIDE;
    };

    class ModEGUnit : public EGUnit {
        public:
            ModEGUnit(SF2SignalUnitRack* rack): EGUnit(rack) { }

            virtual void  Trigger() OVERRIDE;
            virtual void  Increment() OVERRIDE;
    };

    class ModLfoUnit : public SFSignalUnit, public LFOSigned {
        public:
            ModLfoUnit(SF2SignalUnitRack* rack): SFSignalUnit(rack), LFOSigned(1200.0f) { }
            virtual bool  Active() OVERRIDE { return true; }
            virtual void  Trigger() OVERRIDE;
            virtual void  Increment() OVERRIDE;
            virtual float GetLevel() OVERRIDE { return Level; }
    };

    class VibLfoUnit : public SFSignalUnit, public LFOSigned {
        public:
            VibLfoUnit(SF2SignalUnitRack* rack): SFSignalUnit(rack), LFOSigned(1200.0f) { }
            virtual bool  Active() OVERRIDE { return true; }
            virtual void  Trigger() OVERRIDE;
            virtual void  Increment() OVERRIDE;
            virtual float GetLevel() OVERRIDE { return Level; }
    };
    
    class EndpointUnit : public EndpointSignalUnit {
        public:
            Voice* pVoice;

            Parameter *prmVolEg, *prmModEgPitch, *prmModEgCutoff, *prmModLfoVol,
                      *prmModLfoPitch, *prmModLfoCutoff, *prmVibLfo;

            EndpointUnit(SF2SignalUnitRack* rack);

            virtual void Trigger() OVERRIDE;

            /** The endpoint should be active until the volume EG is active. */
            virtual bool Active() OVERRIDE;
            
            virtual float GetVolume() OVERRIDE;
            virtual float GetFilterCutoff() OVERRIDE;
            virtual float GetPitch() OVERRIDE;
            virtual float GetResonance() OVERRIDE;
            virtual float GetPan() OVERRIDE { return 0; }
            virtual uint8_t CalculatePan(int pan) OVERRIDE { return pan; }
    };
    
    class SF2SignalUnitRack : public SignalUnitRack {
        private:
            VolEGUnit     suVolEG;
            ModEGUnit     suModEG;
            ModLfoUnit    suModLfo;
            VibLfoUnit    suVibLfo;
            EndpointUnit  suEndpoint;
            

        public:
            Voice* const pVoice;
        
            /**
             * @param Voice The voice to which this rack belongs.
             */
            SF2SignalUnitRack(Voice* Voice);

            virtual EndpointSignalUnit* GetEndpointUnit() OVERRIDE;
            virtual void EnterFadeOutStage() OVERRIDE;
            virtual void EnterFadeOutStage(int maxFadeOutSteps) OVERRIDE;

            void CalculateFadeOutCoeff(float FadeOutTime, float SampleRate);
            
            virtual void UpdateEqSettings(EqSupport* pEqSupport) OVERRIDE { }
    };
    
}} // namespace LinuxSampler::sf2

#endif	/* __LS_SF2SIGNALUNITRACK_H__ */
