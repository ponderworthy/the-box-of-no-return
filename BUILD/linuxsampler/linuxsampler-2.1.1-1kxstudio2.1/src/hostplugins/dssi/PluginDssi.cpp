/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2013 Andreas Persson                             *
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

#include <algorithm>
#include <cstring>
#include <exception>
#include <sstream>
#include <string>

#include "PluginDssi.h"

#include "../../engines/AbstractEngineChannel.h"
#include "../../engines/EngineChannelFactory.h"

namespace {

// *************** PluginDssi ***************
// *

    PluginDssi::PluginDssi(unsigned long SampleRate) :
        RefCount(0) {
        // As there is no way in DSSI of knowing a max value of the
        // output buffer size, we set the audio device buffer size to
        // 128 and let RunSynth call Render in a loop if needed.
        Init(SampleRate, 128);
    }

    PluginDssi* PluginInstance::plugin = 0;

    PluginInstance::PluginInstance(unsigned long SampleRate) {
        Out[0] = 0;
        Out[1] = 0;

        if (!plugin) {
            plugin = new PluginDssi(SampleRate);
        }
        plugin->RefCount++;

        pChannel = plugin->global->pSampler->AddSamplerChannel();
        pChannel->SetEngineType("gig");
        pChannel->SetAudioOutputDevice(plugin->pAudioDevice);

        if (plugin->RefCount > 1) {
            plugin->pMidiDevice->AddMidiPort();
            plugin->pAudioDevice->AddChannels(2);
        }

        int i = plugin->RefCount - 1;

        pChannel->SetMidiInput(plugin->pMidiDevice, i, LinuxSampler::midi_chan_all);
        LinuxSampler::AbstractEngineChannel* engineChannel =
            static_cast<LinuxSampler::AbstractEngineChannel*>(pChannel->GetEngineChannel());
        engineChannel->SetOutputChannel(0, i * 2);
        engineChannel->SetOutputChannel(1, i * 2 + 1);

        pPort = plugin->pMidiDevice->GetPort(i);
        pChannelLeft = plugin->pAudioDevice->Channel(i * 2);
        pChannelRight = plugin->pAudioDevice->Channel(i * 2 + 1);
    }

    PluginInstance::~PluginInstance() {
        if (--plugin->RefCount == 0) {
            delete plugin;
            plugin = 0;
        } else {
            LinuxSampler::AbstractEngineChannel* engineChannel =
                static_cast<LinuxSampler::AbstractEngineChannel*>(pChannel->GetEngineChannel());
            int oldChannelNumber = engineChannel->OutputChannel(0);

            plugin->global->pSampler->RemoveSamplerChannel(pChannel);
            plugin->pMidiDevice->RemoveMidiPort(pPort);
            plugin->pAudioDevice->RemoveChannel(pChannelLeft);
            plugin->pAudioDevice->RemoveChannel(pChannelRight);

            const std::set<LinuxSampler::EngineChannel*>& engineChannels =
                LinuxSampler::EngineChannelFactory::EngineChannelInstances();
            for (std::set<LinuxSampler::EngineChannel*>::iterator i = engineChannels.begin();
                 i != engineChannels.end() ; ++i) {
                if ((*i)->GetAudioOutputDevice() == plugin->pAudioDevice) {
                    int channelNumber = (*i)->OutputChannel(0);
                    if (channelNumber > oldChannelNumber) {
                        (*i)->SetOutputChannel(0, channelNumber - 2);
                        (*i)->SetOutputChannel(1, channelNumber - 1);
                    }
                }
            }
        }
    }

    void PluginInstance::ConnectPort(unsigned long Port, LADSPA_Data* DataLocation) {
        if (Port < 2) Out[Port] = DataLocation;
    }

    char* PluginInstance::Configure(const char* Key, const char* Value) {
        dmsg(2, ("linuxsampler: configure Key=%s Value=%s\n", Key, Value));

        if (strcmp(Key, "instrument") == 0 && strcmp(Value, "") != 0) {
            String filename(Value);
            String::size_type colon = filename.rfind(':');
            int instrument = 0;
            if (colon != String::npos) {
                std::stringstream(String(filename, colon + 1)) >> instrument;
                filename.erase(colon);
            }
            try {
                LinuxSampler::EngineChannel* engineChannel =
                    pChannel->GetEngineChannel();
                dmsg(2, ("  before LoadInstrument\n"));
                engineChannel->PrepareLoadInstrument(filename.c_str(),
                                                      instrument);
                engineChannel->LoadInstrument();
                dmsg(2, ("  after LoadInstrument\n"));
            }
            catch (std::exception& e) {
                return strdup(e.what());
            }
        }

        return 0;
    }

