/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005, 2006, 2014 Christian Schoenebeck                  *
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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_MIDI_H__
#define __LS_MIDI_H__

#include <string.h>

namespace LinuxSampler {

    /////////////////////////////////////////////////////////////////
    // global type definitions

    /**
     * MIDI channels
     */
    enum midi_chan_t {
        midi_chan_1   = 0,
        midi_chan_2   = 1,
        midi_chan_3   = 2,
        midi_chan_4   = 3,
        midi_chan_5   = 4,
        midi_chan_6   = 5,
        midi_chan_7   = 6,
        midi_chan_8   = 7,
        midi_chan_9   = 8,
        midi_chan_10  = 9,
        midi_chan_11  = 10,
        midi_chan_12  = 11,
        midi_chan_13  = 12,
        midi_chan_14  = 13,
        midi_chan_15  = 14,
        midi_chan_16  = 15,
        midi_chan_all = 16
    };

    /**
     * MIDI program index
     */
    struct midi_prog_index_t {
        uint8_t midi_bank_msb; ///< coarse MIDI bank index
        uint8_t midi_bank_lsb; ///< fine MIDI bank index
        uint8_t midi_prog;     ///< MIDI program index

        bool operator< (const midi_prog_index_t& other) const {
            return memcmp(this, &other, sizeof(midi_prog_index_t)) < 0;
        }
    };

    inline bool isValidMidiChan(const midi_chan_t& ch) {
        return ch >= 0 && ch <= midi_chan_all;
    }

} // namsepace LinuxSampler

#endif // __LS_MIDI_H__
