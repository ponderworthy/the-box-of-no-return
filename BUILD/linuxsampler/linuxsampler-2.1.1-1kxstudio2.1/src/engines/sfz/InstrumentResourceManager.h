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

#ifndef __LS_SFZ_INSTRUMENTRESOURCEMANAGER_H__
#define __LS_SFZ_INSTRUMENTRESOURCEMANAGER_H__

#include "../../common/global.h"

#include "sfz.h"

#include "../../common/global.h"
#include "../../common/Pool.h"
#include "../../common/ResourceManager.h"
#include "../../drivers/audio/AudioOutputDevice.h"
#include "../InstrumentManagerBase.h"
#include "../common/Sample.h"
#include "../../common/ArrayList.h"

namespace LinuxSampler { namespace sfz {

    typedef ResourceConsumer< ::sfz::Instrument> InstrumentConsumer;

}} // namespace LinuxSampler::sfz

#include "../../plugins/InstrumentEditor.h"

namespace LinuxSampler { namespace sfz {
    class Engine;
    class EngineChannel;

    /** @brief sfz instrument manager
     *
     * Manager to share sfz instruments between multiple sfz
     * engine channels. The engine channels Borrow() instruments when they
     * need them and HandBack() when they don't need them anymore. The
     * InstrumentResourceManager loads the corresponding sfz file and sfz
     * instrument if needed, if it's already in use by another engine
     * channel, then it just returns the same resource, if a sfz
     * instrument / file is not needed anymore, then it will be freed from
     * memory.
     */
    class InstrumentResourceManager : public InstrumentManagerBase< ::sfz::File, ::sfz::Instrument, ::sfz::Region, Sample> {
        public:
            InstrumentResourceManager() : Sfzs(this) {}
            virtual ~InstrumentResourceManager() {}

            // implementation of derived abstract methods from 'InstrumentManager'
            virtual String GetInstrumentName(instrument_id_t ID);
            virtual std::vector<instrument_id_t> GetInstrumentFileContent(String File) throw (InstrumentManagerException);
            virtual instrument_info_t GetInstrumentInfo(instrument_id_t ID) throw (InstrumentManagerException);

            ::sfz::SampleManager* GetSampleManager() { return &Sfzs.sampleManager; }

        protected:
            // implementation of derived abstract methods from 'ResourceManager'
            virtual ::sfz::Instrument* Create(instrument_id_t Key, InstrumentConsumer* pConsumer, void*& pArg);
            virtual void               Destroy(::sfz::Instrument* pResource, void* pArg);
            virtual void               DeleteRegionIfNotUsed(::sfz::Region* pRegion, region_info_t* pRegInfo);
            virtual void               DeleteSampleIfNotUsed(Sample* pSample, region_info_t* pRegInfo);
        private:
            typedef ResourceConsumer< ::sfz::File> SfzConsumer;

            class SfzResourceManager : public ResourceManager<String, ::sfz::File> {
                protected:
                    // implementation of derived abstract methods from 'ResourceManager'
                    virtual ::sfz::File* Create(String Key, SfzConsumer* pConsumer, void*& pArg);
                    virtual void         Destroy(::sfz::File* pResource, void* pArg);
                    virtual void         OnBorrow(::sfz::File* pResource, SfzConsumer* pConsumer, void*& pArg) {} // ignore
                public:
                    SfzResourceManager(InstrumentResourceManager* parent) : parent(parent) {}
                    virtual ~SfzResourceManager() {}
                    friend class InstrumentResourceManager;
                private:
                    InstrumentResourceManager* parent;
                    ::sfz::SampleManager sampleManager;
            } Sfzs;
    };

}} // namespace LinuxSampler::sfz

#endif // __LS_SFZ_INSTRUMENTRESOURCEMANAGER_H__
