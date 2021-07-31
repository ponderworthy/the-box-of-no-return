/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#ifndef __LS_STREAMBASE_H__
#define __LS_STREAMBASE_H__

#include "Stream.h"
#include "../../common/global.h"
#include "../../common/RingBuffer.h"
#include <iostream>

namespace LinuxSampler {

    /** @brief Buffered Disk Stream
     *
     * This encapsulation of a disk stream uses a ring buffer to allow
     * thread safe refilling the stream's buffer with one thread (disk
     * thread) and actual use / extraction of the audio data from the
     * stream's buffer with another thread (audio thread).
     */
    template <class R>
    class StreamBase : public Stream {
        public:
            // Methods
            StreamBase(uint BufferSize, uint BufferWrapElements) : Stream(BufferSize, BufferWrapElements) {
                this->pRegion      = NULL;
                this->SampleOffset = 0;
            }

            virtual ~StreamBase() {
                Reset();
                if (pRingBuffer) delete pRingBuffer;
        	UnusedStreams--;
        	TotalStreams--;
            }

            /// Returns number of refilled sample points or a value < 0 on error.
            virtual int ReadAhead(unsigned long SampleCount) {
                if (this->State == state_unused) return -1;
                if (this->State == state_end)    return  0;
                if (!SampleCount)                return  0;
                if (!pRingBuffer->write_space()) return  0;

                long samplestoread = SampleCount / SampleInfo.ChannelsPerFrame;
                uint8_t* pBuf = pRingBuffer->get_write_ptr();
                long total_readsamples = Read(pBuf, samplestoread);

                // we must delay the increment_write_ptr_with_wrap() after the while() loop because we need to
                // ensure that we read exactly SampleCount sample, otherwise the buffer wrapping code will fail
                pRingBuffer->increment_write_ptr_with_wrap(int(total_readsamples * SampleInfo.FrameSize));

                return (int)total_readsamples;
            }

            virtual void WriteSilence(unsigned long SilenceSampleWords) {
                memset(pRingBuffer->get_write_ptr(), 0, SilenceSampleWords * SampleInfo.BytesPerSample);
                pRingBuffer->increment_write_ptr_with_wrap(int(SilenceSampleWords * SampleInfo.BytesPerSample));
            }

            /// Called by disk thread to activate the disk stream.
            void Launch (
                Stream::Handle         hStream,
                reference_t*           pExportReference,
                R*                     pRgn,
                SampleDescription      SampleInfo,
                Sample::PlaybackState  PlaybackState,
                unsigned long          SampleOffset,
                bool                   DoLoop
            ) {
                UnusedStreams--;
                this->pExportReference  = pExportReference;
                this->hThis             = hStream;
                this->pRegion           = pRgn;
                this->SampleInfo        = SampleInfo;
                this->PlaybackState     = PlaybackState;
                this->SampleOffset      = SampleOffset;
                this->DoLoop            = DoLoop;
                SetState(state_active);
            }

        protected:
            // Attributes
            unsigned long               SampleOffset;
            R*                          pRegion;
            bool                        DoLoop;

            virtual void Reset() {
                SampleOffset                   = 0;
                pRegion                        = NULL;
                PlaybackState.position         = 0;
                PlaybackState.reverse          = false;
                hThis                          = 0;
                pRingBuffer->init(); // reset ringbuffer
                if (State != state_unused) {
                    // we can't do 'SetPos(state_unused)' here, due to possible race conditions)
                    if (pExportReference) {
                        pExportReference->State = state_unused;
                        pExportReference        = NULL;
                    }
                    State = state_unused;
                    UnusedStreams++;
                }
            }

        private:

            // Methods
            
    };
} // namespace LinuxSampler

#endif // __LS_STREAMBASE_H__
