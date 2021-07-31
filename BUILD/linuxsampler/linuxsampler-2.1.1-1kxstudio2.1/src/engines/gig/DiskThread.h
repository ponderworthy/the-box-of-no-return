/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2019 Grigor Iliev                                  *
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

#ifndef __LS_GIG_DISKTHREAD_H__
#define	__LS_GIG_DISKTHREAD_H__

#include "../InstrumentManagerBase.h"
#include "../common/DiskThreadBase.h"
#include "InstrumentResourceManager.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

namespace LinuxSampler {
    template <>
    LinuxSampler::Stream* LinuxSampler::DiskThreadBase< ::gig::DimensionRegion, gig::InstrumentResourceManager>::SLOT_RESERVED;

namespace gig {

    class DiskThread: public LinuxSampler::DiskThreadBase< ::gig::DimensionRegion, InstrumentResourceManager> {
        protected:
            ::gig::buffer_t DecompressionBuffer; ///< Used for thread safe streaming.

            virtual LinuxSampler::Stream* CreateStream(long BufferSize, uint BufferWrapElements);

            virtual void LaunchStream (
                LinuxSampler::Stream*    pStream,
                Stream::Handle           hStream,
                Stream::reference_t*     pExportReference,
                ::gig::DimensionRegion*  pRgn,
                unsigned long            SampleOffset,
                bool                     DoLoop
            );

        public:
            DiskThread(int MaxStreams, uint BufferWrapElements, InstrumentResourceManager* pInstruments);
            virtual ~DiskThread();
    };


}} // namespace LinuxSampler::gig

#endif	/* __LS_GIG_STREAM_H__ */

