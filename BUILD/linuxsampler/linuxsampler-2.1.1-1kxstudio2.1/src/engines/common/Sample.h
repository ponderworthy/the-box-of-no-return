/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003 - 2016 Christian Schoenebeck                       *
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

#ifndef __LS_SAMPLE_H__
#define __LS_SAMPLE_H__

#include "../../common/global.h"

namespace LinuxSampler {
    class Sample {
        public:

            /** Pointer address and size of a buffer. */
            struct buffer_t {
                void*         pStart;            ///< Points to the beginning of the buffer.
                unsigned long Size;              ///< Size of the actual data in the buffer in bytes.

                unsigned long NullExtensionSize; /*/< The buffer might be bigger than the actual data, if that's the case
                              that unused space at the end of the buffer is filled with NULLs and NullExtensionSize reflects
                              that unused buffer space in bytes. Those NULL extensions are mandatory for differential
                              algorithms that have to take the following data words into account, thus have to access past
                              the buffer's boundary. If you don't know what I'm talking about, just forget this variable. :) */
                buffer_t() {
                    pStart            = NULL;
                    Size              = 0;
                    NullExtensionSize = 0;
                }
            };

            /** Reflects the current playback state for a sample. */
            class PlaybackState {
                public:
                    unsigned long position;          ///< Current position within the sample.
                    bool          reverse;           ///< If playback direction is currently backwards (in case there is a pingpong or reverse loop defined).
                    unsigned long loop_cycles_left;  ///< How many times the loop has still to be passed, this value will be decremented with each loop cycle.
            };

            uint Offset; // The offset used to play the sample (in sample units)

            uint RAMCacheOffset; // The offset of the RAM cache from the sample start (in sample units)

             /*
              * Specifies the maximum offset (in frames) that can be set without
              * the need to offset the RAM cache.
              */
            uint MaxOffset;

            Sample(): Offset(0), RAMCacheOffset(0), MaxOffset(2000) { }
            virtual ~Sample() { }

            virtual String  GetName() = 0;
            virtual int     GetSampleRate() = 0;
            virtual int     GetChannelCount() = 0;

            /**
             * @returns The frame size in bytes
             */
            virtual int  GetFrameSize() = 0;

            /**
             * @returns The total number of frames in this sample
             */
            virtual long GetTotalFrameCount() = 0;

            /**
             * Loads (and uncompresses if needed) the whole sample wave into RAM. Use
             * ReleaseSampleData() to free the memory if you don't need the cached
             * sample data anymore.
             *
             * @returns  buffer_t structure with start address and size of the buffer
             *           in bytes
             * @see      ReleaseSampleData(), Read(), SetPos()
             */
            virtual buffer_t LoadSampleData() = 0;

            /**
             * Reads (uncompresses if needed) and caches the first \a FrameCount
             * numbers of SamplePoints in RAM. Use ReleaseSampleData() to free the
             * memory space if you don't need the cached samples anymore. There is no
             * guarantee that exactly \a SampleCount samples will be cached; this is
             * not an error. The size will be eventually truncated e.g. to the
             * beginning of a frame of a compressed sample. This is done for
             * efficiency reasons while streaming the wave by your sampler engine
             * later. Read the <i>Size</i> member of the <i>buffer_t</i> structure
             * that will be returned to determine the actual cached samples, but note
             * that the size is given in bytes! You get the number of actually cached
             * samples by dividing it by the frame size of the sample:
             * @code
             * 	buffer_t buf       = pSample->LoadSampleData(acquired_samples);
             * 	long cachedsamples = buf.Size / pSample->FrameSize;
             * @endcode
             *
             * @param FrameCount  - number of sample points to load into RAM
             * @returns             buffer_t structure with start address and size of
             *                      the cached sample data in bytes
             * @see                 ReleaseSampleData(), Read(), SetPos()
             */
            virtual buffer_t  LoadSampleData(unsigned long FrameCount) = 0;

            /**
             * Loads (and uncompresses if needed) the whole sample wave into RAM. Use
             * ReleaseSampleData() to free the memory if you don't need the cached
             * sample data anymore.
             * The method will add \a NullSamplesCount silence samples past the
             * official buffer end (this won't affect the 'Size' member of the
             * buffer_t structure, that means 'Size' always reflects the size of the
             * actual sample data, the buffer might be bigger though). Silence
             * samples past the official buffer are needed for differential
             * algorithms that always have to take subsequent samples into account
             * (resampling/interpolation would be an important example) and avoids
             * memory access faults in such cases.
             *
             * @param NullSamplesCount - number of silence samples the buffer should
             *                           be extended past it's data end
             * @returns                  buffer_t structure with start address and
             *                           size of the buffer in bytes
             * @see                      ReleaseSampleData(), Read(), SetPos()
             */
            virtual buffer_t  LoadSampleDataWithNullSamplesExtension(uint NullFrameCount) = 0;

