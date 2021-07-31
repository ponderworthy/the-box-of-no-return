/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 Christian Schoenebeck                              *
 *   Copyright (C) 2011 Christian Schoenebeck and Grigor Iliev             *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_PULSELFO_H__
#define __LS_PULSELFO_H__

#include "LFOBase.h"

namespace LinuxSampler {

    /** @brief Pulse LFO (int math implementation)
     *
     * This is a pulse Low Frequency Oscillator which uses pure integer
     * math (without branches) to synthesize the pulse wave.
     */
    template<range_type_t RANGE, int WIDTH /* in permilles */>
    class PulseLFO : public LFOBase<RANGE> {
        public:

            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            PulseLFO(float Max) : LFOBase<RANGE>::LFOBase(Max) {
            }

            /**
             * Calculates exactly one sample point of the LFO wave.
             *
             * @returns next LFO level
             */
            inline float render() {
                uiLevel += c;
                if (RANGE == range_unsigned)
                    return uiLevel <= width ? normalizer : 0;
                else /* signed range */
                    return uiLevel <= width ? normalizer : -normalizer;
            }

            /**
             * Update LFO depth with a new external controller value.
             *
             * @param ExtControlValue - new external controller value
             */
            inline void updateByMIDICtrlValue(const uint16_t& ExtControlValue) {
                this->ExtControlValue = ExtControlValue;

                const float max = (this->InternalDepth + ExtControlValue * this->ExtControlDepthCoeff) * this->ScriptDepthFactor;
                normalizer = max;
            }

            /**
             * Will be called by the voice when the key / voice was triggered.
             *
             * @param Frequency       - frequency of the oscillator in Hz
             * @param InternalDepth   - firm, internal oscillator amplitude
             * @param ExtControlDepth - defines how strong the external MIDI
             *                          controller has influence on the
             *                          oscillator amplitude
             * @param FlipPhase       - inverts the oscillator wave against
             *                          a horizontal axis
             * @param SampleRate      - current sample rate of the engines
             *                          audio output signal
             * @param PulseWidth      - the pulse width in percents
             */
            void trigger(float Frequency, uint16_t InternalDepth, uint16_t ExtControlDepth, float PulseWidth, unsigned int SampleRate) {
                this->Frequency            = Frequency;
                this->InternalDepth        = (InternalDepth / 1200.0f) * this->Max;
                this->ExtControlDepthCoeff = (((float) ExtControlDepth / 1200.0f) / 127.0f) * this->Max;
                this->ScriptFrequencyFactor = this->ScriptDepthFactor = 1.f; // reset for new voice

                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                const float freq = Frequency * this->ScriptFrequencyFactor;
                const float r = freq / (float) SampleRate; // frequency alteration quotient
                c = (int) (intLimit * r);
                width = (PulseWidth / 100.0) * intLimit;

                uiLevel = 0;
            }
            
            virtual void trigger(float Frequency, start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) {
                trigger(Frequency, InternalDepth, ExtControlDepth, WIDTH / 10.0f, SampleRate);
            }
            
            /**
             * Should be invoked after the LFO is triggered.
             * @param phase From 0 to 360 degrees.
             */
            void setPhase(float phase) {
                if (phase < 0) phase = 0;
                if (phase > 360) phase = 360;
                phase /= 360.0f;
                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                uiLevel = intLimit * phase;
            }
            
            void setFrequency(float Frequency, unsigned int SampleRate) {
                this->Frequency = Frequency;
                const float freq = Frequency * this->ScriptFrequencyFactor;
                const unsigned int intLimit = (unsigned int) -1; // all 0xFFFF...
                float r = freq / (float) SampleRate; // frequency alteration quotient
                c = (int) (intLimit * r);
            }

            void setScriptDepthFactor(float factor) {
                this->ScriptDepthFactor = factor;
                updateByMIDICtrlValue(this->ExtControlValue);
            }

            void setScriptFrequencyFactor(float factor, unsigned int SampleRate) {
                this->ScriptFrequencyFactor = factor;
                setFrequency(this->Frequency, SampleRate);
            }

        protected:
            unsigned int uiLevel;
            unsigned int width;
            int   c;
            float normalizer;
    };
    
    template<range_type_t RANGE>
    class SquareLFO : public PulseLFO<RANGE, 500> {
        public:
            SquareLFO(float Max) : PulseLFO<RANGE, 500>::PulseLFO(Max) { }
    };

} // namespace LinuxSampler

#endif // __LS_PULSELFO_H__
