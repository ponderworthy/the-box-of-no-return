/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2007 Christian Schoenebeck                       *
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

#ifndef __RESOURCE_MANAGER__
#define __RESOURCE_MANAGER__

#include <set>
#include <map>
#include <vector>

#include "Exception.h"
#include "Mutex.h"

namespace LinuxSampler {

/** @brief Interface class for Resource Consumers.
 *
 * Interface class for consumer classes which use a resource managed
 * by the ResourceManager class. All classes which use the ResourceManager
 * to aquire resources have to derive from this interface class and
 * implement the abstract methods.
 */
template<class T_res>
class ResourceConsumer {
    public:
        /**
         * Will be called by the ResourceManager to inform the
         * consumer that a resource currently used by him is going
         * to be updated. The consumer can then react by stopping
         * usage until resource is updated. The ResourceManager will
         * not update the resource until this method returns. This
         * method needs to be implemented by the consumer.
         *
         * @param pResource  - resource going to be updated
         * @param pUpdateArg - pointer the consumer might use to store
         *                     informations he might need when update
         *                     process was completed
         */
        virtual void ResourceToBeUpdated(T_res* pResource, void*& pUpdateArg) = 0;

        /**
         * Will be called by the ResourceManager to inform the
         * consumer that resource update was completed. This method
         * needs to be implemented by the consumer.
         *
         * @param pOldResource - (now invalid) pointer to the old
         *                       resource
         * @param pNewResource - (valid) pointer to the updated
         *                       resource
         * @param pUpdateArg   - pointer the consumer might have used when
         *                       ResourceToBeUpdated() was called
         */
        virtual void ResourceUpdated(T_res* pOldResource, T_res* pNewResource, void* pUpdateArg) = 0;

        /**
         * Might be called by the ResourceManager periodically during an
         * update / creation of a resource to inform the consumer about the
         * current progress of that process. This method needs to be
         * implemented by the consumer.
         *
         * @param fProgress - current progress as value between 0.0 and 1.0
         */
        virtual void OnResourceProgress(float fProgress) = 0;
};

/** @brief Manager for sharing resources.
 *
 * Abstract base class for sharing resources between multiple consumers.
 * A consumer can borrow a resource from the ResourceManager, if the
 * resource doesn't exist yet it will be created. Other consumers will
 * just be given the same pointer to the resource then. When all consumers
 * gave back their pointer to the resource, the resource will (by default)
 * be destroyed.
 *
 * Descendants of this base class have to implement the (protected)
 * Create() and Destroy() methods to create and destroy a resource.
 *
 * This class is thread safe (by default). Its methods should however not
 * be called in a realtime context due to this! Alternatively one can 
 * call the respective methods with bLock = false, in that case thread
 * safety mechanisms will be omitted - use with care !
 */
template<class T_key, class T_res>
class ResourceManager {
    public:
        /**
         * Defines life-time strategy for resources.
         */
        enum mode_t {
            ON_DEMAND      = 0, ///< Create resource when needed, free it once not needed anymore (default behavior).
            ON_DEMAND_HOLD = 1, ///< Create resource when needed and keep it even if not needed anymore.
            PERSISTENT     = 2  ///< Immediately create resource and keep it.
        };

        typedef std::set<ResourceConsumer<T_res>*> ConsumerSet;

    private:
        struct resource_entry_t {
            T_key       key;
            T_res*      resource;  ///< pointer to the resource
            mode_t      mode;      ///< When should the resource be created? When should it be destroyed?
            ConsumerSet consumers; ///< list of all consumers who currently use the resource
            void*       lifearg;   ///< optional pointer the descendant might use to store informations about a created resource
            void*       entryarg;  ///< optional pointer the descendant might use to store informations about an entry
        };
        typedef std::map<T_key, resource_entry_t> ResourceMap;
        ResourceMap ResourceEntries;
        Mutex       ResourceEntriesMutex; // Mutex for protecting the ResourceEntries map

    public:
        /**
         * Returns (the keys of) all current entries of this
         * ResourceManager instance.
         *
         * @param bLock - use thread safety mechanisms
         */
        std::vector<T_key> Entries(bool bLock = true) {
            std::vector<T_key> result;
            if (bLock) ResourceEntriesMutex.Lock();
            for (typename ResourceMap::iterator iter = ResourceEntries.begin();
                 iter != ResourceEntries.end(); iter++)
            {
                result.push_back(iter->first);
            }
            if (bLock) ResourceEntriesMutex.Unlock();
            return result;
        }

