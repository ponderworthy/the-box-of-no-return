/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2016 Christian Schoenebeck                       *
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

#include "EffectChain.h"

#include "../common/global_private.h"

namespace LinuxSampler {

EffectChain::EffectChain(AudioOutputDevice* pDevice, int iEffectChainId) {
    this->pDevice = pDevice;
    iID = iEffectChainId;
}

void EffectChain::AppendEffect(Effect* pEffect) {
    pEffect->InitEffect(pDevice);
    _ChainEntry entry = { pEffect, true };
    vEntries.push_back(entry);
    pEffect->SetParent(this);
}

void EffectChain::InsertEffect(Effect* pEffect, int iChainPos) throw (Exception) {
    if (iChainPos < 0 || iChainPos >= vEntries.size())
        throw Exception(
            "Cannot insert effect at chain position " +
            ToString(iChainPos) + ", index out of bounds."
        );
    pEffect->InitEffect(pDevice); // might throw Exception !
    std::vector<_ChainEntry>::iterator iter = vEntries.begin();
    for (int i = 0; i < iChainPos; ++i) ++iter;
    _ChainEntry entry = { pEffect, true };
    vEntries.insert(iter, entry);
    pEffect->SetParent(this);
}

void EffectChain::RemoveEffect(int iChainPos) throw (Exception) {
    if (iChainPos < 0 || iChainPos >= vEntries.size())
        throw Exception(
            "Cannot remove effect at chain position " +
            ToString(iChainPos) + ", index out of bounds."
        );
    std::vector<_ChainEntry>::iterator iter = vEntries.begin();
    for (int i = 0; i < iChainPos; ++i) ++iter;
    Effect* pEffect = (*iter).pEffect;
    vEntries.erase(iter);
    pEffect->SetParent(NULL); // mark effect as not in use anymore
}

void EffectChain::RenderAudio(uint Samples) {
    for (int i = 0; i < vEntries.size(); ++i) {
        Effect* pCurrentEffect = vEntries[i].pEffect;
        if (i) { // import signal from previous effect
            Effect* pPrevEffect = vEntries[i - 1].pEffect;
            for (int iChan = 0; iChan < pPrevEffect->OutputChannelCount() && iChan < pCurrentEffect->InputChannelCount(); ++iChan) {
                pPrevEffect->OutputChannel(iChan)->MixTo(
                    pCurrentEffect->InputChannel(iChan),
                    Samples
                );
            }
        }
        if (IsEffectActive(i)) pCurrentEffect->RenderAudio(Samples);
        else { //TODO: lazy, suboptimal implementation of inactive, bypassed effects
            for (int iChan = 0; iChan < pCurrentEffect->OutputChannelCount() && iChan < pCurrentEffect->InputChannelCount(); ++iChan) {
                pCurrentEffect->InputChannel(iChan)->MixTo(
                    pCurrentEffect->OutputChannel(iChan),
                    Samples
                );
            }
        }
    }
}

Effect* EffectChain::GetEffect(int iChainPos) const {
    if (iChainPos < 0 || iChainPos >= vEntries.size()) return NULL;
    return vEntries[iChainPos].pEffect;
}

int EffectChain::EffectCount() const {
    return (int) vEntries.size();
}
    
void EffectChain::Reconnect(AudioOutputDevice* pDevice) {
    for (int i = 0; i < vEntries.size(); ++i) {
        Effect* pEffect = vEntries[i].pEffect;
        pEffect->InitEffect(pDevice);
    }
}

void EffectChain::SetEffectActive(int iChainPos, bool bOn) throw (Exception) {
    if (iChainPos < 0 || iChainPos >= vEntries.size())
        throw Exception(
            "Cannot change active state of effect at chain position " +
            ToString(iChainPos) + ", index out of bounds."
        );
    vEntries[iChainPos].bActive = bOn;
}

bool EffectChain::IsEffectActive(int iChainPos) const {
    if (iChainPos < 0 || iChainPos >= vEntries.size()) return false;
    return vEntries[iChainPos].bActive;
}

void EffectChain::ClearAllChannels() {
    for (int iEffect = 0; iEffect < vEntries.size(); ++iEffect) {
        Effect* pEffect = vEntries[iEffect].pEffect;
        for (int i = 0; i < pEffect->InputChannelCount(); ++i)
            pEffect->InputChannel(i)->Clear(); // zero out buffers
        for (int i = 0; i < pEffect->OutputChannelCount(); ++i)
            pEffect->OutputChannel(i)->Clear(); // zero out buffers
    }
}

int EffectChain::ID() const {
    return iID;
}

} // namespace LinuxSampler
