/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#include "FxSend.h"

#include "../common/global_private.h"
#include "../drivers/audio/AudioOutputDevice.h"
#include "../common/RTMath.h"

#include <map>

#define DEFAULT_FX_SEND_LEVEL	0.0f

namespace LinuxSampler {

    FxSend::FxSend(EngineChannel* pEngineChannel, uint8_t MidiCtrl, String Name) throw (Exception)
        : iDestinationEffectChain(-1), iDestinationEffectChainPos(-1), bInfoChanged(false)
    {
        this->pEngineChannel = pEngineChannel;
        AudioOutputDevice* pDevice = pEngineChannel->GetAudioOutputDevice();
        const int iChanOffset = (pDevice) ? pDevice->ChannelCount() - pEngineChannel->Channels() : 0;
        for (int i = 0; i < pEngineChannel->Channels(); i++) {
            const int iDestination = iChanOffset + i;
            Routing.push_back(iDestination);
        }
        SetMidiController(MidiCtrl);
        sName = Name;

        // create an EngineChannel unique ID for this FxSend instance
        if (!pEngineChannel->GetFxSendCount()) iId = 0;
        else {
            // get the highest existing map ID
            uint highestIndex = 0;
            for (uint i = 0; i < pEngineChannel->GetFxSendCount(); i++)
                highestIndex = RTMath::Max(highestIndex, pEngineChannel->GetFxSend(i)->Id());
            // check if we reached the index limit
            if (highestIndex + 1 < highestIndex) {
                // search for an unoccupied map ID starting from 0
                for (uint i = 0; i < highestIndex; i++) {
                    bool bOccupied = false;
                    for (uint j = 0; j < pEngineChannel->GetFxSendCount(); j++) {
                        if (pEngineChannel->GetFxSend(j)->Id() == i) {
                            bOccupied = true;
                            break;
                        }
                    }
                    if (!bOccupied) {
                        iId = i;
                        goto __done;
                    }
                }
                throw Exception("Internal error: could not find unoccupied FxSend ID.");
            }
            iId = highestIndex + 1;
        }
        __done:

        fLevel = DEFAULT_FX_SEND_LEVEL;
    }

    int FxSend::DestinationEffectChain() const {
        return iDestinationEffectChain;
    }

    int FxSend::DestinationEffectChainPosition() const {
        return iDestinationEffectChainPos;
    }

    void FxSend::SetDestinationEffect(int iChain, int iChainPos) throw (Exception) {
        AudioOutputDevice* pDevice = pEngineChannel->GetAudioOutputDevice();
        bool chainFound = false;
        if (iChain != -1) {
            if (pDevice->SendEffectChainByID(iChain) != NULL)  chainFound = true;
            if (!chainFound) throw Exception(
                "Could not assign FX Send to send effect chain " +
                ToString(iChain) + ": effect chain doesn't exist."
            );
        }
        if (chainFound && (iChainPos < 0 || iChainPos >= pDevice->SendEffectChainByID(iChain)->EffectCount()))
            throw Exception(
                "Could not assign FX Send to send effect chain position " +
                ToString(iChainPos) + " of send effect chain " + ToString(iChain) +
                ": effect chain position out of bounds."
            );
        iDestinationEffectChain    = iChain;
        iDestinationEffectChainPos = (iChain == -1 ? -1 : iChainPos);
    }

    // TODO: to be removed
    int FxSend::DestinationMasterEffectChain() const {
        return DestinationEffectChain();
    }

    // TODO: to be removed
    int FxSend::DestinationMasterEffect() const {
        return DestinationEffectChainPosition();
    }

    // TODO: to be removed
    void FxSend::SetDestinationMasterEffect(int iChain, int iChainPos) throw (Exception) {
        SetDestinationEffect(iChain, iChainPos);
    }

    int FxSend::DestinationChannel(int SrcChan) {
        if (SrcChan >= pEngineChannel->Channels()) return -1;
        return Routing[SrcChan];
    }

    void FxSend::SetDestinationChannel(int SrcChan, int DstChan) throw (Exception) {
        if (SrcChan < 0 || SrcChan >= pEngineChannel->Channels())
            throw Exception("Cannot alter FxSend routing, source channel out of bounds");
        AudioOutputDevice* pDevice = pEngineChannel->GetAudioOutputDevice();
        if (pDevice) {
            if (DstChan < 0 || DstChan >= pDevice->ChannelCount())
                throw Exception("Cannot alter FxSend routing, destination channel out of bounds");
        } else { // no audio device assigned yet
            if (DstChan < 0 || DstChan >= pEngineChannel->Channels())
                throw Exception(
                    "there is no audio device yet, so you cannot set a "
                    "FxSend destination channel higher than the engine "
                    "channel's amount of channels"
                );
        }
        Routing[SrcChan] = DstChan;
    }

    void FxSend::UpdateChannels() {
        if (Routing.size() > pEngineChannel->Channels()) {
            // add routings with default destinations
            AudioOutputDevice* pDevice = pEngineChannel->GetAudioOutputDevice();
            const int iChanOffset = (pDevice) ? pDevice->ChannelCount() - pEngineChannel->Channels() : 0;
            for (int i = (int)Routing.size(); i < pEngineChannel->Channels(); i++) {
                const int iDestination = iChanOffset + i;
                Routing.push_back(iDestination);
            }
        } else if (Routing.size() < pEngineChannel->Channels()) {
            // shrink routing vector
            Routing.resize(pEngineChannel->Channels());
        }
    }

    float FxSend::Level() {
        return fLevel;
    }

    void FxSend::SetLevel(float f) {
        if(fLevel == f) return;
        fLevel = f;
        SetInfoChanged(true);
    }

    void FxSend::SetLevel(uint8_t iMidiValue) {
        fLevel = float(iMidiValue & 0x7f) / 127.0f;
        SetInfoChanged(true);
    }

    void FxSend::Reset() {
        SetLevel(DEFAULT_FX_SEND_LEVEL);
    }

    uint8_t FxSend::MidiController() {
        return MidiFxSendController;
    }

    void FxSend::SetMidiController(uint8_t MidiCtrl) throw (Exception) {
        if (MidiCtrl >> 7)
            throw Exception("Invalid MIDI controller " + ToString((int)MidiCtrl));
        MidiFxSendController = MidiCtrl;
    }

    String FxSend::Name() {
        return sName;
    }

    void FxSend::SetName(String Name) {
        sName = Name;
    }

    uint FxSend::Id() {
        return iId;
    }

    void FxSend::SetInfoChanged(bool b) {
        bInfoChanged = b;
    }

    bool FxSend::IsInfoChanged() {
        return bInfoChanged;
    }

} // namespace LinuxSampler
