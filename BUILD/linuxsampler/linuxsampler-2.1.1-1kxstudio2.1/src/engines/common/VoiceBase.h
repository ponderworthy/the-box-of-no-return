/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2011 Christian Schoenebeck and Grigor Iliev        *
 *   Copyright (C) 2012-2016 Christian Schoenebeck                         *
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

#ifndef __LS_VOICEBASE_H__
#define	__LS_VOICEBASE_H__

#include "AbstractVoice.h"

namespace LinuxSampler {

    template <class EC /* Engine Channel */, class R /* Region */, class S /* Sample */, class D /* DiskThread */>
    class VoiceBase : public AbstractVoice {
        public:
            D*   pDiskThread;  ///< Pointer to the disk thread, to be able to order a disk stream and later to delete the stream again
            int  RealSampleWordsLeftToRead; ///< Number of samples left to read, not including the silence added for the interpolator

            VoiceBase(SignalUnitRack* pRack = NULL): AbstractVoice(pRack) {
                pRegion      = NULL;
                pDiskThread  = NULL;
            }
            virtual ~VoiceBase() { }

            virtual R* GetRegion() { return pRegion; }

            virtual unsigned long GetSampleCacheSize() {
                return pSample->GetCache().Size;
            }

            /**
             *  Initializes and triggers the voice, a disk stream will be launched if
             *  needed.
             *
             *  @param pEngineChannel - engine channel on which this voice was ordered
             *  @param itNoteOnEvent  - event that caused triggering of this voice
             *  @param PitchBend      - MIDI detune factor (-8192 ... +8191)
             *  @param pRegion        - points to the region which provides sample wave(s) and articulation data
             *  @param VoiceType      - type of this voice
             *  @param iKeyGroup      - a value > 0 defines a key group in which this voice is member of
             *  @returns 0 on success, a value < 0 if the voice wasn't triggered
             *           (either due to an error or e.g. because no region is
             *           defined for the given key)
             */
            virtual int Trigger (
                AbstractEngineChannel*  pEngineChannel,
                Pool<Event>::Iterator&  itNoteOnEvent,
                int                     PitchBend,
                R*                      pRegion,
                type_t                  VoiceType,
                int                     iKeyGroup
            ) {
                this->pRegion = pRegion;
                this->pSample = pRegion->pSample; // sample won't change until the voice is finished

                return AbstractVoice::Trigger (
                    pEngineChannel, itNoteOnEvent, PitchBend, VoiceType, iKeyGroup
                );
            }

            virtual int OrderNewStream() {
                int res = pDiskThread->OrderNewStream (
                    &DiskStreamRef, pRegion, MaxRAMPos + GetRAMCacheOffset(), !RAMLoop
                );

                if (res < 0) {
                    dmsg(1,("Disk stream order failed!\n"));
                    KillImmediately();
                    return -1;
                }

                return 0;
            }
            
            /** The offset of the RAM cache from the sample start (in sample units). */
            virtual int GetRAMCacheOffset() { return 0; }

