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


namespace LinuxSampler { namespace sfz {

    String InstrumentResourceManager::GetInstrumentName(instrument_id_t ID) {
        Lock();
        ::sfz::Instrument* pInstrument = Resource(ID, false);
        String res = (pInstrument) ? pInstrument->GetName() : "";
        Unlock();
        return res;
    }

    std::vector<InstrumentResourceManager::instrument_id_t> InstrumentResourceManager::GetInstrumentFileContent(String File) throw (InstrumentManagerException) {
        std::vector<instrument_id_t> result;
        instrument_id_t id;
        id.FileName = File;
        id.Index    = 0;
        result.push_back(id);
        return result;
    }

    InstrumentResourceManager::instrument_info_t InstrumentResourceManager::GetInstrumentInfo(instrument_id_t ID) throw (InstrumentManagerException) {
        if (ID.Index) throw InstrumentManagerException("There is no instrument " + ToString(ID.Index) + " in " + ID.FileName);
        Lock();
        ::sfz::Instrument* pInstrument = Resource(ID, false);
        bool loaded = (pInstrument != NULL);
        if (!loaded) Unlock();

        ::sfz::File*  sfz  = NULL;
        try {
            if (!loaded) {
                sfz  = new ::sfz::File(ID.FileName);
                pInstrument = sfz->GetInstrument();
            }

            if (!pInstrument) throw InstrumentManagerException("There is no instrument " + ToString(ID.Index) + " in " + ID.FileName);

            instrument_info_t info;
            info.InstrumentName = Path::getBaseName(ID.FileName);

            for (int i = 0; i < 128; i++) {
                info.KeyBindings[i] = pInstrument->HasKeyBinding(i);
                info.KeySwitchBindings[i] = pInstrument->HasKeySwitchBinding(i);
            }

            if (loaded) Unlock();

            if (sfz)  delete sfz;
            return info;
        } catch (::sfz::Exception e) {
            if (loaded) Unlock();
            if (sfz)  delete sfz;
            throw InstrumentManagerException(e.Message());
        } catch (...) {
            if (loaded) Unlock();
            if (sfz)  delete sfz;
            throw InstrumentManagerException("Unknown exception while trying to parse '" + ID.FileName + "'");
        }
    }

    ::sfz::Instrument* InstrumentResourceManager::Create(instrument_id_t Key, InstrumentConsumer* pConsumer, void*& pArg) {
        // get sfz file from internal sfz file manager
        ::sfz::File* pSfz = Sfzs.Borrow(Key.FileName, reinterpret_cast<SfzConsumer*>(Key.Index)); // conversion kinda hackish :/

        dmsg(1,("Loading sfz instrument ('%s',%d)...",Key.FileName.c_str(),Key.Index));
        if (Key.Index) {
            std::stringstream msg;
            msg << "There's no instrument with index " << Key.Index << ".";
            throw InstrumentManagerException(msg.str());
        }
        ::sfz::Instrument* pInstrument = pSfz->GetInstrument();
        if (!pInstrument) {
            std::stringstream msg;
            msg << "There's no instrument with index " << Key.Index << ".";
            throw InstrumentManagerException(msg.str());
        }
        dmsg(1,("OK\n"));

        // cache initial samples points (for actually needed samples)
        dmsg(1,("Caching initial samples..."));
        int regionCount = (int) pInstrument->regions.size();
        uint maxSamplesPerCycle = GetMaxSamplesPerCycle(pConsumer);
        for (int i = 0 ; i < regionCount ; i++) {
            float localProgress = (float) i / (float) regionCount;
            DispatchResourceProgressEvent(Key, localProgress);
            CacheInitialSamples(pInstrument->regions[i]->GetSample(), maxSamplesPerCycle);
            //pInstrument->regions[i]->GetSample()->Close();
        }
        dmsg(1,("OK\n"));
        DispatchResourceProgressEvent(Key, 1.0f); // done; notify all consumers about progress 100%

        // we need the following for destruction later
        instr_entry_t* pEntry = new instr_entry_t;
        pEntry->ID.FileName   = Key.FileName;
        pEntry->ID.Index      = Key.Index;
        pEntry->pFile         = pSfz;

        // and we save this to check if we need to reallocate for an engine with higher value of 'MaxSamplesPerSecond'
        pEntry->MaxSamplesPerCycle = maxSamplesPerCycle;
        
        pArg = pEntry;

        return pInstrument;
    }

    void InstrumentResourceManager::Destroy(::sfz::Instrument* pResource, void* pArg) {
        instr_entry_t* pEntry = (instr_entry_t*) pArg;
        // we don't need the .sfz file here anymore
        Sfzs.HandBack(pEntry->pFile, reinterpret_cast<SfzConsumer*>(pEntry->ID.Index)); // conversion kinda hackish :/
        delete pEntry;
    }

    void InstrumentResourceManager::DeleteRegionIfNotUsed(::sfz::Region* pRegion, region_info_t* pRegInfo) {
        ::sfz::File* file = pRegInfo->file;
        if (file == NULL) return;

        file->GetInstrument()->DestroyRegion(pRegion);
        if (file->GetInstrument()->regions.empty()) {
            dmsg(2,("No more regions in use - freeing sfz\n"));
            delete file;
        }
    }

    void InstrumentResourceManager::DeleteSampleIfNotUsed(Sample* pSample, region_info_t* pRegInfo) {
        
    }



    // internal sfz file manager

    ::sfz::File* InstrumentResourceManager::SfzResourceManager::Create(String Key, SfzConsumer* pConsumer, void*& pArg) {
        dmsg(1,("Loading sfz file \'%s\'...", Key.c_str()));
        ::sfz::File* pSfz = new ::sfz::File(Key, &sampleManager);
        dmsg(1,("OK\n"));
        return pSfz;
    }

    void InstrumentResourceManager::SfzResourceManager::Destroy(::sfz::File* pResource, void* pArg) {
        dmsg(1,("Freeing sfz file from memory..."));

        // Delete as much as possible of the sfz file. Some of the
        // regions and samples may still be in use - these
        // will be deleted later by the HandBackRegion function.
        bool deleteInstrument = true;
        ::sfz::Instrument* pInstr = pResource->GetInstrument();

        for (int i = (int)pInstr->regions.size() - 1; i >= 0 ; i--) {
            ::sfz::Region* pRegion = pInstr->regions[i];
            std::map< ::sfz::Region*, region_info_t>::iterator iter = parent->RegionInfo.find(pRegion);
            if (iter != parent->RegionInfo.end()) {
                region_info_t& regInfo = (*iter).second;
                regInfo.file = pResource;
                deleteInstrument = false;
            } else {
                pInstr->DestroyRegion(pRegion);
            }
        }

        if (deleteInstrument) delete pResource;
        else dmsg(2,("keeping some samples that are in use..."));

        dmsg(1,("OK\n"));
    }

}} // namespace LinuxSampler::sfz
