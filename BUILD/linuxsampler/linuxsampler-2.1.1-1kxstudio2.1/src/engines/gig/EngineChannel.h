/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
 *   Copyright (C) 2009 - 2012 Christian Schoenebeck and Grigor Iliev      *
 *   Copyright (C) 2012 - 2016 Christian Schoenebeck and Andreas Persson   *
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

#ifndef __LS_GIG_ENGINECHANNEL_H__
#define	__LS_GIG_ENGINECHANNEL_H__

#include "../AbstractEngine.h"
#include "../EngineChannelBase.h"
#include "../EngineChannelFactory.h"
#include "Voice.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

namespace LinuxSampler { namespace gig {
    class Voice;

    class EngineChannel: public LinuxSampler::EngineChannelBase<Voice, ::gig::DimensionRegion, ::gig::Instrument> {
        public:
            virtual void SendProgramChange(uint8_t Program) OVERRIDE;
            virtual void LoadInstrument() OVERRIDE;
            virtual void ResetInternal(bool bResetEngine) OVERRIDE;
            virtual String InstrumentFileName() OVERRIDE;
            virtual String InstrumentFileName(int index) OVERRIDE;

            virtual AbstractEngine::Format GetEngineFormat() OVERRIDE;

            void reloadScript(::gig::Script* script);

            friend class Voice;
            friend class Engine;
            friend class LinuxSampler::EngineChannelFactory;

        protected:
            EngineChannel();
            virtual ~EngineChannel();

            float CurrentKeyDimension;      ///< Current value (0-1.0) for the keyboard dimension, altered by pressing a keyswitching key.
            ::gig::Script* CurrentGigScript; ///< Only used when a script is updated (i.e. by instrument editor), to check whether this engine channel is actually using that specific script reference.

            virtual void ProcessKeySwitchChange(int key) OVERRIDE;

    };

}} // namespace LinuxSampler::gig

#endif	/* __LS_GIG_ENGINECHANNEL_H__ */
