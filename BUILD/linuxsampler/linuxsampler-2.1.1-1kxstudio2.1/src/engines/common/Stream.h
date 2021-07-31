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

#ifndef __LS_STREAM_H__
#define __LS_STREAM_H__

#include "../../common/global.h"
#include "../../common/RingBuffer.h"
#include "Sample.h"

namespace LinuxSampler {

    /** @brief Buffered Disk Stream
     *
     * This encapsulation of a disk stream uses a ring buffer to allow
     * thread safe refilling the stream's buffer with one thread (disk
     * thread) and actual use / extraction of the audio data from the
     * stream's buffer with another thread (audio thread).
     */
    class Stream {
        public:
            // Member Types
            typedef uint32_t OrderID_t;
            typedef uint32_t Handle; ///< unique identifier of a relationship between one stream and a consumer (Voice)
            enum { INVALID_HANDLE = 0 };
            enum state_t {           ///< streams go through severe cyclic state transition (unused->active->end->unused->...)
                state_unused,        ///< stream is not in use, thus can still be launched
                state_active,        ///< stream provides data in it's buffer to be read and hasn't reached the end yet (this is the usual case)
                state_end            ///< stream end reached but still providing data in it's buffer to be read (after the client read all remaining data from the stream buffer, state will change automatically to state_unused)
            };
            struct reference_t {     ///< Defines the current relationship between the stream and a client (voice).
                OrderID_t  OrderID;   ///< Unique identifier that identifies the creation order of a stream requested by a voice.
                Handle     hStream;   ///< Unique identifier of the relationship between stream and client.
                state_t    State;     ///< Current state of the stream that will be pretended to the client (the actual state of the stream might differ though, because the stream might already be in use by another client).
                Stream*    pStream;   ///< Points to the assigned and activated stream or is NULL if the disk thread hasn't launched a stream yet.
            };

            class SampleDescription {
                public:
                    int FrameSize;         ///< Reflects the size (in bytes) of one single sample point
                    int ChannelsPerFrame;
                    int BytesPerSample;    ///< Size of each sample per channel
                    int TotalSampleCount;
            };

            Stream(uint BufferSize, uint BufferWrapElements) {
                this->pExportReference       = NULL;
                this->State                  = state_unused;
                this->hThis                  = 0;
                this->PlaybackState.position = 0;
                this->PlaybackState.reverse  = false;
                this->pRingBuffer            = new RingBuffer<uint8_t,false>(BufferSize * 3, BufferWrapElements * 3);
                UnusedStreams++;
                TotalStreams++;
            }

            virtual ~Stream() { }

            // Methods
            inline int GetReadSpace() {
                return (pRingBuffer && State != state_unused) ? pRingBuffer->read_space() / SampleInfo.BytesPerSample : 0;
            }

            inline int GetWriteSpace() {
                return (pRingBuffer && State == state_active) ? pRingBuffer->write_space() / SampleInfo.BytesPerSample : 0;
            }

            inline int GetWriteSpaceToEnd() {
                return (pRingBuffer && State == state_active) ? pRingBuffer->write_space_to_end_with_wrap() / SampleInfo.BytesPerSample : 0;
            }

            // adjusts the write space to avoid buffer boundaries which would lead to the wrap space
            // within the buffer (needed for interpolation) getting filled only partially
            // for more infos see the docs in ringbuffer.h at adjust_write_space_to_avoid_boundary()
            inline int AdjustWriteSpaceToAvoidBoundary(int cnt, int capped_cnt) {
              return pRingBuffer->adjust_write_space_to_avoid_boundary (cnt * SampleInfo.BytesPerSample, capped_cnt * SampleInfo.BytesPerSample) / SampleInfo.BytesPerSample;
            }

            // gets the current read_ptr within the ringbuffer
            inline uint8_t* GetReadPtr(void) {
                return pRingBuffer->get_read_ptr();
            }

            inline void IncrementReadPos(uint Count)  {
                Count *= SampleInfo.BytesPerSample;
                uint leftspace = pRingBuffer->read_space();
                pRingBuffer->increment_read_ptr((int)Min(Count, leftspace));
                if (State == state_end && Count >= leftspace) {
                    Reset(); // quit relation between consumer (voice) and stream and reset stream right after
                }
            }

            virtual int  ReadAhead(unsigned long SampleCount) = 0;
            virtual void WriteSilence(unsigned long SilenceSampleWords) = 0;

            // Static Method
            inline static uint       GetUnusedStreams() { return UnusedStreams; }

            template<class R, class IM> friend class DiskThreadBase; // only the disk thread should be able to launch and most important kill a disk stream to avoid race conditions

        protected:
            // Attributes
            RingBuffer<uint8_t,false>*  pRingBuffer;
            SampleDescription           SampleInfo;
            Sample::PlaybackState       PlaybackState;
            reference_t*                pExportReference;
            state_t                     State;
            Handle                      hThis;

            // Static Attributes
            static uint UnusedStreams; //< Reflects how many stream objects of all stream instances are currently not in use.
            static uint TotalStreams; //< Reflects how many stream objects currently exist.

            // Methods

            virtual void   Kill()      { pExportReference = NULL; Reset(); } ///< Will be called by disk thread after a 'deletion' command from the audio thread (within the voice class)
            inline Handle  GetHandle() { return hThis; }
            inline state_t GetState()  { return State; }

            inline void SetState(state_t State) {
                if (pExportReference) pExportReference->State = State;
                this->State = State;
            }

            virtual long Read(uint8_t* pBuf, long SamplesToRead) = 0;
            virtual void Reset() = 0;

        private:

            // Methods
            inline long Min(long a, long b) { return (a < b) ? a : b; }
    };
} // namespace LinuxSampler

#endif // __LS_STREAM_H__
