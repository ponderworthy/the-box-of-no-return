/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2012 Christian Schoenebeck and Grigor Iliev      *
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

#ifndef __LS_SFZ_VOICE_H__
#define __LS_SFZ_VOICE_H__

#include "../../common/global_private.h"

#include "../../common/RTMath.h"
#include "../../common/Pool.h"
#include "../../drivers/audio/AudioOutputDevice.h"
#include "DiskThread.h"
#include "../gig/Filter.h"
#include "../common/VoiceBase.h"
#include "../gig/SynthesisParam.h"
#include "EG.h"
#include "EGADSR.h"
#include "SfzSignalUnitRack.h"

namespace LinuxSampler { namespace sfz {
    class Engine;
    class EngineChannel;

    /** SoundFont Voice
     *
     * Renders a voice for the SoundFont format.
     */
    class Voice : public LinuxSampler::VoiceBase<EngineChannel, ::sfz::Region, Sample, DiskThread> {
        public:
            Voice();
            virtual ~Voice();
            void SetOutput(AudioOutputDevice* pAudioOutputDevice);
            void SetEngine(LinuxSampler::Engine* pEngine);
            void CalculateFadeOutCoeff(float FadeOutTime, float SampleRate);
            virtual release_trigger_t GetReleaseTriggerFlags() OVERRIDE;

            virtual void VoiceFreed() OVERRIDE { SignalRack.Reset(); }

        protected:
            virtual SampleInfo       GetSampleInfo() OVERRIDE;
            virtual RegionInfo       GetRegionInfo() OVERRIDE;
            virtual InstrumentInfo   GetInstrumentInfo() OVERRIDE;
            virtual double           CalculateCrossfadeVolume(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual AbstractEngine*  GetEngine() OVERRIDE { return (AbstractEngine*)pEngine; }
            virtual float            GetReleaseTriggerAttenuation(float noteLength) OVERRIDE;
            virtual double           GetEG1ControllerValue(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual EGInfo           CalculateEG1ControllerInfluence(double eg1ControllerValue) OVERRIDE;
            virtual void             TriggerEG1(const EGInfo& egInfo, double velrelease, double velocityAttenuation, uint sampleRate, uint8_t velocity) OVERRIDE { }
            virtual double           GetEG2ControllerValue(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual EGInfo           CalculateEG2ControllerInfluence(double eg2ControllerValue) OVERRIDE;
            virtual void             TriggerEG2(const EGInfo& egInfo, double velrelease, double velocityAttenuation, uint sampleRate, uint8_t velocity) OVERRIDE { }
            virtual void             InitLFO1() OVERRIDE { }
            virtual void             InitLFO2() OVERRIDE { }
            virtual void             InitLFO3() OVERRIDE { }
            virtual float            CalculateCutoffBase(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual float            CalculateFinalCutoff(float cutoffBase) OVERRIDE;
            virtual uint8_t          GetVCFCutoffCtrl() OVERRIDE { return 0; }
            virtual uint8_t          GetVCFResonanceCtrl() OVERRIDE { return 0; }
            virtual void             ProcessCCEvent(RTList<Event>::Iterator& itEvent) OVERRIDE;
            virtual void             ProcessChannelPressureEvent(RTList<Event>::Iterator& itEvent) OVERRIDE;
            virtual void             ProcessPolyphonicKeyPressureEvent(RTList<Event>::Iterator& itEvent) OVERRIDE;
            virtual void             ProcessCutoffEvent(RTList<Event>::Iterator& itEvent) OVERRIDE { }
            virtual double           GetVelocityAttenuation(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual double           GetVelocityRelease(uint8_t MIDIKeyVelocity) OVERRIDE;
            virtual double           GetSampleAttenuation() OVERRIDE;
            virtual void             ProcessGroupEvent(RTList<Event>::Iterator& itEvent) OVERRIDE;
            virtual void             SetSampleStartOffset() OVERRIDE;
            virtual int              GetRAMCacheOffset() OVERRIDE { return pRegion->pSample->RAMCacheOffset; }
            virtual int              CalculatePan(uint8_t pan) OVERRIDE;

        private:
            SfzSignalUnitRack SignalRack;

        public: // FIXME: just made public for debugging (sanity check in Engine::RenderAudio()), should be changed to private before the final release
            // Attributes
            Engine*                     pEngine;            ///< Pointer to the sampler engine, to be able to access the event lists.
            //uint                        LoopCyclesLeft;     ///< In case there is a RAMLoop and it's not an endless loop; reflects number of loop cycles left to be passed

            // Static Methods
            static float CalculateFilterCutoffCoeff();

            // Methods
            void ProcessEvents(uint Samples);
            void processCrossFadeEvent(RTList<Event>::Iterator& itEvent);

            EngineChannel* GetSfzEngineChannel();
            
            friend class CCUnit;
            friend class XFInCCUnit;
            friend class XFOutCCUnit;
            friend class AmpEGUnit;
            friend class FilEGUnit;
            friend class PitchEGUnit;
            friend class PitchLFOUnit;
            friend class AmpLFOUnit;
            friend class FilLFOUnit;
            friend class EndpointUnit;
            friend class SfzSignalUnitRack;

        protected:
            virtual uint8_t CrossfadeAttenuation(uint8_t& CrossfadeControllerValue) OVERRIDE {
                /*uint8_t c = std::max(CrossfadeControllerValue, pRegion->AttenuationControllerThreshold);
                c = (!pRegion->Crossfade.out_end) ? c     // 0,0,0,0 means no crossfade defined
                          : (c < pRegion->Crossfade.in_end) ?
                                ((c <= pRegion->Crossfade.in_start) ? 0
                                : 127 * (c - pRegion->Crossfade.in_start) / (pRegion->Crossfade.in_end - pRegion->Crossfade.in_start))
                          : (c <= pRegion->Crossfade.out_start) ? 127
                          : (c < pRegion->Crossfade.out_end) ? 127 * (pRegion->Crossfade.out_end - c) / (pRegion->Crossfade.out_end - pRegion->Crossfade.out_start)
                          : 0;
                return pRegion->InvertAttenuationController ? 127 - c : c;*/ // TODO: ^^^
                return 0;
            }
    };

}} // namespace LinuxSampler::sfz

#endif // __LS_SFZ_VOICE_H__
