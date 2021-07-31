/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
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

#ifndef __LS_ENGINECHANNELFACTORY_H__
#define __LS_ENGINECHANNELFACTORY_H__

#include <set>

#include "EngineChannel.h"

namespace LinuxSampler {

    /** @brief Manages EngineChannel instances.
     *
     * This class is used to create and destroy EngineChannel instances of the
     * various engine types supported by this sampler. It's basically used to
     * avoid dependencies to palpable sampler engine types /
     * implementations.
     */
    class EngineChannelFactory {
        public:
            /**
             * Create EngineChannel instance of given engine type.
             *
             * @see EngineFactory::AvailableEngineTypes()
             */
            static EngineChannel* Create(String EngineType) throw (Exception);

            /**
             * Destroy given EngineChannel instance.
             */
            static void Destroy(EngineChannel* pEngineChannel);

            /**
             * Returns all EngineChannel instances.
             */
            static const std::set<EngineChannel*>& EngineChannelInstances();

            /**
             * Specifies whether the deallocation of the specified EngineChannel
             * object should be postponed. When the object deletion is diabled
             * it is not freed from memory (when destroyed) until it is enabled.
             * Used to prevent orphaned pointers.
             */
            static void SetDeleteEnabled(const EngineChannel* pEngineChannel, bool enable);

            static Mutex EngineChannelsMutex;

        private:
            static Mutex LockedChannelsMutex;
    };

} // namespace LinuxSampler

#endif // __LS_ENGINECHANNELFACTORY_H__
