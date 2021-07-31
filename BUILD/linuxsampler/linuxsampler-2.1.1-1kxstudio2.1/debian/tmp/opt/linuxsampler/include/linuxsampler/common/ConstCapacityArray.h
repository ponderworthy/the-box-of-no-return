/*
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_CONSTCAPACITYARRAY_H
#define LS_CONSTCAPACITYARRAY_H

#include <stdlib.h>
#include <stdint.h>

namespace LinuxSampler {

    /**
     * Very simple container with array implementation which ensures a constant
     * access time of Theta(1) on individual elements. Internally it uses a
     * consecutive, dense C array (on heap) where its capacity is defined by the
     * constructor of this class. That internal array will never be reallocated
     * during the lifetime of a ConstCapacityArray object and thus C pointers to
     * elements of this array remain always valid.
     *
     * This class has methods to add and remove elements from the array and
     * "resizing" the array. All those operations are guaranteed to be real-time
     * safe. When elements are removed from the array, members past the removed
     * element(s) will be moved towards the array start, to always ensure that
     * the array is kept dense (without any gaps).
     */
    template<typename T>
    class ConstCapacityArray {
    public:
        /**
         * Create a ConstCapacityArray with the given capacity. The initial
         * size of the array will be zero. ConstCapacityArray's size can never
         * grow larger than its capacity defined with the constructor here.
         *
         * This constructor <b>must not</b> be called in a real-time context!
         *
         * @param capacity - maximum size this object may ever grow
         */
        ConstCapacityArray(int capacity) :
            m_data(new T[capacity]), m_capacity(capacity), m_size(0) {}

        /**
         * Destructor. Frees the internal array allocated by this object.
         *
         * This destructor <b>must not</b> be called in a real-time context!
         */
        ~ConstCapacityArray() {
            if (m_data) delete [] m_data;
        }

        /**
         * Amount of elements this array currently holds. The array's size may
         * vary between 0 ... capacity() during its lifetime.
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         *
         * @see capacity()
         */
        inline uint size() const {
            return m_size;
        }

        /**
         * Maximum size this array may ever grow to in its lifetime.
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         *
         * @see size()
         */
        inline uint capacity() const {
            return m_capacity;
        }

        /**
         * Returns true if this array is empty.
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         */
        inline bool empty() const {
            return !m_size;
        }

        /**
         * Add a new element to the end of the array. Size is incremented by
         * one. You cannot add more elements than capacity() reflects.
         *
         * This method is guaranteed to be real-time safe.
         *
         * @param element - value to be added as new element
         */
        inline void append(const T& element) {
            if (m_size == m_capacity) return; // max. size already exhausted
            m_data[m_size++] = element;
        }

        /**
         * Remove @a count elements from the array at position @a index.
         * Elements past the removed element(s) will be moved towards the
         * beginning of the array. So the array always remains dense (without
         * gaps).
         *
         * This method is guaranteed to be real-time safe.
         *
         * @param index - position where element(s) shall be removed
         * @param count - amount of elements to be removed
         */
        inline void remove(uint index, uint count = 1) {
            if (index >= m_size || index + count > m_size)
                return;
            // don't use memmove() here! Since it is not RT safe with all libc
            // implementations and on all architectures
            for (uint i = 0; i < count; ++i)
                m_data[index + i] = m_data[index + i + count];
            m_size -= count;
        }

        /**
         * Returns true in case there is an element with given @a value in this
         * array
         *
         * @param value - value to be searched for
         */
        bool contains(const T& value) const {
            for (uint i = 0; i < m_size; ++i)
                if (m_data[i] == value) return true;
            return false;
        }

        /**
         * Searches the array for an element with given index, if found it
         * returns the element's index, if no such element was found an invalid
         * index will be returned instead.
         *
         * @param value - value to be searched for
         * @returns index of found element
         */
        uint find(const T& value) const {
            for (uint i = 0; i < m_size; ++i)
                if (m_data[i] == value) return i;
            return -1;
        }

        /**
         * Remove all elements from the array. Size of array will be zero after
         * this call.
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         */
        inline void clear() {
            m_size = 0;
        }

        /**
         * Access element at position @a index (for read/write access).
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         *
         * @param index - position of array to access
         */
        inline T& operator[](uint index) {
            return m_data[index];
        }

        /**
         * Access element at position @a index (for read only access).
         *
         * Access time is always constant with Theta(1).
         *
         * This method is guaranteed to be real-time safe.
         *
         * @param index - position of array to access
         */
        inline const T& operator[](uint index) const {
            return m_data[index];
        }

    private:
        T* __restrict const m_data;
        int m_capacity;
        int m_size;
    };

} // namespace LinuxSampler

#endif // LS_CONSTCAPACITYARRAY_H
