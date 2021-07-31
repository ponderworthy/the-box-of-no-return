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

#ifndef __LS_SF2_ENGINECHANNEL_H__
#define	__LS_SF2_ENGINECHANNEL_H__

#include "../AbstractEngine.h"
#include "../EngineChannelBase.h"
#include "../EngineChannelFactory.h"
#include "Voice.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/SF.h>
#else
# include <SF.h>
#endif

namespace LinuxSampler { namespace sf2 {
    class Voice;

    class EngineChannel: public LinuxSampler::EngineChannelBase<Voice, ::sf2::Region, ::sf2::Preset> {
        public:
            virtual void SendProgramChange(uint8_t Program) OVERRIDE;
            virtual void LoadInstrument() OVERRIDE;
            virtual void ResetInternal(bool bResetEngine) OVERRIDE;

            virtual AbstractEngine::Format GetEngineFormat() OVERRIDE;

            friend class Voice;
            friend class Engine;
            friend class LinuxSampler::EngineChannelFactory;

        protected:
            EngineChannel();
            virtual ~EngineChannel();

            virtual void ProcessKeySwitchChange(int key) OVERRIDE;

        private:
            bool PressedKeys[128];
            int LastKeySwitch;
            int LastKey;
    };

}} // namespace LinuxSampler::sf2

#endif	/* __LS_SF2_ENGINECHANNEL_H__ */
