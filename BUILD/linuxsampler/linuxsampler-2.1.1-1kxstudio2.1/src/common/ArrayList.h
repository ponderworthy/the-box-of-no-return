/***************************************************************************
 *                                                                         *
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

#ifndef __LS_ARRAYLIST_H__
#define __LS_ARRAYLIST_H__

#include "Exception.h"
#include <string.h> // for memcpy()

namespace LinuxSampler {

    /**
     * Very simple container with array implementation which ensures a constant
     * access time of Theta(1). We could have used std::vector instead, but due
     * to paranoia in regards of possible implementation differences, we better
     * rely on this class instead in parts where RT stability is mandatory.
     */
    template<typename T>
    class ArrayList {
        public:
            ArrayList() {
                pData = NULL;
                iSize = 0;
            }

            ~ArrayList() {
                clear();
            }

            /**
             * Add a new element to the end of the list.
             */
            void add(T element) {
                T* pNewArray = new T[iSize + 1];
                if (pData) {
                    for (int i = 0; i < iSize; i++)
                        pNewArray[i] = pData[i];
                    delete[] pData;
                }
                pNewArray[iSize] = element;
                pData = pNewArray;
                ++iSize;
            }

            /**
             * Remove the given element at \a iPosition from the list.
             *
             * @throws Exception - if \a iPosition is out of range
             */
            void remove(int iPosition) throw (Exception) {
                if (iPosition < 0 || iPosition >= iSize)
                    throw Exception("ArrayList::remove(): index out of range");
                if (iSize == 1) clear();
                else if (pData) {
                    T* pNewArray = new T[iSize - 1];
                    for (int iSrc = 0, iDst = 0; iSrc < iSize; iSrc++) {
                        if (iSrc == iPosition) continue;
                        pNewArray[iDst] = pData[iSrc];
                        ++iDst;
                    }
                    delete[] pData;
                    pData = pNewArray;
                    --iSize;
                }
            }

            /**
             * Remove the given \a element from the list.
             *
             * @throws Exception - if \a element could not be found
             */
            void remove(const T& element) {
                remove(find(element));
            }

            /**
             * Increase or decrease the size of this list to the new amount of
             * elements given by \a cnt.
             */
            void resize(int cnt) {
                T* pNewArray = new T[cnt];
                if (pData) {
                    for (int i = 0; i < cnt; i++)
                        pNewArray[i] = pData[i];
                    delete[] pData;
                }
                pData = pNewArray;
                iSize = cnt;
            }

            /**
             * Remove all elements from the list.
             */
            void clear() {
                if (pData) {
                    delete[] pData;
                    pData = NULL;
                    iSize = 0;
                }
            }

            /**
             * Returns the index of the given \a element on the list.
             *
             * @throws Exception - if \a element could not be found
             */
            int find(const T& element) {
                for (int i = 0; i < iSize; i++)
                    if (pData[i] == element) return i;
                throw Exception("ArrayList::find(): could not find given element");
            }

            /**
             * Number of elements currently on the list.
             */
            inline int size() const {
                return iSize;
            }

            /**
             * Returns true if the list is empty.
             */
            inline bool empty() const {
                return (bool) !iSize;
            }

            /**
             * Access element at \a iPosition.
             */
            inline T& operator[](int iPosition) {
                return pData[iPosition];
            }

            /**
             * Access element at \a iPosition.
             */
            inline const T& operator[](int iPosition) const {
                return pData[iPosition];
            }

            /**
             * Copy constructor.
             */
            ArrayList(const ArrayList& list) {
                copy(list);
            }

            /**
             * Generalized assignment with size adjustment and deep copy of
             * elements. Note that this method/operator re-allocates, so it is
             * not appropriate for being used in a real-time context!
             *
             * @see copyFlatFrom() for real-time safe copying
             */
            ArrayList& operator=(const ArrayList& list) {
                if (this != &list) {
                    clear();
                    copy(list);
                }
                return *this;
            }

            /**
             * Real-time safe copying elements (flat, no deep copy) from
             * given @a list to this list.
             */
            void copyFlatFrom(const ArrayList& list) {
                const int n = (size() < list.size()) ? size() : list.size();
                memcpy(pData, list.pData, n * sizeof(T));
            }

        private:
            T*   pData;
            int  iSize;

            void copy(const ArrayList& list) {
                iSize = list.iSize;
                if (list.pData) {
                    pData = new T[iSize];
                    for (int i = 0 ; i < iSize ; i++) {
                        pData[i] = list.pData[i];
                    }
                } else {
                    pData = NULL;
                }
            }
    };

} // namespace LinuxSampler

#endif // __ARRAYLIST_H__
