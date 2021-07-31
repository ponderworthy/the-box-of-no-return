/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2014 Christian Schoenebeck                       *
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

#include "MidiInputDeviceAlsa.h"
#include "MidiInputDeviceFactory.h"

#define perm_ok(pinfo,bits) ((snd_seq_port_info_get_capability(pinfo) & (bits)) == (bits))

namespace LinuxSampler {

    /// number of currently existing ALSA midi input devices in LinuxSampler
    static int existingAlsaDevices = 0;

// *************** ParameterName ***************
// *

    MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterName::ParameterName(MidiInputPort* pPort) throw (Exception) : MidiInputPort::ParameterName(pPort, "Port " + ToString(pPort->GetPortNumber())) {
        OnSetValue(ValueAsString()); // initialize port name
    }

    void MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterName::OnSetValue(String s) throw (Exception) {
        if (s.size() > 16) throw Exception("Name too long for ALSA MIDI input port (max. 16 characters)");
        snd_seq_port_info_t* hInfo;
        snd_seq_port_info_malloc(&hInfo);
        snd_seq_get_port_info(((MidiInputDeviceAlsa*)pPort->GetDevice())->hAlsaSeq, pPort->GetPortNumber(), hInfo);
        snd_seq_port_info_set_name(hInfo, s.c_str());
        snd_seq_set_port_info(((MidiInputDeviceAlsa*)pPort->GetDevice())->hAlsaSeq, pPort->GetPortNumber(), hInfo);
        snd_seq_port_info_free(hInfo);
    }



// *************** ParameterAlsaSeqBindings ***************
// *


    MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqBindings::ParameterAlsaSeqBindings(MidiInputPortAlsa* pPort) : DeviceRuntimeParameterStrings( std::vector<String>() ) {
        this->pPort = pPort;
    }

    String MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqBindings::Description() {
        return "Bindings to other Alsa sequencer clients";
    }
    bool MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqBindings::Fix() {
        return false;
    }

    std::vector<String> MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqBindings::PossibilitiesAsString() {
        std::vector<String> res;
        snd_seq_client_info_t* cinfo;
        snd_seq_port_info_t* pinfo;

        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);
        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(pPort->pDevice->hAlsaSeq, cinfo) >= 0) {
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);
            while (snd_seq_query_next_port(pPort->pDevice->hAlsaSeq, pinfo) >= 0) {
                if (perm_ok(pinfo, SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) {
                        String seq_id = ToString(snd_seq_client_info_get_client(cinfo)) + ":" +
                                        ToString(snd_seq_port_info_get_port(pinfo));
                        res.push_back(seq_id);
                }
            }
        }

        return res;
    }

    void MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqBindings::OnSetValue(std::vector<String> vS) throw (Exception) {
        pPort->UnsubscribeAll();

        std::vector<String>::iterator iter = vS.begin();
        for (; iter != vS.end(); iter++) pPort->ConnectToAlsaMidiSource((*iter).c_str());
    }



// *************** ParameterAlsaSeqId ***************
// *

    MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqId::ParameterAlsaSeqId(MidiInputPortAlsa* pPort)
        : DeviceRuntimeParameterString(ToString(pPort->pDevice->hAlsaSeqClient) + ":" + ToString(pPort->portNumber)) {
    }

    String MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqId::Description() {
        return "ALSA Sequencer ID";
    }

    bool MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqId::Fix() {
        return true;
    }

    std::vector<String> MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqId::PossibilitiesAsString() {
        return std::vector<String>(); // nothing
    }

    void MidiInputDeviceAlsa::MidiInputPortAlsa::ParameterAlsaSeqId::OnSetValue(String s) {
        // not possible as parameter is 'fix'
    }



// *************** MidiInputDeviceAlsa::ParameterName ***************
// *

    MidiInputDeviceAlsa::ParameterName::ParameterName() : DeviceCreationParameterString() {
        InitWithDefault(); // use default name
    }

    MidiInputDeviceAlsa::ParameterName::ParameterName(String s) : DeviceCreationParameterString(s) {
    }

    String MidiInputDeviceAlsa::ParameterName::Description() {
        return "Arbitrary ALSA client name";
    }

    bool MidiInputDeviceAlsa::ParameterName::Fix() {
        return true;
    }

    bool MidiInputDeviceAlsa::ParameterName::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDeviceAlsa::ParameterName::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    std::vector<String> MidiInputDeviceAlsa::ParameterName::PossibilitiesAsString(std::map<String,String> Parameters) {
        return std::vector<String>();
    }

    optional<String> MidiInputDeviceAlsa::ParameterName::DefaultAsString(std::map<String,String> Parameters) {
        return (existingAlsaDevices) ? "LinuxSampler" + ToString(existingAlsaDevices) : "LinuxSampler";
    }

    void MidiInputDeviceAlsa::ParameterName::OnSetValue(String s) throw (Exception) {
        // not possible, as parameter is fix
    }

    String MidiInputDeviceAlsa::ParameterName::Name() {
        return "NAME";
    }



