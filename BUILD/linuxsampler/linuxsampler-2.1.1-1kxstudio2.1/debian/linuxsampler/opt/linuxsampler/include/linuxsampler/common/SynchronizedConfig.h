/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2006-2016 Andreas Persson                               *
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

#ifndef SYNCHRONIZEDCONFIG_H
#define SYNCHRONIZEDCONFIG_H

#include <set>
#include <unistd.h>
#include "lsatomic.h"
#include "Mutex.h"

namespace LinuxSampler {

    /**
     * Thread-safe management of configuration data, where the data is
     * updated by a single non real time thread and read by a number
     * of real time threads.
     *
     * The synchronization is achieved by using two instances of the
     * configuration data. The non real time thread gets access to the
     * instance not currently in use by the real time threads by
     * calling GetConfigForUpdate(). After the data is updated, the
     * non real time thread must call SwitchConfig() and redo the
     * update on the other instance. SwitchConfig() blocks until it is
     * safe to modify the other instance.
     *
     * The real time threads need one Reader object each to access the
     * configuration data. This object must be created outside the
     * real time thread. The Lock() function returns a reference to
     * the data to be read, and Unlock() must be called when finished
     * reading the data. (Neither Lock nor Unlock will block the real
     * time thread, or use any system calls.)
     *
     * Note that the non real time side isn't safe for concurrent
     * access, so if there are multiple non real time threads that
     * update the configuration data, a mutex has to be used.
     *
     * Implementation note: the memory order parameters and fences are
     * very carefully chosen to make the code fast but still safe for
     * memory access reordering made by the CPU.
     */
    template<class T>
    class SynchronizedConfig {
        public:
            SynchronizedConfig();

            // methods for the real time thread

            class Reader {
                public:
                    /**
                     * Gets the configuration object for use by the
                     * real time thread. The object is safe to use
                     * (read only) until Unlock() is called.
                     *
                     * @returns a reference to the configuration
                     *          object to be read by the real time
                     *          thread
                     */
                    /*const*/ T& Lock() { //TODO const currently commented for the DoubleBuffer usage below
                        lock.store(lockCount += 2, memory_order_relaxed);
                        atomic_thread_fence(memory_order_seq_cst);
                        return parent->config[parent->indexAtomic.load(
                                memory_order_acquire)];
                    }

                    /**
                     * Unlock the configuration object. Unlock() must
                     * be called by the real time thread after it has
                     * finished reading the configuration object. If
                     * the non real time thread is waiting in
                     * SwitchConfig() it will be awaken when no real
                     * time threads are locked anymore.
                     */
                    void Unlock() {
                        lock.store(0, memory_order_release);
                    }

                    Reader(SynchronizedConfig& config);
                    Reader(SynchronizedConfig* config);
                    virtual ~Reader();
                private:
                    friend class SynchronizedConfig;
                    SynchronizedConfig* parent;
                    int lockCount; // increased in every Lock(),
                                   // lowest bit is always set.
                    atomic<int> lock; // equals lockCount when inside
                                      // critical region, otherwise 0
                    Reader* next; // only used locally in SwitchConfig
                    int prevLock; // only used locally in SwitchConfig
            };


            // methods for the non real time thread

            /**
             * Gets the configuration object for use by the non real
             * time thread. The object returned is not in use by the
             * real time thread, so it can safely be updated. After
             * the update is done, the non real time thread must call
             * SwitchConfig() and the same update must be done again.
             *
             * @returns a reference to the configuration object to be
             *          updated by the non real time thread
             */
            T& GetConfigForUpdate();

            /**
             * Get the data on update side for read-only access.
             */
            const T& GetUnsafeUpdateConfig() const {
                return config[updateIndex];
            }

            /**
             * Atomically switch the newly updated configuration
             * object with the one used by the real time thread, then
             * wait for the real time thread to finish working with
             * the old object before returning the old object.
             * SwitchConfig() must be called by the non real time
             * thread after an update has been done, and the object
             * returned must be updated in the same way as the first.
             *
             * @returns a reference to the configuration object to be
             *          updated by the non real time thread
             */
            T& SwitchConfig();

