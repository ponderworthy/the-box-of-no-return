/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
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

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#define DEFAULT_WRAP_ELEMENTS 0

#include <string.h>

#include "lsatomic.h"

using LinuxSampler::atomic;
using LinuxSampler::memory_order_relaxed;
using LinuxSampler::memory_order_acquire;
using LinuxSampler::memory_order_release;


/** @brief Real-time safe and type safe RingBuffer implementation.
 *
 * This constant size buffer can be used to send data from exactly one
 * sender / writing thread to exactly one receiver / reading thread. It is
 * real-time safe due to the fact that data is only allocated when this
 * RingBuffer is created and no system level mechanisms are used for
 * ensuring thread safety of this class.
 *
 * <b>Important:</b> There are two distinct behaviors of this RingBuffer
 * which has to be given as template argument @c T_DEEP_COPY, which is a
 * boolean flag:
 *
 * - @c true: The RingBuffer will copy elements of type @c T by using type
 *   @c T's assignment operator. This behavior is mandatory for all data
 *   structures (classes) which additionally allocate memory on the heap.
 *   Type @c T's needs to have an assignment operator implementation though,
 *   otherwise this will cause a compilation error. This behavior is more
 *   safe, but usually slower (except for very small buffer sizes, where it
 *   might be even faster).
 * - @c false: The RingBuffer will copy elements of type @c T by flatly
 *   copying their structural data ( i.e. with @c memcpy() ) in one piece.
 *   This will only work if class @c T (and all of its subelements) does not
 *   allocate any additional data on the heap by itself. So use this option
 *   with great care, because otherwise it will result in very ugly behavior
 *   and crashes! For larger buffer sizes, this behavior will most probably
 *   be faster.
 */
template<class T, bool T_DEEP_COPY>
class RingBuffer
{
public:
    RingBuffer (int sz, int wrap_elements = DEFAULT_WRAP_ELEMENTS) :
        write_ptr(0), read_ptr(0)
    {
        _allocBuffer(sz, wrap_elements);
    }
    
    /**
     * Resize this ring buffer to the given size. This operation
     * is not thread safe! Any operations using this RingBuffer
     * have to be stopped before calling this method.
     *
     * @param sz - new size (amount of elements)
     * @param wrap_elements - (optional) if supplied, the new amount
     *                        of wrap elements to be used beyond
     *                        official buffer end, if not provided
     *                        the amount wrap_elements remains as it was
     *                        before
     */
    void resize(int sz, int wrap_elements = -1) {
        if (wrap_elements == -1)
            wrap_elements = this->wrap_elements;
        
        delete [] buf;
        
        _allocBuffer(sz, wrap_elements);
    }

    virtual ~RingBuffer() {
            delete [] buf;
    }

    /**
     * Sets all remaining write space elements to zero. The write pointer
     * will currently not be incremented after, but that might change in
     * future.
     *
     * @e Caution: for @c T_DEEP_COPY=true you might probably @e NOT want
     * to call this method at all, at least not in case type @c T allocates
     * any additional data on the heap by itself.
     */
    inline void fill_write_space_with_null() {
             int w = write_ptr.load(memory_order_relaxed),
                 r = read_ptr.load(memory_order_acquire);
             memset(get_write_ptr(), 0, sizeof(T)*write_space_to_end());
             if (r && w >= r) {
               memset(get_buffer_begin(), 0, sizeof(T)*(r - 1));
             }

             // set the wrap space elements to null
             if (wrap_elements) memset(&buf[size], 0, sizeof(T)*wrap_elements);
           }

    __inline int  read (T *dest, int cnt);
    __inline int  write (T *src, int cnt);

    inline int push(T* src) { return write(src,1); }
    inline int pop(T* dst)  { return read(dst,1);  }

    __inline T *get_buffer_begin();

    __inline T *get_read_ptr(void) {
      return(&buf[read_ptr.load(memory_order_relaxed)]);
    }

    /**
     * Returns a pointer to the element from the current read position,
     * advanced by \a offset elements.
     */
    /*inline T* get_read_ptr(int offset) {
        int r = read_ptr.load(memory_order_relaxed);
        r += offset;
        r &= size_mask;
        return &buf[r];
    }*/

    __inline T *get_write_ptr();
    __inline void increment_read_ptr(int cnt) {
               read_ptr.store((read_ptr.load(memory_order_relaxed) + cnt) & size_mask, memory_order_release);
             }
    __inline void set_read_ptr(int val) {
               read_ptr.store(val, memory_order_release);
             }

    __inline void increment_write_ptr(int cnt) {
               write_ptr.store((write_ptr.load(memory_order_relaxed) + cnt) & size_mask, memory_order_release);
             }

