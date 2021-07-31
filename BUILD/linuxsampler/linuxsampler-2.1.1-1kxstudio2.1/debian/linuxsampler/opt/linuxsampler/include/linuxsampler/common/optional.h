/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2019 Christian Schoenebeck                       *
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

#ifndef __LS_OPTIONAL_H__
#define __LS_OPTIONAL_H__

#include "Exception.h"

namespace LinuxSampler {

    /**
     * Base class of template class optional, not meant to be instantiated
     * directly. It just provides the optional<T>::nothing member.
     */
    class optional_base {
        public:
            class nothing_t { public: nothing_t() {} };

            static const nothing_t nothing;
    };

    /**
     * This class can be used for any variable that might not have a value
     * set. E.g. as a return value type of a function, since in the case of
     * return values of functions it's often difficult to return a pointer
     * variable which might do the trick of an optional return value. It
     * behaves pretty much like a pointer though. That is, it can be checked
     * against NULL and the actual value can be dereferenced with the
     * typical C pointer dereference operators.
     */
    template<class T>
    class optional : public optional_base {
        public:
            optional() {
                initialized = false;
            }

            optional(T data) {
                this->data  = data;
                initialized = true;
            }

            optional(nothing_t) {
                initialized = false;
            }

            template <class T_inner>
            optional(T_inner data) {
                this->data  = T(data);
                initialized = true;
            }

            const T& get() const throw (Exception) {
                if (!initialized) throw Exception("optional variable not initialized");
                return data;
            }

            T& get() throw (Exception) {
                if (!initialized) throw Exception("optional variable not initialized");
                return data;
            }

            optional& operator =(const optional& arg) {
                this->data  = arg.data;
                initialized = arg.initialized;
                return *this;
            }

            optional& operator =(const T& arg) {
                this->data  = arg;
                initialized = true;
                return *this;
            }

            bool operator ==(const optional& o) const {
                if (!initialized || !o.initialized)
                    return initialized == o.initialized;
                return data == o.data;
            }

            bool operator !=(const optional& o) const {
                return !(*this == o);
            }

            const T& operator *() const throw (Exception) { return get(); }
            T&       operator *()       throw (Exception) { return get(); }

            const T* operator ->() const throw (Exception) {
                if (!initialized) throw Exception("optional variable not initialized");
                return &data;
            }

            T* operator ->() throw (Exception) {
                if (!initialized) throw Exception("optional variable not initialized");
                return &data;
            }

            operator bool()   const { return initialized; }
            bool operator !() const { return !initialized; }

        protected:
            T    data;
            bool initialized;
    };

} // namespace LinuxSampler

#endif // __LS_OPTIONAL_H__
