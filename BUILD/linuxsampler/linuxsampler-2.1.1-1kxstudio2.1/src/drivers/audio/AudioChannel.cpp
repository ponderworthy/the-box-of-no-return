/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
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

#include "AudioChannel.h"

#include "../../common/global_private.h"
#include "../../common/Thread.h" // needed for allocAlignedMem() and freeAlignedMem()

#if defined(__APPLE__)
# include <stdlib.h>
#else
# include <malloc.h>
#endif


namespace LinuxSampler {

    /**
     * Create real channel.
     *
     * @param ChannelNr  - channel number of this new channel
     * @param BufferSize - desired audio data buffer size
     */
    AudioChannel::AudioChannel(uint ChannelNr, uint BufferSize) {
        this->ChannelNr          = ChannelNr;
        this->pBuffer            = (float *) Thread::allocAlignedMem(16,BufferSize*sizeof(float));
        this->uiBufferSize       = BufferSize;
        this->pMixChannel        = NULL;
        this->UsesExternalBuffer = false;

        Parameters["NAME"]           = new ParameterName("Channel " + ToString(ChannelNr));
        Parameters["IS_MIX_CHANNEL"] = new ParameterIsMixChannel(false);

        Clear();
    }

    /**
     * Create channel with external (already existing) audio buffer.
     *
     * @param ChannelNr  - channel number of this new channel
     * @param pBuffer    - external audio buffer
     * @param BufferSize - size of the external buffer
     */
    AudioChannel::AudioChannel(uint ChannelNr, float* pBuffer, uint BufferSize) {
        this->ChannelNr          = ChannelNr;
        this->pBuffer            = pBuffer;
        this->uiBufferSize       = BufferSize;
        this->pMixChannel        = NULL;
        this->UsesExternalBuffer = true;

        Parameters["NAME"]           = new ParameterName("Channel " + ToString(ChannelNr));
        Parameters["IS_MIX_CHANNEL"] = new ParameterIsMixChannel(false);

        Clear();
    }

    /**
     * Create mix channel.
     *
     * @param ChannelNr              - channel number of this new channel
     * @param pMixChannelDestination - a real audio channel this new mix
     *                                 channel refers to
     */
    AudioChannel::AudioChannel(uint ChannelNr, AudioChannel* pMixChannelDestination) {
        this->ChannelNr          = ChannelNr;
        this->pBuffer            = pMixChannelDestination->Buffer();
        this->uiBufferSize       = pMixChannelDestination->uiBufferSize;
        this->pMixChannel        = pMixChannelDestination;
        this->UsesExternalBuffer = true;

        Parameters["NAME"]           = new ParameterName("Channel " + ToString(ChannelNr));
        Parameters["IS_MIX_CHANNEL"] = new ParameterIsMixChannel(true);
        //TODO: Parameters["MIX_CHANNEL_DESTINATION"] = new ParameterMixChannelDestination(dest_chan);

        Clear();
    }

    /**
     * Destructor
     */
    AudioChannel::~AudioChannel() {
        std::map<String,DeviceRuntimeParameter*>::iterator iter = Parameters.begin();
        while (iter != Parameters.end()) { delete iter->second; iter++; }
        if (!UsesExternalBuffer) Thread::freeAlignedMem(pBuffer);
    }

    /**
     * Copies audio data (unmodified) from this AudioChannel to the given
     * destination AudioChannel.
     *
     * @e Caution: This method will overwrite the content in the destination
     * channel buffer.
     *
     * @param pDst    - destination channel
     * @param Samples - amount of sample points to be copied
     */
    void AudioChannel::CopyTo(AudioChannel* pDst, const uint Samples) {
        memcpy((float* __restrict)pDst->Buffer(), (const float* __restrict)Buffer(), Samples * sizeof(float));
    }

