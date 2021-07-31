/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2004, 2005 Grame                                        *
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

#ifndef __LS_MIDIINPUTDEVICECOREMIDI_H__
#define __LS_MIDIINPUTDEVICECOREMIDI_H__

#include <CoreMIDI/MIDIServices.h>

#include "../../common/global_private.h"
#include "MidiInputDevice.h"

namespace LinuxSampler {

    /** CoreMidi input driver
     *
     * Implements MIDI input for MacOSX CoreMidi architecture
     */
    class MidiInputDeviceCoreMidi : public MidiInputDevice {

		 public:
			/**
             * MIDI Port implementation for the CoreMidi input driver.
             */
            class MidiInputPortCoreMidi : public MidiInputPort {
                public:
                    /** MIDI Port Parameter 'NAME'
                     *
                     * Used to assign an arbitrary name to the MIDI port.
                     */
                    class ParameterName : public MidiInputPort::ParameterName {
                        public:
                            ParameterName(MidiInputPort* pPort) throw (Exception);
                            virtual void OnSetValue(String s) throw (Exception) OVERRIDE;
                    };

                    /** MIDI Port Parameter 'CORE_MIDI_BINDINGS'
                     *
                     * Used to connect to other Alsa sequencer clients.
                     */

                    class ParameterCoreMidiBindings : public DeviceRuntimeParameterStrings {
                        public:
                            ParameterCoreMidiBindings(MidiInputPortCoreMidi* pPort);
                            virtual String Description() OVERRIDE;
                            virtual bool Fix() OVERRIDE;
                            virtual std::vector<String> PossibilitiesAsString() OVERRIDE;
                            virtual void OnSetValue(std::vector<String> vS) throw (Exception) OVERRIDE;
                        protected:
                            MidiInputPortCoreMidi* pPort;
                    };
				
					/** MIDI Port Parameter 'AUTO_BIND'
					 *
					 * If enabled, the port will automatically be connected to all
					 * CoreMIDI source endpoints at present and future.
					 */
					class ParameterAutoBind : public DeviceRuntimeParameterBool {
						public:
							ParameterAutoBind(MidiInputPortCoreMidi* pPort);
							virtual String Description() OVERRIDE;
							virtual bool Fix() OVERRIDE;
							virtual void OnSetValue(bool b) throw (Exception) OVERRIDE;
						protected:
							MidiInputPortCoreMidi* pPort;
					};
				
					void ProcessMidiEvents(const MIDIPacketList *pktlist);

					static void ReadProc(const MIDIPacketList *pktlist, void *refCon, void *connRefCon);
					static int pPortID;

                protected:
                    MidiInputPortCoreMidi(MidiInputDeviceCoreMidi* pDevice) throw (MidiInputException);
                    ~MidiInputPortCoreMidi();
					void connectToSource(MIDIEndpointRef source);
					void connectToAllSources();
					void onNewSourceAppeared(MIDIEndpointRef source);
					void onNewSourceDisappeared(MIDIEndpointRef source);
                    void onCoreMIDIDeviceAppeared(MIDIDeviceRef device);
                    void onCoreMIDIDeviceDisappeared(MIDIDeviceRef device);
                    void onCoreMIDIEntityAppeared(MIDIEntityRef entity);
                    void onCoreMIDIEntityDisappeared(MIDIEntityRef entity);
                    friend class MidiInputDeviceCoreMidi;
                private:
                    MidiInputDeviceCoreMidi* pDevice;
					MIDIEndpointRef pDestination;
					std::vector<MIDIEndpointRef> bindings; //TODO: shall probably be protected by a mutex (since the CoreMIDI notification callback thread might also modify it when new sources appear or disappear)

                    friend class ParameterName;
                    friend class ParameterCoreMidiBindings;
            };

            MidiInputDeviceCoreMidi(std::map<String,DeviceCreationParameter*> Parameters, void* pSampler);
            virtual ~MidiInputDeviceCoreMidi();

            // derived abstract methods from class 'MidiInputDevice'
            void Listen() OVERRIDE {}
            void StopListen() OVERRIDE {}
			virtual String Driver() OVERRIDE;
			static String Name();
			static String Description();
            static String Version();

			MidiInputPortCoreMidi* CreateMidiPort() OVERRIDE;

			// CoreMidi callback
			static void NotifyProc(const MIDINotification* message, void* refCon);

        private:
			MIDIClientRef   hCoreMidiClient;
			MIDIPortRef pBridge;
    };
}

#endif // __LS_MIDIINPUTDEVICECOREMIDI_H__
