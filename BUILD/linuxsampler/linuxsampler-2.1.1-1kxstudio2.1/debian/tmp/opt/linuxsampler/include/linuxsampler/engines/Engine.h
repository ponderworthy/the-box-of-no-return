/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2013 Christian Schoenebeck                         *
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

#ifndef __LS_ENGINE_H__
#define __LS_ENGINE_H__

#include "../common/global.h"
#include "InstrumentManager.h"

namespace LinuxSampler {

    // just symbol prototyping
    class MidiInputPort;

    /** @brief LinuxSampler Sampler Engine Interface
     *
     * Abstract base interface class for all LinuxSampler engines which
     * defines all mandatory methods which have to be implemented by all
     * sampler engine implementations.
     */
    class Engine {
        public:

            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            virtual int    RenderAudio(uint Samples) = 0;
            virtual void   SendSysex(void* pData, uint Size, MidiInputPort* pSender) = 0;
            virtual void   Reset() = 0;
            virtual void   Enable() = 0;
            virtual void   Disable() = 0;
            virtual uint   VoiceCount() = 0;
            virtual uint   VoiceCountMax() = 0;
            virtual int    MaxVoices() = 0;
            virtual void   SetMaxVoices(int iVoices) throw (Exception) = 0;
            virtual bool   DiskStreamSupported() = 0;
            virtual uint   DiskStreamCount() = 0;
            virtual uint   DiskStreamCountMax() = 0;
            virtual int    MaxDiskStreams() = 0;
            virtual void   SetMaxDiskStreams(int iStreams) throw (Exception) = 0;
            virtual String DiskStreamBufferFillBytes() = 0;
            virtual String DiskStreamBufferFillPercentage() = 0;
            virtual String Description() = 0;
            virtual String Version() = 0;
            virtual String EngineName() = 0;

            /**
             * Returns pointer to the Engine's InstrumentManager or NULL if
             * the Engine does not provide an InstrumentManager.
             *
             * <b>Important:</b> All engine instances of the same engine
             * type have to return the same InstrumentManager, that is all
             * instances of the same engine type have to share one and
             * the same InstrumentManager object.
             */
            virtual InstrumentManager* GetInstrumentManager() = 0;
        
            /**
             * Will be called by audio output drivers in case some
             * fundamental audio driver parameter like sample rate or
             * max. samples per cycle changed.
             */
            virtual void ReconnectAudioOutputDevice() = 0;
            
            /**
             * Modifies the scale tuning from standard well tempered chromatic
             * scaling to any other kind of scaling.
             * 
             * ScaleTunes - detune factors (-64..+63 cents) for all 12
             *              semitones of an octave
             */
            virtual void AdjustScaleTuning(const int8_t ScaleTunes[12]) = 0;
            
            /**
             * Expects a byte array with 12 elements as argument @a pScaleTunes,
             * where the currently effectve scale tuning setup is written to.
             *
             * pScaleTunes - output: detune factors (-64..+63 cents) for all 12
             *               semitones of an octave
             */
            virtual void GetScaleTuning(int8_t* pScaleTunes) = 0;
            
            /**
             * Reset to standard well tempered chromatic scaling, i.e. after
             * being altered with AdjustScaleTuning() or after having sent the
             * respective standard scale tuning SysEx MIDI message.
             */
            virtual void ResetScaleTuning() = 0;

        protected:
            virtual ~Engine() {}; // MUST only be destroyed by EngineFactory
            void Unregister();    // Remove self from EngineFactory.
            friend class EngineFactory;
    };

} // namespace LinuxSampler

#endif // __LS_ENGINE_H__
