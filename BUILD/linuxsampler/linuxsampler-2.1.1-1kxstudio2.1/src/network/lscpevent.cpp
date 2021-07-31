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

/*********************************************************
 * This class helps to constuct valid events per
 * LSCP protocol specification
 *
 * It also helps LSCPServer lookup the events when
 * a client is trying to subscribe to an event
 *
 * ******************************************************/

#include "lscpevent.h"

#include "../common/global_private.h"

namespace LinuxSampler {

std::map<LSCPEvent::event_t, String> LSCPEvent::EventNames = std::map<LSCPEvent::event_t, String>();

LSCPEvent::LSCPEvent(String eventName) throw (Exception) {
	for (std::map<event_t, String>::iterator iter = EventNames.begin(); iter != EventNames.end(); iter++) {
		if (iter->second == eventName) {
			this->type = iter->first;
			return;
		}
	}
	throw Exception("Event does not exist");
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData) {
	this->type = eventType;
	this->storage = ToString(uiData);
}

LSCPEvent::LSCPEvent(event_t eventType, String sData) {
	this->type = eventType;
	this->storage = sData;
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData1, int uiData2) {
	this->type = eventType;
	this->storage = ToString(uiData1) + " " + ToString(uiData2);
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData1, int uiData2, String sData3, int uiData4, int uiData5) {
	this->type = eventType;
	this->storage = ToString(uiData1) + " " + ToString(uiData2) + " " +
	                sData3 + " " + ToString(uiData4) + " " + ToString(uiData5);
}

LSCPEvent::LSCPEvent(event_t eventType, String sData, int uiData) {
	this->type = eventType;
	this->storage = sData + " " + ToString(uiData);
}

LSCPEvent::LSCPEvent(event_t eventType, String sData, double dData) {
	this->type = eventType;
	this->storage = sData + " " + ToString(dData);
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData, String sData) {
	this->type = eventType;
	this->storage = ToString(uiData) + " " + sData;
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData1, String sData2, int uiData3, int uiData4) {
	this->type = eventType;
	this->storage = ToString(uiData1) + " " + sData2 + " " +
	                ToString(uiData3) + " " + ToString(uiData4);
}

LSCPEvent::LSCPEvent(event_t eventType, int uiData1, int uiData2, int uiData3) {
	this->type = eventType;
	this->storage = ToString(uiData1) + " " + ToString(uiData2) + " " + ToString(uiData3);
}

LSCPEvent::LSCPEvent(event_t eventType, String sData1, String sData2, String sData3) {
	this->type = eventType;
	this->storage = sData1 + " " + sData2 + " " + sData3;
}

//Produce event string
String LSCPEvent::Produce(void) {
	String result = "NOTIFY:";
	result += EventNames[type];
	result += ":";
	result += storage;
	result += "\r\n";
	return result;
}

//This can later return handle and then we can get rid of event_t enum
//and make events fully dynamic
void LSCPEvent::RegisterEvent(event_t eventType, String EventName) {
	EventNames[eventType] = EventName;
}

void LSCPEvent::UnregisterEvent(event_t eventType) {
	EventNames.erase(eventType);
}

//The following static method is used to get names from types
String LSCPEvent::Name(event_t eventType) {
	if (EventNames.count(eventType))
		return EventNames[eventType];
	return "UNKNOWN";
}

std::list<LSCPEvent::event_t> LSCPEvent::List( void ) {
	std::list<event_t> result;
	for (std::map<event_t, String>::iterator iter = EventNames.begin(); iter != EventNames.end(); iter++)
		result.push_back(iter->first);
	return result;
}

}
