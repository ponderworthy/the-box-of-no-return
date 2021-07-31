/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2010 Andreas Persson                                    *
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

#include "EG.h"

#include "../../common/global_private.h"

namespace LinuxSampler { namespace sfz {

    // Modeled after output from Rapture Demo 1.0, which means the
    // stages are using a pow function and the "curve" parameter of
    // the stage is ignored. The "shape" parameter decides the slope
    // of the stage, a value of 1.0 or 0.0 means linear shape. The
    // length of the stages are dependent on the velocity.

    // TODO: add support for loops
    // TODO: optimization: use segment_lin for linear stages
    // TODO: support cancel_release events?

    void EG::update(event_t Event, uint SampleRate) {
        if (atEnd(Event)) return;

        // ignore duplicated release events
        if (Event == event_release) {
            if (GotRelease) return;
            GotRelease = true;
        }

        if (Event == event_stage_end || Event == event_release) {
            if (Stage == eg->node.size() - 1) {
                enterFadeOutStage();
            } else if (Stage == eg->sustain && Stage != 0 &&
                       Event != event_release) {
                enterSustainStage();
            } else {
                if (Event == event_release) {
                    Stage = eg->sustain;
                } else {
                    Level = eg->node[Stage].level;
                }

                Stage++;

                float shape = eg->node[Stage].shape;
                if (shape < 0.000001) shape = 1;

                float xd = eg->node[Stage].time * SampleRate * TimeCoeff;
                float yd = eg->node[Stage].level - Level;

                if (eg->node[Stage - 1].shape > 0.999999 ||
                    eg->node[Stage - 1].shape < 0.000001) {
                    Exp = 1 / shape;
                    Offset = Level;
                    X = 0;
                    XDelta = 1 / xd;
                    Coeff = yd;
                } else {
                    Exp = shape;
                    Offset = eg->node[Stage].level;
                    X = 1;
                    XDelta = -1 / xd;
                    Coeff = -yd;
                }
                Segment = segment_pow;
                StepsLeft = int(xd);
            }
        }
    }

    void EG::trigger(::sfz::EG& eg, uint SampleRate, uint8_t Velocity) {
        Stage = 0;
        this->eg = &eg;
        TimeCoeff = exp(0.0054578518 * Velocity); // pow(2, Velcocity / 127)
        GotRelease = false;

        enterFirstStage();
        update(event_stage_end, SampleRate);
    }

    void EG::enterSustainStage() {
        Segment = segment_lin;
        Coeff   = 0.0f; // don't change the envelope level in this stage
        const int intMax = (unsigned int) -1 >> 1;
        StepsLeft = intMax; // we use the highest value possible (we refresh StepsLeft in update() in case)
        Level = eg->node[Stage].level;
    }
}} // namespace LinuxSampler::sfz