        /**
         * Returns a list of all consumers that use the resource given by
         * \a Key .
         *
         * @e Caution: this is not thread safe! You might want to call
         * @c Lock() and @c Unlock() respectively to avoid race conditions
         * while using this method and its result set!
         *
         * @param Key - resource identifier
         */
        ConsumerSet ConsumersOf(T_key Key) {
            // search for an entry for the given resource key
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            return (iterEntry == ResourceEntries.end()) ?
                   ConsumerSet() : iterEntry->second.consumers;
        }

        /**
         * Returns a list of all consumers that use the resource given by
         * \a pResource .
         *
         * @e Caution: this is not thread safe! You might want to call
         * @c Lock() and @c Unlock() respectively to avoid race conditions
         * while using this method and its result set!
         *
         * @param pResource - resource pointer
         */
        ConsumerSet ConsumersOf(T_res* pResource) {
            // search for the entry associated with the given resource
            typename ResourceMap::iterator iter = ResourceEntries.begin();
            typename ResourceMap::iterator end  = ResourceEntries.end();
            for (; iter != end; iter++) {
                if (iter->second.resource == pResource) { // found entry for resource
                    return iter->second.consumers;
                }
            }
            // no entry found for resource ...
            return ConsumerSet();
        }

        /**
         * Borrow a resource identified by \a Key. The ResourceManager will
         * mark the resource as in usage by the consumer given with
         * \a pConsumer. If the Resource doesn't exist yet it will be
         * created.
         *
         * @param Key       - resource identifier
         * @param pConsumer - identifier of the consumer who borrows it
         * @param bLock     - use thread safety mechanisms
         * @returns  pointer to resource
         */
        T_res* Borrow(T_key Key, ResourceConsumer<T_res>* pConsumer, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            // search for an entry for this resource
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            if (iterEntry == ResourceEntries.end()) { // entry doesn't exist yet
                // already create an entry for the resource
                resource_entry_t entry;
                entry.key      = Key;
                entry.resource = NULL;
                entry.mode     = ON_DEMAND; // default mode
                entry.lifearg  = NULL;
                entry.entryarg = NULL;
                entry.consumers.insert(pConsumer);
                ResourceEntries[Key] = entry;
                try {
                    // actually create the resource
                    entry.resource = Create(Key, pConsumer, entry.lifearg);
                } catch (...) {
                    // creating the resource failed, so remove the entry
                    ResourceEntries.erase(Key);
                    if (bLock) ResourceEntriesMutex.Unlock();
                    // rethrow the same exception
                    throw;
                }
                // now update the entry with the created resource
                ResourceEntries[Key] = entry;
                OnBorrow(entry.resource, pConsumer, entry.lifearg);
                if (bLock) ResourceEntriesMutex.Unlock();
                return entry.resource;
            } else { // entry already exists
                resource_entry_t& entry = iterEntry->second;
                if (!entry.resource) { // create resource if not created already
                    try {
                        entry.resource = Create(Key, pConsumer, entry.lifearg);
                    } catch (...) {
                        entry.resource = NULL;
                        if (bLock) ResourceEntriesMutex.Unlock();
                        throw; // rethrow the same exception
                    }
                }
                entry.consumers.insert(pConsumer);
                OnBorrow(entry.resource, pConsumer, entry.lifearg);
                if (bLock) ResourceEntriesMutex.Unlock();
                return entry.resource;
            }
        }

        /**
         * Give back a resource. This tells the ResourceManager that the
         * consumer given by \a pConsumer doesn't need the resource anymore.
         * If the resource is not needed by any consumer anymore and the
         * resource has a life-time strategy of ON_DEMAND (which is the
         * default setting) then the resource will be destroyed.
         *
         * @param pResource - pointer to resource
         * @param pConsumer - identifier of the consumer who borrowed the
         *                    resource
         * @param bLock     - use thread safety mechanisms
         */
        void HandBack(T_res* pResource, ResourceConsumer<T_res>* pConsumer, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            // search for the entry associated with the given resource
            typename ResourceMap::iterator iter = ResourceEntries.begin();
            typename ResourceMap::iterator end  = ResourceEntries.end();
            for (; iter != end; iter++) {
                if (iter->second.resource == pResource) { // found entry for resource
                    resource_entry_t& entry = iter->second;
                    entry.consumers.erase(pConsumer);
                    // remove entry if necessary
                    if (entry.mode == ON_DEMAND && !entry.entryarg && entry.consumers.empty()) {
                        T_res* resource = entry.resource;
                        void* arg       = entry.lifearg;
                        ResourceEntries.erase(iter);
                        // destroy resource if necessary
                        if (resource) Destroy(resource, arg);
                    }
                    if (bLock) ResourceEntriesMutex.Unlock();
                    return;
                }
            }
            if (bLock) ResourceEntriesMutex.Unlock();
        }

