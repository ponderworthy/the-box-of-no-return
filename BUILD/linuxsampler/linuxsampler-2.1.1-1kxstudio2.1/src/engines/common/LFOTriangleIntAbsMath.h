/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 Christian Schoenebeck                              *
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

#ifndef __LS_LFOTRIANGLEINTABSMATH_H__
#define __LS_LFOTRIANGLEINTABSMATH_H__

#include <stdlib.h>
#include "LFOTriangleIntMath.h"

namespace LinuxSampler {

    /** @brief Triangle LFO (int math implementation)
     *
     * This is a triangle Low Frequency Oscillator which uses pure integer
     * math (without branches) to synthesize the triangular wave.
     */
    template<range_type_t RANGE>
    class LFOTriangleIntAbsMath : public LFOTriangleIntMath<RANGE> {
        public:
            /**
             * Constructor
             *
             * @param Max - maximum value of the output levels
             */
            LFOTriangleIntAbsMath(float Max) : LFOTriangleIntMath<RANGE>::LFOTriangleIntMath(Max) {
            }

            /**
             * Calculates exactly one sample point of the LFO wave.
             *
             * @returns next LFO level
             */
            inline float render() {
                this->iLevel += this->c;
                if (RANGE == range_unsigned)
                    return this->normalizer * (float) (abs(this->iLevel));
                else /* signed range */
                    return this->normalizer * (float) (abs(this->iLevel)) + this->offset;
            }
    };

} // namespace LinuxSampler

#endif // __LS_LFOTRIANGLEINTABSMATH_H__
