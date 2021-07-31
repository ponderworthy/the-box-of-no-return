/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003 - 2009 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2014 Grigor Iliev                                *
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

#include "SampleFile.h"
#include "../../common/global_private.h"
#include "../../common/Exception.h"

#include <cstring>

#define CONVERT_BUFFER_SIZE 4096

namespace LinuxSampler {
    #if CONFIG_DEVMODE
    int SampleFile_OpenFilesCount = 0;
    #endif

    SampleFile::SampleFile(String File, bool DontClose) {
        this->File      = File;
        this->pSndFile  = NULL;
        pConvertBuffer  = NULL;

        SF_INFO sfInfo;
        sfInfo.format = 0;
        pSndFile = sf_open(File.c_str(), SFM_READ, &sfInfo);
        if(pSndFile == NULL) throw Exception(File + ": Can't get sample info: " + String(sf_strerror (NULL)));
        #if CONFIG_DEVMODE
        std::cout << "Number of opened sample files: " << ++SampleFile_OpenFilesCount << std::endl;
        #endif
        SampleRate = sfInfo.samplerate;
        ChannelCount = sfInfo.channels;
        Format = sfInfo.format;

        switch(Format & SF_FORMAT_SUBMASK) {
            case SF_FORMAT_PCM_S8:
            case SF_FORMAT_PCM_U8:
            case SF_FORMAT_DPCM_8:
                FrameSize = ChannelCount;
                break;
            case SF_FORMAT_PCM_16:
            case SF_FORMAT_DPCM_16:
                FrameSize = 2 * ChannelCount;
                break;
            case SF_FORMAT_PCM_24:
            case SF_FORMAT_DWVW_24:
            case SF_FORMAT_PCM_32:
            case SF_FORMAT_FLOAT:
                FrameSize = 3 * ChannelCount;
                break;
            default:
                FrameSize = 2 * ChannelCount;
        }
        TotalFrameCount = sfInfo.frames;

        Loops = 0;
        LoopStart = 0;
        LoopEnd = 0;
        SF_INSTRUMENT instrument;
        if (sf_command(pSndFile, SFC_GET_INSTRUMENT,
                       &instrument, sizeof(instrument)) != SF_FALSE) {
            // TODO: instrument.basenote
#if HAVE_SF_INSTRUMENT_LOOPS
            if (instrument.loop_count && instrument.loops[0].mode != SF_LOOP_NONE) {
                Loops = 1;
                LoopStart = instrument.loops[0].start;
                LoopEnd = instrument.loops[0].end;
            }
#endif
        }
        if(!DontClose) Close();

        if (FrameSize == 3 * ChannelCount && (
#if HAVE_DECL_SF_FORMAT_FLAC
                (Format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC ||
#endif
                (Format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT ||
                (Format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_32)) {
            pConvertBuffer = new int[CONVERT_BUFFER_SIZE];
        }
    }

    SampleFile::~SampleFile() {
        Close();
        ReleaseSampleData();
        delete[] pConvertBuffer;
    }

    void SampleFile::Open() {
        if(pSndFile) return; // Already opened
        SF_INFO sfInfo;
        sfInfo.format = 0;
        pSndFile = sf_open(File.c_str(), SFM_READ, &sfInfo);
        if(pSndFile == NULL) throw Exception(File + ": Can't load sample");
        #if CONFIG_DEVMODE
        std::cout << "Number of opened sample files: " << ++SampleFile_OpenFilesCount << std::endl;
        #endif
    }

    void SampleFile::Close() {
        if(pSndFile == NULL) return;
        if(sf_close(pSndFile)) std::cerr << "Sample::Close() " << "Failed to close " << File << std::endl;
        pSndFile = NULL;
        #if CONFIG_DEVMODE
        std::cout << "Number of opened sample files: " << --SampleFile_OpenFilesCount << std::endl;
        #endif
    }

    long SampleFile::SetPos(unsigned long FrameOffset) {
        return SetPos(FrameOffset, SEEK_SET);
    }

    long SampleFile::SetPos(unsigned long FrameCount, int Whence) {
        if(pSndFile == NULL) {
            std::cerr << "Sample::SetPos() " << File << " not opened" << std::endl;
            return -1;
        }

        return sf_seek(pSndFile, FrameCount, Whence);
    }

    long SampleFile::GetPos() {
        if(pSndFile == NULL) {
            std::cerr << "Sample::GetPos() " << File << " not opened" << std::endl;
            return -1;
        }

        return sf_seek(pSndFile, 0, SEEK_CUR);
    }

    Sample::buffer_t SampleFile::LoadSampleData() {
        return LoadSampleDataWithNullSamplesExtension(GetTotalFrameCount(), 0); // 0 amount of NullSamples
    }

    Sample::buffer_t SampleFile::LoadSampleData(unsigned long FrameCount) {
        return LoadSampleDataWithNullSamplesExtension(FrameCount, 0); // 0 amount of NullSamples
    }

    Sample::buffer_t SampleFile::LoadSampleDataWithNullSamplesExtension(uint NullFrameCount) {
        return LoadSampleDataWithNullSamplesExtension(GetTotalFrameCount(), NullFrameCount);
    }

    Sample::buffer_t SampleFile::LoadSampleDataWithNullSamplesExtension(unsigned long FrameCount, uint NullFramesCount) {
        Open();
        if (FrameCount > GetTotalFrameCount()) FrameCount = GetTotalFrameCount();
        
        if (Offset > MaxOffset && FrameCount < GetTotalFrameCount()) {
            FrameCount = FrameCount + Offset > GetTotalFrameCount() ? GetTotalFrameCount() - Offset : FrameCount;
            // Offset the RAM cache
            RAMCacheOffset = Offset;
        }
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        unsigned long allocationsize = (FrameCount + NullFramesCount) * this->FrameSize;
        SetPos(RAMCacheOffset, SEEK_SET); // reset read position to playback start point
        RAMCache.pStart            = new int8_t[allocationsize];

        RAMCache.Size = Read(RAMCache.pStart, FrameCount) * this->FrameSize;
        RAMCache.NullExtensionSize = allocationsize - RAMCache.Size;
        // fill the remaining buffer space with silence samples
        memset((int8_t*)RAMCache.pStart + RAMCache.Size, 0, RAMCache.NullExtensionSize);
        Close();
        return GetCache();
    }

    long SampleFile::Read(void* pBuffer, unsigned long FrameCount) {
        Open();
        
        if (GetPos() + FrameCount > GetTotalFrameCount()) FrameCount = GetTotalFrameCount() - GetPos(); // For the cases where a different sample end is specified (not the end of the file)

        // ogg and flac files must be read with sf_readf, not
        // sf_read_raw. On big endian machines, sf_readf_short is also
        // used for 16 bit wav files, to get automatic endian
        // conversion (for 24 bit samples this is handled in
        // Synthesize::GetSample instead).

#if WORDS_BIGENDIAN || HAVE_DECL_SF_FORMAT_VORBIS || HAVE_DECL_SF_FORMAT_FLAC
        if (
#if WORDS_BIGENDIAN
            FrameSize == 2 * ChannelCount
#else
#if HAVE_DECL_SF_FORMAT_VORBIS
            ((Format & SF_FORMAT_SUBMASK) == SF_FORMAT_VORBIS)
#if HAVE_DECL_SF_FORMAT_FLAC
            ||
#endif
#endif
#if HAVE_DECL_SF_FORMAT_FLAC
            (FrameSize == 2 * ChannelCount &&
             (Format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC)
#endif
#endif
            ) {
            return sf_readf_short(pSndFile, static_cast<short*>(pBuffer), FrameCount);
        } else if (FrameSize == 3 * ChannelCount && (
#if HAVE_DECL_SF_FORMAT_FLAC
                       (Format & SF_FORMAT_TYPEMASK) == SF_FORMAT_FLAC ||
#endif
                       (Format & SF_FORMAT_SUBMASK) == SF_FORMAT_FLOAT ||
                       (Format & SF_FORMAT_SUBMASK) == SF_FORMAT_PCM_32)) {
            // 24 bit flac needs to be converted from the 32 bit
            // integers returned by libsndfile. Float and 32 bit pcm
            // are treated in the same way.
            int j = 0;
            sf_count_t count = FrameCount;
            const sf_count_t bufsize = CONVERT_BUFFER_SIZE / ChannelCount;
            unsigned char* const dst = static_cast<unsigned char*>(pBuffer);
            while (count > 0) {
                int n = (int) sf_readf_int(pSndFile, pConvertBuffer, std::min(count, bufsize));
                if (n <= 0) break;
                for (int i = 0 ; i < n * ChannelCount ; i++) {
                    dst[j++] = pConvertBuffer[i] >> 8;
                    dst[j++] = pConvertBuffer[i] >> 16;
                    dst[j++] = pConvertBuffer[i] >> 24;
                }
                count -= n;
            }
            return FrameCount - count;
        } else
#endif
        {
            int bytes = (int)sf_read_raw(pSndFile, pBuffer, FrameCount * GetFrameSize());
            return bytes / GetFrameSize();
        }
    }

    unsigned long SampleFile::ReadAndLoop (
        void*           pBuffer,
        unsigned long   FrameCount,
        PlaybackState*  pPlaybackState
    ) {
        // TODO:
        SetPos(pPlaybackState->position);
        unsigned long count = Read(pBuffer, FrameCount);
        pPlaybackState->position = GetPos();
        return count;
    }

    void SampleFile::ReleaseSampleData() {
        if (RAMCache.pStart) delete[] (int8_t*) RAMCache.pStart;
        RAMCache.pStart = NULL;
        RAMCache.Size   = 0;
        RAMCache.NullExtensionSize = 0;
    }

    Sample::buffer_t SampleFile::GetCache() {
        // return a copy of the buffer_t structure
        buffer_t result;
        result.Size              = this->RAMCache.Size;
        result.pStart            = this->RAMCache.pStart;
        result.NullExtensionSize = this->RAMCache.NullExtensionSize;
        return result;
    }
} // namespace LinuxSampler
