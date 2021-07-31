/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2012 Christian Schoenebeck and Grigor Iliev      *
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

#include <sstream>

#include "InstrumentResourceManager.h"
#include "EngineChannel.h"
#include "Engine.h"

#include "../../common/global_private.h"
#include "../../common/Path.h"
#include "../../plugins/InstrumentEditorFactory.h"


namespace LinuxSampler { namespace sf2 {

    String InstrumentResourceManager::GetInstrumentName(instrument_id_t ID) {
        Lock();
        ::sf2::Preset* pInstrument = Resource(ID, false);
        String res = (pInstrument) ? pInstrument->GetName() : "";
        Unlock();
        return res;
    }

    std::vector<InstrumentResourceManager::instrument_id_t> InstrumentResourceManager::GetInstrumentFileContent(String File) throw (InstrumentManagerException) {
        ::RIFF::File* riff = NULL;
        ::sf2::File*  sf2  = NULL;
        try {
            std::vector<instrument_id_t> result;
            riff = new ::RIFF::File(File);
            sf2  = new ::sf2::File(riff);
            for (int i = 0; i < GetSfInstrumentCount(sf2); i++) {
                instrument_id_t id;
                id.FileName = File;
                id.Index    = i;
                result.push_back(id);
            }
            if (sf2)  delete sf2;
            if (riff) delete riff;
            return result;
        } catch (::RIFF::Exception e) {
            if (sf2)  delete sf2;
            if (riff) delete riff;
            throw InstrumentManagerException(e.Message);
        } catch (...) {
            if (sf2)  delete sf2;
            if (riff) delete riff;
            throw InstrumentManagerException("Unknown exception while trying to parse '" + File + "'");
        }
    }

    InstrumentResourceManager::instrument_info_t InstrumentResourceManager::GetInstrumentInfo(instrument_id_t ID) throw (InstrumentManagerException) {
        Lock();
        ::sf2::Preset* pInstrument = Resource(ID, false);
        bool loaded = (pInstrument != NULL);
        if (!loaded) Unlock();

        ::RIFF::File* riff = NULL;
        ::sf2::File*  sf2  = NULL;
        try {
            if (!loaded) {
                riff = new ::RIFF::File(ID.FileName);
                sf2 = new ::sf2::File(riff);
                pInstrument = GetSfInstrument(sf2, ID.Index);
            }

            instrument_info_t info;
            for (int i = 0; i < 128; i++) { info.KeyBindings[i] = info.KeySwitchBindings[i] = 0; }

            ::sf2::File* pFile = pInstrument->GetFile();

            info.FormatVersion = ToString(pFile->pInfo->pVer->Major);
            info.Product = pFile->pInfo->Product;
            info.Artists = pFile->pInfo->Engineers;

            info.InstrumentName = pInstrument->Name;

            for (int i = 0; i < pInstrument->GetRegionCount(); i++) {
                int low = pInstrument->GetRegion(i)->loKey;
                int high = pInstrument->GetRegion(i)->hiKey;
                if (low == ::sf2::NONE || high == ::sf2::NONE) {
                    ::sf2::Instrument* layer = pInstrument->GetRegion(i)->pInstrument;
                    for (int j = 0; j < layer->GetRegionCount(); j++) {
                        int lo = layer->GetRegion(j)->loKey;
                        int hi = layer->GetRegion(j)->hiKey;
                        SetKeyBindings(info.KeyBindings, lo, hi, ::sf2::NONE);
                    }
                } else {
                    SetKeyBindings(info.KeyBindings, low, high, ::sf2::NONE);
                }
            }

            if (loaded) Unlock();

            if (sf2)  delete sf2;
            if (riff) delete riff;
            return info;
        } catch (::sf2::Exception e) {
            if (loaded) Unlock();
            if (sf2)  delete sf2;
            if (riff) delete riff;
            throw InstrumentManagerException(e.Message);
        } catch (...) {
            if (loaded) Unlock();
            if (sf2)  delete sf2;
            if (riff) delete riff;
            throw InstrumentManagerException("Unknown exception while trying to parse '" + ID.FileName + "'");
        }
    }

