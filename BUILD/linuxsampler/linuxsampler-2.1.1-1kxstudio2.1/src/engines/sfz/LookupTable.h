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

#ifndef SFZ_LOOKUPTABLE_H
#define SFZ_LOOKUPTABLE_H

#include <vector>

#include "sfz.h"
#include "../../common/ArrayList.h"

namespace sfz {

    /**
     * Lookup table for fast access to regions in an sfz instrument.
     *
     * The table is organized in two levels. First, the values of the
     * dimensions actually in use by the instrument (could be key,
     * velocity, controller values, etc) are used as indexes in a set
     * of integer arrays. The resulting integers are then added
     * together and the sum is used as an index in an array of lists
     * of regions.
     *
     * The first level is used to make the second array smaller. An
     * instrument with three key zones and two velocity zones will
     * have two integer arrays, each of size 128, one for key number,
     * one for velocity. The region array only needs to be six items
     * big, as there are only 2 * 3 possible zone variations.
     */
    class LookupTable {
    public:
        /**
         * Constructs a lookup table for the instrument. If triggercc
         * is specified, the lookup table is made for regions
         * triggered by the MIDI controller, otherwise the table is
         * made for ordinary note-on triggered regions.
         *
         * @param instrument - instrument
         * @param triggercc - controller number or -1
         */
        LookupTable(const Instrument* instrument, int triggercc = -1);

        ~LookupTable();

        /**
         * Performs a lookup in the table of regions.
         *
         * @param q - query with constraints on key, velocity, etc.
         * @returns list of regions matching the query
         */
        LinuxSampler::ArrayList<Region*>& query(const Query& q) const;

    private:
        struct DimDef;

        static const DimDef dimDefs[]; // list of possible dimensions
        std::vector<int> dims; // dimensions used by the instrument

        // control change dimensions used by the instrument
        std::vector<int> ccs;

        // arrays mapping dimension values to regionArr offsets
        int** mapArr;

        // second level array with lists of regions
        LinuxSampler::ArrayList<Region*>* regionArr;

        // pointers to used dimension arguments in the Query object
        const uint8_t Query::** qargs;

        // array with CCs used by the instrument
        int* ccargs;


        // helper functions for the constructor
        static int fillMapArr(const std::vector<Region*>& regions,
                              const int Definition::* lo,
                              const int Definition::* hi,
                              int min, int max, int* a);
        static int fillMapArr(const std::vector<Region*>& regions,
                              int cc, int* a, int triggercc);
        void fillRegionArr(const int* len, Region* region,
                           std::vector<int>::size_type dim, int j,
                           int triggercc);
    };
}

#endif
