/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2011 Andreas Persson                                    *
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

#include "Filter.h"

namespace LinuxSampler {
    const LowpassFilter1p         Filter::lp1p;
    const LowpassFilter           Filter::lp2p;
    const LowpassFilter4p         Filter::lp4p;
    const LowpassFilter6p         Filter::lp6p;
    const BandpassFilter          Filter::bp2p;
    const BandrejectFilter        Filter::br2p;
    const HighpassFilter1p        Filter::hp1p;
    const HighpassFilter          Filter::hp2p;
    const HighpassFilter4p        Filter::hp4p;
    const HighpassFilter6p        Filter::hp6p;
    const gig::HighpassFilter     Filter::HPFilter;
    const gig::BandpassFilter     Filter::BPFilter;
    const gig::LowpassFilter      Filter::LPFilter;
    const gig::BandrejectFilter   Filter::BRFilter;
    const gig::LowpassTurboFilter Filter::LPTFilter;
}
