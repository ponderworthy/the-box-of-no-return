/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
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

#ifndef __LS_SF2_STREAM_H__
#define	__LS_SF2_STREAM_H__

#include "../common/StreamBase.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/SF.h>
#else
# include <SF.h>
#endif

namespace LinuxSampler { namespace sf2 {

    class Stream: public LinuxSampler::StreamBase< ::sf2::Region> {
        public:
            Stream(uint BufferSize, uint BufferWrapElements);
            virtual long Read(uint8_t* pBuf, long SamplesToRead);
            virtual void Kill();

            void Launch (
                Stream::Handle  hStream,
                reference_t*    pExportReference,
                ::sf2::Region*  pRgn,
                unsigned long   SampleOffset,
                bool            DoLoop
            );
    };


}} // namespace LinuxSampler::sf2

#endif	/* __LS_SF2_STREAM_H__ */

