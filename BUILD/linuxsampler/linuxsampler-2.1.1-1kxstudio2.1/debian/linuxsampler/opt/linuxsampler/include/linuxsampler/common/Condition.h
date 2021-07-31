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

#ifndef __CONDITION_H__
#define __CONDITION_H__

#include "Mutex.h"

#if defined(WIN32)
#include <windows.h>
#endif

namespace LinuxSampler {

#if defined(WIN32)
class ConditionInternal;
#endif

/**
 * Thread safe boolean condition.
 *
 * This is not meant to be used for real time operation!
 */
class Condition : public Mutex {
    public:
        /** @brief Constructor
         *
         * Creates a new thread safe condition variable.
         *
         * Note that the default bahavior of the underlying mutex is
         * @c NON_RECURSIVE by default, because in general if your design
         * requires the Condition object's lock state to be recursive instead,
         * then most probably this may result in dead locks or even undefined
         * behavior, because the underlying OS API for conditions may not be
         * compatible with recursive mutexes!
         *
         * @param bInitialCondition - optional: starting condition
         *                            (default = false)
         * @param mutexType - optional: fundamental behavior of underlying mutex
         *                    (default: @c NON_RECURSIVE)
         */
        Condition(bool bInitialCondition = false, Mutex::type_t mutexType = Mutex::NON_RECURSIVE);

        /**
         * Destructor
         */
        virtual ~Condition();

        /**
         * Blocks the calling thread if current condition equals
         * \a bCondition, in this case the calling thread will be blocked
         * until condition turns. Upon successful return the Condition
         * object is locked, so the calling thread can safely run it's
         * critical section and has to explicitly call Unlock() right after
         * it left it's critcal section.
         *
         * @e Note: If you don't provide a timeout value or if you provide a
         * timeout value of exactly 0s and 0ns, then this call will block
         * without any timeout, or in other words: @e infinity!
         *
         * @param bCondition         - block in case of this condition
         * @param TimeoutSeconds     - optional: max. wait time in seconds
         *                             (default: 0s)
         * @param TimeoutNanoSeconds - optional: max wait time in nano
         *                             seconds (default: 0ns)
         * @returns  0 on success, a value less than 0 if timeout exceeded
         */
        int WaitIf(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);

        /**
         * Same as WaitIf(), except that WaitAndUnlockIf() will unlock the
         * Condition object, so only use this call if you don't need to
         * enter a thread critical section, otherwise use WaitIf() instead!
         *
         * @e Note: If you don't provide a timeout value or if you provide a
         * timeout value of exactly 0s and 0ns, then this call will block
         * without any timeout, or in other words: @e infinity!
         *
         * @param bCondition         - block in case of this condition
         * @param TimeoutSeconds     - optional: max. wait time in seconds
         *                             (default: 0s)
         * @param TimeoutNanoSeconds - optional: max wait time in nano
         *                             seconds (default: 0ns)
         * @returns  0 on success, a value less than 0 if timeout exceeded
         * @see WaitIf()
         */
        int WaitAndUnlockIf(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);

        /**
         * You should use this method instead of WaitIf() in case the calling
         * thread already owns the Condition object's underlying mutex lock by
         * previously calling Lock() before. Essentially the only difference to
         * WaitIf() is that PreLockedWaitIf() does not call Lock() by itself.
         */
        int PreLockedWaitIf(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);

        /**
         * You should use this method instead of WaitAndUnlockIf() in case the
         * calling thread already owns the Condition object's underlying mutex
         * lock by previously calling Lock() before. Essentially the only
         * difference to WaitAndUnlockIf() is that PreLockedWaitAndUnlockIf()
         * does not call Lock() by itself.
         */
        int PreLockedWaitAndUnlockIf(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);

        /**
         * Set Condition object to \a bCondition. Upon change of the
         * condition, other threads waiting for \a bCondition will be
         * awakened. (Note the condition will not be locked for the calling
         * thread after this method returns!)
         *
         * @param bCondition - new condition
         */
        void Set(bool bCondition);

        /**
         * You should use this method instead of Set() in case the calling
         * thread already owns the Condition object's underlying mutex lock by
         * previously calling Lock() before. Essentially the only difference to
         * Set() is that PreLockedSet() does not call Lock() by itself.
         */
        void PreLockedSet(bool bCondition);

        /**
         * Returns the current boolean state of this condition object. This
         * method never blocks, it returns immediately and doesn't use any
         * system calls.
         *
         * @e Caution: this method is not thread safe! If you need to use
         * the condition state in a thread critical context you must call
         * @c Lock() and @c Unlock() respectively by yourself!
         */
        bool GetUnsafe();

#ifdef WIN32
        /**
         * Resets the condition. This is only needed on Windows, after
         * a thread waiting for a condition has been stopped with
         * StopThread.
         */
        void Reset();
#endif

    protected:
        int WaitIfInternal(bool bLock, bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds);
        void SetInternal(bool bLock, bool bCondition);

    #if defined(WIN32)
        friend class ConditionInternal;
        struct win32thread_cond_t {
            int waiters_count_; ///< Number of waiting threads.
            CRITICAL_SECTION waiters_count_lock_; ///< Serialize access to <waiters_count_>.
            HANDLE sema_; ///< Semaphore used to queue up threads waiting for the condition to become signaled.
            HANDLE waiters_done_; ///< An auto-reset event used by the broadcast/signal thread to wait for all the waiting thread(s) to wake up and be released from the semaphore.
            size_t was_broadcast_; ///< Keeps track of whether we were broadcasting or signaling. This allows us to optimize the code if we're just signaling.
        } __win32_true_condition, __win32_false_condition;
    #else
        pthread_cond_t __posix_true_condition;
        pthread_cond_t __posix_false_condition;
    #endif
        bool bCondition;
};

} // namespace LinuxSampler

#endif // __CONDITION_H__
