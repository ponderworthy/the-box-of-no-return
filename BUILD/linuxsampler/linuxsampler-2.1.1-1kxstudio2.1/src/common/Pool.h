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

#ifndef __LS_POOL_H__
#define __LS_POOL_H__

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

// we just use exceptions for debugging, better not in the final realtime thread !
#ifndef CONFIG_RT_EXCEPTIONS
# define CONFIG_RT_EXCEPTIONS 0
#endif

#if CONFIG_RT_EXCEPTIONS
# include <stdexcept>
# include <string>
#endif // CONFIG_RT_EXCEPTIONS

#include <iostream>

#if CONFIG_DEVMODE
# include <string>
# include <stdexcept>
const std::string __err_msg_iterator_invalidated = "Pool/RTList iterator invalidated";
#endif // CONFIG_DEVMODE

const std::string __err_msg_resize_while_in_use = "Pool::resizePool() ERROR: elements still in use!";

/**
 * Unique numeric ID for exactly one incarnation of one element allocated from
 * a Pool. As soon as the respective element is once freed back to the Pool,
 * the ID becomes invalid. Such an ID may be used to safely store an abstract
 * reference to one Pool element for longer time. Since the Pool classes
 * automatically detect whether an ID became invalid, using such an ID is thus
 * safer than storing an Iterator or even a raw pointer in use case scenarios of
 * storing long term references to Pool elements.
 * 
 * This ID type is currently set (constrained) to 32-bit because the current
 * real-time instrument script infrastructure implementation, which heavily
 * relies on element IDs, is currently using 32-bit for its integer script
 * variable type.
 */
typedef uint32_t pool_element_id_t;

// just symbol prototyping
template<typename T> class Pool;
template<typename T> class RTList;

template<typename T>
class RTListBase {
    protected:
        template<typename T1>
        struct _Node {
            _Node<T1>* next;
            _Node<T1>* prev;
            T1* data;
            #if CONFIG_DEVMODE
            RTListBase<T1>* list; // list to which this node currently belongs to
            #endif // CONFIG_DEVMODE
            uint reincarnation; // just for Pool::fromID()

            _Node() {
                next = NULL;
                prev = NULL;
                data = NULL;
                #if CONFIG_DEVMODE
                list = NULL;
                #endif // CONFIG_DEVMODE
                reincarnation = 0;
            }

            inline void bumpReincarnation(uint bits) {
                reincarnation++;
                // constrain the bitrange of "reincarnation", because Pool::fromID() will shift up/down for pool_element_id_t and compare this bitwise
                reincarnation &= ((1 << bits) - 1);
            }
        };
        typedef _Node<T> Node;

    public:
        /**
         * Pointer-like object which allows to iterate over elements of a RTList,
         * similar to iterators of STL container classes. Note that the main
         * purpose of this class is to access elements of a list / pool i.e.
         * within a @c while() loop. If you rather want to keep a reference to
         * one particular element (i.e. for longer time) then you might
         * consider using @c pool_element_id_t variables instead.
         */
        template<typename T1>
        class _Iterator {
            public:
                _Iterator() {
                    current  = NULL;
                    fallback = NULL;
                    #if CONFIG_DEVMODE
                    list = NULL;
                    #endif // CONFIG_DEVMODE
                }