    inline void PluginInstance::RunMultipleSynths(unsigned long InstanceCount,
                                                  LADSPA_Handle* Instances,
                                                  unsigned long SampleCount,
                                                  snd_seq_event_t** Events,
                                                  unsigned long* EventCounts) {
        if (InstanceCount == 0) return;

        LinuxSampler::AudioOutputDevicePlugin* audioDevice =
            static_cast<PluginInstance*>(Instances[0])->plugin->pAudioDevice;

        unsigned eventPosArr[InstanceCount];
        for (unsigned long i = 0 ; i < InstanceCount ; i++) eventPosArr[i] = 0;

        int samplePos = 0;
        while (SampleCount) {
            int samples = std::min(SampleCount, 128UL);

            for (unsigned long i = 0 ; i < InstanceCount ; i++) {
                PluginInstance* instance = static_cast<PluginInstance*>(Instances[i]);
                LinuxSampler::MidiInputPort* port = instance->pPort;

                snd_seq_event_t* events = Events[i];
                unsigned& eventPos = eventPosArr[i];

                for ( ; eventPos < EventCounts[i] ; eventPos++) {
                    snd_seq_event_t* ev = &events[eventPos];
                    int time = ev->time.tick - samplePos;
                    if (time >= samples) break;
                    switch (ev->type) {
                    case SND_SEQ_EVENT_CONTROLLER:
                        port->DispatchControlChange(ev->data.control.param,
                                                    ev->data.control.value,
                                                    ev->data.control.channel, time);
                        break;

                    case SND_SEQ_EVENT_CHANPRESS:
                        port->DispatchControlChange(128, ev->data.control.value,
                                                    ev->data.control.channel, time);
                        break;

                    case SND_SEQ_EVENT_PITCHBEND:
                        port->DispatchPitchbend(ev->data.control.value,
                                                ev->data.control.channel, time);
                        break;

                    case SND_SEQ_EVENT_NOTEON:
                        port->DispatchNoteOn(ev->data.note.note,
                                             ev->data.note.velocity,
                                             ev->data.control.channel, time);
                        break;

                    case SND_SEQ_EVENT_NOTEOFF:
                        port->DispatchNoteOff(ev->data.note.note,
                                              ev->data.note.velocity,
                                              ev->data.control.channel, time);
                        break;

                    case SND_SEQ_EVENT_SYSEX:
                        port->DispatchSysex(ev->data.ext.ptr, ev->data.ext.len);
                        break;
                    }
                }

                instance->pChannelLeft->SetBuffer(instance->Out[0] + samplePos);
                instance->pChannelRight->SetBuffer(instance->Out[1] + samplePos);
            }

            audioDevice->Render(samples);

            samplePos += samples;
            SampleCount -= samples;
        }
    }

    void PluginInstance::Activate() {
        dmsg(2, ("linuxsampler: activate instance=%p\n", static_cast<void*>(this)));
        pPort->DispatchControlChange(123, 0, 0, 0); // all sound off
    }


// *************** LADSPA callback functions ***************
// *

    LADSPA_Handle instantiate(const LADSPA_Descriptor* Descriptor,
                              unsigned long SampleRate) {
        return new PluginInstance(SampleRate);
    }

    void cleanup(LADSPA_Handle Instance) {
        dmsg(2, ("linuxsampler: cleanup Instance=%p\n", static_cast<void*>(Instance)));
        delete static_cast<PluginInstance*>(Instance);
    }

    void activate(LADSPA_Handle Instance) {
        static_cast<PluginInstance*>(Instance)->Activate();
    }

    void connect_port(LADSPA_Handle Instance, unsigned long Port,
                      LADSPA_Data* DataLocation) {
        static_cast<PluginInstance*>(Instance)->ConnectPort(Port, DataLocation);
    }

    void run(LADSPA_Handle Instance, unsigned long SampleCount) {
        return;
    }


// *************** DSSI callback functions ***************
// *

    void run_multiple_synths(unsigned long InstanceCount,
                             LADSPA_Handle* Instances,
                             unsigned long SampleCount,
                             snd_seq_event_t** Events,
                             unsigned long* EventCounts) {
        PluginInstance::RunMultipleSynths(InstanceCount, Instances,
                                          SampleCount, Events,
                                          EventCounts);
    }

    char* configure(LADSPA_Handle Instance, const char* Key, const char* Value) {
        return static_cast<PluginInstance*>(Instance)->Configure(Key, Value);
    }


// *************** PluginInfo ***************
// *

    PluginInfo PluginInfo::Instance;

    PluginInfo::PluginInfo() {
        Ladspa.UniqueID = 3781; // (received from ladspa.org)
        Ladspa.Label = "LinuxSampler";
        Ladspa.Properties = 0;
        Ladspa.Name = "LinuxSampler";
        Ladspa.Maker = "linuxsampler.org";
        Ladspa.Copyright = "(C) 2003,2004 Benno Senoner and Christian Schoenebeck, "
            "2005-2013 Christian Schoenebeck";
        Ladspa.PortCount = 2;
        Ladspa.ImplementationData = 0;
        Ladspa.PortDescriptors = PortDescriptors;
        Ladspa.PortRangeHints = PortRangeHints;
        Ladspa.PortNames = PortNames;

        PortDescriptors[0] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        PortNames[0] = "Output Left";
        PortRangeHints[0].HintDescriptor = 0;
        PortDescriptors[1] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        PortNames[1] = "Output Right";
        PortRangeHints[1].HintDescriptor = 0;

        Ladspa.activate = activate;
        Ladspa.cleanup = cleanup;
        Ladspa.connect_port = connect_port;
        Ladspa.deactivate = 0;
        Ladspa.instantiate = instantiate;
        Ladspa.run = run;
        Ladspa.run_adding = 0;
        Ladspa.set_run_adding_gain = 0;

        Dssi.DSSI_API_Version = 1;
        Dssi.LADSPA_Plugin = &Ladspa;
        Dssi.configure = configure;
        Dssi.get_program = 0;
        Dssi.get_midi_controller_for_port = 0;
        Dssi.select_program = 0;
        Dssi.run_synth = 0;
        Dssi.run_synth_adding = 0;
        Dssi.run_multiple_synths = run_multiple_synths;
        Dssi.run_multiple_synths_adding = 0;
    }
}


extern "C" {
    const LADSPA_Descriptor* ladspa_descriptor(unsigned long Index) {
        dmsg(2, ("linuxsampler: ladspa_descriptor\n"));
        return Index == 0 ? PluginInfo::LadspaDescriptor() : 0;
    }

    const DSSI_Descriptor *dssi_descriptor(unsigned long Index) {
        dmsg(2, ("linuxsampler: dssi_descriptor\n"));
        return Index == 0 ? PluginInfo::DssiDescriptor() : 0;
    }
}
