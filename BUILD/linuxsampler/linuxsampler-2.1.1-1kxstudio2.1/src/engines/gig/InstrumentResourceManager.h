/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2017 Christian Schoenebeck                       *
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

#ifndef __LS_GIG_INSTRUMENTRESOURCEMANAGER_H__
#define __LS_GIG_INSTRUMENTRESOURCEMANAGER_H__

#include "../../common/global.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

#include "../../common/Pool.h"
#include "../../common/ResourceManager.h"
#include "../InstrumentManagerBase.h"
#include "../../drivers/audio/AudioOutputDevice.h"
#include "../InstrumentManager.h"
#include "../../common/ArrayList.h"

//namespace libgig = gig;

namespace LinuxSampler { namespace gig {

    typedef ResourceConsumer< ::gig::Instrument> InstrumentConsumer;

}} // namespace LinuxSampler::gig

#include "../../plugins/InstrumentEditor.h"

namespace LinuxSampler { namespace gig {

    class Engine;
    class EngineChannel;

    /** @brief Gig instrument manager
     *
     * Manager to share gig instruments between multiple Gigasampler
     * engine channels. The engine channels Borrow() instruments when they
     * need them and HandBack() when they don't need them anymore. The
     * InstrumentResourceManager loads the corresponding gig file and gig
     * instrument if needed, if it's already in use by another engine
     * channel, then it just returns the same resource, if an gig
     * instrument / file is not needed anymore, then it will be freed from
     * memory.
     */
    class InstrumentResourceManager : public InstrumentManagerBase< ::gig::File, ::gig::Instrument, ::gig::DimensionRegion, ::gig::Sample>, public InstrumentEditorListener {
        public:
            InstrumentResourceManager() : Gigs(this) {}
            virtual ~InstrumentResourceManager() {}
            static void OnInstrumentLoadingProgress(::gig::progress_t* pProgress);

            // implementation of derived abstract methods from 'InstrumentManager'
            virtual String GetInstrumentName(instrument_id_t ID) OVERRIDE;
            virtual String GetInstrumentDataStructureName(instrument_id_t ID) OVERRIDE;
            virtual String GetInstrumentDataStructureVersion(instrument_id_t ID) OVERRIDE;
            virtual InstrumentEditor* LaunchInstrumentEditor(LinuxSampler::EngineChannel* pEngineChannel, instrument_id_t ID, void* pUserData = NULL) throw (InstrumentManagerException) OVERRIDE;
            virtual std::vector<instrument_id_t> GetInstrumentFileContent(String File) throw (InstrumentManagerException) OVERRIDE;
            virtual instrument_info_t GetInstrumentInfo(instrument_id_t ID) throw (InstrumentManagerException) OVERRIDE;

            // implementation of derived abstract methods from 'InstrumentEditorListener'
            virtual void OnInstrumentEditorQuit(InstrumentEditor* pSender) OVERRIDE;
            virtual void OnSamplesToBeRemoved(std::set<void*> Samples, InstrumentEditor* pSender) OVERRIDE;
            virtual void OnSamplesRemoved(InstrumentEditor* pSender) OVERRIDE;
            virtual void OnDataStructureToBeChanged(void* pStruct, String sStructType, InstrumentEditor* pSender) OVERRIDE;
            virtual void OnDataStructureChanged(void* pStruct, String sStructType, InstrumentEditor* pSender) OVERRIDE;
            virtual void OnSampleReferenceChanged(void* pOldSample, void* pNewSample, InstrumentEditor* pSender) OVERRIDE;

#if 0 // currently unused :
            void TrySendNoteOnToEditors(uint8_t Key, uint8_t Velocity, ::gig::Instrument* pInstrument);
            void TrySendNoteOffToEditors(uint8_t Key, uint8_t Velocity, ::gig::Instrument* pInstrument);
#endif // unused

        protected:
            // implementation of derived abstract methods from 'ResourceManager'
            virtual ::gig::Instrument* Create(instrument_id_t Key, InstrumentConsumer* pConsumer, void*& pArg) OVERRIDE;
            virtual void               Destroy(::gig::Instrument* pResource, void* pArg) OVERRIDE;
            virtual void               DeleteRegionIfNotUsed(::gig::DimensionRegion* pRegion, region_info_t* pRegInfo) OVERRIDE;
            virtual void               DeleteSampleIfNotUsed(::gig::Sample* pSample, region_info_t* pRegInfo) OVERRIDE;
        private:
            void                       CacheInitialSamples(::gig::Sample* pSample, AbstractEngine* pEngine);
            void                       CacheInitialSamples(::gig::Sample* pSample, EngineChannel* pEngineChannel);
            void                       CacheInitialSamples(::gig::Sample* pSample, uint maxSamplesPerCycle);

            typedef ResourceConsumer< ::gig::File> GigConsumer;

            class GigResourceManager : public ResourceManager<String, ::gig::File> {
                protected:
                    // implementation of derived abstract methods from 'ResourceManager'
                    virtual ::gig::File* Create(String Key, GigConsumer* pConsumer, void*& pArg) OVERRIDE;
                    virtual void         Destroy(::gig::File* pResource, void* pArg) OVERRIDE;
                    virtual void         OnBorrow(::gig::File* pResource, GigConsumer* pConsumer, void*& pArg) OVERRIDE {} // ignore
                public:
                    GigResourceManager(InstrumentResourceManager* parent) : parent(parent) {}
                    virtual ~GigResourceManager() {}
                private:
                    InstrumentResourceManager* parent;
            } Gigs;

            void UncacheInitialSamples(::gig::Sample* pSample);
            std::vector< ::gig::Instrument*> GetInstrumentsCurrentlyUsedOf(::gig::File* pFile, bool bLock);
            std::set<EngineChannel*> GetEngineChannelsUsingScriptSourceCode(const String& code, bool bLock);
            std::set<EngineChannel*> GetEngineChannelsUsing(::gig::Instrument* pInstrument, bool bLock);
            std::set<Engine*> GetEnginesUsing(::gig::Instrument* pInstrument, bool bLock);
            std::set<Engine*> GetEnginesUsing(::gig::File* pFile, bool bLock);
            bool SampleReferencedByInstrument(::gig::Sample* pSample, ::gig::Instrument* pInstrument);
            void SuspendEnginesUsing(::gig::Instrument* pInstrument);
            void SuspendEnginesUsing(::gig::File* pFile);
            void ResumeAllEngines();

            Mutex InstrumentEditorProxiesMutex; ///< protects the 'InstrumentEditorProxies' map
            ArrayList<InstrumentConsumer*> InstrumentEditorProxies; ///< here we store the objects that react on instrument specific notifications on behalf of the respective instrument editor
            std::set<Engine*> suspendedEngines; ///< all engines currently completely suspended
            Mutex             suspendedEnginesMutex; ///< protects 'suspendedEngines' set
            std::map< ::gig::Script*,String> pendingScriptUpdates; ///< Used to prepare updates of instrument scripts (value of the map is the original source code of the script before it is modified).
            Mutex                            pendingScriptUpdatesMutex; ///< Protectes 'pendingScriptUpdates'.
    };

}} // namespace LinuxSampler::gig

#endif // __LS_GIG_INSTRUMENTRESOURCEMANAGER_H__
