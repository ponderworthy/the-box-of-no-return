/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2011 Grigor Iliev                                       *
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

#include "SignalUnitRack.h"
#include "../../effects/EffectFactory.h"

namespace LinuxSampler {
    
    /**
     * Searches for know EQ effects and create one if the search succeed.
     */
    EqSupport::EqSupport() {
        pEffect = pEffect2 = NULL;
        BandCount = 0;
        GainIdxs = FreqIdxs = BandwidthIdxs = NULL;
        
        Install();
    }
    
    void EqSupport::Install() {
        Uninstall();
        for (int i = 0; i < EffectFactory::AvailableEffectsCount(); i++) {
            EffectInfo* pEffectInfo = EffectFactory::GetEffectInfo(i);
            /*if (!pEffectInfo->Name().compare("tap_equalizer_bw")) {
                try { pEffect = EffectFactory::Create(pEffectInfo); }
                catch(Exception e) { std::cerr << e.Message(); continue; }
                
                BandCount = 8;
                GainIdxs = new int[BandCount];
                FreqIdxs = new int[BandCount];
                BandwidthIdxs = new int[BandCount];
                for(int i = 0; i < BandCount; i++) {
                    GainIdxs[i] = i;
                    FreqIdxs[i] = i + 8;
                    BandwidthIdxs[i] = i + 16;
                }
                dmsg(1,("EQ support: %s\n", pEffectInfo->Description().c_str()));
                break;
            }*/
            
            if (!pEffectInfo->Name().compare("triplePara")) {
                try {
                    pEffect = EffectFactory::Create(pEffectInfo);
                    pEffect2 = EffectFactory::Create(pEffectInfo);
                } catch(Exception e) { std::cerr << e.Message(); continue; }
                
                BandCount = 3;
                GainIdxs = new int[BandCount];
                FreqIdxs = new int[BandCount];
                BandwidthIdxs = new int[BandCount];
                for(int i = 0; i < BandCount; i++) {
                    GainIdxs[i] = i*3 + 3;
                    FreqIdxs[i] = i*3 + 4;
                    BandwidthIdxs[i] = i*3 + 5;
                }
                
                pEffect->InputControl(0)->SetValue(0); // Low-shelving gain (0dB)
                pEffect->InputControl(12)->SetValue(0); // High-shelving gain (0dB)
                
                pEffect2->InputControl(0)->SetValue(0); // Low-shelving gain (0dB)
                pEffect2->InputControl(12)->SetValue(0); // High-shelving gain (0dB)
                
                break;
            }
        }
        
        if (pEffect == NULL) return;
        
        Reset();
    }
    
    void EqSupport::PrintInfo() {
        if (!HasSupport()) {
            dmsg(1,("EQ support: no\n"));
        } else {
            dmsg(1,("EQ support: %s\n", pEffect->GetEffectInfo()->Description().c_str()));
        }
    }
    
    void EqSupport::SetGain(int band, float gain) {
        if (!HasSupport()) return;
        if (band < 0 || band >= BandCount) throw Exception("EQ support: invalid band");
        
        EffectControl* ctrl = pEffect->InputControl(GainIdxs[band]);
        gain = check(ctrl->MinValue(), ctrl->MaxValue(), gain);
        ctrl->SetValue(gain);
        if (pEffect2 != NULL) pEffect2->InputControl(GainIdxs[band])->SetValue(gain);
    }
    
    void EqSupport::SetFreq(int band, float freq) {
        if (!HasSupport()) return;
        if (band < 0 || band >= BandCount) throw Exception("EQ support: invalid band");
        
        EffectControl* ctrl = pEffect->InputControl(FreqIdxs[band]);
        freq = check(ctrl->MinValue(), ctrl->MaxValue(), freq);
        ctrl->SetValue(freq);
        if (pEffect2 != NULL) pEffect2->InputControl(FreqIdxs[band])->SetValue(freq);
    }
    
    void EqSupport::SetBandwidth(int band, float octaves) {
        if (!HasSupport()) return;
        if (band < 0 || band >= BandCount) throw Exception("EQ support: invalid band");
        
        EffectControl* ctrl = pEffect->InputControl(BandwidthIdxs[band]);
        octaves = check(ctrl->MinValue(), ctrl->MaxValue(), octaves);
        ctrl->SetValue(octaves);
        if (pEffect2 != NULL) pEffect2->InputControl(BandwidthIdxs[band])->SetValue(octaves);
    }
    
    void EqSupport::Uninstall() {
        if (pEffect != NULL) EffectFactory::Destroy(pEffect);
        if (pEffect2 != NULL) EffectFactory::Destroy(pEffect2);
        if (GainIdxs != NULL) delete[] GainIdxs;
        if (FreqIdxs != NULL) delete[] FreqIdxs;
        if (BandwidthIdxs != NULL) delete[] BandwidthIdxs;

        pEffect = pEffect2 = NULL;
        BandCount = 0;
        GainIdxs = FreqIdxs = BandwidthIdxs = NULL;
    }
    
    EqSupport::~EqSupport() {
        Uninstall();
    }
} // namespace LinuxSampler
