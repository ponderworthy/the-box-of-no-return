/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009 Christian Schoenebeck and Grigor Iliev             *
 *   Copyright (C) 2016 Christian Schoenebeck                              *
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

#include "Stream.h"
#include "../../common/global_private.h"

namespace LinuxSampler { namespace gig {

    Stream::Stream (
        ::gig::buffer_t* pDecompressionBuffer,
        uint             BufferSize,
        uint             BufferWrapElements) : LinuxSampler::StreamBase< ::gig::DimensionRegion>(BufferSize, BufferWrapElements)
    {
        this->pDecompressionBuffer = pDecompressionBuffer;
    }

    long Stream::Read(uint8_t* pBuf, long SamplesToRead) {
        ::gig::Sample* pSample = pRegion->pSample;
        long total_readsamples = 0, readsamples = 0;
        bool endofsamplereached;

        // refill the disk stream buffer
        if (this->DoLoop) { // honor looping
            ::gig::playback_state_t pbs;
            pbs.position = PlaybackState.position;
            pbs.reverse = PlaybackState.reverse;
            pbs.loop_cycles_left = PlaybackState.loop_cycles_left;

            total_readsamples  = pSample->ReadAndLoop(pBuf, SamplesToRead, &pbs, pRegion, pDecompressionBuffer);
            PlaybackState.position = pbs.position;
            PlaybackState.reverse = pbs.reverse;
            PlaybackState.loop_cycles_left = pbs.loop_cycles_left;
            endofsamplereached = (this->PlaybackState.position >= pSample->SamplesTotal);
            dmsg(5,("Refilled stream %d with %ld (SamplePos: %lu)", this->hThis, total_readsamples, this->PlaybackState.position));
        }
        else { // normal forward playback

            pSample->SetPos(this->SampleOffset); // recover old position

            do {
                readsamples        = pSample->Read(&pBuf[total_readsamples * pSample->FrameSize], SamplesToRead, pDecompressionBuffer);
                SamplesToRead     -= readsamples;
                total_readsamples += readsamples;
            } while (SamplesToRead && readsamples > 0);

            // we have to store the position within the sample, because other streams might use the same sample
            this->SampleOffset = pSample->GetPos();

            endofsamplereached = (SampleOffset >= pSample->SamplesTotal);
            dmsg(5,("Refilled stream %d with %ld (SamplePos: %lu)", this->hThis, total_readsamples, this->SampleOffset));
        }

        // update stream state
        if (endofsamplereached) SetState(state_end);
        else                    SetState(state_active);

        return total_readsamples;
    }

    void Stream::Launch (
        Stream::Handle           hStream,
        reference_t*             pExportReference,
        ::gig::DimensionRegion*  pRgn,
        unsigned long            SampleOffset,
        bool                     DoLoop
    ) {
        SampleDescription info;
        info.ChannelsPerFrame = pRgn->pSample->Channels;
        info.FrameSize        = pRgn->pSample->FrameSize;
        info.BytesPerSample   = pRgn->pSample->BitDepth / 8;
        info.TotalSampleCount = (int)pRgn->pSample->SamplesTotal;

        Sample::PlaybackState playbackState;
        playbackState.position         = SampleOffset;
        playbackState.reverse          = false;
        playbackState.loop_cycles_left = pRgn->pSample->LoopPlayCount;

        LinuxSampler::StreamBase< ::gig::DimensionRegion>::Launch (
            hStream, pExportReference, pRgn, info, playbackState, SampleOffset, DoLoop
        );
    }

}} // namespace LinuxSampler::gig
