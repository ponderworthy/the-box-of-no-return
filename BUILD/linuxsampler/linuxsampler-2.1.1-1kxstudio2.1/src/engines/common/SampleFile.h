/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003 - 2009 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2013 Grigor Iliev                                *
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

#ifndef __LS_SAMPLEFILE_H__
#define __LS_SAMPLEFILE_H__

#include "Sample.h"

#include <sndfile.h>
#include "../../common/global.h"

namespace LinuxSampler {
    class SampleFile : public Sample {
        public:
            SampleFile(String File, bool DontClose = false);
            virtual ~SampleFile();

            String GetFile() { return File; }

            virtual String  GetName() { return File; }
            virtual int     GetSampleRate() { return SampleRate; }
            virtual int     GetChannelCount() { return ChannelCount; }
            virtual long    GetTotalFrameCount() { return TotalFrameCount; }
            virtual int     GetFrameSize() { return FrameSize; }
            virtual int     GetLoops() { return Loops; }
            virtual uint    GetLoopStart() { return LoopStart; }
            virtual uint    GetLoopEnd() { return LoopEnd; }

            virtual buffer_t  LoadSampleData();
            virtual buffer_t  LoadSampleData(unsigned long FrameCount);
            virtual buffer_t  LoadSampleDataWithNullSamplesExtension(uint NullFrameCount);
            virtual buffer_t  LoadSampleDataWithNullSamplesExtension(unsigned long FrameCount, uint NullFramesCount);
            virtual void      ReleaseSampleData();
            virtual buffer_t  GetCache();
            virtual long      Read(void* pBuffer, unsigned long FrameCount);

            virtual unsigned long ReadAndLoop (
                void*           pBuffer,
                unsigned long   FrameCount,
                PlaybackState*  pPlaybackState
            );

            virtual long SetPos(unsigned long FrameOffset);
            virtual long GetPos();

            void Open();
            void Close();

        private:
            String File;
            int    SampleRate;
            int    ChannelCount;
            int    Format;
            int    FrameSize;         ///< In bytes
            long   TotalFrameCount;
            int    Loops;
            uint   LoopStart;
            uint   LoopEnd;

            SNDFILE* pSndFile;

            buffer_t RAMCache;        ///< Buffers samples (already uncompressed) in RAM.

            int* pConvertBuffer;

            long SetPos(unsigned long FrameCount, int Whence);
    };

    template <class R>
    class SampleFileBase : public SampleFile {
        public:
            SampleFileBase(String File, bool DontClose = false) : SampleFile(File, DontClose) { }
            virtual ~SampleFileBase() { }



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
            unsigned long ReadAndLoop (
                void*           pBuffer,
                unsigned long   FrameCount,
                PlaybackState*  pPlaybackState,
                R*              pRegion
            ) {
                // TODO: startAddrsCoarseOffset, endAddrsCoarseOffset
                unsigned long samplestoread = FrameCount, totalreadsamples = 0, readsamples, samplestoloopend;
                uint8_t* pDst = (uint8_t*) pBuffer;
                SetPos(pPlaybackState->position);
                if (pRegion->HasLoop()) {
                    do {
                        if (GetPos() > pRegion->GetLoopEnd()) SetPos(pRegion->GetLoopStart());
                        samplestoloopend  = pRegion->GetLoopEnd() - GetPos();
                        readsamples       = Read(&pDst[totalreadsamples * GetFrameSize()], Min(samplestoread, samplestoloopend));
                        samplestoread    -= readsamples;
                        totalreadsamples += readsamples;
                        if (readsamples == samplestoloopend) {
                            SetPos(pRegion->GetLoopStart());
                        }
                    } while (samplestoread && readsamples);
                } else {
                    totalreadsamples = Read(pBuffer, FrameCount);
                }

                pPlaybackState->position = GetPos();

                return totalreadsamples;
            }

        protected:
            inline long Min(long A, long B) { return (A > B) ? B : A; }
    };
} // namespace LinuxSampler

#endif // __LS_SAMPLEFILE_H__