    /* this function increments the write_ptr by cnt, if the buffer wraps then
       subtract size from the write_ptr value so that it stays within 0<write_ptr<size
       use this function to increment the write ptr after you filled the buffer
       with a number of elements given by write_space_to_end_with_wrap().
       This ensures that the data that is written to the buffer fills up all
       the wrap space that resides past the regular buffer. The wrap_space is needed for
       interpolation. So that the audio thread sees the ringbuffer like a linear space
       which allows us to use faster routines.
       When the buffer wraps the wrap part is memcpy()ied to the beginning of the buffer
       and the write ptr incremented accordingly.
    */
    __inline void increment_write_ptr_with_wrap(int cnt) {
               int w = write_ptr.load(memory_order_relaxed);
               w += cnt;
               if(w >= size) {
                 w -= size;
                 copy(&buf[0], &buf[size], w);
//printf("DEBUG !!!! increment_write_ptr_with_wrap: buffer wrapped, elements wrapped = %d (wrap_elements %d)\n",w,wrap_elements);
               }
               write_ptr.store(w, memory_order_release);
             }

    /* this function returns the available write space in the buffer
       when the read_ptr > write_ptr it returns the space inbetween, otherwise
       when the read_ptr < write_ptr it returns the space between write_ptr and
       the buffer end, including the wrap_space.
       There is an exception to it. When read_ptr <= wrap_elements. In that
       case we return the write space to buffer end (-1) without the wrap_elements,
       this is needed because a subsequent increment_write_ptr which copies the
       data that resides into the wrap space to the beginning of the buffer and increments
       the write_ptr would cause the write_ptr overstepping the read_ptr which would be an error.
       So basically the if(r<=wrap_elements) we return the buffer space to end - 1 which
       ensures that at the next call there will be one element free to write before the buffer wraps
       and usually (except in EOF situations) the next write_space_to_end_with_wrap() will return
       1 + wrap_elements which ensures that the wrap part gets fully filled with data
    */
    __inline int write_space_to_end_with_wrap() {
              int w, r;

              w = write_ptr.load(memory_order_relaxed);
              r = read_ptr.load(memory_order_acquire);
//printf("write_space_to_end: w=%d r=%d\n",w,r);
              if(r > w) {
                //printf("DEBUG: write_space_to_end_with_wrap: r>w r=%d w=%d val=%d\n",r,w,r - w - 1);
                return(r - w - 1);
              }
              if(r <= wrap_elements) {
                //printf("DEBUG: write_space_to_end_with_wrap: ATTENTION r <= wrap_elements: r=%d w=%d val=%d\n",r,w,size - w -1);
                return(size - w -1);
              }
              if(r) {
                //printf("DEBUG: write_space_to_end_with_wrap: r=%d w=%d val=%d\n",r,w,size - w + wrap_elements);
                return(size - w + wrap_elements);
              }
              //printf("DEBUG: write_space_to_end_with_wrap: r=0 w=%d val=%d\n",w,size - w - 1 + wrap_elements);
              return(size - w - 1 + wrap_elements);
            }

           /* this function adjusts the number of elements to write into the ringbuffer
              in a way that the size boundary is avoided and that the wrap space always gets
              entirely filled.
              cnt contains the write_space_to_end_with_wrap() amount while
              capped_cnt contains a capped amount of samples to read.
              normally capped_cnt == cnt but in some cases eg when the disk thread needs
              to refill tracks with smaller blocks because lots of streams require immediate
              refill because lots of notes were started simultaneously.
              In that case we set for example capped_cnt to a fixed amount (< cnt, eg 64k),
              which helps to reduce the buffer refill latencies that occur between streams.
              the first if() checks if the current write_ptr + capped_cnt resides within
              the wrap area but is < size+wrap_elements. in that case we cannot return
              capped_cnt because it would lead to a write_ptr wrapping and only a partial fill
              of wrap space which would lead to errors. So we simply return cnt which ensures
              that the the entire wrap space will get filled correctly.
              In all other cases (which are not problematic because no write_ptr wrapping
              occurs) we simply return capped_cnt.
           */
    __inline int adjust_write_space_to_avoid_boundary(int cnt, int capped_cnt) {
               int w;
               w = write_ptr.load(memory_order_relaxed);
               if((w+capped_cnt) >= size && (w+capped_cnt) < (size+wrap_elements)) {
//printf("adjust_write_space_to_avoid_boundary returning cnt = %d\n",cnt);
                 return(cnt);
               }
//printf("adjust_write_space_to_avoid_boundary returning capped_cnt = %d\n",capped_cnt);
               return(capped_cnt);
             }