    ::sf2::Preset* InstrumentResourceManager::Create(instrument_id_t Key, InstrumentConsumer* pConsumer, void*& pArg) {
        // get sfz file from internal sfz file manager
        ::sf2::File* pSf2 = Sf2s.Borrow(Key.FileName, reinterpret_cast<Sf2Consumer*>(Key.Index)); // conversion kinda hackish :/

        dmsg(1,("Loading sf2 instrument ('%s',%d)...",Key.FileName.c_str(),Key.Index));
        ::sf2::Preset* pInstrument = GetSfInstrument(pSf2, Key.Index);
        dmsg(1,("OK\n"));

        // cache initial samples points (for actually needed samples)
        dmsg(1,("Caching initial samples..."));
        float regTotal = 0, regCurrent = 0;
        for (int i = 0 ; i < pInstrument->GetRegionCount() ; i++) {
            ::sf2::Instrument* sf2Instr = pInstrument->GetRegion(i)->pInstrument;
            if (sf2Instr) regTotal += sf2Instr->GetRegionCount();
        }
        uint maxSamplesPerCycle = GetMaxSamplesPerCycle(pConsumer);
        for (int i = 0 ; i < pInstrument->GetRegionCount() ; i++) {
            ::sf2::Instrument* sf2Instr = pInstrument->GetRegion(i)->pInstrument;
            if (sf2Instr) {
                // pInstrument is ::sf2::Preset
                for (int j = 0 ; j < sf2Instr->GetRegionCount() ; j++) {
                    float localProgress = regCurrent++ / regTotal;
                    DispatchResourceProgressEvent(Key, localProgress);
                    CacheInitialSamples(sf2Instr->GetRegion(j)->GetSample(), maxSamplesPerCycle);
                }
            }
        }
        dmsg(1,("OK\n"));
        DispatchResourceProgressEvent(Key, 1.0f); // done; notify all consumers about progress 100%

        // we need the following for destruction later
        instr_entry_t* pEntry = new instr_entry_t;
        pEntry->ID.FileName   = Key.FileName;
        pEntry->ID.Index      = Key.Index;
        pEntry->pFile         = pSf2;

        // and we save this to check if we need to reallocate for an engine with higher value of 'MaxSamplesPerSecond'
        pEntry->MaxSamplesPerCycle = maxSamplesPerCycle;

        pArg = pEntry;

        return pInstrument;
    }

    void InstrumentResourceManager::Destroy(::sf2::Preset* pResource, void* pArg) {
        instr_entry_t* pEntry = (instr_entry_t*) pArg;
        // we don't need the .sf2 file here anymore
        Sf2s.HandBack(pEntry->pFile, reinterpret_cast<Sf2Consumer*>(pEntry->ID.Index)); // conversion kinda hackish :/
        delete pEntry;
    }

    void InstrumentResourceManager::DeleteRegionIfNotUsed(::sf2::Region* pRegion, region_info_t* pRegInfo) {
        // TODO: we could delete Region and Instrument here if they have become unused
    }

    void InstrumentResourceManager::DeleteSampleIfNotUsed(::sf2::Sample* pSample, region_info_t* pRegInfo) {
        ::sf2::File*  sf2 = pRegInfo->file;
        ::RIFF::File* riff = static_cast< ::RIFF::File*>(pRegInfo->pArg);
        if (sf2) {
            sf2->DeleteSample(pSample);
            if (!sf2->HasSamples()) {
                dmsg(2,("No more samples in use - freeing sf2\n"));
                delete sf2;
                delete riff;
            }
        }
    }



    // internal sfz file manager

    ::sf2::File* InstrumentResourceManager::Sf2ResourceManager::Create(String Key, Sf2Consumer* pConsumer, void*& pArg) {
        dmsg(1,("Loading sf2 file \'%s\'...", Key.c_str()));
        ::RIFF::File* pRIFF = new ::RIFF::File(Key);
        ::sf2::File* pSf2   = new ::sf2::File(pRIFF);
        pArg                = pRIFF;
        dmsg(1,("OK\n"));
        return pSf2;
    }

    void InstrumentResourceManager::Sf2ResourceManager::Destroy(::sf2::File* pResource, void* pArg) {
        dmsg(1,("Freeing sf2 file from memory..."));

        // Delete as much as possible of the sf2 file. Some of the
        // regions and samples may still be in use - these
        // will be deleted later by the HandBackRegion function.
        bool deleteFile = true;

        for (int i = pResource->GetInstrumentCount() - 1; i >= 0; i--) {
            ::sf2::Instrument* pInstr = pResource->GetInstrument(i);
            bool deleteInstrument = true;

            for (int j = pInstr->GetRegionCount() - 1; j >= 0 ; j--) {
                ::sf2::Region* pRegion = pInstr->GetRegion(j);
                std::map< ::sf2::Region*, region_info_t>::iterator iter = parent->RegionInfo.find(pRegion);
                if (iter != parent->RegionInfo.end()) {
                    region_info_t& regInfo = (*iter).second;
                    regInfo.file = pResource;
                    deleteFile = deleteInstrument = false;
                } else {
                    pInstr->DeleteRegion(pRegion);
                }
            }

            if (deleteInstrument) pResource->DeleteInstrument(pInstr);
        }

        if (deleteFile) {
            delete pResource;
            delete (::RIFF::File*) pArg;
        } else {
            dmsg(2,("keeping some samples that are in use..."));
            for (int i = pResource->GetSampleCount() - 1; i >= 0; i--) {
                ::sf2::Sample* sample = pResource->GetSample(i);
                if (parent->SampleRefCount.find(sample) == parent->SampleRefCount.end()) {
                    pResource->DeleteSample(sample);
                }
            }
        }

        dmsg(1,("OK\n"));
    }

    int InstrumentResourceManager::GetSfInstrumentCount(::sf2::File* pFile) {
        return pFile->GetPresetCount();
    }

    ::sf2::Preset* InstrumentResourceManager::GetSfInstrument(::sf2::File* pFile, int idx) {
        if (idx >= pFile->GetPresetCount()) {
            throw InstrumentManagerException("There is no instrument with index " + ToString(idx));
        }
        return pFile->GetPreset(idx);
    }

}} // namespace LinuxSampler::sfz