                /// prefix increment op.
                inline _Iterator& operator++() {
                    #if CONFIG_DEVMODE
                    if (!isValid()) {
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return *(_Iterator*)NULL; // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    fallback = current;
                    current  = current->next;
                    return *this;
                }

                /// postfix increment op.
                inline _Iterator operator++(int) {
                    _Iterator preval = *this;
                    ++*this; // use prefix operator implementation
                    return preval;
                }

                /// prefix decrement op.
                inline _Iterator& operator--() {
                    #if CONFIG_DEVMODE
                    if (!isValid()) {
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return *(_Iterator*)NULL; // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    fallback = current;
                    current  = current->prev;
                    return *this;
                }

                /// postfix decrement op.
                inline _Iterator operator--(int) {
                    _Iterator preval = *this;
                    --*this; // use prefix operator implementation
                    return preval;
                }

                inline T1& operator*() {
                    #if CONFIG_DEVMODE
                    if (!isValid()) { // if iterator became invalidated
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return *((T1*)NULL); // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    return *current->data;
                }

                inline const T1& operator*() const {
                    #if CONFIG_DEVMODE
                    if (!isValid()) { // if iterator became invalidated
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return *((const T1*)NULL); // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    return *current->data;
                }

                inline T1* operator->() {
                    #if CONFIG_DEVMODE
                    if (!isValid()) { // if iterator became invalidated
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return (T1*)NULL; // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    return current->data;
                }

                inline const T1* operator->() const {
                    #if CONFIG_DEVMODE
                    if (!isValid()) { // if iterator became invalidated
                        #if CONFIG_RT_EXCEPTIONS
                        throw std::runtime_error(__err_msg_iterator_invalidated);
                        #else
                        std::cerr << __err_msg_iterator_invalidated << std::endl << std::flush;
                        return (const T1*)NULL; // force segfault if iterator became invalidated
                        #endif // CONFIG_RT_EXCEPTIONS
                    }
                    #endif // CONFIG_DEVMODE
                    return current->data;
                }

                inline bool operator==(const _Iterator<T1> other) const {
                    return current == other.current;
                }

                inline bool operator!=(const _Iterator<T1> other) const {
                    return current != other.current;
                }

                inline operator bool() const {
                    return current && current->data;
                }

                inline bool operator!() const {
                    return !(current && current->data);
                }

                /**
                 * Moves the element pointed by this Iterator from its current
                 * list to the beginning of the destination list @a pDstList.
                 *
                 * @b CAUTION: When this method returns, this Iterator does
                 * @b NOT point to the element on the new list anymore, instead it
                 * points at a completely different element! In case of a
                 * forward Iterator this Iterator object will point to the
                 * previous element on the source list, in case of a backward
                 * Iterator it will point to the subsequent element on the
                 * source list. This behavior is enforced to avoid breaking an
                 * active loop code working with this Iterator object.
                 *
                 * Thus if you intend to continue working with the same element,
                 * you should do like this:
                 * @code
                 * it = it.moveToEndOf(anotherList);
                 * @endcode
                 *
                 * @param pDstList - destination list
                 * @returns Iterator object pointing at the moved element on
                 *          the destination list
                 */
                inline _Iterator moveToEndOf(RTListBase<T1>* pDstList) {
                    detach();
                    pDstList->append(*this);
                    _Iterator iterOnDstList = _Iterator(current);
                    current = fallback;
                    return iterOnDstList;
                }

                /**
                 * Moves the element pointed by this Iterator from its current
                 * list to the end of destination list @a pDstList.
                 *
                 * @b CAUTION: When this method returns, this Iterator does
                 * @b NOT point to the element on the new list anymore, instead it
                 * points at a completely different element! In case of a
                 * forward Iterator this Iterator object will point to the
                 * previous element on the source list, in case of a backward
                 * Iterator it will point to the subsequent element on the
                 * source list. This behavior is enforced to avoid breaking an
                 * active loop code working with this Iterator object.
                 *
                 * Thus if you intend to continue working with the same element,
                 * you should do like this:
                 * @code
                 * it = it.moveToBeginOf(anotherList);
                 * @endcode
                 *
                 * @param pDstList - destination list
                 * @returns Iterator object pointing at the moved element on
                 *          the destination list
                 */
                inline _Iterator moveToBeginOf(RTListBase<T1>* pDstList) {
                    detach();
                    pDstList->prepend(*this);
                    _Iterator iterOnDstList = _Iterator(current);
                    current = fallback;
                    return iterOnDstList;
                }

                /**
                 * Moves the element pointed by this Iterator from its current
                 * position to the position right before @a itDst. That move
                 * may either be from and to the same list, or to a another
                 * list.
                 *
                 * @b CAUTION: When this method returns, this Iterator does
                 * @b NOT point to the element on the new list anymore, instead it
                 * points at a completely different element! In case of a
                 * forward Iterator this Iterator object will point to the
                 * previous element on the source list, in case of a backward
                 * Iterator it will point to the subsequent element on the
                 * source list. This behavior is enforced to avoid breaking an
                 * active loop code working with this Iterator object.
                 *
                 * Thus if you intend to continue working with the same element,
                 * you should do like this:
                 * @code
                 * itSourceElement = itSourceElement.moveBefore(itDestinationElement);
                 * @endcode
                 *
                 * @param itDst - destination element to be inserted before
                 * @returns Iterator object pointing at the moved element on
                 *          the destination list
                 */
                inline _Iterator moveBefore(_Iterator<T1> itDst) {
                    detach();
                    RTList<T1>::prependBefore(*this, itDst);
                    _Iterator iterOnDstList = _Iterator(current);
                    current = fallback;
                    return iterOnDstList;
                }

                /**
                 * Moves the element pointed by this Iterator from its current
                 * position to the position right after @a itDst. That move
                 * may either be from and to the same list, or to a another
                 * list.
                 *
                 * @b CAUTION: When this method returns, this Iterator does
                 * @b NOT point to the element on the new list anymore, instead it
                 * points at a completely different element! In case of a
                 * forward Iterator this Iterator object will point to the
                 * previous element on the source list, in case of a backward
                 * Iterator it will point to the subsequent element on the
                 * source list. This behavior is enforced to avoid breaking an
                 * active loop code working with this Iterator object.
                 *
                 * Thus if you intend to continue working with the same element,
                 * you should do like this:
                 * @code
                 * itSourceElement = itSourceElement.moveAfter(itDestinationElement);
                 * @endcode
                 *
                 * @param itDst - destination element to be inserted after
                 * @returns Iterator object pointing at the moved element on
                 *          the destination list
                 */
                inline _Iterator moveAfter(_Iterator<T1> itDst) {
                    detach();
                    RTList<T1>::appendAfter(*this, itDst);
                    _Iterator iterOnDstList = _Iterator(current);
                    current = fallback;
                    return iterOnDstList;
                }

                #if CONFIG_DEVMODE
                inline bool isValid() const {
                    return current->list == list;
                }
                #endif // CONFIG_DEVMODE

            protected:
                Node* current;
                Node* fallback;
                enum dir_t {
                    dir_forward,
                    dir_backward
                };
                #if CONFIG_DEVMODE
                RTListBase<T1>* list;
                #endif // CONFIG_DEVMODE

                _Iterator(Node* pNode, dir_t direction = dir_forward) {
                    current  = pNode;
                    fallback = (direction == dir_forward) ? pNode->prev : pNode->next;
                    #if CONFIG_DEVMODE
                    list = pNode->list;
                    #endif // CONFIG_DEVMODE
                }

                inline Node* node() {
                    #if CONFIG_DEVMODE
                    #if CONFIG_RT_EXCEPTIONS
                    if (isValid()) return current;
                    else throw std::runtime_error(__err_msg_iterator_invalidated);
                    #else
                    return (isValid()) ? current : (Node*)NULL; // force segfault if iterator became invalidated
                    #endif // CONFIG_RT_EXCEPTIONS
                    #else
                    return current;
                    #endif // CONFIG_DEVMODE
                }

                inline const Node* node() const {
                    #if CONFIG_DEVMODE
                    #if CONFIG_RT_EXCEPTIONS
                    if (isValid()) return current;
                    else throw std::runtime_error(__err_msg_iterator_invalidated);
                    #else
                    return (isValid()) ? current : (const Node*)NULL; // force segfault if iterator became invalidated
                    #endif // CONFIG_RT_EXCEPTIONS
                    #else
                    return current;
                    #endif // CONFIG_DEVMODE
                }

                inline void detach() {
                    RTListBase<T1>::detach(*this);
                }

                friend class RTListBase<T1>;
                friend class RTList<T1>;
                friend class Pool<T1>;
        };
        typedef _Iterator<T> Iterator;

        inline Iterator first() {
            return Iterator(_begin.next, Iterator::dir_forward);
        }

        inline Iterator last() {
            return Iterator(_end.prev, Iterator::dir_backward);
        }

        inline Iterator begin() {
            return Iterator(&_begin, Iterator::dir_forward);
        }

        inline Iterator end() {
            return Iterator(&_end, Iterator::dir_backward);
        }

        inline bool isEmpty() const {
            return _begin.next == &_end;
        }

        inline int count() {
            int elements = 0;
            for (Iterator it = first(); it != end(); ++it) ++elements;
            return elements;
        }

    protected:
        Node _begin; // fake node (without data) which represents the begin of the list - not the first element!
        Node _end;   // fake node (without data) which represents the end of the list - not the last element!

        RTListBase() {
            init();
        }

        void init() {
            // initialize boundary nodes
            _begin.prev = &_begin;
            _begin.next = &_end;
            _begin.data = NULL;
            _end.next = &_end;
            _end.prev = &_begin;
            _end.data = NULL;
            #if CONFIG_DEVMODE
            _begin.list = this;
            _end.list   = this;
            #endif // CONFIG_DEVMODE
        }

        inline void append(Iterator itElement) {
            Node* pNode = itElement.current;
            Node* last  = _end.prev;
            last->next  = pNode;
            pNode->prev = last; // if a segfault happens here, then because 'itElement' Iterator became invalidated
            pNode->next = &_end;
            _end.prev   = pNode;
            #if CONFIG_DEVMODE
            pNode->list = this;
            #endif // CONFIG_DEVMODE
        }

        inline void append(Iterator itFirst, Iterator itLast) {
            Node* pFirst = itFirst.current;
            Node* pLast  = itLast.current;
            Node* last   = _end.prev;
            last->next   = pFirst;
            pFirst->prev = last;  // if a segfault happens here, then because 'itFirst' Iterator became invalidated
            pLast->next  = &_end; // if a segfault happens here, then because 'itLast' Iterator became invalidated
            _end.prev    = pLast;
            #if CONFIG_DEVMODE
            for (Node* pNode = pFirst; true; pNode = pNode->next) {
                pNode->list = this;
                if (pNode == pLast) break;
            }
            #endif // CONFIG_DEVMODE
        }

        inline void prepend(Iterator itElement) {
            Node* pNode = itElement.current;
            Node* first = _begin.next;
            _begin.next = pNode;
            pNode->prev = &_begin; // if a segfault happens here, then because 'itElement' Iterator became invalidated
            pNode->next = first;
            first->prev = pNode;
            #if CONFIG_DEVMODE
            pNode->list = this;
            #endif // CONFIG_DEVMODE
        }

        inline void prepend(Iterator itFirst, Iterator itLast) {
            Node* pFirst = itFirst.current;
            Node* pLast  = itLast.current;
            Node* first  = _begin.next;
            _begin.next  = pFirst;
            pFirst->prev = &_begin; // if a segfault happens here, then because 'itFirst' Iterator became invalidated
            pLast->next  = first;   // if a segfault happens here, then because 'itLast' Iterator became invalidated
            first->prev  = pLast;
            #if CONFIG_DEVMODE
            for (Node* pNode = pFirst; true; pNode = pNode->next) {
                pNode->list = this;
                if (pNode == pLast) break;
            }
            #endif // CONFIG_DEVMODE
        }

        static inline void prependBefore(Iterator itSrc, Iterator itDst) {
            Node* src = itSrc.current;
            Node* dst = itDst.current;
            Node* prev = dst->prev;
            prev->next = src;
            dst->prev  = src;
            src->prev  = prev;
            src->next  = dst;
            #if CONFIG_DEVMODE
            src->list = dst->list;
            #endif // CONFIG_DEVMODE
        }

        static inline void appendAfter(Iterator itSrc, Iterator itDst) {
            Node* src = itSrc.current;
            Node* dst = itDst.current;
            Node* next = dst->next;
            next->prev = src;
            dst->next  = src;
            src->prev  = dst;
            src->next  = next;
            #if CONFIG_DEVMODE
            src->list = dst->list;
            #endif // CONFIG_DEVMODE
        }

        static inline void detach(Iterator itElement) {
            Node* pNode = itElement.node();
            Node* prev = pNode->prev; // if a segfault happens here, then because 'itElement' Iterator became invalidated
            Node* next = pNode->next;
            prev->next = next;
            next->prev = prev;
        }

        static inline void detach(Iterator itFirst, Iterator itLast) {
            Node* prev = itFirst.node()->prev; // if a segfault happens here, then because 'itFirst' Iterator became invalidated
            Node* next = itLast.node()->next;  // if a segfault happens here, then because 'itLast' Iterator became invalidated
            prev->next = next;
            next->prev = prev;
        }

        friend class _Iterator<T>;
        friend class RTList<T>;
        friend class Pool<T>;
};

template<typename T>
class RTList : public RTListBase<T> {
    public:
        typedef typename RTListBase<T>::Node     Node;
        typedef typename RTListBase<T>::Iterator Iterator;

        /**
         * Constructor
         *
         * @param pPool - pool this list uses for allocation and
         *                deallocation of elements
         */
        RTList(Pool<T>* pPool) : RTListBase<T>::RTListBase() {
            this->pPool = pPool;
        }
        
        /**
         * Copy constructor
         */
        RTList(RTList<T>& list) : RTListBase<T>::RTListBase() {
            this->pPool = list.pPool;
            Iterator it = list.first();
            Iterator end = list.end();
            for(; it != end; ++it) {
                if (poolIsEmpty()) break;
                *(allocAppend()) = *it;
            }
        }

        virtual ~RTList() {
            clear();
        }

        inline bool poolIsEmpty() const {
            return pPool->poolIsEmpty();
        }

        inline Iterator allocAppend() {
            if (pPool->poolIsEmpty()) return RTListBase<T>::begin();
            Iterator element = pPool->alloc();
            this->append(element);
            #if CONFIG_DEVMODE
            element.list = this;
            #endif // CONFIG_DEVMODE
            return element;
        }

        inline Iterator allocPrepend() {
            if (pPool->poolIsEmpty()) return RTListBase<T>::end();
            Iterator element = pPool->alloc();
            prepend(element);
            #if CONFIG_DEVMODE
            element.list = this;
            #endif // CONFIG_DEVMODE
            return element;
        }

        inline void free(Iterator& itElement) {
            itElement.detach();
            pPool->freeToPool(itElement);
            itElement.current = itElement.fallback;
        }

        inline void clear() {
            if (!RTListBase<T>::isEmpty()) {
                Node* first = RTListBase<T>::_begin.next;
                Node* last  = RTListBase<T>::_end.prev;
                RTListBase<T>::detach(first, last);
                pPool->freeToPool(first, last);
            }
        }

        inline pool_element_id_t getID(const T* obj) const {
            return pPool->getID(obj);
        }

        inline pool_element_id_t getID(const Iterator& it) const {
            return pPool->getID(&*it);
        }

        inline Iterator fromID(pool_element_id_t id) const {
            return pPool->fromID(id);
        }

        inline Iterator fromPtr(const T* obj) const {
            return pPool->fromPtr(obj);
        }

    protected:
        Pool<T>* pPool;
};

template<typename T>
class Pool : public RTList<T> {
    public:
        typedef typename RTList<T>::Node     Node;
        typedef typename RTList<T>::Iterator Iterator;

        Node*         nodes;
        T*            data;
        RTListBase<T> freelist; // not yet allocated elements
        uint          poolsize;
        // following 3 used for element ID generation (and vice versa)
        uint          poolsizebits; ///< Amount of bits required to index all elements of this pool (according to current pool size).
        uint          reservedbits; ///< 3rd party reserved bits on the left side of id (default: 0).
        uint          reincarnationbits; ///< Amount of bits allowed for reincarnation counter.

        Pool(int Elements) : RTList<T>::RTList(this), reservedbits(0) {
            _init(Elements);
        }

        virtual ~Pool() {
            if (nodes) delete[] nodes;
            if (data)  delete[] data;
        }

        /**
         * Returns true if there is at least one free element that could be
         * allocated from the pool with i.e. allocAppend() or allocPreprend().
         *
         * @see poolHasFreeElements()
         */
        inline bool poolIsEmpty() const {
            return freelist.isEmpty();
        }

        /**
         * Returns true if at least the requested amount of free @a elements is
         * currently available for being allocated from the pool with i.e.
         * allocAppend() or allocPreprend().
         *
         * @see poolIsEmpty()
         */
        bool poolHasFreeElements(int elements) {
            for (Iterator it = freelist.first(); it != freelist.end() && elements >= 0; ++it)
                --elements;
            return elements <= 0;
        }

        int countFreeElements() {
            return freelist.count();
        }

        /**
         * Returns the current size of the pool, that is the amount of
         * pre-allocated elements from the operating system. It equals the
         * amount of elements given to the constructor unless resizePool()
         * is called.
         *
         * @see resizePool()
         */
        uint poolSize() const {
            return poolsize;
        }

        /**
         * Alters the amount of elements to be pre-allocated from the
         * operating system for this pool object.
         *
         * @e CAUTION: you MUST free all elements in use before calling this
         * method ( e.g. by calling clear() )! Also make sure that no
         * references of elements before this call will still be used after this
         * call, since all elements will be reallocated and their old memory
         * addresses become invalid!
         *
         * @see poolSize()
         */
        void resizePool(int Elements) {
            if (freelist.count() != poolsize) {
                #if CONFIG_DEVMODE
                throw std::runtime_error(__err_msg_resize_while_in_use);
                #else
                std::cerr << __err_msg_resize_while_in_use << std::endl << std::flush;
                // if we're here something's terribly wrong, but we try to do the best
                RTList<T>::clear();
                #endif
            }
            if (nodes) delete[] nodes;
            if (data)  delete[] data;
            freelist.init();
            RTListBase<T>::init();
            _init(Elements);
        }

        /**
         * Sets the amount of bits on the left hand side of pool_element_id_t
         * numbers to be reserved for 3rd party usage. So if you pass @c 1 for
         * argument @a bits for example, then all generated element IDs will be
         * maximum 31 bit large.
         * 
         * By default there are no reserved bits, and thus by default all IDs
         * are max. 32 bit large.
         *
         * @param bits - amount of bits to reserve on every ID for other purposes
         * @see pool_element_id_t
         */
        void setPoolElementIDsReservedBits(uint bits) {
            reservedbits = bits;
            updateReincarnationBits();
        }

        /**
         * Returns an abstract, unique numeric ID for the given object of
         * this pool, it returns 0 in case the passed object is not a member
         * of this Pool, i.e. because it is simply an invalid pointer or member
         * of another Pool. The returned ID is unique among all elements of this
         * Pool and it differs with each reincarnation of an object. That means
         * each time you free an element to and allocate the same element back
         * from the Pool, it will have a different ID.
         * 
         * A valid ID will never be zero, so you may use ID values of 0 in your
         * data structures for special purposes (i.e. reflecting an invalid
         * object ID or not yet assigned object).
         *
         * Members are always translated both, from Iterators/pointers to IDs,
         * and from IDs to Iterators/pointers in constant time.
         *
         * You might want to use this alternative approach of referencing Pool
         * members under certain scenarios. For example if you need to expose
         * an ID to the end user and/or if you want to represent an object of
         * this pool by a smaller number instead of a native pointer (i.e. 16
         * bits vs. 64 bits). You can also detect this way whether the object
         * has already been freed / reallocated from the Pool in the meantime.
         *
         * @param obj - raw pointer to a data member of this Pool
         * @returns unique numeric ID (!= 0) of @a obj or 0 if pointer was invalid
         */
        pool_element_id_t getID(const T* obj) const {
            if (!poolsize) return 0;
            int index = int( obj - &data[0] );
            if (index < 0 || index >= poolsize) return 0;
            return ((nodes[index].reincarnation << poolsizebits) | index) + 1;
        }

        /**
         * Overridden convenience method, behaves like the method above.
         */
        pool_element_id_t getID(const Iterator& it) const {
            return getID(&*it);
        }

        /**
         * Returns an Iterator object of the Pool data member reflected by the
         * given abstract, unique numeric ID, it returns an invalid Iterator in
         * case the ID is invalid or if the Pool's data element reflected by
         * given ID was at least once released/freed back to the Pool in the
         * meantime.
         *
         * Members are always translated both, from Iterators/pointers to IDs,
         * and from IDs to Iterators/pointers in constant time.
         *
         * You might want to use this alternative approach of referencing Pool
         * members under certain scenarios. For example if you need to expose
         * an ID to the end user and/or if you want to represent an object of
         * this pool by a smaller number instead of a native pointer (i.e. 16
         * bits vs. 64 bits). You can also detect this way whether the object
         * has already been freed / reallocated from the Pool in the meantime.
         *
         * @param id - unique ID (!= 0) of a Pool's data member
         * @returns Iterator object pointing to Pool's data element, invalid
         *          Iterator in case ID was invalid or data element was freed
         */
        Iterator fromID(pool_element_id_t id) const {
            //TODO: -1 check here is a relict from older versions of Pool.h, once it is certain that no existing code base is still using -1 for "invalid" Pool elements then this -1 check can be removed
            if (id == 0 || id == -1) return Iterator(); // invalid iterator
            id--;
            const uint bits = poolsizebits;
            uint index = id & ((1 << bits) - 1);
            if (index >= poolsize) return Iterator(); // invalid iterator
            Node* node = &nodes[index];
            uint reincarnation = id >> bits;
            if (reincarnation != node->reincarnation) return Iterator(); // invalid iterator 
            return Iterator(node);
        }

        /**
         * Returns an Iterator object for the object pointed by @a obj. This
         * method will check whether the supplied object is actually part of
         * this pool, and if it is not part of this pool an invalid Iterator is
         * returned instead.
         * 
         * @param obj - raw pointer to an object managed by this pool
         * @returns Iterator object pointing to the supplied object, invalid
         *          Iterator in case object is not part of this pool
         */
        Iterator fromPtr(const T* obj) const {
            if (!poolsize) return Iterator(); // invalid iterator
            int index = int( obj - &data[0] );
            if (index < 0 || index >= poolsize) return Iterator(); // invalid iterator
            return Iterator(&nodes[index]);
        }

    protected:
        // caution: assumes pool (that is freelist) is not empty!
        inline Iterator alloc() {
            Iterator element = freelist.last();
            element.detach();
            return element;
        }

        inline void freeToPool(Iterator itElement) {
            itElement.node()->bumpReincarnation(reincarnationbits);
            freelist.append(itElement);
        }

        inline void freeToPool(Iterator itFirst, Iterator itLast) {
            for (Node* n = itFirst.node(); true; n = n->next) {
                n->bumpReincarnation(reincarnationbits);
                if (n == itLast.node()) break;
            }
            freelist.append(itFirst, itLast);
        }

        friend class RTList<T>;

    private:
        void _init(int Elements) {
            data  = new T[Elements];
            nodes = new Node[Elements];
            for (int i = 0; i < Elements; i++) {
                nodes[i].data = &data[i];
                freelist.append(&nodes[i]);
            }
            poolsize = Elements;
            poolsizebits = bitsForSize(poolsize + 1); // +1 here just because IDs are always incremented by one (to avoid them ever being zero)
            updateReincarnationBits();
        }

        inline void updateReincarnationBits() {
            reincarnationbits = sizeof(pool_element_id_t) * 8 - poolsizebits - reservedbits;
        }

        inline static int bitsForSize(int size) {
            if (!size) return 0;
            size--;
            int bits = 0;
            for (; size > 1; bits += 2, size >>= 2);
            return bits + size;
        }
};

#endif // __LS_POOL_H__
