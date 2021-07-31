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

#ifndef __LS_SIGNALUNITRACK_H__
#define	__LS_SIGNALUNITRACK_H__

#include "Event.h"
#include "SignalUnit.h"
#include "../../common/Pool.h"


namespace LinuxSampler {
    
    class EqSupport {
        private:
            int  BandCount;
            int* GainIdxs; ///< the indices of the gain controls
            int* FreqIdxs; ///< the indices of the frequency controls
            int* BandwidthIdxs; ///< the indices of the bandwidth controls
            Effect* pEffect;
            Effect* pEffect2; // If the effect is mono we'll need two effects
            
            inline float check(optional<float> minimum, optional<float> maximum, float value) {
                if (minimum) {
                    float min = *minimum;
                    if (value < min) value = min;
                }
                if (maximum) {
                    float max = *maximum;
                    if (value > max) value = max;
                }
                
                return value;
            }
            
        public:
            EqSupport();
            ~EqSupport();
            
            void PrintInfo();

            /**
             * Searches for know EQ effects and create one if the search succeed.
             * If the initialization is successful and EQ effect is selected,
             * HasSupport() returns true;
             */
            void Install();

            void Uninstall();

            /** Returns true if an EQ is created and is ready for use. */
            bool HasSupport() { return pEffect != NULL; }
 
            /** Reset the gains of all bands to 0dB. */
            void Reset() {
                if (!HasSupport()) return;
                for (int i = 0; i < BandCount; i++) {
                    pEffect->InputControl(GainIdxs[i])->SetValue(0); // 0dB
                    if (pEffect2 != NULL) pEffect2->InputControl(GainIdxs[i])->SetValue(0); // 0dB
                }
            }
            
            void InitEffect(AudioOutputDevice* pDevice) {
                if (pEffect != NULL) pEffect->InitEffect(pDevice);
                if (pEffect2 != NULL) pEffect2->InitEffect(pDevice);
            }
                
            int GetBandCount() { return BandCount; }

            void SetGain(int band, float gain);
            void SetFreq(int band, float freq);
            void SetBandwidth(int band, float octaves);
            
            AudioChannel* GetInChannelLeft() {
                return pEffect->InputChannel(0);
            }
            
            AudioChannel* GetInChannelRight() {
                return pEffect2 != NULL ? pEffect2->InputChannel(0) : pEffect->InputChannel(1);
            }
            
            AudioChannel* GetOutChannelLeft() {
                return pEffect->OutputChannel(0);
            }
            
            AudioChannel* GetOutChannelRight() {
                return pEffect2 != NULL ? pEffect2->OutputChannel(0) : pEffect->OutputChannel(1);
            }
            
            void RenderAudio(uint Samples) {
                pEffect->RenderAudio(Samples);
                if (pEffect2 != NULL) pEffect2->RenderAudio(Samples);
            }
    };
            
            
    
    class SignalUnitRack {
        protected:
            uint CurrentStep; // The current time step
            bool bHasEq, releaseStageEntered;

        public:
            FixedArray<SignalUnit*> Units; // A list of all signal units in this rack

            /**
             * @param maxUnitCount We are using fixed size array because of the real-time safe requirements.
             */
            SignalUnitRack(int maxUnitCount): CurrentStep(0), bHasEq(false),
                                              releaseStageEntered(false), Units(maxUnitCount) { }
            
            uint GetCurrentStep() { return CurrentStep; }

            virtual EndpointSignalUnit* GetEndpointUnit() = 0;
            
            virtual void EnterFadeOutStage() = 0;
            virtual void EnterFadeOutStage(int maxFadeOutSteps) = 0;
            
            /**
             * Will be called to increment the time with one sample point.
             * Each unit should recalculate its current level on every call of this function.
             */
            virtual void Increment() {
                CurrentStep++;

                for (int i = 0; i < Units.size(); i++) {
                    Units[i]->Increment();
                }
            }
            
            virtual void ProcessCCEvent(RTList<Event>::Iterator& itEvent) {
                if ( !(itEvent->Type == Event::type_control_change && itEvent->Param.CC.Controller) ) return;
                for (int i = 0; i < Units.size(); i++) {
                    Units[i]->ProcessCCEvent(itEvent->Param.CC.Controller, itEvent->Param.CC.Value);
                }
            }
            
            /** Initializes and triggers the rack. */
            virtual void Trigger() {
                releaseStageEntered = false;
                CurrentStep = 0;
                for (int i = 0; i < Units.size(); i++) {
                    Units[i]->Trigger();
                }
            }
            
            /**
             * When the rack belongs to a voice, this method is
             * called when the voice enter the release stage.
             */
            virtual void EnterReleaseStage() {
                releaseStageEntered = true;
                for (int i = 0; i < Units.size(); i++) {
                    Units[i]->EnterReleaseStage();
                }
            }
            
            bool isReleaseStageEntered() { return releaseStageEntered; }
            
            /**
             * When the rack belongs to a voice, this method is
             * called when the voice is of type which ignore note off.
             */
            virtual void CancelRelease() {
                for (int i = 0; i < Units.size(); i++) {
                    Units[i]->CancelRelease();
                }
            }
            
            /**
             * Determines whether an equalization is applied to the voice.
             * Used for optimization.
             */
            bool HasEq() { return bHasEq; }
            
            virtual void UpdateEqSettings(EqSupport* pEqSupport) = 0;
    };
} // namespace LinuxSampler

#endif	/* __LS_SIGNALUNITRACK_H__ */