            /**
             * Reads (uncompresses if needed) and caches the first \a SampleCount
             * numbers of SamplePoints in RAM. Use ReleaseSampleData() to free the
             * memory space if you don't need the cached samples anymore. There is no
             * guarantee that exactly \a SampleCount samples will be cached; this is
             * not an error. The size will be eventually truncated e.g. to the
             * beginning of a frame of a compressed sample. This is done for
             * efficiency reasons while streaming the wave by your sampler engine
             * later. Read the <i>Size</i> member of the <i>buffer_t</i> structure
             * that will be returned to determine the actual cached samples, but note
             * that the size is given in bytes! You get the number of actually cached
             * samples by dividing it by the frame size of the sample:
             * @code
             * 	buffer_t buf       = pSample->LoadSampleDataWithNullSamplesExtension(acquired_samples, null_samples);
             * 	long cachedsamples = buf.Size / pSample->FrameSize;
             * @endcode
             * The method will add \a NullSamplesCount silence samples past the
             * official buffer end (this won't affect the 'Size' member of the
             * buffer_t structure, that means 'Size' always reflects the size of the
             * actual sample data, the buffer might be bigger though). Silence
             * samples past the official buffer are needed for differential
             * algorithms that always have to take subsequent samples into account
             * (resampling/interpolation would be an important example) and avoids
             * memory access faults in such cases.
             *
             * @param FrameCount      - number of sample points to load into RAM
             * @param NullFramesCount - number of silence samples the buffer should
             *                          be extended past it's data end
             * @returns                 buffer_t structure with start address and
             *                          size of the cached sample data in bytes
             * @see                     ReleaseSampleData(), Read()
             */
            virtual buffer_t  LoadSampleDataWithNullSamplesExtension(unsigned long FrameCount, uint NullFramesCount) = 0;

            /**
             * Frees the cached sample from RAM if loaded with
             * <i>LoadSampleData()</i> previously.
             *
             * @see  LoadSampleData();
             */
            virtual void ReleaseSampleData() = 0;

            /**
             * Returns current cached sample points. A buffer_t structure will be
             * returned which contains address pointer to the begin of the cache and
             * the size of the cached sample data in bytes. Use
             * <i>LoadSampleData()</i> to cache a specific amount of sample points in
             * RAM.
             *
             * @returns  buffer_t structure with current cached sample points
             * @see      LoadSampleData();
             */
            virtual buffer_t  GetCache() = 0;

            /**
             * Reads \a FrameCount number of frames from the current
             * position into the buffer pointed by \a pBuffer and increments the
             * position within the sample. Use this method
             * and <i>SetPos()</i> if you don't want to load the sample into RAM,
             * thus for disk streaming.
             *
             * For 16 bit samples, the data in the buffer will be int16_t
             * (using native endianness). For 24 bit, the buffer will
             * contain 4 bytes per sample.
             *
             * @param pBuffer      destination buffer
             * @param SampleCount  number of sample points to read
             * @returns            number of successfully read sample points
             */
            virtual long Read(void* pBuffer, unsigned long FrameCount) = 0;

            /**
             * Reads \a SampleCount number of sample points from the position stored
             * in \a pPlaybackState into the buffer pointed by \a pBuffer and moves
             * the position within the sample respectively, this method honors the
             * looping informations of the sample (if any). Use this
             * method if you don't want to load the sample into RAM, thus for disk
             * streaming. All this methods needs to know to proceed with streaming
             * for the next time you call this method is stored in \a pPlaybackState.
             * You have to allocate and initialize the playback_state_t structure by
             * yourself before you use it to stream a sample:
             * @code
             * PlaybackState playbackstate;
             * playbackstate.position         = 0;
             * playbackstate.reverse          = false;
             * playbackstate.loop_cycles_left = pSample->LoopPlayCount;
             * @endcode
             * You don't have to take care of things like if there is actually a loop
             * defined or if the current read position is located within a loop area.
             * The method already handles such cases by itself.
             *
             * @param pBuffer          destination buffer
             * @param FrameCount       number of sample points to read
             * @param pPlaybackState   will be used to store and reload the playback
             *                         state for the next ReadAndLoop() call
             * @returns                number of successfully read sample points
             */
            virtual unsigned long ReadAndLoop (
                void*           pBuffer,
                unsigned long   FrameCount,
                PlaybackState*  pPlaybackState
            ) = 0;

            virtual long SetPos(unsigned long FrameOffset) = 0;
            virtual long GetPos() = 0;
    };
} // namespace LinuxSampler

#endif // __LS_SAMPLE_H__