        private:
            atomic<int> indexAtomic;
            int updateIndex;
            T config[2];
            std::set<Reader*> readers;
    };

    template<class T> SynchronizedConfig<T>::SynchronizedConfig() :
        indexAtomic(0) {
        updateIndex = 1;
    }

    template<class T> T& SynchronizedConfig<T>::GetConfigForUpdate() {
        return config[updateIndex];
    }

    template<class T> T& SynchronizedConfig<T>::SwitchConfig() {
        indexAtomic.store(updateIndex, memory_order_release);
        atomic_thread_fence(memory_order_seq_cst);

        // first put all locking readers in a linked list
        Reader* lockingReaders = 0;
        for (typename std::set<Reader*>::iterator iter = readers.begin() ;
             iter != readers.end() ;
             iter++) {
            (*iter)->prevLock = (*iter)->lock.load(memory_order_acquire);
            if ((*iter)->prevLock) {
                (*iter)->next = lockingReaders;
                lockingReaders = *iter;
            }
        }

        // wait until there are no locking readers left
        while (lockingReaders) {
            usleep(50000);
            Reader** prev = &lockingReaders;
            for (Reader* p = lockingReaders ; p ; p = p->next) {
                if (p->lock.load(memory_order_acquire) == p->prevLock) {
                    prev = &p->next;
                } else {
                    *prev = p->next; // unlink
                }
            }
        }

        updateIndex ^= 1;
        return config[updateIndex];
    }


    // ----- Reader ----

    template <class T>
    SynchronizedConfig<T>::Reader::Reader(SynchronizedConfig& config) :
        parent(&config), lockCount(1), lock(0) {
        parent->readers.insert(this);
    }
    
    template <class T>
    SynchronizedConfig<T>::Reader::Reader(SynchronizedConfig* config) :
        parent(config), lockCount(1), lock(0) {
        parent->readers.insert(this);
    }

    template <class T>
    SynchronizedConfig<T>::Reader::~Reader() {
        parent->readers.erase(this);
    }
    
    
    // ----- Convenience classes on top of SynchronizedConfig ----


    /**
     * Base interface class for classes that implement synchronization of data
     * shared between multiple threads.
     */
    template<class T>
    class Synchronizer {
    public:
        /**
         * Signal intention to enter a synchronized code block. Depending
         * on the actual implementation, this call may block the calling
         * thread until it is safe to actually use the protected data. After
         * this call returns, it is safe for the calling thread to access and
         * modify the shared data. As soon as the thread is done with accessing
         * the shared data, it MUST call endSync().
         *
         * @return the shared protected data
         */
        virtual void beginSync() = 0; //TODO: or call it lock() instead ?

        /**
         * Retrieve reference to critical, shared data. This method shall be
         * called between a beginSync() and endSync() call pair, to be sure
         * that shared data can be accessed safely.
         */
        virtual T& syncedData() = 0;

        /**
         * Signal that the synchronized code block has been left. Depending
         * on the actual implementation, this call may block the calling
         * thread for a certain amount of time.
         */
        virtual void endSync() = 0; //TODO: or call it unlock() instead ?
    };

    /**
     * Wraps as a kind of pointer class some data object shared with other
     * threads, to protect / synchronize the shared data against
     * undeterministic concurrent access. It does so by locking the shared
     * data in the Sync constructor and unlocking the shared data in the Sync
     * destructor. Accordingly it can always be considered safe to access the
     * shared data during the whole life time of the Sync object. Due to
     * this design, a Sync object MUST only be accessed and destroyed
     * by exactly one and the same thread which created that same Sync object.
     */
    template<class T>
    class Sync {
    public:
        Sync(Synchronizer<T>* syncer) {
            this->syncer = syncer;
            syncer->beginSync();
        }
        
        virtual ~Sync() {
            syncer->endSync();
        }
        
        /*Sync& operator =(const Sync& arg) {
            *this->data = *arg.data;
            return *this;
        }*/

        /*Sync& operator =(const T& arg) {
            *this->data = arg;
            return *this;
        }*/
        
        const T& operator *() const { return syncer->syncedData(); }
        T&       operator *()       { return syncer->syncedData(); }

