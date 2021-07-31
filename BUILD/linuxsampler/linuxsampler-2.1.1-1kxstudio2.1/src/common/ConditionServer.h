/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2008 Christian Schoenebeck                       *
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

#ifndef __CONDITIONSERVER_H__
#define __CONDITIONSERVER_H__

#include "Mutex.h"
#include "SynchronizedConfig.h"

namespace LinuxSampler {

/**
 * Thread safe condition for semi real time operation
 *
 * Sense behind the ConditionServer is to sync a boolean condition between
 * one real time thread (RTT) and n non real time threads (NRTT).
 *
 *   1 Real Time Thread <--> Condition Object <--> n Non Real Time Threads
 *
 * The non real time threads set the condition by calling Push(), this
 * method will immediately return if the current condition already equals
 * the desired condition given with Push(), if it's not equal then Push()
 * will cause a request to change the condition and will block until the
 * condition actually changed, which happens when the RTT is outside its
 * critical region, or has called the Pop() method, which will also return
 * the new condition to the RTT.
 *
 * Advantage of this technique is that the RTT doesn't has to execute system
 * calls whenever it wants to check for a condition change. Disadvantage on
 * the other hand is that the NRTTs might be blocked for some time, but this
 * is usually unproblematic for NRTTs.
 */
class ConditionServer {
    public:
        ConditionServer();


        // methods for non realtime threads (0..n NRTTs)

        /**
         * Set condition to \a bCondition. If current condition already
         * equals \a bCondition, then this method will immediately return,
         * if not it will block the calling thread until the condition
         * actually changed to the requested condition (which happens when
         * Pop() has been called by the real time thread, or when the real
         * time thread is outside its critical region). If there are multiple
         * non real time threads calling Push() only one by one will be
         * served, all others blocked meanwhile. When the calling thread
         * returns from Push() the Push() method will still be blocked
         * against other NRTTs, so the thread can safely enter a critical
         * section and has to Unlock() right after it left it's critical
         * section, so other NRTTs can pass Push().
         *
         * @param bCondition         - condition to set
         * @param TimeoutSeconds     - optional: max. wait time in seconds
         *                             (default: 0s)
         * @param TimeoutNanoSeconds - optional: max wait time in nano
         *                             seconds (default: 0ns)
         * @returns  bool pointer with condition before Push() call, NULL if
         *           timeout exceeded
         * @see Unlock()
         */
        bool* Push(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);

        /**
         * Same as Push(), except that PushAndUnlock() will unlock the
         * ConditionServer right after so that other NRTTs can follow to
         * pass push. You should only call this method if you're sure that
         * no other NRTT will change the condition, otherwise you should
         * call Push() instead, execute the critical section and manually
         * unlock at the end of the critical section.
         *
         * @param bCondition         - condition to set
         * @param TimeoutSeconds     - optional: max. wait time in seconds
         *                             (default: 0s)
         * @param TimeoutNanoSeconds - optional: max wait time in nano
         *                             seconds (default: 0ns)
         * @param bAlreadyLocked     - optional: you must set this to true if
         *                             you have called Push() before and are
         *                             using PushAndUnlock to end the
         *                             critical region (default: false)
         * @returns  bool pointer with condition before PushAndUnlock()
         *           call, NULL if timeout exceeded
         */
        bool* PushAndUnlock(bool bCondition, long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L, bool bAlreadyLocked = false);

        void PushAndUnlock2(bool bCondition);

        /**
         * Should be called by the NRTT after it left it's critical section
         * to unlock the ConditionServer and give other NRTTs the chance to
         * pass Push().
         */
        void Unlock();

        /**
         * Returns unsafely the current condition. This method will not
         * block and should only be used in a not thread critical context.
         *
         * @returns  current condition (unsafe)
         */
        bool GetUnsafe();



        // method for real time thread (1 RTT)

        /**
         * Should be called by the real time thread (RTT) at the
         * beginning of the critical region to check for a condition
         * change. If an NRTT requested a condition change by calling
         * Push() or PushAndUnlock() the NRTT will be blocked until
         * it knows that the RTT will not go into its critical region
         * without being notified about the condition change.
         *
         * @returns  current condition
         */
        bool Pop() {
            return Reader.Lock();
        }

        /**
         * Should be called by the real time thread (RTT) at the end of
         * the critical region.
         */
        void RttDone() {
            Reader.Unlock();
        }

    protected:
        SynchronizedConfig<bool> Condition;
        SynchronizedConfig<bool>::Reader Reader;
        bool      bOldCondition;
        Mutex     PushMutex;
};

} // namespace LinuxSampler

#endif // __CONDITIONSERVER_H__
