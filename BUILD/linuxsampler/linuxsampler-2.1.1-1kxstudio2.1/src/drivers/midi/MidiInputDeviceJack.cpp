/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2010 Andreas Persson                             *
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
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301 USA.                                                    *
 ***************************************************************************/

#include "MidiInputDeviceJack.h"
#include "../../common/global_private.h"
#include "../audio/AudioOutputDeviceJack.h"
#include <errno.h>

#ifndef HAVE_JACK_MIDI_GET_EVENT_COUNT
#define jack_midi_get_event_count(port_buf, nframes) jack_midi_port_get_info(port_buf, nframes)->event_count
#endif

namespace LinuxSampler {

    /// number of currently existing JACK midi input devices in LinuxSampler
    static int existingJackDevices = 0;

// *************** MidiInputPortJack::ParameterName ***************
// *

    MidiInputDeviceJack::MidiInputPortJack::ParameterName::ParameterName(MidiInputPortJack* pPort) throw (Exception) : MidiInputPort::ParameterName(pPort, "midi_in_" + ToString(pPort->GetPortNumber())) {
        this->pPort = pPort;
    }

    void MidiInputDeviceJack::MidiInputPortJack::ParameterName::OnSetValue(String s) throw (Exception) {
        if (jack_port_set_name(pPort->hJackPort, s.c_str())) throw Exception("Failed to rename JACK port");
    }


// *************** MidiInputPortJack::ParameterJackBindings ***************
// *

    MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::ParameterJackBindings(MidiInputPortJack* pPort) : DeviceRuntimeParameterStrings(std::vector<String>()) {
        this->pPort = pPort;
    }

    String MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::Description() {
        return "Bindings to other JACK clients";
    }

    bool MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::Fix() {
        return false;
    }

    std::vector<String> MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::PossibilitiesAsString() {
        const char** pPortNames = jack_get_ports(pPort->pDevice->hJackClient, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
        if (!pPortNames) return std::vector<String>();
        std::vector<String> result;
        for (int i = 0; pPortNames[i]; i++) result.push_back(pPortNames[i]);
        free(pPortNames);
        return result;
    }

    void MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::OnSetValue(std::vector<String> vS) {
        String dst_name = ((DeviceCreationParameterString*)pPort->pDevice->Parameters["NAME"])->ValueAsString() + ":" +
                          ((DeviceRuntimeParameterString*)pPort->Parameters["NAME"])->ValueAsString();
        // disconnect all current bindings first
        for (int i = 0; i < Bindings.size(); i++) {
            String src_name = Bindings[i];
            /*int res =*/ jack_disconnect(pPort->pDevice->hJackClient, src_name.c_str(), dst_name.c_str());
        }
        // connect new bindings
        for (int i = 0; i < vS.size(); i++) {
            String src_name = vS[i];
            int res = jack_connect(pPort->pDevice->hJackClient, src_name.c_str(), dst_name.c_str());
            if (res == EEXIST) throw MidiInputException("Jack: Connection to port '" + dst_name + "' already established");
            else if (res)      throw MidiInputException("Jack: Cannot connect port '" + src_name + "' to port '" + dst_name + "'");
        }
        // remember bindings
        Bindings = vS;
    }

    String MidiInputDeviceJack::MidiInputPortJack::ParameterJackBindings::Name() {
        return "JACK_BINDINGS";
    }



// *************** MidiInputPortJack ***************
// *

    MidiInputDeviceJack::MidiInputPortJack::MidiInputPortJack(MidiInputDeviceJack* pDevice) throw (MidiInputException) : MidiInputPort(pDevice, pDevice->Ports.size()) {
        this->pDevice = pDevice;

        String port_id = "midi_in_" + ToString(portNumber);
        hJackPort = jack_port_register(pDevice->hJackClient, port_id.c_str(), JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!hJackPort) throw MidiInputException("Jack: Cannot register Jack MIDI input port.");

        delete Parameters["NAME"];
        Parameters["NAME"]          = new ParameterName(this);
        Parameters["JACK_BINDINGS"] = new ParameterJackBindings(this);
    }

    MidiInputDeviceJack::MidiInputPortJack::~MidiInputPortJack() {
        jack_port_unregister(pDevice->hJackClient, hJackPort);
    }


// *************** MidiInputDeviceJack::ParameterName ***************
// *

    MidiInputDeviceJack::ParameterName::ParameterName() : DeviceCreationParameterString() {
        InitWithDefault(); // use default name
    }

    MidiInputDeviceJack::ParameterName::ParameterName(String s) : DeviceCreationParameterString(s) {
    }

    String MidiInputDeviceJack::ParameterName::Description() {
        return "Arbitrary JACK client name";
    }

    bool MidiInputDeviceJack::ParameterName::Fix() {
        return true;
    }

    bool MidiInputDeviceJack::ParameterName::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDeviceJack::ParameterName::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    std::vector<String> MidiInputDeviceJack::ParameterName::PossibilitiesAsString(std::map<String,String> Parameters) {
        return std::vector<String>();
    }

    optional<String> MidiInputDeviceJack::ParameterName::DefaultAsString(std::map<String,String> Parameters) {
        return (existingJackDevices) ? "LinuxSampler" + ToString(existingJackDevices) : "LinuxSampler";
    }

    void MidiInputDeviceJack::ParameterName::OnSetValue(String s) throw (Exception) {
        // not possible, as parameter is fix
    }

    String MidiInputDeviceJack::ParameterName::Name() {
        return "NAME";
    }



// *************** MidiInputDeviceJack ***************
// *

    MidiInputDeviceJack::MidiInputDeviceJack(std::map<String,DeviceCreationParameter*> Parameters, void* pSampler) : MidiInputDevice(Parameters, pSampler) {
        pJackClient = JackClient::CreateMidi(((DeviceCreationParameterString*)Parameters["NAME"])->ValueAsString());
        hJackClient = pJackClient->hJackClient;
        existingJackDevices++;

        AcquirePorts(((DeviceCreationParameterInt*)Parameters["PORTS"])->ValueAsInt());
        if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
            Listen();
        }
    }

