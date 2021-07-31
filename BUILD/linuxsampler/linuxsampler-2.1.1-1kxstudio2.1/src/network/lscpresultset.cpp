/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

/*********************************************************
 * This class helps to constuct valid resultsets per
 * LSCP protocol specification
 *
 * Valid results include:
 * OK - to ack the request
 * Single line to ack the requests and give status
 * Several lines of information in the following format:
 * LABEL0: VALUE0
 * LABEL1: VALUE1
 * VALELx: VALUEx
 * .
 *
 * ******************************************************/

#include "lscpresultset.h"
#include <iomanip>
#include "../common/global_private.h"


namespace LinuxSampler {

//Construct an empty resultset
//Default index is -1 meaning the resultset doesn't have an index
LSCPResultSet::LSCPResultSet(int index) {
	result_index = index;
	count = 0;
	storage = "";
	result_type = result_type_success;
}

//Construct a resultset with a single line
//Default index is -1 meaning the resultset doesn't have an index
LSCPResultSet::LSCPResultSet(String Value, int index) {
	result_index = index;
	count = 1;
	storage = Value + "\r\n";
	result_type = result_type_success;
}

//Add a label/value pair to the resultset
//Values could be of different types for now supports String, int and float.
void LSCPResultSet::Add(String Label, String Value) {
	if (count == -1)
        	throw Exception("Attempting to change already produced resultset");
	if (result_type != result_type_success)
		throw Exception("Attempting to create illegal resultset");
	storage = storage + Label + ": " + Value + "\r\n";
        count = 2; // results in form of "Label: Value" should always be handled as multi line responses
}

void LSCPResultSet::Add(String Label, const char* pValue) {
    Add(Label, String(pValue));
}

//Add SQL resultset row
void LSCPResultSet::Add(int columns, char** argv) {
	for (int i = 0; i < columns; i++)
	{
		storage += argv[i];
		if ((i+1) < columns)
			storage += "|";
	}
	storage += "\r\n";
	count = 2; //This result is always multiline.
}

void LSCPResultSet::Add(int Value) {
	Add(ToString(Value));
}

void LSCPResultSet::Add(String Label, int Value) {
	Add(Label, ToString(Value));
}

void LSCPResultSet::Add(String Label, float Value) {
    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << std::fixed << std::setprecision(3) << Value;
    Add(Label, ss.str());
}

void LSCPResultSet::Add(String Label, bool Value) {
    String s = (Value) ? "true" : "false";
    Add(Label, s);
}

//Add a single string to the resultset
void LSCPResultSet::Add(String Value) {
	if (result_type != result_type_success)
		throw Exception("Attempting to create illegal resultset");
	if (count == -1)
        	throw Exception("Attempting to change already produced resultset");
	if (count != 0)
		throw Exception("Attempting to create illegal resultset");
	storage = Value + "\r\n";
        count = 1;
}

void LSCPResultSet::Add(String Label, const std::vector<float>& v) {
    std::stringstream ss;
    ss.imbue(std::locale::classic());
    for (int i = 0; i < v.size(); i++) {
        if (!ss.str().empty()) ss << ",";
        ss << std::fixed << std::setprecision(3) << v[i];
    }
    Add(Label, ss.str());    
}

//Generate an error result set from an exception.
//Per LSCP spec, error result is a sinle line in the following format:
//ERR:<CODE>:Message text\r\n
//This method will be used to generate unknown errors only (code 0)
//To generate errors with other codes as well as warnings use other methods (below).
//Because this is an unknown error, this method will also print message to the stderr.
void LSCPResultSet::Error(Exception e) {
        e.PrintMessage();
	Error(e.Message());
}

//This will construct an error with a string and error code
//code has a default of 0
//String has a default of "Undefined Error"
void LSCPResultSet::Error (String message, int code) {
        //Even though this is must be a single line resultset we won't throw
        //anything here because this is already part of exception handling.
        //We'll just 'forget' all previous results (if any) from this resultset.
	result_type = result_type_error;
        storage = "ERR:" + ToString(code) + ":" + message + "\r\n";
        count = 1;
}

//This will construct a warning with a string and error code
//code has a default of 0
//String has a default of "Undefined Error"
void LSCPResultSet::Warning (String message, int code) {
	//FIXME: DO we want warnings as part of the resultset or
	//do we want them to work like errors??? For now, make them work like errors.
	result_type = result_type_warning;
	if (result_index == -1)
        	storage = "WRN:" + ToString(code) + ":" + message + "\r\n";
	else
        	storage = "WRN[" + ToString(result_index) + "]:" + ToString(code) + ":" + message + "\r\n";
        count = 1;
}

//Produce resultset
String LSCPResultSet::Produce(void) {
	//FIXME: I'm assuming that only a sinle like "OK" can have index
	if (count == 0) { //When there is nothing in the resultset we just send "OK" to ack the request
		if (result_index == -1)
			return "OK\r\n";
		else
			return "OK[" + ToString(result_index) + "]\r\n";
    }
	if (count == 1) //Single line results are just that, single line
		return storage;
	//Multiline results MUST end with a line with a single dot
	return storage + ".\r\n";
}

}
