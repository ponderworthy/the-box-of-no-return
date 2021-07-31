/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
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

#ifndef __LS_SAMPLEMANAGER_H__
#define __LS_SAMPLEMANAGER_H__

#include <map>
#include <vector>
#include <set>

#include "../../common/Exception.h"

namespace LinuxSampler {

    /**
     * Used to determine and manage the relations between samples and consumers
     * (e.g. regions). Even though this class is currently stored at a shared
     * source file location, this class is actually only used by the sfz engine
     * ATM. So the gig and sf2 engines are not using this class at all.
     *
     * NOTE: This class currently uses a somewhat unclean design by maintaining
     * a separate set of information of "consumers of a sample" vs. "consumers
     * actually using a sample" (see bug #308). We might need to revise this
     * design of this class in case new problems appear (e.g. when encountering
     * memory leaks after closing an sfz instrument).
     */
    template <class S /* Sample */, class C /* Sample Consumer */>
    class SampleManager {
        public:
            /**
             * Adds the specified sample to the sample manager
             */
            void AddSample(S* pSample) {
                if (pSample == NULL) return;
                sampleMap[pSample];
            }

            void RemoveSample(S* pSample) throw (Exception) {
                if (sampleMap.find(pSample) == sampleMap.end()) return;
                if (!sampleMap[pSample].empty()) {
                    throw Exception("Can't remove. Sample has consumers");
                }

                sampleMap.erase(sampleMap.find(pSample));
            }

            /**
             * Adds pConsumer as consumer to the specified sample.
             * The sample is automatically added to the sample manager if
             * it is not added yet. This method does nothing if pConsumer is
             * already added as consumer to pSample.
             */
            void AddSampleConsumer(S* pSample, C* pConsumer) {
                if (pSample == NULL || pConsumer == NULL) return;
                if(sampleMap[pSample].find(pConsumer) != sampleMap[pSample].end()) return;
                sampleMap[pSample].insert(pConsumer);
            }

            std::vector<C*> GetConsumers(S* pSample) throw (Exception) {
                if (sampleMap.find(pSample) == sampleMap.end()) {
                    throw Exception("SampleManager::GetConsumers: unknown sample");
                }
                std::set<C*>* pConsumers = &sampleMap[pSample];
                std::vector<C*> v;
                v.insert(pConsumers->begin(), pConsumers->end());
                return v ;
            }

            /**
             * @return true if the specified consumer was in the list
             * of consumers for the specified sample.
             */
            bool RemoveSampleConsumer(S* pSample, C* pConsumer) throw (Exception) {
                if (sampleMap.find(pSample) == sampleMap.end()) {
                    throw Exception("SampleManager::RemoveConsumer: unknown sample");
                }

                std::set<C*>* consumers = &sampleMap[pSample];
                typename std::set<C*>::iterator it = consumers->find(pConsumer);
                if (it != consumers->end()) {
                    consumers->erase(it);
                    return true;
                }
                return false;
            }

            /**
             * Determines whether pSample is managed by this sample manager
             */
            bool HasSample(S* pSample) {
                return sampleMap.find(pSample) != sampleMap.end();
            }

            bool HasSampleConsumers(S* pSample) throw (Exception) {
                if (sampleMap.find(pSample) == sampleMap.end()) {
                    throw Exception("SampleManager::HasConsumers: unknown sample");
                }

                return !sampleMap[pSample].empty();
            }

            /**
             * Determines whether pConsumer is consumer of pSample.
             */
            bool IsSampleConsumerOf(S* pSample, C* pConsumer) {
                if (sampleMap.find(pSample) == sampleMap.end()) {
                    throw Exception("SampleManager::IsSampleConsumerOf: unknown sample");
                }

                typename std::set<C*>::iterator it = sampleMap[pSample].find(pConsumer);
                return it != sampleMap[pSample].end();
            }

            /**
             * Sets that pSample is now in use by pConsumer.
             */
            void SetSampleInUse(S* pSample, C* pConsumer) {
                verifyPair(pSample, pConsumer, "SampleManager::SetSampleInUse");

                bool inUse = !samplesInUseMap[pSample].empty();
                samplesInUseMap[pSample].insert(pConsumer);
                if(!inUse) OnSampleInUse(pSample);
            }

            /**
             * Sets that pSample is now not in use by pConsumer.
             */
            void SetSampleNotInUse(S* pSample, C* pConsumer) {
                verifyPair(pSample, pConsumer, "SampleManager::SetSampleNotInUse");

                bool inUse = !samplesInUseMap[pSample].empty();
                // Remove only one consumer at a time
                typename std::multiset<C*>::iterator it = samplesInUseMap[pSample].find(pConsumer);
                if (it != samplesInUseMap[pSample].end()) {
                    samplesInUseMap[pSample].erase(it);
                }
                bool inUseNew = !samplesInUseMap[pSample].empty();
                // Wih no consumers, erase sample
                if (!inUseNew) samplesInUseMap.erase(pSample);
                if(inUse && !inUseNew) OnSampleNotInUse(pSample);
            }

        protected:
            std::map<S*, std::set<C*> > sampleMap;
            std::map<S*, std::multiset<C*> > samplesInUseMap; // std::multiset as data type fixes bug #308.

            void verifyPair(S* pSample, C* pConsumer, String caller) {
                if(!HasSample(pSample)) {
                    throw Exception(caller + ": unknown sample");
                }

                if(!IsSampleConsumerOf(pSample, pConsumer)) {
                    throw Exception("SampleManager::SetSampleInUse: unknown consumer");
                }
            }

            /**
             * Override this method to handle the state change (not in use -> in use)
             * of the specified sample.
             */
            virtual void OnSampleInUse(S* pSample) = 0;

            /**
             * Override this method to handle the state change (in use -> not in use)
             * of the specified sample.
             */
            virtual void OnSampleNotInUse(S* pSample) = 0;
    };
} // namespace LinuxSampler

#endif	/* __LS_SAMPLEMANAGER_H__ */