        /**
         * Request update of a resource. All consumers will be informed
         * about the pending update of the resource so they can safely react
         * by stopping its usage first, then the resource will be recreated
         * and finally the consumers will be informed once the update was
         * completed, so they can continue to use the resource.
         *
         * @param pResource - resource to be updated
         * @param pConsumer - consumer who requested the update
         * @param bLock     - use thread safety mechanisms
         */
        void Update(T_res* pResource, ResourceConsumer<T_res>* pConsumer, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            typename ResourceMap::iterator iter = ResourceEntries.begin();
            typename ResourceMap::iterator end  = ResourceEntries.end();
            for (; iter != end; iter++) {
                if (iter->second.resource == pResource) {
                    resource_entry_t& entry = iter->second;
                    // inform all consumers about pending update
                    std::map<ResourceConsumer<T_res>*,void*> updateargs;
                    typename ConsumerSet::iterator iterCons = entry.consumers.begin();
                    typename ConsumerSet::iterator endCons  = entry.consumers.end();
                    for (; iterCons != endCons; iterCons++) {
                        if (*iterCons == pConsumer) continue;
                        void* updatearg = NULL;
                        (*iterCons)->ResourceToBeUpdated(entry.resource, updatearg);
                        if (updatearg) updateargs[*iterCons] = updatearg;
                    }
                    // update resource
                    T_res* pOldResource = entry.resource;
                    Destroy(entry.resource, entry.lifearg);
                    entry.resource = Create(entry.key, pConsumer, entry.lifearg);
                    // inform all consumers about update completed
                    iterCons = entry.consumers.begin();
                    endCons  = entry.consumers.end();
                    for (; iterCons != endCons; iterCons++) {
                        if (*iterCons == pConsumer) continue;
                        typename std::map<ResourceConsumer<T_res>*,void*>::iterator iterArg = updateargs.find(*iterCons);
                        void* updatearg = (iterArg != updateargs.end()) ? iterArg->second : NULL;
                        (*iterCons)->ResourceUpdated(pOldResource, entry.resource, updatearg);
                    }
                    if (bLock) ResourceEntriesMutex.Unlock();
                    return;
                }
            }
            if (bLock) ResourceEntriesMutex.Unlock();
        }

        /**
         * Returns the life-time strategy of the given resource.
         *
         * @param Key   - ID of the resource
         * @param bLock - use thread safety mechanisms
         */
        mode_t AvailabilityMode(T_key Key, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            if (iterEntry == ResourceEntries.end()) {
                if (bLock) ResourceEntriesMutex.Unlock();
                return ON_DEMAND; // resource entry doesn't exist, so we return the default mode
            }
            resource_entry_t& entry = iterEntry->second;
            if (bLock) ResourceEntriesMutex.Unlock();
            return entry.mode;
        }

