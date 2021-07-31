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

#ifndef __LSCPEVENT_H_
#define __LSCPEVENT_H_
#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include "../Sampler.h"
#include "../common/global.h"
#include "../common/Exception.h"

namespace LinuxSampler {

/**
 * Helper class for producing result sets
 */
class LSCPEvent {
    public:
	    /**
	     * Event types
	     **/
	    enum event_t {
		    event_audio_device_count,
		    event_audio_device_info,
		    event_midi_device_count,
		    event_midi_device_info,
		    event_channel_count,
		    event_voice_count,
		    event_stream_count,
		    event_buffer_fill,
		    event_channel_info,
		    event_fx_send_count,
		    event_fx_send_info,
		    event_midi_instr_map_count,
		    event_midi_instr_map_info,
		    event_midi_instr_count,
		    event_midi_instr_info,
            event_db_instr_dir_count,
		    event_db_instr_dir_info,
            event_db_instr_count,
		    event_db_instr_info,
            event_db_instrs_job_info,
		    event_misc,
		    event_total_stream_count,
		    event_total_voice_count,
		    event_global_info,
		    event_channel_midi,
		    event_device_midi,
                    event_fx_instance_count,
                    event_fx_instance_info,
                    event_send_fx_chain_count,
                    event_send_fx_chain_info
	    };

	    /* This constructor will do type lookup based on name
	     **/
	    LSCPEvent(String eventName) throw (Exception);

	    /* These constructors are used to create event and fill it with data for sending
	     * These will be used by the thread that wants to send an event to all clients
	     * that are subscribed to it
	     **/
	    LSCPEvent(event_t eventType, int uiData);
	    LSCPEvent(event_t eventType, String sData);
	    LSCPEvent(event_t eventType, int uiData1, int uiData2);
	    LSCPEvent(event_t eventType, int uiData1, int uiData2, String sData3, int uiData4, int uiData5);
	    LSCPEvent(event_t eventType, String sData, int uiData);
	    LSCPEvent(event_t eventType, String sData, double dData);
	    LSCPEvent(event_t eventType, int uiData, String sData);
	    LSCPEvent(event_t eventType, int uiData1, String sData2, int uiData3, int uiData4);
	    LSCPEvent(event_t eventType, int uiData1, int uiData2, int uiData3);
        LSCPEvent(event_t eventType, String sData1, String sData2, String sData3);
	    String Produce( void );

	    /* Returns event type */
	    event_t GetType( void ) { return type; }

	    /* These methods are used to registed and unregister an event */
	    static void RegisterEvent(event_t eventType, String EventName);
	    static void UnregisterEvent(event_t eventType);

	    /* This method returns a name for events of a given type */
	    static String Name(event_t eventType);

	    /* This method returs a list of all event types registered */
	    static std::list<event_t> List( void );

    private:
	    String storage;
	    event_t type;

	    static std::map<event_t, String> EventNames;
};

}

#endif // __LSCPEVENT_H_
