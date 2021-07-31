/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2010 Christian Schoenebeck                       *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef LS_EG_H
#define LS_EG_H

#include <cmath>
#include "../../common/RTMath.h"

namespace LinuxSampler {
    class EG {
    public:

        /**
         * Used to define what kind of segment the envelope currently is at.
         */
        enum segment_t {
            segment_end = 0, ///< final end state of envelope reached
            segment_lin = 1, ///< envelope is currently at a linear segment
            segment_exp = 2, ///< envelope is currently at a exponential segment
            segment_pow = 3  ///< envelope is currently at a power segment
        };

        /**
         * Used to inform the EG about an event.
         */
        enum event_t {
            event_stage_end,
            event_release,
            event_cancel_release,
            event_hold_end
        };

        /**
         * Change fade out time.
         */
        void CalculateFadeOutCoeff(float FadeOutTime, float SampleRate);

        /**
         * Returns true in case envelope hasn't reached its final end state yet.
         */
        bool active() {
            return Segment;
        }

        /**
         * Returns what kind of segment the envelope currently is at.
         */
        segment_t getSegmentType() {
            return Segment;
        }

        /**
         * Advance envelope by \a SamplePoints steps.
         */
        void increment(int SamplePoints) {
            StepsLeft = RTMath::Max(0, StepsLeft - SamplePoints);
        }

        /**
         * Returns amount of steps until the end of current envelope stage.
         */
        int toStageEndLeft() {
            return StepsLeft;
        }

        /**
         * Should be called to inform the EG about an external event and
         * also whenever an envelope stage is completed. This will handle
         * the envelope's transition to the respective next stage.
         *
         * @param Event        - what happened
         */
        virtual void update(event_t Event, uint SampleRate) = 0;

        /**
         * Calculates exactly one, the next sample point of EG
         * (linear segment). Call this if envelope is currently in a linear
         * segment.
         *
         * @returns next envelope level
         */
        float processLin() {
            return (Level += Coeff);
        }

        /**
         * Calculates exactly one, the next sample point of EG
         * (exponential segment). Call this if envelope is currently in an
         * exponential segment.
         *
         * @returns next envelope level
         */
        float processExp() {
            return (Level = Level * Coeff + Offset);
        }

        /**
         * Calculates exactly one, the next sample point of EG (power
         * segment). Call this if envelope is currently in an power
         * segment.
         *
         * @returns next envelope level
         */
        float processPow() {
            // TODO: this could be optimised by a pow approximation
            Level = Offset + Coeff * powf(X, Exp);
            X += XDelta;
            return Level;
        }

        /**
         * Returns current envelope level without modifying anything. This
         * might be needed once the envelope reached its final end state,
         * because calling processLin() or processExp() at this point will
         * result in undesired behavior.
         */
        float getLevel() {
            return Level;
        }

        void enterFadeOutStage();
        void enterFadeOutStage(int maxFadeOutSteps);

    protected:
        float     Level;
        float     Coeff;
        float     Offset;
        float     Exp;
        float     X;
        float     XDelta;
        int       StepsLeft;
        segment_t Segment;

        EG();
        void enterFirstStage() { // must be called in trigger
            Stage = stage_main;
        }
        bool atEnd(event_t Event); // must be called first in update
        void enterEndStage();

    private:
        enum stage_t {
            stage_main,
            stage_fadeout,
            stage_end
        };

        stage_t   Stage;
        float     FadeOutCoeff; ///< very fast ramp down for e.g. voice stealing
};
}

#endif