        /**
         * Change life-time strategy of the given resource. If a life-time
         * strategy of PERSISTENT was given and the resource was not created
         * yet, it will immediately be created and this method will block
         * until the resource creation was completed.
         *
         * @param Key - ID of the resource
         * @param Mode - life-time strategy of resource to be set
         * @param bLock - use thread safety mechanisms
         * @throws Exception in case an invalid Mode was given
         */
        void SetAvailabilityMode(T_key Key, mode_t Mode, bool bLock = true) {
            if (Mode != ON_DEMAND && Mode != ON_DEMAND_HOLD && Mode != PERSISTENT)
                throw Exception("ResourceManager::SetAvailabilityMode(): invalid mode");

            if (bLock) ResourceEntriesMutex.Lock();
            // search for an entry for this resource
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            resource_entry_t* pEntry = NULL;
            if (iterEntry == ResourceEntries.end()) { // resource entry doesn't exist
                if (Mode == ON_DEMAND) {
                    if (bLock) ResourceEntriesMutex.Unlock();
                    return; // we don't create an entry for the default value
                }
                // create an entry for the resource
                pEntry = &ResourceEntries[Key];
                pEntry->key      = Key;
                pEntry->resource = NULL;
                pEntry->mode     = Mode;
                pEntry->lifearg  = NULL;
                pEntry->entryarg = NULL;
            } else { // resource entry exists
                pEntry = &iterEntry->second;
                // remove entry if necessary
                if (Mode == ON_DEMAND && !pEntry->entryarg && pEntry->consumers.empty()) {
                    T_res* resource = pEntry->resource;
                    void* arg       = pEntry->lifearg;
                    ResourceEntries.erase(iterEntry);
                    // destroy resource if necessary
                    if (resource) Destroy(resource, arg);
                    // done
                    if (bLock) ResourceEntriesMutex.Unlock();
                    return;
                }
                pEntry->mode = Mode; // apply new mode
            }

            // already create the resource if necessary
            if (pEntry->mode == PERSISTENT && !pEntry->resource) {
                try {
                    // actually create the resource
                    pEntry->resource = Create(Key, NULL /*no consumer yet*/, pEntry->lifearg);
                } catch (...) {
                    // creating the resource failed, so skip it for now
                    pEntry->resource = NULL;
                    if (bLock) ResourceEntriesMutex.Unlock();
                    // rethrow the same exception
                    throw;
                }
            }
            if (bLock) ResourceEntriesMutex.Unlock();
        }

        /**
         * Returns true in case the resource associated with \a Key is
         * currently created / "alive".
         *
         * @param Key - ID of resource
         * @param bLock - use thread safety mechanisms
         */
        bool IsCreated(T_key Key, bool bLock = true) {
            return Resource(Key, bLock) != NULL;
        }

        /**
         * Returns custom data sticked to the given resource previously by
         * a SetCustomData() call.
         *
         * @param Key - ID of resource
         * @param bLock - use thread safety mechanisms
         */
        void* CustomData(T_key Key, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            if (iterEntry == ResourceEntries.end()) {
                if (bLock) ResourceEntriesMutex.Unlock();
                return NULL; // resource entry doesn't exist, so we return the default mode
            }
            resource_entry_t entry = iterEntry->second;
            void* res = entry.entryarg;
            if (bLock) ResourceEntriesMutex.Unlock();
            return res;
        }

        /**
         * This method can be used to stick custom data to an resource
         * entry. In case the custom data is not needed anymore, you should
         * call this method again and set \a pData to NULL, so the
         * ResourceManager might safe space by removing the respective
         * entry if not needed anymore.
         *
         * @param Key - ID of resource
         * @param pData - pointer to custom data, or NULL if not needed anymore
         * @param bLock - use thread safety mechanisms
         */
        void SetCustomData(T_key Key, void* pData, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            if (pData) {
                if (iterEntry == ResourceEntries.end()) { // entry doesnt exist, so create one
                    resource_entry_t* pEntry = &ResourceEntries[Key];
                    pEntry->key      = Key;
                    pEntry->resource = NULL;
                    pEntry->mode     = ON_DEMAND;
                    pEntry->lifearg  = NULL;
                    pEntry->entryarg = pData; // set custom data
                } else { // entry exists, so just update its custom data
                    iterEntry->second.entryarg = pData;
                }
            } else { // !pData
                if (iterEntry == ResourceEntries.end()) {
                    if (bLock) ResourceEntriesMutex.Unlock();
                    return; // entry doesnt exist, so nothing to do
                }
                // entry exists, remove it if necessary
                resource_entry_t* pEntry = &iterEntry->second;
                if (pEntry->mode == ON_DEMAND && pEntry->consumers.empty()) {
                    ResourceEntries.erase(iterEntry);
                } else iterEntry->second.entryarg = NULL;
            }
            if (bLock) ResourceEntriesMutex.Unlock();
        }

        /**
         * Prevent this ResourceManager instance to be used by another
         * thread at the same time. Most methods of this class are thread
         * safe by default. However sometimes you might need atomicity among
         * a sequence of method calls. In this case you would first call
         * this Lock() method, call the respective operational methods of
         * this class (<b>Important:</b> each one by setting bLock = false,
         * this avoids double locking). And once the required sequence of
         * atomic method calls is done, you have to call Unlock() to
         * reenable accessibility of this ResourceManager instance for other
         * threads.
         *
         * @see Mutex::Lock()
         */
        void Lock() {
            ResourceEntriesMutex.Lock();
        }