    MidiInputDeviceJack::~MidiInputDeviceJack() {
        StopListen();
        // free the midi ports (we can't let the base class do this,
        // as the MidiInputPortJack destructors need access to
        // hJackClient)
        for (std::map<int,MidiInputPort*>::iterator iter = Ports.begin(); iter != Ports.end() ; iter++) {
            delete static_cast<MidiInputPortJack*>(iter->second);
        }
        Ports.clear();

        JackClient::ReleaseMidi(((DeviceCreationParameterString*)Parameters["NAME"])->ValueAsString());
        existingJackDevices--; //FIXME: this is too simple, can lead to multiple clients with the same name
    }

    MidiInputDeviceJack::MidiInputPortJack* MidiInputDeviceJack::CreateMidiPort() {
        return new MidiInputPortJack(this);
    }

    String MidiInputDeviceJack::Name() {
        return "JACK";
    }

    String MidiInputDeviceJack::Driver() {
        return Name();
    }

    void MidiInputDeviceJack::Listen() {
        pJackClient->SetMidiInputDevice(this);
    }

    void MidiInputDeviceJack::StopListen() {
        pJackClient->SetMidiInputDevice(0);
    }

    String MidiInputDeviceJack::Description() {
        return "JACK Audio Connection Kit";
    }

    String MidiInputDeviceJack::Version() {
        String s = "$Revision: 2494 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    void MidiInputDeviceJack::Process(int nsamples) {
        int nbPorts = Ports.size();
        for (int i = 0 ; i < nbPorts ; i++) {
            MidiInputPortJack* port = static_cast<MidiInputPortJack*>(Ports[i]);
            void* port_buffer = jack_port_get_buffer(port->hJackPort, nsamples);

#if JACK_MIDI_FUNCS_NEED_NFRAMES
            int event_count = jack_midi_get_event_count(port_buffer, nsamples); // old jack version
#else
            int event_count = jack_midi_get_event_count(port_buffer);
#endif
            for (int i = 0 ; i < event_count ; i++) {
                jack_midi_event_t ev;
#if JACK_MIDI_FUNCS_NEED_NFRAMES
                jack_midi_event_get(&ev, port_buffer, i, nsamples); // old jack version
#else
                jack_midi_event_get(&ev, port_buffer, i);
#endif
                if (ev.buffer) { // jack2 1.9.5 has been seen sending NULL here
                    port->DispatchRaw(ev.buffer, ev.time);
                }
            }
        }
    }
}