// *************** MidiInputPortAlsa ***************
// *

    MidiInputDeviceAlsa::MidiInputPortAlsa::MidiInputPortAlsa(MidiInputDeviceAlsa* pDevice) throw (MidiInputException) : MidiInputPort(pDevice, -1) {
        this->pDevice = pDevice;

        // create Alsa sequencer port
        int alsaPort = snd_seq_create_simple_port(pDevice->hAlsaSeq, "unnamed port",
                                                  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                  SND_SEQ_PORT_TYPE_APPLICATION);
        if (alsaPort < 0) throw MidiInputException("Error creating sequencer port");
        this->portNumber = alsaPort;

        delete Parameters["NAME"];
        Parameters["NAME"]              = new ParameterName(this);
        Parameters["ALSA_SEQ_BINDINGS"] = new ParameterAlsaSeqBindings(this);
        Parameters["ALSA_SEQ_ID"]       = new ParameterAlsaSeqId(this);
    }

    MidiInputDeviceAlsa::MidiInputPortAlsa::~MidiInputPortAlsa() {
        UnsubscribeAll();
        snd_seq_delete_simple_port(pDevice->hAlsaSeq, portNumber);
    }

    /**
     * Connects this Alsa midi input device with an Alsa MIDI source.
     *
     * @param Client - Alsa sequencer client and port ID of a MIDI source
     *                (e.g. "64:0")
     * @throws MidiInputException  if connection cannot be established
     */
    void MidiInputDeviceAlsa::MidiInputPortAlsa::ConnectToAlsaMidiSource(const char* MidiSource) {
        snd_seq_addr_t sender, dest;
        snd_seq_port_subscribe_t* subs;
        int hExtClient, hExtPort;

        sscanf(MidiSource, "%d:%d", &hExtClient, &hExtPort);
        sender.client = (char) hExtClient;
        sender.port   = (char) hExtPort;
        dest.client   = (char) pDevice->hAlsaSeqClient;
        dest.port     = (char) portNumber;
        snd_seq_port_subscribe_malloc(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, 1);
        snd_seq_port_subscribe_set_time_update(subs, 1);
        snd_seq_port_subscribe_set_time_real(subs, 1);
        if (snd_seq_subscribe_port(pDevice->hAlsaSeq, subs) < 0) {
            snd_seq_port_subscribe_free(subs);
            throw MidiInputException(String("Unable to connect to Alsa seq client \'") + MidiSource + "\' (" + snd_strerror(errno) + ")");
        }

        subscriptions.push_back(subs);
    }

    void MidiInputDeviceAlsa::MidiInputPortAlsa::UnsubscribeAll() {
        for (std::vector<snd_seq_port_subscribe_t*>::iterator it = subscriptions.begin();
             it != subscriptions.end(); it++) {
            if (snd_seq_unsubscribe_port(pDevice->hAlsaSeq, *it)) {
                dmsg(1,("MidiInputPortAlsa::UnsubscribeAll: Can't unsubscribe port connection!.\n"));
            }
            snd_seq_port_subscribe_free(*it);
        }
        subscriptions.clear();
    }

