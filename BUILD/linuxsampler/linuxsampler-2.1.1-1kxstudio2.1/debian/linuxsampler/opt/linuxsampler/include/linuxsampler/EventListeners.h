/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2009 Grigor Iliev                                *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef __LS_EVENTLISTENERS_H__
#define __LS_EVENTLISTENERS_H__

#include <vector>
#include "common/global.h"

namespace LinuxSampler {

    // just symbol prototyping
    class SamplerChannel;
    class MidiInputDevice;
    class MidiInputPort;

    template<class L>
    class ListenerList {
        public:
            /**
             * Registers the specified listener for receiving event messages.
             */
            void AddListener(L l) {
                 vListenerList.push_back(l);
            }

            /**
             * Removes the specified listener.
             */
            void RemoveListener(L l) {
                typename std::vector<L>::iterator it;
                it = vListenerList.begin();
                for (; it != vListenerList.end(); it++) {
                    if (*it == l) {
                        vListenerList.erase(it);
                        return;
                    }
                }
            }
            
            /**
             * Removes all listeners.
             */
            void RemoveAllListeners() {
                vListenerList.clear();
            }
            
            /**
             * Gets the number of the registered listeners.
             */
            int GetListenerCount() {
                return (int) vListenerList.size();
            }
            
            /**
             * Gets the listener at the specified position.
             * @param index The position of the listener to return.
             */
            L GetListener(int index) {
                return vListenerList.at(index);
            }
            
        private:
            std::vector<L> vListenerList;
    };

#define REGISTER_FIRE_EVENT_METHOD(method) virtual void method() \
    { for(int i = 0; i < GetListenerCount(); i++) GetListener(i)->method(); }

#define REGISTER_FIRE_EVENT_METHOD_ARG1(method, T1) virtual void method(T1 _evt_arg1_) \
    { for(int i = 0; i < GetListenerCount(); i++) GetListener(i)->method(_evt_arg1_); }

#define REGISTER_FIRE_EVENT_METHOD_ARG2(method, T1, T2) virtual void method(T1 _evt_arg1_, T2 _evt_arg2_) \
    { for(int i = 0; i < GetListenerCount(); i++) GetListener(i)->method(_evt_arg1_, _evt_arg2_); }

    /**
     * This class is used as a listener, which is notified
     * when the number of sampler channels is changed.
     */
    class ChannelCountListener {
        public:
            /**
             * Invoked when the number of sampler channels has changed.
             * @param NewCount The new number of sampler channels.
             */
            virtual void ChannelCountChanged(int NewCount) = 0;
            virtual void ChannelAdded(SamplerChannel* pChannel) = 0;
            virtual void ChannelToBeRemoved(SamplerChannel* pChannel) = 0;
    };

    /**
     * This class exists as convenience for creating listener objects.
     * The methods in this class are empty.
     */
    class ChannelCountAdapter : public ChannelCountListener {
        public:
            virtual void ChannelCountChanged(int NewCount) { };
            virtual void ChannelAdded(SamplerChannel* pChannel) { };
            virtual void ChannelToBeRemoved(SamplerChannel* pChannel) { };
    };