    /**
     * Copies audio data from this AudioChannel to the given destination
     * AudioChannel and applies the given volume coefficient to the
     * destination audio signal.
     *
     * @e Caution: This method will overwrite the content in the destination
     * channel buffer.
     *
     * @param pDst    - destination channel
     * @param Samples - amount of sample points to be copied
     * @param fLevel  - volume coefficient to be applied
     */
    void AudioChannel::CopyTo(AudioChannel* pDst, const uint Samples, const float fLevel) {
        if (fLevel == 1.0f) CopyTo(pDst, Samples);
        else {
            const float* __restrict pSrcBuf = Buffer();
            float* __restrict pDstBuf = pDst->Buffer();
            #if HAVE_GCC_VECTOR_EXTENSIONS
            if ((size_t)pSrcBuf % 16 == 0 && (size_t)pDstBuf % 16 == 0) {
                const v4sf vcoeff = { fLevel, fLevel, fLevel, fLevel };
                const v4sf* __restrict src =
                    static_cast<const v4sf* __restrict>(
                        (const void* __restrict)pSrcBuf
                    );
                v4sf* __restrict dst =
                     static_cast<v4sf* __restrict>(
                         (void* __restrict)pDstBuf
                     );
                const int cells = Samples / 4;
                for (int i = 0; i < cells; ++i)
                    dst[i] = src[i] * vcoeff;
            } else {
            #endif
                for (int i = 0; i < Samples; i++)
                    pDstBuf[i] = pSrcBuf[i] * fLevel;
            #if HAVE_GCC_VECTOR_EXTENSIONS
            }
            #endif
        }
    }

    /**
     * Copies audio data (unmodified) from this AudioChannel and mixes it to the
     * given destination AudioChannel.
     *
     * @param pDst    - destination channel
     * @param Samples - amount of sample points to be mixed over
     */
    void AudioChannel::MixTo(AudioChannel* pDst, const uint Samples) {
        const float* __restrict pSrcBuf = Buffer();
        float* __restrict pDstBuf = pDst->Buffer();
        #if HAVE_GCC_VECTOR_EXTENSIONS
        if ((size_t)pSrcBuf % 16 == 0 && (size_t)pDstBuf % 16 == 0) {
            const v4sf* __restrict src =
                static_cast<const v4sf* __restrict>(
                    (const void* __restrict)pSrcBuf
                );
            v4sf* __restrict dst =
                static_cast<v4sf* __restrict>(
                    (void* __restrict)pDstBuf
                );
            const int cells = Samples / 4;
            for (int i = 0; i < cells; ++i)
                dst[i] += src[i];
        } else {
        #endif
            for (int i = 0; i < Samples; i++)
                pDstBuf[i] += pSrcBuf[i];
        #if HAVE_GCC_VECTOR_EXTENSIONS
        }
        #endif
    }

    /**
     * Copies audio data from this AudioChannel, applies the given volume
     * coefficient to the audio signal and mixes it to the given destination
     * channel.
     *
     * @param pDst    - destination channel
     * @param Samples - amount of sample points to be mixed over
     * @param fLevel  - volume coefficient to be applied
     */
    void AudioChannel::MixTo(AudioChannel* pDst, const uint Samples, const float fLevel) {
        if (fLevel == 1.0f) MixTo(pDst, Samples);
        else {
            const float* __restrict pSrcBuf = Buffer();
            float* __restrict pDstBuf = pDst->Buffer();
            #if HAVE_GCC_VECTOR_EXTENSIONS
            if ((size_t)pSrcBuf % 16 == 0 && (size_t)pDstBuf % 16 == 0) {
                const v4sf vcoeff = { fLevel, fLevel, fLevel, fLevel };
                const v4sf* __restrict src =
                    static_cast<const v4sf* __restrict>(
                        (const void* __restrict)pSrcBuf
                    );
                v4sf* __restrict dst =
                    static_cast<v4sf* __restrict>(
                        (void* __restrict)pDstBuf
                    );
                const int cells = Samples / 4;
                for (int i = 0; i < cells; ++i)
                    dst[i] += src[i] * vcoeff;
            } else {
            #endif
                for (int i = 0; i < Samples; i++)
                    pDstBuf[i] += pSrcBuf[i] * fLevel;
            #if HAVE_GCC_VECTOR_EXTENSIONS
            }
            #endif
        }
    }

    std::map<String,DeviceRuntimeParameter*> AudioChannel::ChannelParameters() {
        return Parameters;
    }
}
