/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009 Christian Schoenebeck and Grigor Iliev             *
 *   Copyright (C) 2014-2016 Christian Schoenebeck                         *
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

#ifndef __LS_GIG_ENGINE_H__
#define	__LS_GIG_ENGINE_H__

#include "DiskThread.h"
#include "../EngineBase.h"
#include "Voice.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

namespace LinuxSampler { namespace gig {

    class Engine: public LinuxSampler::EngineBase<Voice, ::gig::Region, ::gig::DimensionRegion, DiskThread, InstrumentResourceManager, ::gig::Instrument> {
        public:
            Engine() { }
            virtual ~Engine() { }
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
            friend class Voice;

        protected:
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

}} // namespace LinuxSampler::gig

#endif	/* __LS_GIG_ENGINE_H__ */