// *************** MidiInputDeviceAlsa ***************
// *

    MidiInputDeviceAlsa::MidiInputDeviceAlsa(std::map<String,DeviceCreationParameter*> Parameters, void* pSampler) : MidiInputDevice(Parameters, pSampler), Thread(true, true, 1, -1) {
        if (snd_seq_open(&hAlsaSeq, "default", SND_SEQ_OPEN_INPUT, 0) < 0) {
            throw MidiInputException("Error opening ALSA sequencer");
        }
        existingAlsaDevices++;
        this->hAlsaSeqClient = snd_seq_client_id(hAlsaSeq);
        snd_seq_set_client_name(hAlsaSeq, ((DeviceCreationParameterString*)Parameters["NAME"])->ValueAsString().c_str());
        AcquirePorts(((DeviceCreationParameterInt*)Parameters["PORTS"])->ValueAsInt());
        if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
            Listen();
        }
    }

    MidiInputDeviceAlsa::~MidiInputDeviceAlsa() {
        // free the midi ports (we can't let the base class do this,
        // as the MidiInputPortAlsa destructors need access to
        // hAlsaSeq)
        for (std::map<int,MidiInputPort*>::iterator iter = Ports.begin(); iter != Ports.end() ; iter++) {
            delete static_cast<MidiInputPortAlsa*>(iter->second);
        }
        Ports.clear();

        snd_seq_close(hAlsaSeq);
        existingAlsaDevices--; //FIXME: this is too simple, can lead to multiple clients with the same name
    }

    MidiInputDeviceAlsa::MidiInputPortAlsa* MidiInputDeviceAlsa::CreateMidiPort() {
        return new MidiInputPortAlsa(this);
    }

    String MidiInputDeviceAlsa::Name() {
	    return "ALSA";
    }

    String MidiInputDeviceAlsa::Driver() {
	    return Name();
    }

    void MidiInputDeviceAlsa::Listen() {
        StartThread();
    }

    void MidiInputDeviceAlsa::StopListen() {
        StopThread();
    }

    String MidiInputDeviceAlsa::Description() {
	    return "Advanced Linux Sound Architecture";
    }

    String MidiInputDeviceAlsa::Version() {
	    String s = "$Revision: 2559 $";
	    return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    int MidiInputDeviceAlsa::Main() {
        int npfd;
        struct pollfd* pfd;
        snd_seq_event_t* ev;

        npfd = snd_seq_poll_descriptors_count(hAlsaSeq, POLLIN);
        pfd = (struct pollfd*) alloca(npfd * sizeof(struct pollfd));
        snd_seq_poll_descriptors(hAlsaSeq, pfd, npfd, POLLIN);
        while (true) {
            if (poll(pfd, npfd, 100000) > 0) {
                do {
                    snd_seq_event_input(hAlsaSeq, &ev);
                    int port = (int) ev->dest.port;
                    MidiInputPort* pMidiInputPort = Ports[port];

                    switch (ev->type) {
                        case SND_SEQ_EVENT_CONTROLLER:
                            if (ev->data.control.param == 0)
                                pMidiInputPort->DispatchBankSelectMsb(ev->data.control.value, ev->data.control.channel);
                            else if (ev->data.control.param == 32)
                                pMidiInputPort->DispatchBankSelectLsb(ev->data.control.value, ev->data.control.channel);
                            pMidiInputPort->DispatchControlChange(ev->data.control.param, ev->data.control.value, ev->data.control.channel);
                            break;

                        case SND_SEQ_EVENT_CHANPRESS:
                            pMidiInputPort->DispatchChannelPressure(ev->data.control.value, ev->data.control.channel);
                            break;

                        case SND_SEQ_EVENT_KEYPRESS:
                            pMidiInputPort->DispatchPolyphonicKeyPressure(ev->data.note.note, ev->data.note.velocity, ev->data.control.channel);
                            break;

                        case SND_SEQ_EVENT_PITCHBEND:
                            pMidiInputPort->DispatchPitchbend(ev->data.control.value, ev->data.control.channel);
                            break;

                        case SND_SEQ_EVENT_NOTEON:
                            if (ev->data.note.velocity != 0) {
                                pMidiInputPort->DispatchNoteOn(ev->data.note.note, ev->data.note.velocity, ev->data.control.channel);
                            }
                            else {
                                pMidiInputPort->DispatchNoteOff(ev->data.note.note, 0, ev->data.control.channel);
                            }
                            break;

                        case SND_SEQ_EVENT_NOTEOFF:
                            pMidiInputPort->DispatchNoteOff(ev->data.note.note, ev->data.note.velocity, ev->data.control.channel);
                            break;

                        case SND_SEQ_EVENT_SYSEX:
                            pMidiInputPort->DispatchSysex(ev->data.ext.ptr, ev->data.ext.len);
                            break;

                        case SND_SEQ_EVENT_PGMCHANGE:
                            pMidiInputPort->DispatchProgramChange(ev->data.control.value, ev->data.control.channel);
                            break;
                    }
                    snd_seq_free_event(ev);
                } while (snd_seq_event_input_pending(hAlsaSeq, 0) > 0);
            }
        }
        // just to avoid a compiler warning
        return EXIT_FAILURE;
    }

} // namespace LinuxSampler