    /**
     * This class is used as a listener, which is notified
     * when the number of audio output devices is changed.
     */
    class AudioDeviceCountListener {
        public:
            /**
             * Invoked when the number of audio output devices has changed.
             * @param NewCount The new number of audio output devices.
             */
            virtual void AudioDeviceCountChanged(int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the number of MIDI input devices is changed.
     */
    class MidiDeviceCountListener {
        public:
            /**
             * Invoked when the number of MIDI input devices has changed.
             * @param NewCount The new number of MIDI input devices.
             */
            virtual void MidiDeviceCountChanged(int NewCount) = 0;

            /**
             * Invoked right before the supplied MIDI input device is going
             * to be destroyed.
             * @param pDevice MidiInputDevice to be deleted
             */
            virtual void MidiDeviceToBeDestroyed(MidiInputDevice* pDevice) = 0;

            /**
             * Invoked to inform that a new MidiInputDevice has just been
             * created.
             * @param pDevice newly created MidiInputDevice
             */
            virtual void MidiDeviceCreated(MidiInputDevice* pDevice) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the number of MIDI input ports is changed.
     */
    class MidiPortCountListener {
        public:
            /**
             * Invoked when the number of MIDI input ports has changed.
             * @param NewCount The new number of MIDI input ports.
             */
            virtual void MidiPortCountChanged(int NewCount) = 0;

            /**
             * Invoked right before the supplied MIDI input port is going
             * to be destroyed.
             * @param pPort MidiInputPort to be deleted
             */
            virtual void MidiPortToBeRemoved(MidiInputPort* pPort) = 0;

            /**
             * Invoked to inform that a new MidiInputPort has just been
             * added.
             * @param pPort newly created MidiInputPort
             */
            virtual void MidiPortAdded(MidiInputPort* pPort) = 0;
    };

    /**
     * This class is used as a listener, which is notified when the number
     * of MIDI instruments on a particular MIDI instrument map is changed.
     */
    class MidiInstrumentCountListener {
        public:
            /**
             * Invoked when the number of MIDI instruments has changed.
             * @param MapId The numerical ID of the MIDI instrument map.
             * @param NewCount The new number of MIDI instruments.
             */
            virtual void MidiInstrumentCountChanged(int MapId, int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when a MIDI instrument in a MIDI instrument map is changed.
     */
    class MidiInstrumentInfoListener {
        public:
            /**
             * Invoked when a MIDI instrument in a MIDI instrument map is changed.
             * @param MapId The numerical ID of the MIDI instrument map.
             * @param Bank The index of the MIDI bank, containing the instrument.
             * @param Program The MIDI program number of the instrument.
             */
            virtual void MidiInstrumentInfoChanged(int MapId, int Bank, int Program) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the number of MIDI instrument maps is changed.
     */
    class MidiInstrumentMapCountListener {
        public:
            /**
             * Invoked when the number of MIDI instrument maps has changed.
             * @param NewCount The new number of MIDI instruments.
             */
            virtual void MidiInstrumentMapCountChanged(int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the settings of a MIDI instrument map are changed.
     */
    class MidiInstrumentMapInfoListener {
        public:
            /**
             * Invoked when the settings of a MIDI instrument map are changed.
             * @param MapId The numerical ID of the MIDI instrument map.
             */
            virtual void MidiInstrumentMapInfoChanged(int MapId) = 0;
    };

    /**
     * This class is used as a listener, which is notified when the number
     * of effect sends on a particular sampler channel is changed.
     */
    class FxSendCountListener {
        public:
            /**
             * Invoked when the number of effect sends
             * on the specified sampler channel has changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of effect sends.
             */
            virtual void FxSendCountChanged(int ChannelId, int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified when the number
     * of active voices in a particular sampler channel is changed.
     */
    class VoiceCountListener {
        public:
            /**
             * Invoked when the number of active voices
             * on the specified sampler channel has changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of active voices.
             */
            virtual void VoiceCountChanged(int ChannelId, int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified when the number
     * of active disk streams in a particular sampler channel is changed.
     */
    class StreamCountListener {
        public:
            /**
             * Invoked when the number of active disk streams
             * on the specified sampler channel has changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of active disk streams.
             */
            virtual void StreamCountChanged(int ChannelId, int NewCount) = 0;
    };
    
    /**
     * This class is used as a listener, which is notified when the fill state
     * of the disk stream buffers on a particular sampler channel is changed.
     */
    class BufferFillListener {
        public:
            /**
             * Invoked when the fill state of the disk stream
             * buffers on the specified sampler channel is changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param FillData The buffer fill data for the specified sampler channel.
             */
            virtual void BufferFillChanged(int ChannelId, String FillData) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the total number of active streams is changed.
     */
    class TotalStreamCountListener {
        public:
            /**
             * Invoked when the total number of active streams is changed.
             * @param NewCount The new number of active streams.
             */
            virtual void TotalStreamCountChanged(int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the total number of active voices is changed.
     */
    class TotalVoiceCountListener {
        public:
            /**
             * Invoked when the total number of active voices is changed.
             * @param NewCount The new number of active voices.
             */
            virtual void TotalVoiceCountChanged(int NewCount) = 0;
    };

    /**
     * This class is used as a listener, which is notified
     * when the engine type of a particular sampler channel is changed.
     */
    class EngineChangeListener {
        public:
            /**
             * Invoked when the engine type of the specified sampler channel
             * is going to be changed soon.
             * @param ChannelId The numerical ID of the sampler channel
             */
            virtual void EngineToBeChanged(int ChannelId) = 0;

            /**
             * Invoked when the engine type of the
             * specified sampler channel was changed.
             * @param ChannelId The numerical ID of the sampler
             * channel, which engine type has been changed.
             */
            virtual void EngineChanged(int ChannelId) = 0;
    };
}
#endif
