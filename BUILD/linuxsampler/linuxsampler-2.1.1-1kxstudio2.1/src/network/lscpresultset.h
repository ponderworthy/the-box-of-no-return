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

#ifndef __LSCPRESULTSET_H_
#define __LSCPRESULTSET_H_
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../Sampler.h"
#include "../common/global.h"
#include "../common/Exception.h"

namespace LinuxSampler {

/**
 * Helper class for producing result sets
 */
class LSCPResultSet {
    public:
        LSCPResultSet(int index = -1);
        LSCPResultSet(String, int index = -1);
	void Add(String);
	void Add(String, String);
        void Add(String, const char*);
	void Add(int columns, char** argv);
	void Add(String, float);
	void Add(String, int);
        void Add(String, bool);
	void Add(int);
    void Add(String Label, const std::vector<float>& v);
	void Error(String message = "Undefined Error", int code = 0);
	void Error(Exception e);
	void Warning(String message = "Undefined Warning", int code = 0);
	String Produce(void);

    private:
	String storage;
	int count;
	int result_type;
	int result_index;

};

}

#endif // __LSCPRESULTSET_H_
