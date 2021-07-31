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

#ifndef LS_SFZ_EG_H
#define LS_SFZ_EG_H

#include "../common/EG.h"
#include "../../common/global.h"
#include "../../common/RTMath.h"
#include "sfz.h"

namespace LinuxSampler { namespace sfz {

/**
 * Multi Stage Envelope Generator
 *
 * SFZ v2 envelope generator with multiple stages for modulating
 * arbitrary synthesis parameters.
 */
class EG : public ::LinuxSampler::EG {
    public:
        /**
         * Will be called by the voice when the key / voice was triggered.
         *
         * @param eg              - EG from sfz::Definition
         * @param SampleRate      - sample rate of used audio output driver
         * @param Velocity        - MIDI velocity
         */
        void trigger(::sfz::EG& eg, uint SampleRate, uint8_t Velocity);

        /**
         * Should be called to inform the EG about an external event and
         * also whenever an envelope stage is completed. This will handle
         * the envelope's transition to the respective next stage.
         *
         * @param Event        - what happened
         * @param SampleRate   - sample rate
         */
        void update(event_t Event, uint SampleRate);

    private:
        int Stage;
        ::sfz::EG* eg;
        float TimeCoeff;
        bool GotRelease;
        void enterSustainStage();
};

}} // namespace LinuxSampler::sfz

#endif