    __inline int write_space_to_end() {
              int w, r;

              w = write_ptr.load(memory_order_relaxed);
              r = read_ptr.load(memory_order_acquire);
//printf("write_space_to_end: w=%d r=%d\n",w,r);
              if(r > w) return(r - w - 1);
              if(r) return(size - w);
              return(size - w - 1);
            }

    __inline int read_space_to_end() {
              int w, r;

              w = write_ptr.load(memory_order_acquire);
              r = read_ptr.load(memory_order_relaxed);
              if(w >= r) return(w - r);
              return(size - r);
            }
    __inline void init() {
                   write_ptr.store(0, memory_order_relaxed);
                   read_ptr.store(0, memory_order_relaxed);
                 //  wrap=0;
            }

    int write_space () {
            int w, r;

            w = write_ptr.load(memory_order_relaxed);
            r = read_ptr.load(memory_order_acquire);

            if (w > r) {
                    return ((r - w + size) & size_mask) - 1;
            } else if (w < r) {
                    return (r - w) - 1;
            } else {
                    return size - 1;
            }
    }

    int read_space () {
            int w, r;

            w = write_ptr.load(memory_order_acquire);
            r = read_ptr.load(memory_order_relaxed);

            if (w >= r) {
                    return w - r;
            } else {
                    return (w - r + size) & size_mask;
            }
    }

    int size;
    int wrap_elements;

    /**
     * Independent, random access reading from a RingBuffer. This class
     * allows to read from a RingBuffer without being forced to free read
     * data while reading / positioning.
     */
    template<class T1, bool T1_DEEP_COPY>
    class _NonVolatileReader {
        public:
            int read_space() {
                int r = read_ptr;
                int w = pBuf->write_ptr.load(memory_order_acquire);
                return (w >= r) ? w - r : (w - r + pBuf->size) & pBuf->size_mask;
            }

            /**
             * Prefix decrement operator, for reducing NonVolatileReader's
             * read position by one.
             */
            inline void operator--() {
                if (read_ptr == pBuf->read_ptr.load(memory_order_relaxed)) return; //TODO: or should we react oh this case (e.g. force segfault), as this is a very odd case?
                read_ptr = (read_ptr-1) & pBuf->size_mask;
            }

            /**
             * Postfix decrement operator, for reducing NonVolatileReader's
             * read position by one.
             */
            inline void operator--(int) {
                --*this;
            }
    
            /**
             * "Increment assign" operator, for advancing NonVolatileReader's
             * read position by @a n elements.
             *
             * @param n - amount of elements to advance read position
             */
            inline void operator+=(int n) {
                if (read_space() < n) return;
                read_ptr = (read_ptr+n) & pBuf->size_mask;
            }

            /**
             * Returns pointer to the RingBuffer data of current
             * NonVolatileReader's read position and increments
             * NonVolatileReader's read position by one.
             *
             * @returns pointer to element of current read position
             */
            T* pop() {
                if (!read_space()) return NULL;
                T* pData = &pBuf->buf[read_ptr];
                read_ptr++;
                read_ptr &= pBuf->size_mask;
                return pData;
            }

            /**
             * Reads one element from the NonVolatileReader's current read
             * position and copies it to the variable pointed by \a dst and
             * finally increments the NonVolatileReader's read position by
             * one.
             *
             * @param dst - where the element is copied to
             * @returns 1 on success, 0 otherwise
             */
            int pop(T* dst) { return read(dst,1); }

            /**
             * Reads \a cnt elements from the NonVolatileReader's current
             * read position and copies it to the buffer pointed by \a dest
             * and finally increments the NonVolatileReader's read position
             * by the number of read elements.
             *
             * @param dest - destination buffer
             * @param cnt  - number of elements to read
             * @returns number of read elements
             */
            int read(T* dest, int cnt) {
                int free_cnt;
                int cnt2;
                int to_read;
                int n1, n2;
                int priv_read_ptr;

                priv_read_ptr = read_ptr;

                if ((free_cnt = read_space()) == 0) return 0;

                to_read = cnt > free_cnt ? free_cnt : cnt;

                cnt2 = priv_read_ptr + to_read;

                if (cnt2 > pBuf->size) {
                    n1 = pBuf->size - priv_read_ptr;
                    n2 = cnt2 & pBuf->size_mask;
                } else {
                    n1 = to_read;
                    n2 = 0;
                }

                copy(dest, &pBuf->buf[priv_read_ptr], n1);
                priv_read_ptr = (priv_read_ptr + n1) & pBuf->size_mask;

                if (n2) {
                    copy(dest+n1, pBuf->buf, n2);
                    priv_read_ptr = n2;
                }

                this->read_ptr = priv_read_ptr;
                return to_read;
            }

