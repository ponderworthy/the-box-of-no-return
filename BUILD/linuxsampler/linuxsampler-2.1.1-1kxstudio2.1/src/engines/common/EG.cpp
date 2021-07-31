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

#include "EG.h"

namespace LinuxSampler {

    EG::EG() {
        enterEndStage();
        Level = 0.0;
        CalculateFadeOutCoeff(CONFIG_EG_MIN_RELEASE_TIME, 44100.0); // even if the sample rate will be 192kHz it won't hurt at all
    }

    void EG::CalculateFadeOutCoeff(float FadeOutTime, float SampleRate) {
        const float killSteps = FadeOutTime * SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        FadeOutCoeff = -1.0f / killSteps;
    }

    void EG::enterFadeOutStage() {
        Stage     = stage_fadeout;
        Segment   = segment_lin;
        StepsLeft = int(Level / (-FadeOutCoeff));
        Coeff     = FadeOutCoeff;
        if (StepsLeft <= 0) enterEndStage();
    }

    void EG::enterFadeOutStage(int maxFadeOutSteps) {
        Stage     = stage_fadeout;
        Segment   = segment_lin;
        StepsLeft = int(Level / (-FadeOutCoeff));
        if (StepsLeft > maxFadeOutSteps) {
            StepsLeft = maxFadeOutSteps;
            Coeff = -Level / maxFadeOutSteps;
        } else {
            Coeff = FadeOutCoeff;
        }
        if (StepsLeft <= 0) enterEndStage();
    }

    bool EG::atEnd(event_t Event) {
        if (Stage == stage_end) return true;
        if (Stage == stage_fadeout) {
            if (Event == event_stage_end) enterEndStage();
            return true;
        }
        return false;
    }

    void EG::enterEndStage() {
        Stage   = stage_end;
        Segment = segment_end;
        Level   = 0;
    }
}
