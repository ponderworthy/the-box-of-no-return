/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_INSTRUMENTLOADER_H__
#define __LS_INSTRUMENTLOADER_H__

#include "../common/global.h"
#include "../common/Thread.h"
#include "../common/Condition.h"
#include "../common/Mutex.h"
#include "../Sampler.h"
#include "../EventListeners.h"
#include "InstrumentManager.h"

#include <list>

namespace LinuxSampler {

    /** @brief Used by InstrumentManager for background tasks.
     *
     * This class is just an encapsulation of tasks that should be done by
     * the InstrumentManager in the background, that is in a separate thread
     * without blocking the calling thread. This class is thus not exported
     * to the API.
     */
    class InstrumentManagerThread : public Thread {
        friend class EventHandler;
        
        public:
            InstrumentManagerThread();
            void StartNewLoad(String Filename, uint uiInstrumentIndex, EngineChannel* pEngineChannel);
            void StartSettingMode(InstrumentManager* pManager, const InstrumentManager::instrument_id_t& ID, InstrumentManager::mode_t Mode);
            virtual ~InstrumentManagerThread();
#if (defined(__APPLE__) && !defined(__x86_64__)) || defined(WIN32)
            int StopThread();
#endif
        protected:
            struct command_t {
                enum cmd_type_t {
                    DIRECT_LOAD, ///< command was created by a StartNewLoad() call
                    INSTR_MODE   ///< command was created by a StartSettingMode() call
                } type;
                EngineChannel*                     pEngineChannel; ///< only for DIRECT_LOAD commands
                InstrumentManager*                 pManager;     ///< only for INSTR_MODE commands
                InstrumentManager::instrument_id_t instrumentId; ///< for both DIRECT_LOAD and INSTR_MODE
                InstrumentManager::mode_t          mode;         ///< only for INSTR_MODE commands
            };

            // Instance variables.
            std::list<command_t> queue; ///< queue with commands for loading new instruments.
            Mutex                mutex; ///< for making the queue thread safe 
            Condition            conditionJobsLeft; ///< synchronizer to block this thread until a new job arrives

            int Main(); ///< Implementation of virtual method from class Thread.
        private:
            class EventHandler : public ChannelCountAdapter {
                public:
                    InstrumentManagerThread* pThread;
                    virtual void ChannelToBeRemoved(SamplerChannel* pChannel);
            };
            
            EventHandler eventHandler;
    };

} // namespace LinuxSampler

#endif // __LS_INSTRUMENTLOADER_H__
