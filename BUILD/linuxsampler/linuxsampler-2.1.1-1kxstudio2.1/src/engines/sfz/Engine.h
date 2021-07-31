/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2010 Grigor Iliev                                  *
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

#ifndef __LS_SFZ_ENGINE_H__
#define	__LS_SFZ_ENGINE_H__

#include "DiskThread.h"
#include "../EngineBase.h"
#include "Voice.h"
#include "sfz.h"
#include "EngineChannel.h" // prevents compile errors with clang 2.x

namespace LinuxSampler { namespace sfz {
    const int MaxCCPerVoice = 128; // FIXME: too much?
    
    class Engine: public LinuxSampler::EngineBase<Voice, ::sfz::Region, ::sfz::Region, DiskThread, InstrumentResourceManager, ::sfz::Instrument> {
        public:
            Engine();
            virtual ~Engine();
            // implementation of abstract methods derived from class 'LinuxSampler::Engine'
            virtual bool    DiskStreamSupported() OVERRIDE;
            virtual String  Description() OVERRIDE;
            virtual String  Version() OVERRIDE;
            
            virtual Format  GetEngineFormat() OVERRIDE;

            virtual void ProcessControlChange (
                LinuxSampler::EngineChannel*  pEngineChannel,
                Pool<Event>::Iterator&        itControlChangeEvent
            ) OVERRIDE;
            virtual void ProcessChannelPressure(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itChannelPressureEvent) OVERRIDE;
            virtual void ProcessPolyphonicKeyPressure(LinuxSampler::EngineChannel* pEngineChannel, Pool<Event>::Iterator& itNotePressureEvent) OVERRIDE;
            virtual void CreateInstrumentScriptVM() OVERRIDE;

            virtual void PostSetMaxVoices(int iVoices) OVERRIDE;

            friend class Voice;
            friend class SfzSignalUnitRack;

        protected:
            Pool<CCSignalUnit::CC>* pCCPool;
            Pool<Smoother>* pSmootherPool;
            
            virtual DiskThread* CreateDiskThread() OVERRIDE;

            virtual Pool<Voice>::Iterator LaunchVoice (
                LinuxSampler::EngineChannel* pEngineChannel,
                Pool<Event>::Iterator&       itNoteOnEvent,
                int                          iLayer,
                bool                         ReleaseTriggerVoice,
                bool                         VoiceStealing,
                bool                         HandleKeyGroupConflicts
            ) OVERRIDE;

            virtual void TriggerNewVoices (
                LinuxSampler::EngineChannel*  pEngineChannel,
                RTList<Event>::Iterator&      itNoteOnEvent,
                bool                          HandleKeyGroupConflicts
            ) OVERRIDE;

            void TriggerReleaseVoices (
                LinuxSampler::EngineChannel*  pEngineChannel,
                RTList<Event>::Iterator&      itNoteOffEvent
            ) OVERRIDE;
    };

}} // namespace LinuxSampler::sfz

#endif	/* __LS_SFZ_ENGINE_H__ */

