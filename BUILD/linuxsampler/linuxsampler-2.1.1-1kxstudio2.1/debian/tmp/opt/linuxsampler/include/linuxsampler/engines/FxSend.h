/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2010 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef LS_FXSEND_H
#define LS_FXSEND_H

#include "../common/global.h"
#include "../drivers/audio/AudioChannel.h"
#include "EngineChannel.h"

#include <vector>

namespace LinuxSampler {

    // just symbol prototyping
    class EngineChannel;

    /** @brief Engine Channel Effect Send
     *
     * This class is used to manage effect sends on Engine Channels. An effect
     * send is used to route sampler channel's audio signals to either sampler
     * external effect processors (by routing the effect send to dedicated
     * audio output channels of the sampler channel's audio output device) or
     * to sampler internal effect processors (send effects). Each effect send
     * entity can define an arbitrary MIDI controller number which can alter
     * the effect send's send level.
     *
     * Regarding sampler internal effects: See AudioOutputDevice regarding
     * management of send effects, since send effects only live in the context
     * of exactly @e ONE AudioOutputDevice.
     *
     * Note: effect sends cannot be routed to a different AudioOutputDevice
     * than assigned to the FxSend's EngineChannel. Also note that an effect
     * send always has as much audio channels as its EngineChannel.
     */
    class FxSend {
        public:
            /**
             * Constructor. By default all effect send channels are routed to
             * the @e last available audio channels on the EngineChannel's
             * AudioOutputDevice.
             *
             * @param pEngineChannel - engine channel on which the effect send
             *                         is added to
             * @param MidiCtrl - MIDI controller number which can alter the
             *                   effect send level
             * @param Name - (optional) name for the effect send entity
             *
             * @throws Exception - in case no free ID could be found on
             *                     given EngineChannel or @a MidiCtrl is
             *                     invalid
             */
            FxSend(EngineChannel* pEngineChannel, uint8_t MidiCtrl, String Name = "") throw (Exception);

            /**
             * Index of the send effect chain this FX send is routed to or
             * -1 if FX send is not routed to a send effect.
             */
            int DestinationEffectChain() const;

            /**
             * Index of the send effect of the send effect chain given by
             * DestinationEffectChain(), in case FX send is routed to
             * a send effect or -1 otherwise. This is the effect chain
             * position, not the effect ID!
             */
            int DestinationEffectChainPosition() const;

            /**
             * Route this FX send to the given send effect given by index
             * @a iChainPos of the send effect chain given by @a iChain .
             *
             * If you want to remove the routing of an FX send, currently
             * directed to a send effect processor, and want to route it
             * directly to an audio output device channel instead, then set
             * both arguments to @c -1 .
             *
             * @throw Exception - if given effect chain or effect chain position
             *                    doesn't exist
             *
             * @see AudioOutputDevice::SendEffectChain()
             */
            void SetDestinationEffect(int iChain, int iChainPos) throw (Exception);

            /**
             * @deprecated This method will be removed, use DestinationEffectChain() instead!
             */
            int DestinationMasterEffectChain() const DEPRECATED_API;

            /**
             * @deprecated This method will be removed, use DestinationEffectChainPosition() instead!
             */
            int DestinationMasterEffect() const DEPRECATED_API;

            /**
             * @deprecated This method will be removed, use SetDestinationEffect() instead!
             */
            void SetDestinationMasterEffect(int iChain, int iChainPos) throw (Exception) DEPRECATED_API;

            /**
             * Returns the audio output device's audio channel to which effect
             * send's channel \a SrcChan is currently routed to.
             */
            int DestinationChannel(int SrcChan);

            /**
             * Alters the routing of an audio channel. By default all audio
             * channels of an effect send are routed in consecutive same order
             * to its destination. You can use this method to change this
             * default routing. If this effect send is routed to an internel
             * effect, then @a DstChan is the input channel of that destination
             * effect. Otherwise, if this effect send is not routed to an
             * internal effect, then @a DstChan is the output channel of the
             * sampler channel's audio output device.
             *
             * @param SrcChan - the effect send's source channel
             * @param DstChan - the audio output device's destination channel
             *                  or send effect's input channel
             * @throws Exception - in case arguments out of range
             */
            void SetDestinationChannel(int SrcChan, int DstChan) throw (Exception);

            /**
             * Should be called by the engine channel whenever the amount of
             * audio channel has changed, so the FxSend object can adjust the
             * amount of channels to that new number and establish default
             * routings for new channels if needed.
             */
            void UpdateChannels();

            /**
             * The effect send's current send level ( usually a value between
             * @c 0.0f and @c 1.0f ).
             */
            float Level();

            /**
             * Alter the effect send's send level ( usually a value between
             * @c 0.0f and @c 1.0f ).
             */
            void SetLevel(float f);

            /**
             * Alter the effect send's send level by supplying the MIDI
             * controller's MIDI value. This method is usually only called
             * by the engine channel.
             */
            void SetLevel(uint8_t iMidiValue);

            /**
             * Reset send level to the default send level (i.e. due to a
             * MIDI "reset all controllers" message).
             */
            void Reset();

            /**
             * Returns the MIDI controller number which can alter the effect
             * send's send level.
             */
            uint8_t MidiController();

            /**
             * Alter the MIDI controller number which should alter the effect
             * send's send level.
             *
             * @param MidiCtrl - MIDI controller number
             * @throws Exception - if MIDI controller number is invalid
             */
            void SetMidiController(uint8_t MidiCtrl) throw (Exception);

            /**
             * Returns the (optional) name of this effect send entity.
             */
            String Name();

            /**
             * Sets the name of this effect send entity.
             * @param Name The new name of this effect send entity.
             */
            void SetName(String Name);

            /**
             * Returns the (at least sampler-channel-) unique ID of the
             * effect send instance. This is actually not used by the engine
             * at all. It is at the moment only used by the LSCP server to
             * associate an unique numerical ID with each effect send entity.
             */
            uint Id();

            /**
             * Determines whether the effect send's settings are changed.
             */
            bool IsInfoChanged();

            /**
             * Sets whether the effect send's settings are changed.
             */
            void SetInfoChanged(bool b);

        protected:
            EngineChannel*   pEngineChannel;
            int              iDestinationEffectChain;
            int              iDestinationEffectChainPos;
            std::vector<int> Routing;
            uint8_t          MidiFxSendController;
            String           sName;
            uint             iId;
            float            fLevel;
            bool             bInfoChanged;  // Determines whether there are changes to the settings.
    };

} // namespace LinuxSampler

#endif // LS_FXSEND_H
