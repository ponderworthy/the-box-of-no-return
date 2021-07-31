/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
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

#ifndef __FEATURES_H__
#define __FEATURES_H__

#include "global_private.h"

/**
 * Detects system / CPU specific features at runtime.
 */
class Features {
    public:
        static void   detect();
        static bool   enableDenormalsAreZeroMode();
        static String featuresAsString();

        #if CONFIG_ASM && ARCH_X86
        inline static bool supportsMMX() { return bMMX; }
        inline static bool supportsSSE() { return bSSE; }
        inline static bool supportsSSE2() { return bSSE2; }
        #endif // CONFIG_ASM && ARCH_X86
    private:
        #if CONFIG_ASM && ARCH_X86
        static bool bMMX;
        static bool bSSE;
        static bool bSSE2;
        #endif // CONFIG_ASM && ARCH_X86
};

#endif // __FEATURES_H__