            /**
             * Finally when the read data is not needed anymore, this method
             * should be called to free the data in the RingBuffer up to the
             * current read position of this NonVolatileReader.
             *
             * @see RingBuffer::increment_read_ptr()
             */
            void free() {
                pBuf->read_ptr.store(read_ptr, memory_order_release);
            }

        protected:
            _NonVolatileReader(RingBuffer<T1,T1_DEEP_COPY>* pBuf) {
                this->pBuf     = pBuf;
                this->read_ptr = pBuf->read_ptr.load(memory_order_relaxed);
            }

            RingBuffer<T1,T1_DEEP_COPY>* pBuf;
            int read_ptr;

            friend class RingBuffer<T1,T1_DEEP_COPY>;
    };

    typedef _NonVolatileReader<T,T_DEEP_COPY> NonVolatileReader;

    NonVolatileReader get_non_volatile_reader() { return NonVolatileReader(this); }

  protected:
    T *buf;
    atomic<int> write_ptr;
    atomic<int> read_ptr;
    int size_mask;

    /**
     * Copies \a n amount of elements from the buffer given by
     * \a pSrc to the buffer given by \a pDst.
     */
    inline static void copy(T* pDst, T* pSrc, int n);
    
    void _allocBuffer(int sz, int wrap_elements) {
        this->wrap_elements = wrap_elements;
            
        // the write-with-wrap functions need wrap_elements extra
        // space in the buffer to be able to copy the wrap space
        sz += wrap_elements;

        int power_of_two;
        for (power_of_two = 1;
             1<<power_of_two < sz;
             power_of_two++);

        size = 1<<power_of_two;
        size_mask = size;
        size_mask -= 1;
        buf = new T[size + wrap_elements];   
    }

    friend class _NonVolatileReader<T,T_DEEP_COPY>;
};

template<class T, bool T_DEEP_COPY>
T* RingBuffer<T,T_DEEP_COPY>::get_write_ptr (void) {
  return(&buf[write_ptr.load(memory_order_relaxed)]);
}

template<class T, bool T_DEEP_COPY>
T* RingBuffer<T,T_DEEP_COPY>::get_buffer_begin (void) {
  return(buf);
}



template<class T, bool T_DEEP_COPY>
int RingBuffer<T,T_DEEP_COPY>::read(T* dest, int cnt)
{
        int free_cnt;
        int cnt2;
        int to_read;
        int n1, n2;
        int priv_read_ptr;

        priv_read_ptr = read_ptr.load(memory_order_relaxed);

        if ((free_cnt = read_space ()) == 0) {
                return 0;
        }

        to_read = cnt > free_cnt ? free_cnt : cnt;

        cnt2 = priv_read_ptr + to_read;

        if (cnt2 > size) {
                n1 = size - priv_read_ptr;
                n2 = cnt2 & size_mask;
        } else {
                n1 = to_read;
                n2 = 0;
        }

        copy(dest, &buf[priv_read_ptr], n1);
        priv_read_ptr = (priv_read_ptr + n1) & size_mask;

        if (n2) {
                copy(dest+n1, buf, n2);
                priv_read_ptr = n2;
        }

        read_ptr.store(priv_read_ptr, memory_order_release);
        return to_read;
}

template<class T, bool T_DEEP_COPY>
int RingBuffer<T,T_DEEP_COPY>::write(T* src, int cnt)
{
        int free_cnt;
        int cnt2;
        int to_write;
        int n1, n2;
        int priv_write_ptr;

        priv_write_ptr = write_ptr.load(memory_order_relaxed);

        if ((free_cnt = write_space ()) == 0) {
                return 0;
        }

        to_write = cnt > free_cnt ? free_cnt : cnt;

        cnt2 = priv_write_ptr + to_write;

        if (cnt2 > size) {
                n1 = size - priv_write_ptr;
                n2 = cnt2 & size_mask;
        } else {
                n1 = to_write;
                n2 = 0;
        }

        copy(&buf[priv_write_ptr], src, n1);
        priv_write_ptr = (priv_write_ptr + n1) & size_mask;

        if (n2) {
                copy(buf, src+n1, n2);
                priv_write_ptr = n2;
        }
        write_ptr.store(priv_write_ptr, memory_order_release);
        return to_write;
}

template<class T, bool T_DEEP_COPY>
void RingBuffer<T,T_DEEP_COPY>::copy(T* pDst, T* pSrc, int n) {
    if (T_DEEP_COPY) { // deep copy - won't work for data structures without assignment operator implementation
        for (int i = 0; i < n; i++) pDst[i] = pSrc[i];
    } else { // flat copy - won't work for complex data structures !
        memcpy(pDst, pSrc, n * sizeof(T));
    }
}

#endif /* RINGBUFFER_H */
