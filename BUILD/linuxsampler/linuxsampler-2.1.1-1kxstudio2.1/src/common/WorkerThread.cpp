/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2013 Christian Schoenebeck, Grigor Iliev         *
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

#include "WorkerThread.h"

#include "Exception.h"

namespace LinuxSampler {

    WorkerThread::WorkerThread() : Thread(true, false, 0, -4) { }

    void WorkerThread::Execute(Runnable* pJob) {
        {
            LockGuard lock(mutex);
            queue.push_back(pJob);
        }
        StartThread(); // ensure thread is running
        conditionJobsLeft.Set(true); // wake up thread
    }

    // Entry point for the worker thread.
    int WorkerThread::Main() {
        while (true) {

            #if CONFIG_PTHREAD_TESTCANCEL
            TestCancel();
            #endif
            while (true) {
                Runnable* pJob;

                // grab a new job from the queue
                {
                    LockGuard lock(mutex);
                    if (queue.empty()) break;

                    pJob = queue.front();
                    queue.pop_front();
                }

                try {
                    pJob->Run();
                } catch (Exception e) {
                    e.PrintMessage();
                } catch (...) {
                    std::cerr << "WorkerThread: an exception occured, could not finish the job";
                    std::cerr << std::endl << std::flush;
                }

                delete pJob;
            }

            // nothing left to do, sleep until new jobs arrive
            conditionJobsLeft.WaitIf(false);
            // reset flag
            conditionJobsLeft.Set(false);
            // unlock condition object so it can be turned again by other thread
            conditionJobsLeft.Unlock();
        }
        return 0;
    }

} // namespace LinuxSampler
