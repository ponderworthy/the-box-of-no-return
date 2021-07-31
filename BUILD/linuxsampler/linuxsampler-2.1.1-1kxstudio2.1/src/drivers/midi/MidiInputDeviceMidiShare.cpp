/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2004 Grame											   *
 *   Copyright (C) 2014 Christian Schoenebeck                              *
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

#include "MidiInputDeviceMidiShare.h"

namespace LinuxSampler {

	#define MSHSlotName		"LinuxSampler"
	#define MSHDriverName	"LinuxSampler"
	
	#define MidiShareDrvRef		127

   /**
     * Create MidiShare input device for LinuxSampler. 
	 *
     * @param AutoConnectPortID - (optional) Alsa client and port ID of a
     *                            MIDI source we should auto connect to
     *                            (e.g. "64:0")
     * @throws MidiInputException  if initialization failed
     */
    MidiInputDeviceMidiShare::MidiInputDeviceMidiShare(char* AutoConnectPortID) : MidiInputDevice(MidiInputDevice::type_midishare) 
	{
		if (!MidiShare())
			throw MidiInputException("MidiShare not installed");
			
		#if defined(MIDISHARE_DRIVER)
			OpenDriver();
		#else
			OpenAppl();
		#endif
		
		hMidiFilter = MidiNewFilter();
		
		if (hMidiFilter == 0) {
			MidiClose(hRefnum);
			throw MidiInputException("MidiShare filter can not be allocated");	
		}
		  
		for (int i = 0 ; i < 256; i++) {
			MidiAcceptPort(hMidiFilter, i, 1); /* accept all ports */
			MidiAcceptType(hMidiFilter, i, 0); /* reject all types */ 
		} 
		  
		for (int i = 0 ; i < 16; i++) {
			MidiAcceptChan(hMidiFilter, i, 1); /* accept all chan */ 
		} 
		/* accept only the following types */
		MidiAcceptType(hMidiFilter, typeNote, 1);
		MidiAcceptType(hMidiFilter, typeKeyOn, 1);
		MidiAcceptType(hMidiFilter, typeKeyOff, 1);
		MidiAcceptType(hMidiFilter, typeCtrlChange, 1);
		MidiAcceptType(hMidiFilter, typeProgChange, 1);
		MidiAcceptType(hMidiFilter, typePitchWheel, 1);
		MidiAcceptType(hMidiFilter, typeChanPress, 1);
		MidiAcceptType(hMidiFilter, typeKeyPress, 1);
		  
		/* set the filter */
		MidiSetFilter(hRefnum, hMidiFilter);
	
		MidiSetRcvAlarm(hRefnum,ReceiveEvents);
		MidiSetApplAlarm(hRefnum,ApplAlarm);
		MidiSetInfo(hRefnum,this);
		MidiConnect(0,hRefnum,true);
	}
   
   
    MidiInputDeviceMidiShare::~MidiInputDeviceMidiShare() 
	{
		#if defined(MIDISHARE_DRIVER)
			CloseDriver();
		#else
			CloseAppl(); 
		#endif
		MidiFreeFilter(hMidiFilter);
    }
	
	void MidiInputDeviceMidiShare::OpenAppl() 
	{
	   hRefnum = MidiOpen(MSHDriverName);
		if (hRefnum < 0) {
			throw MidiInputException("MidiShare client can not be opened");	
		}
		MidiSetRcvAlarm(hRefnum,ReceiveEvents);
		MidiSetApplAlarm(hRefnum,ApplAlarm);
    }

	void MidiInputDeviceMidiShare::CloseAppl() 
	{
		MidiClose(hRefnum);
	}
	
	void MidiInputDeviceMidiShare::OpenDriver()
	{
		/* gcc wanted me to use {0,0} to initialize the reserved[2] fields */
		TDriverInfos infos = { MSHDriverName, 100, 0, { 0, 0 } };
		TDriverOperation op = { WakeUp, Sleep, { 0, 0, 0 } }; 
		hRefnum = MidiRegisterDriver(&infos, &op);
		if (hRefnum < 0) {
			throw MidiInputException("MidiShare driver can not be opened");	
		}
		hSlotRef = MidiAddSlot(hRefnum,MSHSlotName,MidiOutputSlot);
		MidiSetRcvAlarm(hRefnum,ReceiveEvents);
	}
	
	void MidiInputDeviceMidiShare::CloseDriver() 
	{
		MidiUnregisterDriver(hRefnum);
	}
	
	void MidiInputDeviceMidiShare::WakeUp(short r)
	{
		MidiConnect(MidiShareDrvRef, r, true);
		MidiConnect(r, MidiShareDrvRef, true);
	}

	void MidiInputDeviceMidiShare::Sleep(short r){}

		
    void MidiInputDeviceMidiShare::SetInputPort(const char * MidiSource) 
	{
	    
    }
  	
	void MidiInputDeviceMidiShare::ApplAlarm(short ref, long code)
	{
	
	}
	
	void MidiInputDeviceMidiShare::KeyOffTask(long date, short ref, long a1, long a2, long a3)
	{
		MidiInputDeviceMidiShare* driver = (MidiInputDeviceMidiShare*)MidiGetInfo(ref);
		MidiEvPtr ev =(MidiEvPtr)a1;
		driver->DispatchNoteOn(Pitch(ev),Vel(ev),Chan(ev));
		MidiFreeEv(ev);
	}
	
	void MidiInputDeviceMidiShare::ReceiveEvents(short ref)
	{
		MidiInputDeviceMidiShare* driver = (MidiInputDeviceMidiShare*)MidiGetInfo(ref);
		MidiEvPtr ev;
		
		while ((ev = MidiGetEv(ref)))
		
			switch(EvType(ev)) { 
			
				case typeCtrlChange:
					if (MidiGetField(ev,0) == 0)
						driver->DispatchBankSelectMsb(MidiGetField(ev,0),Chan(ev));
					else if (MidiGetField(ev,0) == 32)
						driver->DispatchBankSelectLsb(MidiGetField(ev,0),Chan(ev));
					driver->DispatchControlChange(MidiGetField(ev,0),MidiGetField(ev,0),Chan(ev));
					MidiFreeEv(ev);
					break;
					
				case typePitchWheel:
					driver->DispatchPitchbend(((MidiGetField(ev,0)+(MidiGetField(ev,1) << 7)) - 8192),Chan(ev));
					MidiFreeEv(ev);
					break;

				case typeChanPress:
					driver->DispatchChannelPressure(MidiGetField(ev,0),Chan(ev));
					MidiFreeEv(ev);
					break;

				case typeKeyPress:
					driver->DispatchPolyphonicKeyPressure(Pitch(ev),Vel(ev),Chan(ev));
					MidiFreeEv(ev);
					break;

				case typeNote:
					driver->DispatchNoteOn(Pitch(ev),Vel(ev),Chan(ev));
					MidiTask(KeyOffTask,Date(ev)+Dur(ev),ref,(long)ev,0,0);
					break;

				case typeKeyOn:
					if (Vel(ev) > 0)
						driver->DispatchNoteOn(Pitch(ev),Vel(ev),Chan(ev));
					else
						driver->DispatchNoteOff(Pitch(ev),Vel(ev),Chan(ev));
					MidiFreeEv(ev);
					break;
				
				case typeKeyOff:
					driver->DispatchNoteOff(Pitch(ev),Vel(ev),Chan(ev));
					MidiFreeEv(ev);
					break;
			}
		}


} // namespace LinuxSampler
