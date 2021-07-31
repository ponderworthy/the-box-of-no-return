/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005, 2006 Christian Schoenebeck                        *
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

#ifndef __LS_EXCEPTION_H
#define __LS_EXCEPTION_H

#include <stdexcept>
#include <iostream>
#include "global.h"

namespace LinuxSampler {

    /**
     * Exception that will be thrown in NON REAL TIME PARTS of the
     * LinuxSampler application.
     */
    class Exception : public std::runtime_error {
        public:
            /**
             * Constructor
             *
             * @param msg - cause of the exception
             */
            Exception(const String& msg) : runtime_error(msg) {
            }

            /**
             * Returns the cause of the exception.
             */
            String Message() {
                return what();
            }

            /**
             * Print the cause of the exception to the standard error
             * output channel.
             */
            void PrintMessage() {
                std::cerr << what() << std::endl << std::flush;
            }
    };

} // namespace LinuxSampler

#endif // __LS_EXCEPTION_H