        const T* operator ->() const { return &syncer->syncedData(); }
        T*       operator ->()       { return &syncer->syncedData(); }

    private:
        Synchronizer<T>* syncer; ///< Points to the object that shall be responsible to protect the shared data.
    };

    /**
     * BackBuffer object to be accessed by multiple non-real-time threads.
     *
     * Since a back buffer is designed for being accessed by non-real-time
     * threads, its methods involved may block the calling thread for a long
     * amount of time.
     */
    template<class T>
    class BackBuffer : public SynchronizedConfig<T>, public Synchronizer<T> {
    public:
        virtual void beginSync() OVERRIDE {
            mutex.Lock();
        }
        
        virtual T& syncedData() OVERRIDE {
            return SynchronizedConfig<T>::GetConfigForUpdate();
        }

        virtual void endSync() OVERRIDE {
            const T clone = SynchronizedConfig<T>::GetConfigForUpdate();
            SynchronizedConfig<T>::SwitchConfig() = clone;
            mutex.Unlock();
        }

        const T& unsafeData() const {
            return SynchronizedConfig<T>::GetUnsafeUpdateConfig();
        }

    private:
        Mutex mutex;
    };

    /**
     * FrontBuffer object to be accessed by exactly ONE real-time thread.
     * A front buffer is designed for real-time access. That is, its methods
     * involved are lock free, that is none of them block the calling thread
     * for a long time.
     *
     * If you need the front buffer's data to be accessed by multiple real-time
     * threads instead, then you need to create multiple instances of the
     * FrontBuffer object. They would point to the same data, but ensure
     * protection against concurrent access among those real-time threads.
     */
    template<class T>
    class FrontBuffer : public SynchronizedConfig<T>::Reader, public Synchronizer<T> {
    public:
        FrontBuffer(BackBuffer<T>& backBuffer) : SynchronizedConfig<T>::Reader::Reader(&backBuffer) {}
        virtual void beginSync() OVERRIDE { data = &SynchronizedConfig<T>::Reader::Lock(); }
        virtual T& syncedData() OVERRIDE { return *data; }
        virtual void endSync() OVERRIDE { SynchronizedConfig<T>::Reader::Unlock(); }
    private:
        T* data;
    };

    /**
     * Synchronization / protection of data shared between multiple threads by
     * using a double buffer design. The FrontBuffer is meant to be accessed by
     * exactly one real-time thread, whereas the BackBuffer is meant to be
     * accessed by multiple non-real-time threads.
     *
     * This class is built on top of SynchronizedConfig as convenient API to
     * reduce the amount of code required to protect shared data.
     */
    template<class T>
    class DoubleBuffer {
    public:
        DoubleBuffer() : m_front(m_back) {}
        
        /**
         * Synchronized access of the shared data for EXACTLY one real-time
         * thread.
         *
         * The returned shared data is wrapped into a Sync object, which
         * ensures that the shared data is protected against concurrent access
         * during the life time of the returned Sync object.
         */
        inline
        Sync<T> front() { return Sync<T>(&m_front); }
        
        /**
         * Synchronized access of the shared data for multiple non-real-time
         * threads.
         *
         * The returned shared data is wrapped into a Sync object, which
         * ensures that the shared data is protected against concurrent access
         * during the life time of the returned Sync object.
         *
         * As soon as the returned Sync object is destroyed, the FrontBuffer
         * will automatically be exchanged by the hereby modified BackBuffer.
         */
        inline
        Sync<T> back() { return Sync<T>(&m_back); }

        /**
         * Get the backbuffer data <b>unprotected</b>, that is <b>without</b>
         * locking or any means of synchronizations.
         *
         * Due to its nature this must only be called for read access and
         * you have to make sure by yourself, that the data/member you
         * access is really safe for concurrent read access (i.e. SGI's
         * implementation of std::vector::size() would be safe).
         *
         * Only use this when you are absolutely sure what you are doing!
         */
        const T& unsafeBack() const { return m_back.unsafeData(); }

    private:
        BackBuffer<T> m_back; ///< Back buffer (non real-time thread(s) side).
        FrontBuffer<T> m_front; ///< Front buffer (real-time thread side).
    };

} // namespace LinuxSampler

#endif
