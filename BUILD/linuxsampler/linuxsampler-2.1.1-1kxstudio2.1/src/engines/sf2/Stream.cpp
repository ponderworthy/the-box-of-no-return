/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2015 Christian Schoenebeck and Grigor Iliev        *
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

#include <signal.h>


#include "Stream.h"
#include "../../common/global_private.h"

namespace LinuxSampler { namespace sf2 {

    Stream::Stream (
        uint BufferSize,
        uint BufferWrapElements
    ) : LinuxSampler::StreamBase< ::sf2::Region>(BufferSize, BufferWrapElements) {
        
    }

    long Stream::Read(uint8_t* pBuf, long SamplesToRead) {
        ::sf2::Sample* pSample = pRegion->pSample;
        long total_readsamples = 0, readsamples = 0;
        bool endofsamplereached;

        // refill the disk stream buffer
        if (this->DoLoop) { // honor looping
            ::sf2::Sample::PlaybackState pbs;
            pbs.position = PlaybackState.position;
            pbs.reverse = PlaybackState.reverse;
            pbs.loop_cycles_left = PlaybackState.loop_cycles_left;

            total_readsamples  = pSample->ReadAndLoop(pBuf, SamplesToRead, &pbs, pRegion);
            PlaybackState.position = pbs.position;
            PlaybackState.reverse = pbs.reverse;
            PlaybackState.loop_cycles_left = pbs.loop_cycles_left;
            endofsamplereached = (this->PlaybackState.position >= pSample->GetTotalFrameCount());
            dmsg(5,("Refilled stream %d with %ld (SamplePos: %lu)", this->hThis, total_readsamples, this->PlaybackState.position));
        }
        else { // normal forward playback

            pSample->SetPos(this->SampleOffset); // recover old position

            do {
                readsamples        = pSample->Read(&pBuf[total_readsamples * pSample->GetFrameSize()], SamplesToRead);
                SamplesToRead     -= readsamples;
                total_readsamples += readsamples;
            } while (SamplesToRead && readsamples > 0);

            // we have to store the position within the sample, because other streams might use the same sample
            this->SampleOffset = pSample->GetPos();

            endofsamplereached = (SampleOffset >= pSample->GetTotalFrameCount());
            dmsg(5,("Refilled stream %d with %ld (SamplePos: %lu)", this->hThis, total_readsamples, this->SampleOffset));
        }

        // update stream state
        if (endofsamplereached) SetState(state_end);
        else                    SetState(state_active);

        return total_readsamples;
    }

    void Stream::Kill() {
        StreamBase< ::sf2::Region>::Kill();
    }

    void Stream::Launch (
        Stream::Handle  hStream,
        reference_t*    pExportReference,
        ::sf2::Region*  pRgn,
        unsigned long   SampleOffset,
        bool            DoLoop
    ) {
        SampleDescription info;
        info.ChannelsPerFrame = pRgn->pSample->GetChannelCount();
        info.FrameSize        = pRgn->pSample->GetFrameSize();
        info.BytesPerSample   = pRgn->pSample->GetFrameSize() / pRgn->pSample->GetChannelCount();
        info.TotalSampleCount = (int)pRgn->pSample->GetTotalFrameCount();

        Sample::PlaybackState playbackState;
        playbackState.position         = SampleOffset;
        playbackState.reverse          = false;
        playbackState.loop_cycles_left = 0; // TODO: pRgn->pSample->LoopPlayCount;

        LinuxSampler::StreamBase< ::sf2::Region>::Launch (
            hStream, pExportReference, pRgn, info, playbackState, SampleOffset, DoLoop
        );
    }

}} // namespace LinuxSampler::sf2
