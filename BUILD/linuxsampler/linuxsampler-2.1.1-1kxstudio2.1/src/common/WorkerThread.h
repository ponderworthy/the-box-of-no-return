/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 Grigor Iliev                                       *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef __WORKER_THREAD_H__
#define __WORKER_THREAD_H__

#include <list>

#include "Condition.h"
#include "Mutex.h"
#include "global_private.h"
#include "Thread.h"

namespace LinuxSampler {
    class WorkerThread : public Thread {
        public:
            WorkerThread();

            /**
             * Schedules the specified job for execution.
             * Note that the specified Runnable object must be allocated
             * using the new operator. The object is automatically deleted
             * when the job is finished.
             */
            void Execute(Runnable* pJob);

            /**
             * The entry point of the thread.
             */
            virtual int Main() OVERRIDE;

        private:
            std::list<Runnable*> queue; // the queue, which holds the jobs to be executed
            Mutex                mutex; // used to synchronize the queue
            Condition            conditionJobsLeft; // synchronizer to block this thread until a new job arrives
    };

} // namespace LinuxSampler

#endif // __WORKER_THREAD_H__
