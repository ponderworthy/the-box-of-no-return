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

#include "DiskThread.h"
#include "Stream.h"
#include "../../common/global_private.h"

namespace LinuxSampler {
    // just a placeholder to mark a cell in the pickup array as 'reserved'
    template <>
    LinuxSampler::Stream* LinuxSampler::DiskThreadBase< ::sfz::Region, sfz::InstrumentResourceManager>::SLOT_RESERVED = (LinuxSampler::Stream*) &SLOT_RESERVED;

namespace sfz {

    DiskThread::DiskThread(int MaxStreams, uint BufferWrapElements, InstrumentResourceManager* pInstruments) :
        DiskThreadBase< ::sfz::Region, InstrumentResourceManager>(MaxStreams, BufferWrapElements, pInstruments)
    {
        CreateAllStreams(MaxStreams, BufferWrapElements);
    }

    DiskThread::~DiskThread() {
        
    }

    LinuxSampler::Stream* DiskThread::CreateStream(long BufferSize, uint BufferWrapElements) {
        return new Stream((uint)BufferSize, BufferWrapElements, pInstruments->GetSampleManager()); // 131072 sample words
    }

    void DiskThread::LaunchStream (
        LinuxSampler::Stream*  pStream,
        Stream::Handle         hStream,
        Stream::reference_t*   pExportReference,
        ::sfz::Region*         pRgn,
        unsigned long          SampleOffset,
        bool                   DoLoop
    ) {
        Stream* pSfzStream = dynamic_cast<Stream*>(pStream);
        if(!pSfzStream) throw Exception("Invalid stream type");
        pSfzStream->Launch(hStream, pExportReference, pRgn, SampleOffset, DoLoop);
    }
}} // namespace LinuxSampler::sfz

