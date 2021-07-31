/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005, 2006 Christian Schoenebeck                        *
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

#ifndef __LS_GIG_EGDECAY_H__
#define __LS_GIG_EGDECAY_H__

#include "../../common/global.h"
#include "../../common/RTMath.h"

namespace LinuxSampler { namespace gig {

    /** @brief Decay Envelope Generator (linear)
     *
     * Simple Envelope Generator with only one stage: 'Decay' which is a
     * linear segment. The initial envelope level (given by 'Depth') will
     * raise / drop in 'DecayTime' seconds to a level of exactly 1.0. If
     * the initial level ('Depth') is already 1.0, nothing happens.
     */
    class EGDecay {
        public:
            EGDecay();

            /**
             * Will be called by the voice when the key / voice was
             * triggered and initialize the envelope generator.
             *
             * @param Depth      - initial level of the envelope
             * @param DecayTime  - decay time of the envelope (0.000 - 10.000s)
             * @param SampleRate - sample rate of used audio driver
             */
            void trigger(float Depth, float DecayTime, unsigned int SampleRate); //FIXME: we should better use 'float' for SampleRate

            /**
             * Returns true if envelope is still in stage 'Decay', returns
             * false if end of envelope is reached.
             */
            inline bool active() {
                return (bool) Coeff;
            }

            /**
             * Advance envelope by \a SamplePoints steps.
             */
            inline void increment(const int SamplePoints) {
                StepsLeft = RTMath::Max(0, StepsLeft - SamplePoints);
            }

            /**
             * Returns amount of steps until end of envelope is reached.
             */
            inline int toEndLeft() {
                return StepsLeft;
            }

            /**
             * Should be called once the end of the envelope is reached. It
             * will neutralize the envelope coefficient to not alter the
             * envelope anymore and will set the output level to final level
             * of exactly 1.0f. So after this call, render() can still
             * safely be called from the sampler's main synthesis loop.
             */
            inline void update() {
                Level = 1.0f;
                Coeff = 0.0f;
            }

            /**
             * Calculates exactly one level of the envelope.
             *
             * @returns next envelope level
             */
            inline float render() {
                return (Level += Coeff);
            }

            /**
             * Returns the level which this envelope will have in
             * \a SamplePoints steps. It will not alter anything.
             */
            inline float level(const int SamplePoints) const {
                return Level + RTMath::Min(SamplePoints, StepsLeft) * Coeff;
            }

        private:
            float Level;     ///< current EG output level
            float Coeff;     ///< linear coefficient for changing the output level in time
            int   StepsLeft; ///< how many steps left until end is reached
    };

}} // namespace LinuxSampler::gig

#endif // __LS_GIG_EGDECAY_H__
