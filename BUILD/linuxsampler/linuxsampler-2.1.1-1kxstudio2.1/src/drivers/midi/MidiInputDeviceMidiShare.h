/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2004 Grame											   *
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

#ifndef __LS_MIDIINPUTDEVICEMIDISHARE_H__
#define __LS_MIDIINPUTDEVICEMIDISHARE_H__

#include <MidiShare.h>

#include "../common/global_private.h"
#include "MidiInputDevice.h"

namespace LinuxSampler {

    /** MidiShare input driver
     *
     * Implements MIDI input using MidiShare (http://midishare.sourceforge.net/)
     */
    class MidiInputDeviceMidiShare : public MidiInputDevice {
        public:
            MidiInputDeviceMidiShare(std::map<String,DeviceCreationParameter*> Parameters);
            ~MidiInputDeviceMidiShare();

            // derived abstract methods from class 'MidiInputDevice'
            void Listen() OVERRIDE {}
            void StopListen() OVERRIDE {}
            virtual String Driver() OVERRIDE;
            static String Name();
            static String Description();
            static String Version();

            // own methods
            void ConnectToCoreMidiSource(const char* MidiSource);

			void OpenAppl();
			void CloseAppl();
			void OpenDriver();
			void CloseDriver();

			// MidiShare callback
			static void ApplAlarm(short ref, long code);
			static void ReceiveEvents(short ref);
			static void KeyOffTask(long date, short ref, long a1, long a2, long a3);
			static void WakeUp(short r);
			static void Sleep(short r);

        private:
			short			hRefnum;
			MidiFilterPtr   hMidiFilter;
			SlotRefNum		hSlotRef;
   };
}

#endif // __LS_MIDIINPUTDEVICEMIDISHARE_H__
