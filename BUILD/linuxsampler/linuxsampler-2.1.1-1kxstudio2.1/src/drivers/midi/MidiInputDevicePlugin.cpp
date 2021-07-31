/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2012 Andreas Persson                             *
 *   Copyright (C) 2014 - 2016 Christian Schoenebeck                       *
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

#include "MidiInputDevicePlugin.h"
#include "../../common/global_private.h"

namespace LinuxSampler {

// *************** MidiInputPortPlugin ***************
// *

    MidiInputDevicePlugin::MidiInputPortPlugin::MidiInputPortPlugin(MidiInputDevicePlugin* pDevice,
                                                                    int portNumber) :
        MidiInputPort(pDevice, portNumber) {
    }



// *************** ParameterPortsPlugin  ***************
// *

    void MidiInputDevicePlugin::ParameterPortsPlugin::ForceSetValue(int ports) {
        OnSetValue(ports);
        iVal = ports;
    }


// *************** MidiInputDevicePlugin ***************
// *

    MidiInputDevicePlugin::MidiInputDevicePlugin(std::map<String, DeviceCreationParameter*> Parameters,
                                                 void* pSampler) :
        MidiInputDevice(Parameters, pSampler) {
        AcquirePorts(((DeviceCreationParameterInt*)Parameters["PORTS"])->ValueAsInt());
    }

    MidiInputDevicePlugin::~MidiInputDevicePlugin() {
        for (std::map<int, MidiInputPort*>::iterator iter =
                 Ports.begin() ; iter != Ports.end() ; iter++) {
            delete dynamic_cast<MidiInputPortPlugin*>(iter->second);
        }
        Ports.clear();
    }

    void MidiInputDevicePlugin::Listen() {
    }

    void MidiInputDevicePlugin::StopListen() {
    }

    String MidiInputDevicePlugin::Driver() {
        return Name();
    }

    String MidiInputDevicePlugin::Name() {
        return "Plugin";
    }

    String MidiInputDevicePlugin::Version() {
        String s = "$Revision: 3054 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    String MidiInputDevicePlugin::Description() {
        return Name();
    }

    MidiInputPort* MidiInputDevicePlugin::CreateMidiPort() {
        return new MidiInputPortPlugin(this, (int)Ports.size());
    }

    void MidiInputDevicePlugin::AddMidiPort() {
        static_cast<ParameterPortsPlugin*>(
            Parameters["PORTS"])->ForceSetValue((int)Ports.size() + 1);
    }

    void MidiInputDevicePlugin::RemoveMidiPort(MidiInputPort* pPort) {
        // reorder map so pPort is last
        int portNumber = 0;
        std::map<int, MidiInputPort*>::iterator i = Ports.begin();
        for ( ; i != Ports.end(); ++i, portNumber++) {
            if (i->second == pPort) break;
        }
        std::map<int, MidiInputPort*>::iterator previ = i;
        for (++i ; i != Ports.end(); ++i, portNumber++) {
            previ->second = i->second;
            static_cast<MidiInputPortPlugin*>(previ->second)->portNumber = portNumber;
            previ->second->PortParameters()["NAME"]->SetValue("Port " + ToString(portNumber));
            previ = i;
        }
        previ->second = pPort;

        // delete the last port
        static_cast<ParameterPortsPlugin*>(
            Parameters["PORTS"])->ForceSetValue((int)Ports.size() - 1);
    }

    bool MidiInputDevicePlugin::isAutonomousDevice() {
        return false;
    }

    bool MidiInputDevicePlugin::isAutonomousDriver() {
        return false;
    }
}
