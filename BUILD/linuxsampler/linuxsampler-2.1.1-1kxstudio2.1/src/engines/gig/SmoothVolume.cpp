/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2006 Andreas Persson                                    *
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

#include <cmath>
#include "SmoothVolume.h"

#include <cstdio>

namespace LinuxSampler { namespace gig {

    void SmoothVolume::trigger(float volume, float sampleRate) {
        float d = 1 / sampleRate;
        a1 = exp(-44 * d);
        b0 = 1 - a1;
        decay = exp(-11 * d);
        coeff = 0.33f * d;
        this->volume = volume;
        moving = false;
    }

    float SmoothVolume::process() {
        if (goal < volume) {
            // decreasing
            float newVolume = volume > 0.059f ? volume * decay : volume - coeff;
            if (newVolume > goal) volume = newVolume;
            else {
                volume = goal;
                moving = false;
            }
        } else {
            // increasing
            if (goal - volume > 0.013f)
                volume = b0 * goal + a1 * volume; // one-pole LP filter
            else {
                float newVolume = volume + coeff;
                if (newVolume < goal) volume = newVolume;
                else {
                    volume = goal;
                    moving = false;
                }
            }
        }
        return volume;
    }

}};