            /**
             *  Renders the audio data for this voice for the current audio fragment.
             *  The sample input data can either come from RAM (cached sample or sample
             *  part) or directly from disk. The output signal will be rendered by
             *  resampling / interpolation. If this voice is a disk streaming voice and
             *  the voice completely played back the cached RAM part of the sample, it
             *  will automatically switch to disk playback for the next RenderAudio()
             *  call.
             *
             *  @param Samples - number of samples to be rendered in this audio fragment cycle
             */
            void Render(uint Samples) {
                // select default values for synthesis mode bits
                SYNTHESIS_MODE_SET_LOOP(SynthesisMode, false);

                switch (this->PlaybackState) {

                    case Voice::playback_state_init:
                        this->PlaybackState = Voice::playback_state_ram; // we always start playback from RAM cache and switch then to disk if needed
                        // no break - continue with playback_state_ram

                    case Voice::playback_state_ram: {
                            if (RAMLoop) SYNTHESIS_MODE_SET_LOOP(SynthesisMode, true); // enable looping

                            // render current fragment
                            Synthesize(Samples, (sample_t*) pSample->GetCache().pStart, Delay);

                            if (DiskVoice) {
                                // check if we reached the allowed limit of the sample RAM cache
                                if (finalSynthesisParameters.dPos > MaxRAMPos) {
                                    dmsg(5,("VoiceBase: switching to disk playback (Pos=%f)\n", finalSynthesisParameters.dPos));
                                    this->PlaybackState = Voice::playback_state_disk;
                                }
                            } else if (finalSynthesisParameters.dPos >= pSample->GetCache().Size / SmplInfo.FrameSize) {
                                this->PlaybackState = Voice::playback_state_end;
                            }
                        }
                        break;

                    case Voice::playback_state_disk: {
                            if (!DiskStreamRef.pStream) {
                                // check if the disk thread created our ordered disk stream in the meantime
                                DiskStreamRef.pStream = pDiskThread->AskForCreatedStream(DiskStreamRef.OrderID);
                                if (!DiskStreamRef.pStream) {
                                    std::cerr << "Disk stream not available in time!\n" << std::flush;
                                    KillImmediately();
                                    return;
                                }
                                DiskStreamRef.pStream->IncrementReadPos(uint(
                                    SmplInfo.ChannelCount * (int(finalSynthesisParameters.dPos) - MaxRAMPos)
                                ));
                                finalSynthesisParameters.dPos -= int(finalSynthesisParameters.dPos);
                                RealSampleWordsLeftToRead = -1; // -1 means no silence has been added yet
                            }

                            const int sampleWordsLeftToRead = DiskStreamRef.pStream->GetReadSpace();

                            // add silence sample at the end if we reached the end of the stream (for the interpolator)
                            if (DiskStreamRef.State == Stream::state_end) {
                                const int maxSampleWordsPerCycle = (GetEngine()->MaxSamplesPerCycle << CONFIG_MAX_PITCH) * SmplInfo.ChannelCount + 6; // +6 for the interpolator algorithm
                                if (sampleWordsLeftToRead <= maxSampleWordsPerCycle) {
                                    // remember how many sample words there are before any silence has been added
                                    if (RealSampleWordsLeftToRead < 0) RealSampleWordsLeftToRead = sampleWordsLeftToRead;
                                    DiskStreamRef.pStream->WriteSilence(maxSampleWordsPerCycle - sampleWordsLeftToRead);
                                }
                            }

                            sample_t* ptr = (sample_t*)DiskStreamRef.pStream->GetReadPtr(); // get the current read_ptr within the ringbuffer where we read the samples from

                            // render current audio fragment
                            Synthesize(Samples, ptr, Delay);

                            const int iPos = (int) finalSynthesisParameters.dPos;
                            const int readSampleWords = iPos * SmplInfo.ChannelCount; // amount of sample words actually been read
                            DiskStreamRef.pStream->IncrementReadPos(readSampleWords);
                            finalSynthesisParameters.dPos -= iPos; // just keep fractional part of playback position

                            // change state of voice to 'end' if we really reached the end of the sample data
                            if (RealSampleWordsLeftToRead >= 0) {
                                RealSampleWordsLeftToRead -= readSampleWords;
                                if (RealSampleWordsLeftToRead <= 0) this->PlaybackState = Voice::playback_state_end;
                            }
                        }
                        break;

                    case Voice::playback_state_end:
                        std::cerr << "VoiceBase::Render(): entered with playback_state_end, this is a bug!\n" << std::flush;
                        break;
                }

                // Reset delay
                Delay = 0;

                itTriggerEvent = Pool<Event>::Iterator();

                // If sample stream or release stage finished, kill the voice
                if (PlaybackState == Voice::playback_state_end || EG1Finished()) {
                    KillImmediately();
                }
            }

            /**
             *  Immediately kill the voice. This method should not be used to kill
             *  a normal, active voice, because it doesn't take care of things like
             *  fading down the volume level to avoid clicks and regular processing
             *  until the kill event actually occured!
             *
             * If it's necessary to know when the voice's disk stream was actually
             * deleted, then one can set the optional @a bRequestNotification
             * parameter and this method will then return the handle of the disk
             * stream (unique identifier) and one can use this handle to poll the
             * disk thread if this stream has been deleted. In any case this method
             * will return immediately and will not block until the stream actually
             * was deleted.
             *
             * @param bRequestNotification - (optional) whether the disk thread shall
             *                                provide a notification once it deleted
             *                               the respective disk stream
             *                               (default=false)
             * @returns handle to the voice's disk stream or @c Stream::INVALID_HANDLE
             *          if the voice did not use a disk stream at all
             * @see Kill()
             */
            Stream::Handle KillImmediately(bool bRequestNotification = false) {
                Stream::Handle hStream = Stream::INVALID_HANDLE;
                if (DiskVoice && DiskStreamRef.State != Stream::state_unused) {
                    pDiskThread->OrderDeletionOfStream(&DiskStreamRef, bRequestNotification);
                    hStream = DiskStreamRef.hStream;
                }
                Reset();
                return hStream;
            }

        protected:
            S*  pSample;   ///< Pointer to the sample to be played back
            R*  pRegion;   ///< Pointer to the articulation information of current region of this voice

            virtual MidiKeyBase* GetMidiKeyInfo(int MIDIKey) {
                EC* pChannel = static_cast<EC*>(pEngineChannel);
                return &pChannel->pMIDIKeyInfo[MIDIKey];
            }

            virtual unsigned long GetNoteOnTime(int MIDIKey) {
                EC* pChannel = static_cast<EC*>(pEngineChannel);
                return pChannel->pMIDIKeyInfo[MIDIKey].NoteOnTime;
            }

            void GetFirstEventOnKey(uint8_t MIDIKey, RTList<Event>::Iterator& itEvent) {
                EC* pChannel = static_cast<EC*>(pEngineChannel);
                itEvent = pChannel->pMIDIKeyInfo[MIDIKey].pEvents->first();
            }
    };
} // namespace LinuxSampler

#endif	/* __LS_VOICEBASE_H__ */

