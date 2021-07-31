/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 - 2017 Christian Schoenebeck                       *
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

#ifndef __LS_LFOBASE_H__
#define __LS_LFOBASE_H__

#include "../../common/global.h"
#include "../../common/RTMath.h"

// IDs of the two possible implementations
// we get the implementation to pick from config.h
// the implementation IDs should be the same like in benchmarks/triang.cpp !
#define INT_MATH_SOLUTION       2
#define DI_HARMONIC_SOLUTION    3
#define INT_ABS_MATH_SOLUTION   5

#include <math.h>
#include <stdint.h>

namespace LinuxSampler {

    // *************** types ***************
    // *

    /**
     * Whether the LFO should have positive AND negative value range
     * (signed) or only a positive value range (unsigned).
     */
    enum range_type_t {
        range_signed,  ///< LFO's level will wave between -max ... +max
        range_unsigned ///< LFO's level will wave between 0 ... +max
    };

    /**
     * Defines the start level of the LFO wave within the given value range.
     */
    enum start_level_t {
        start_level_max, ///< wave starts from given max. level
        start_level_mid, ///< wave starts from the middle of the given value range
        start_level_min  ///< wave starts from given min. level
    };

    /** @brief LFO (abstract base class)
     * 
     * Abstract base class for all Low Frequency Oscillator implementations.
     */
    template<range_type_t RANGE>
    class LFOBase {
        public:

            // *************** attributes ***************
            // *

            uint8_t ExtController; ///< MIDI control change controller number if the LFO is controlled by an external controller, 0 otherwise.


            // *************** methods ***************
            // *

            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOBase(float Max) {
                this->ExtController = 0;
                this->Max           = Max;
                this->InternalDepth = 0;
                this->Frequency = 20.f;
                this->ExtControlValue = 0;
                this->ExtControlDepthCoeff = 0;
                this->ScriptDepthFactor = 1.f;
                this->ScriptFrequencyFactor = 1.f;
            }

            virtual ~LFOBase() {
            }

            /**
             * Calculates exactly one sample point of the LFO wave. This
             * inline method has to be implemented by the descendant.
             *
             * @returns next LFO level
             */
            //abstract inline float render(); //< what a pity that abstract inliners are not supported by C++98 (probably by upcoming C++0x?)

            /**
             * Update LFO depth with a new external controller value. This
             * inline method has to be implemented by the descendant.
             *
             * @param ExtControlValue - new external controller value
             */
            //abstract inline void updateByMIDICtrlValue(const uint16_t& ExtControlValue); //< what a pity that abstract inliners are not supported by C++98 (probably by upcoming C++0x?)

            /**
             * Will be called by the voice when the key / voice was triggered.
             *
             * @param Frequency       - frequency of the oscillator in Hz
             * @param StartLevel      - on which level the wave should start
             * @param InternalDepth   - firm, internal oscillator amplitude
             * @param ExtControlDepth - defines how strong the external MIDI
             *                          controller has influence on the
             *                          oscillator amplitude
             * @param FlipPhase       - inverts the oscillator wave against
             *                          a horizontal axis
             * @param SampleRate      - current sample rate of the engines
             *                          audio output signal
             */
            virtual void trigger(float Frequency, start_level_t StartLevel, uint16_t InternalDepth, uint16_t ExtControlDepth, bool FlipPhase, unsigned int SampleRate) = 0;

        protected:
            float Max;
            float InternalDepth;
            float Frequency; ///< Internal base frequency for this LFO.
            float ExtControlValue; ///< The current external MIDI controller value (0-127).
            float ExtControlDepthCoeff; ///< A usually constant factor used to convert a new MIDI controller value from range 0-127 to the required internal implementation dependent value range.
            float ScriptDepthFactor; ///< Usually neutral (1.0), only altered by external RT instrument script functions.
            float ScriptFrequencyFactor; ///< Usually neutral (1.0), only altered by external RT instrument script functions.
    };

} // namespace LinuxSampler

#endif // __LS_LFOBASE_H__