        /**
         * Has to be called after a Lock() call to reenable this
         * ResourceManager instance to be accessible by another thread
         * again.
         *
         * @see Mutex::Unlock()
         */
        void Unlock() {
            ResourceEntriesMutex.Unlock();
        }

        virtual ~ResourceManager() {} // due to C++'s nature we cannot destroy created resources here

    protected:
        /**
         * Has to be implemented by the descendant to create (allocate) a
         * resource identified by \a Key.
         *
         * @param Key       - identifier of the resource
         * @param pConsumer - identifier of the consumer who borrows the
         *                    resource
         * @param pArg      - pointer the descendant can use to store
         *                    informations he might need for destruction of
         *                    the resource
         * @returns  pointer to new resource
         */
        virtual T_res* Create(T_key Key, ResourceConsumer<T_res>* pConsumer, void*& pArg) = 0;

        /**
         * Has to be implemented by the descendant to destroy (free) a
         * resource pointed by \a pResource.
         *
         * @param pResource - pointer to the resource
         * @param pArg      - pointer the descendant might have used when
         *                    Create() was called to store informations
         *                    about the resource
         */
        virtual void Destroy(T_res* pResource, void* pArg) = 0;

        /**
         * Has to be implemented by the descendant to react when a consumer
         * borrows a resource (no matter if freshly created or an already
         * created one). Of course reacting is optional, but the descendant
         * at least has to provide a method with empty body.
         *
         * @param pResource - pointer to the resource
         * @param pConsumer - identifier of the consumer who borrows the
         *                    resource
         * @param pArg      - pointer the descendant might have used when
         *                    Create() was called to store informations
         *                    about the resource, this information can be
         *                    updated by the descendant here
         */
        virtual void OnBorrow(T_res* pResource, ResourceConsumer<T_res>* pConsumer, void*& pArg) = 0;

        /**
         * Dispatcher method which should be periodically called by the
         * descendant during update or creation of the resource associated
         * with \a Key. This method will inform all associated consumers
         * of the given resource about the current progress.
         *
         * @param Key       - unique identifier of the resource which is
         *                    currently creating or updating
         * @param fProgress - current progress of that creation / update
         *                    process as value between 0.0 and 1.0
         */
        void DispatchResourceProgressEvent(T_key Key, float fProgress) {
            // no Mutex here, since Create() is already protected
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            if (iterEntry != ResourceEntries.end()) {
                resource_entry_t& entry = iterEntry->second;
                // inform all consumers of that resource about current progress
                typename ConsumerSet::iterator iterCons = entry.consumers.begin();
                typename ConsumerSet::iterator endCons  = entry.consumers.end();
                for (; iterCons != endCons; iterCons++) {
                    (*iterCons)->OnResourceProgress(fProgress);
                }
            }
        }

        /**
         * Returns pointer to the resource associated with \a Key if
         * currently created / "alive", NULL otherwise. This method
         * should be taken with great care in multi-threaded scenarios,
         * since the returned resource might be destroyed by a concurrent
         * HandBack() call.
         *
         * @param Key - ID of resource
         * @param bLock - use thread safety mechanisms
         */
        T_res* Resource(T_key Key, bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            typename ResourceMap::iterator iterEntry = ResourceEntries.find(Key);
            T_res* result = (iterEntry == ResourceEntries.end()) ? NULL : iterEntry->second.resource;
            if (bLock) ResourceEntriesMutex.Unlock();
            return result;
        }

        /**
         * Returns a list with all currently created / "living" resources.
         * This method should be taken with great care in multi-threaded
         * scenarios, since the returned resources might be destroyed by a
         * concurrent HandBack() call.
         *
         * @param bLock - use thread safety mechanisms
         */
        std::vector<T_res*> Resources(bool bLock = true) {
            if (bLock) ResourceEntriesMutex.Lock();
            std::vector<T_res*> result;
            typename ResourceMap::iterator iter = ResourceEntries.begin();
            typename ResourceMap::iterator end  = ResourceEntries.end();
            for (; iter != end; ++iter)
                if (iter->second.resource)
                    result.push_back(iter->second.resource);
            if (bLock) ResourceEntriesMutex.Unlock();
            return result;
        }
};

} // namespace LinuxSampler

#endif // __RESOURCE_MANAGER__
