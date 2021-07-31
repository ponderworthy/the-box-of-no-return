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

#ifndef __LS_GIG_SMOOTHVOLUME_H__
#define __LS_GIG_SMOOTHVOLUME_H__

namespace LinuxSampler { namespace gig {

    /**
     * Filter that smoothens out sudden changes in volume level.
     */
    class SmoothVolume {
        public:

            /**
             * Initializes the smooth volume.
             *
             * @param volume     - initial volume value
             * @param sampleRate - rate at which the render function
             *                     will be called
             */
            void trigger(float volume, float sampleRate);

            /**
             * Sets a new volume. The render function will return
             * values gradually approaching this value.
             *
             * @param newVolume - new volume value
             */
            void update(float newVolume) {
                goal = newVolume;
                moving = true;
            }

            /**
             * Calculates exactly one sample point of the volume
             * curve.
             *
             * @returns next volume level
             */
            float render() { return moving ? process() : volume; }

        private:
            bool moving;
            float goal;
            float volume;
            float coeff;
            float decay;
            float a1;
            float b0;
            float process();
    };

}};
#endif
