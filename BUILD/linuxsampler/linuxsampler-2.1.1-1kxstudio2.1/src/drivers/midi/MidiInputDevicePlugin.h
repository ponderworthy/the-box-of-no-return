/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2012 Andreas Persson                             *
 *   Copyright (C) 2013 - 2016 Christian Schoenebeck                       *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef LS_MIDIINPUTDEVICEPLUGIN_H
#define LS_MIDIINPUTDEVICEPLUGIN_H

#include "MidiInputDevice.h"

namespace LinuxSampler {

    /** Plugin MIDI input driver
     *
     * Implements MIDI input when LinuxSampler is running as a plugin.
     *
     * The plugin implementation is given access to the MidiInputPort,
     * to which MIDI events can be dispatched.
     */
    class MidiInputDevicePlugin : public MidiInputDevice {
    public:
        MidiInputDevicePlugin(std::map<String, DeviceCreationParameter*> Parameters,
                              void* pSampler);
        ~MidiInputDevicePlugin();

        /**
         * MIDI Port implementation for the plugin MIDI input driver.
         */
        class MidiInputPortPlugin : public MidiInputPort {
        protected:
            MidiInputPortPlugin(MidiInputDevicePlugin* pDevice, int portNumber);
            friend class MidiInputDevicePlugin;
        };

        /**
         * Device Parameter 'PORTS'
         */
        class ParameterPortsPlugin : public ParameterPorts {
        public:
            ParameterPortsPlugin() : ParameterPorts() { }
            ParameterPortsPlugin(String s) : ParameterPorts(s) { }
            virtual bool Fix() OVERRIDE { return true; }
            void ForceSetValue(int ports);
        };

        // derived abstract methods from class 'MidiInputDevice'
        void Listen() OVERRIDE;
        void StopListen() OVERRIDE;
        String Driver() OVERRIDE;
        bool isAutonomousDevice() OVERRIDE;
        static String Name();
        static String Version();
        static String Description();
        MidiInputPort* CreateMidiPort() OVERRIDE;
        static bool isAutonomousDriver();

        /**
         * Returns the MIDI port to which events can be dispatched.
         *
         * @returns MIDI port
         */
        MidiInputPort* Port() {
            return Ports[0];
        }

        void AddMidiPort();
        void RemoveMidiPort(MidiInputPort* pPort);
    };
}

#endif
