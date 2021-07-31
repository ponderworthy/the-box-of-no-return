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

#ifndef __LS_THREAD_H__
#define __LS_THREAD_H__

//FIXME: this is a temorary solution because of problems with condition variables we use a polling lock in SignalStartThread()
#if defined(WIN32)
#define WIN32_SIGNALSTARTTHREAD_WORKAROUND 1
#endif

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <sched.h>
#include <sys/mman.h>
#include <memory.h>
#include <pthread.h>
#endif
#include <errno.h>

#include "Condition.h"

namespace LinuxSampler {

/**
 * Abstract base class for classes that need to run in an own thread. The
 * deriving class needs to implement the abstract method Main() for the actual
 * task of the thread to be implemented.
 *
 * @b IMPORTANT: You @b MUST call StopThread() before destructing a running
 * Thread object! Destructing the Thread object without stopping it first can
 * lead to undefined behavior! The destructor will detect if the thread is still
 * running and will stop it automatically in this case, however once the
 * destructor is entered, this Thread object's vpointer is already been
 * modified, so if the thread is still running at this point this would cause a
 * data race condition since the active thread is calling virtual methods and
 * thus this can lead to undefined behavior.
 */
class Thread {
    public:
        Thread(bool LockMemory, bool RealTime, int PriorityMax, int PriorityDelta);
        virtual ~Thread();
        virtual int  StartThread();
        virtual int  StopThread();
        virtual int  SignalStopThread();
        virtual bool IsRunning();

        /**
         * Allocates an aligned block of memory. Allocated memory blocks
         * need to be freed using freeAlignedMem().
         *
         * @param boundary - the alignement boundary, usually a power of 2
         *                   e.g. 4 but it can be an arbitrary number
         *                   between 1 and 128
         * @param size     - size in bytes to be allocated
         * @returns  pointer to the allocated memory block
         */
        static void* allocAlignedMem(size_t boundary, size_t size) {
            unsigned char *ptr = (unsigned char *)malloc(size+boundary);
            size_t offset = boundary - ((size_t)ptr % boundary);
            ptr[offset-1] = (unsigned char)offset;
            return (ptr + offset);
        }

        /**
         * Frees an aligned block of memory allocated with allocAlignedMem()
         *
         * @param ptr - pointer to the memory block
         */
        static void freeAlignedMem(void *ptr) {
            unsigned char *p = (unsigned char *)ptr;
            p -= p[-1];
            free(p);
        }

        /**
         * Locks a region of memory in physical RAM.
         *
         * @param addr - address of the memory block
         * @param size - size of the memory block
         * @return true if the locking succeded, otherwise false
         */
        static bool lockMemory(void *addr, size_t size) {
            #if defined(WIN32)
            return VirtualLock(addr, size);
            #else
            return !mlock(addr, size);
            #endif
        }

        /**
         * Unlocks a region of memory in physical RAM.
         *
         * @param addr - address of the memory block
         * @param size - size of the memory block
         * @return true if the unlocking succeded, otherwise false
         */
        static bool unlockMemory(void *addr, size_t size) {
            #if defined(WIN32)
            return VirtualUnlock(addr, size);
            #else
            return !munlock(addr, size);
            #endif
        }

    protected:
        /**
         * This method needs to be implemented by the descending class and is
         * the entry point for the new thread.
         *
         * @b NOTE: If your thread runs for a longer time, i.e. if it is running
         * a loop, then you should explicitly call TestCancel() once in a while
         * in your Main() implementation, especially if your implementation does
         * not use any system calls.
         */
        virtual int Main() = 0;

        /**
         * Synchronization point for potentially terminating the thread. Like
         * already described in Main() you should call TestCancel() in your
         * Main() implementation once in a while to provide the system a chance
         * to perform a clean termination of your thread. Depending on the
         * underlying OS, and also depending on whether your are using any
         * system call in your Main() implementation, it might otherwise be
         * possible that the thread cannot be terminated at all! And even if the
         * underlying OS supports terminating busy threads which do not call
         * TestCancel(), this might still cause undefined behavior on such OSes!
         */
        void TestCancel();

        virtual int  SignalStartThread();
        virtual int  SetSchedulingPriority();
        virtual int  LockMemory();
        virtual void EnableDestructor();
        virtual int  onThreadEnd();

    private:
        enum state_t {
            NOT_RUNNING,
            RUNNING,
            PENDING_JOIN,
            DETACHED
        };

    #if defined(WIN32)
        HANDLE hThread;
        DWORD lpThreadId;
        #if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
        bool win32isRunning;
        #endif
    #else
        pthread_attr_t  __thread_attr;
        pthread_t       __thread_id;
        pthread_key_t   __thread_destructor_key;
    #endif
        Condition       RunningCondition;
        int             PriorityMax;
        int             PriorityDelta;
        bool            isRealTime;
        bool            bLockedMemory;
        state_t         state;

    #if defined(WIN32)
        static DWORD WINAPI win32threadLauncher(LPVOID lpParameter);
    #else
        static void* pthreadLauncher(void* thread);
        static void  pthreadDestructor(void* thread);
    #endif
};

} // namespace LinuxSampler

#endif // __LS_THREAD_H__
