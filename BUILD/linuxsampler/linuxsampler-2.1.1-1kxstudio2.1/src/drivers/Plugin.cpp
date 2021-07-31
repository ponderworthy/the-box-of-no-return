/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2011 Andreas Persson                             *
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

#include <limits>
#include <sstream>

#include "Plugin.h"
#include "audio/AudioOutputDeviceFactory.h"
#include "midi/MidiInputDeviceFactory.h"

namespace LinuxSampler {

// *************** PluginGlobal ***************
// *

    PluginGlobal::PluginGlobal() :
        RefCount(0) {
        // we need to remove the ASIO driver, otherwise the lscp info
        // methods will lock up the audio device
        AudioOutputDeviceFactory::Unregister("ASIO");

        REGISTER_AUDIO_OUTPUT_DRIVER(AudioOutputDevicePlugin);
        REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePlugin, ParameterActive);
        REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePlugin, ParameterSampleRate);
        REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePlugin, ParameterChannelsPlugin);
        REGISTER_AUDIO_OUTPUT_DRIVER_PARAMETER(AudioOutputDevicePlugin, ParameterFragmentSize);

        REGISTER_MIDI_INPUT_DRIVER(MidiInputDevicePlugin);
        REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDevicePlugin, ParameterActive);
        REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDevicePlugin, ParameterPortsPlugin);

        pSampler = new Sampler;

        #if defined(__APPLE__)
        // AU plugin sometimes hangs if bound to loopback
        pLSCPServer = new LSCPServer(pSampler, htonl(INADDR_ANY), htons(LSCP_PORT));
        #else
        // using LOOPBACK instead of ANY to prevent windows firewall
        // warnings
        pLSCPServer = new LSCPServer(pSampler, htonl(INADDR_LOOPBACK), htons(LSCP_PORT));
        #endif

        pLSCPServer->StartThread();
        pLSCPServer->WaitUntilInitialized();

        pEventThread = new EventThread(pSampler);
        pEventThread->StartThread();
    }


    PluginGlobal::~PluginGlobal() {
        pEventThread->StopThread();
        pLSCPServer->StopThread();
        pLSCPServer->RemoveListeners();

        delete pEventThread;
        delete pSampler;
        delete pLSCPServer;
    }


// *************** EventThread ***************
// *


    EventThread::EventThread(Sampler* pSampler) :
        Thread(false, false, 0, 0),
        pSampler(pSampler) {
    }

    int EventThread::Main() {
        for (;;) {
            TestCancel();
            sleep(1);
            pSampler->fireStatistics();
        }
        return 0;
    }


// *************** Plugin ***************
// *

    PluginGlobal* Plugin::global = 0;

    Plugin::Plugin(bool bDoPreInit) :
        pAudioDevice(0),
        pMidiDevice(0) {
        bPreInitDone = false;
        if (bDoPreInit) PreInit();
    }

    void Plugin::PreInit() {
        if (bPreInitDone) return;

        bPreInitDone = true;
        if (!global) {
            global = new PluginGlobal;
        }
        global->RefCount++;
    }

    void Plugin::Init(int SampleRate, int FragmentSize, int Channels) {
        if (pAudioDevice && SampleRate == pAudioDevice->SampleRate() &&
            FragmentSize == pAudioDevice->MaxSamplesPerCycle()) {
            return; // nothing has changed
        }

        String oldState;
        if (pAudioDevice) {
            oldState = GetState();
            RemoveChannels();
            AudioOutputDeviceFactory::DestroyPrivate(pAudioDevice);
        }
        std::map<String, String> params;
        params["SAMPLERATE"] = ToString(SampleRate);
        params["FRAGMENTSIZE"] = ToString(FragmentSize);
        if (Channels > 0) params["CHANNELS"] = ToString(Channels);
        pAudioDevice = dynamic_cast<AudioOutputDevicePlugin*>(
            AudioOutputDeviceFactory::CreatePrivate(
                AudioOutputDevicePlugin::Name(), params
            )
        );

        if (!pMidiDevice) {
            pMidiDevice = dynamic_cast<MidiInputDevicePlugin*>(
                MidiInputDeviceFactory::CreatePrivate(
                    MidiInputDevicePlugin::Name(), std::map<String,String>(),
                    global->pSampler
                )
            );
        }

        if (!oldState.empty()) {
            SetState(oldState);
        }
    }

    Plugin::~Plugin() {
        RemoveChannels();
        if (pAudioDevice) AudioOutputDeviceFactory::DestroyPrivate(pAudioDevice);
        if (pMidiDevice) MidiInputDeviceFactory::DestroyPrivate(pMidiDevice);
        if (bPreInitDone) {
            if (--global->RefCount == 0) {
                delete global;
                global = 0;
            }
        }
    }

    void Plugin::InitState() {
        SamplerChannel* channel = global->pSampler->AddSamplerChannel();
        channel->SetEngineType("gig");
        channel->SetAudioOutputDevice(pAudioDevice);
        channel->SetMidiInputDevice(pMidiDevice);
        channel->SetMidiInputChannel(midi_chan_1);
    }

    /*
      These methods can be overloaded by different plugin types to map
      file names to/from file names to be used in the state text, making
      it possible for state to be self-contained and/or movable.
    */

    String Plugin::PathToState(const String& string) {
        return string;
    }

    String Plugin::PathFromState(const String& string) {
        return string;
    }

    /*
      The sampler state is stored in a text base format, designed to
      be easy to parse with the istream >> operator. Values are
      separated by spaces or newlines. All string values that may
      contain spaces end with a newline.

      The first line contains the global volume.

      The rest of the lines have an integer first representing the
      type of information on the line, except for the two lines that
      describe each sampler channel. The first of these two starts
      with an integer from 0 to 16 (the midi channel for the sampler
      channel).

      Note that we should try to keep the parsing of this format both
      backwards and forwards compatible between versions. The parser
      ignores lines with unknown type integers and accepts that new
      types are missing.
    */

    enum {
        FXSEND = 17,
        MIDIMAP,
        MIDIMAPPING,
        DEFAULTMIDIMAP
    };

    String Plugin::GetState() {
        std::stringstream s;

        s << GLOBAL_VOLUME << '\n';

        std::vector<int> maps = MidiInstrumentMapper::Maps();
        for (int i = 0 ; i < maps.size() ; i++) {
            s << MIDIMAP << ' ' <<
                maps[i] << ' ' <<
                MidiInstrumentMapper::MapName(maps[i]) << '\n';

            std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t> mappings = MidiInstrumentMapper::Entries(maps[i]);
            for (std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t>::iterator iter = mappings.begin() ;
                 iter != mappings.end(); iter++) {
                s << MIDIMAPPING << ' ' <<
                    ((int(iter->first.midi_bank_msb) << 7) |
                     int(iter->first.midi_bank_lsb)) << ' ' <<
                    int(iter->first.midi_prog) << ' ' <<
                    iter->second.EngineName << ' ' <<
                    PathToState(iter->second.InstrumentFile) << '\n' <<
                    MIDIMAPPING << ' ' <<
                    iter->second.InstrumentIndex << ' ' <<
                    iter->second.Volume << ' ' <<
                    iter->second.LoadMode << ' ' <<
                    iter->second.Name << '\n';
            }
        }
        if (maps.size()) {
            s << DEFAULTMIDIMAP << ' ' <<
                MidiInstrumentMapper::GetDefaultMap() << '\n';
        }

        std::map<uint, SamplerChannel*> channels = global->pSampler->GetSamplerChannels();
        for (std::map<uint, SamplerChannel*>::iterator iter = channels.begin() ;
             iter != channels.end() ; iter++) {
            SamplerChannel* channel = iter->second;
            if (channel->GetAudioOutputDevice() == pAudioDevice) {
                EngineChannel* engine_channel = channel->GetEngineChannel();
                String filename = engine_channel->InstrumentFileName();
                s << channel->GetMidiInputChannel() << ' ' <<
                    engine_channel->Volume() << ' ' <<
                    PathToState(filename) << '\n' <<
                    engine_channel->InstrumentIndex() << ' ' <<
                    engine_channel->GetSolo() << ' ' <<
                    (engine_channel->GetMute() == 1) << ' ' <<
                    engine_channel->OutputChannel(0) << ' ' <<
                    engine_channel->OutputChannel(1) << ' ' <<
                    (engine_channel->UsesNoMidiInstrumentMap() ? -2 :
                     (engine_channel->UsesDefaultMidiInstrumentMap() ? -1 :
                      engine_channel->GetMidiInstrumentMap())) << ' ' <<
                    engine_channel->EngineName() << '\n';

                for (int i = 0 ; i < engine_channel->GetFxSendCount() ; i++) {
                    FxSend* fxsend = engine_channel->GetFxSend(i);
                    s << FXSEND << ' ' <<
                        fxsend->Level() << ' ' <<
                        int(fxsend->MidiController()) << ' ' <<
                        fxsend->DestinationChannel(0) << ' ' <<
                        fxsend->DestinationChannel(1) << ' ' <<
                        fxsend->Name() << '\n';
                }
            }
        }
        return s.str();
    }

    void Plugin::RemoveChannels() {
        if(global == NULL) return;

        std::map<uint, SamplerChannel*> channels = global->pSampler->GetSamplerChannels();

        for (std::map<uint, SamplerChannel*>::iterator iter = channels.begin() ;
             iter != channels.end() ; iter++) {
            if (iter->second->GetAudioOutputDevice() == pAudioDevice) {
                global->pSampler->RemoveSamplerChannel(iter->second);
            }
        }
    }

    bool Plugin::SetState(String State) {
        RemoveChannels();
        MidiInstrumentMapper::RemoveAllMaps();

        std::stringstream s(State);
        s >> GLOBAL_VOLUME;

        EngineChannel* engine_channel;
        int midiMapId;
        std::map<int, int> oldToNewId;
        int type;
        while (s >> type) {

            if (type <= 16) { // sampler channel
                int midiChannel = type;
                float volume;
                s >> volume;
                s.ignore();
                String filename;
                std::getline(s, filename);
                int index;
                bool solo;
                bool mute;
                s >> index >> solo >> mute;

                int left = -1;
                int right;
                int oldMapId;
                String engineType = "gig";
                if (s.get() == ' ') {
                    s >> left >> right >> oldMapId;
                    if (s.get() == ' ') {
                        s >> engineType;
                        // skip rest of line
                        s.ignore(std::numeric_limits<int>::max(), '\n');
                    }
                }
                SamplerChannel* channel = global->pSampler->AddSamplerChannel();
                channel->SetEngineType(engineType);
                channel->SetAudioOutputDevice(pAudioDevice);
                channel->SetMidiInputDevice(pMidiDevice);
                channel->SetMidiInputChannel(midi_chan_t(midiChannel));

                engine_channel = channel->GetEngineChannel();
                engine_channel->Volume(volume);

                if (left != -1) {
                    engine_channel->SetOutputChannel(0, left);
                    engine_channel->SetOutputChannel(1, right);

                    if (oldMapId == -1) {
                        engine_channel->SetMidiInstrumentMapToDefault();
                    } else if (oldMapId >= 0) {
                        engine_channel->SetMidiInstrumentMap(oldToNewId[oldMapId]);
                    }
                }
                if (!filename.empty() && index != -1) {
                    InstrumentManager::instrument_id_t id;
                    id.FileName = PathFromState(filename);
                    id.Index    = index;
                    InstrumentManager::LoadInstrumentInBackground(id, engine_channel);
                }
                if (solo) engine_channel->SetSolo(solo);
                if (mute) engine_channel->SetMute(1);

            } else if (type == FXSEND) {
                float level;
                int controller;
                int fxleft;
                int fxright;
                String name;

                s >> level >> controller >> fxleft >> fxright;
                s.ignore();
                std::getline(s, name);
                FxSend* send = engine_channel->AddFxSend(controller, name);
                send->SetLevel(level);
                send->SetDestinationChannel(0, fxleft);
                send->SetDestinationChannel(1, fxright);

            } else if (type == MIDIMAP) {
                int oldId;
                s >> oldId;
                String name;
                s.ignore();
                std::getline(s, name);
                midiMapId = MidiInstrumentMapper::AddMap(name);
                oldToNewId[oldId] = midiMapId;

            } else if (type == MIDIMAPPING) {
                int bank;
                int prog;
                String engine;
                String file;
                int index;
                float volume;
                int loadmode;
                String name;

                s >> bank >> prog >> engine;
                s.ignore();
                std::getline(s, file);
                s >> type >> index >> volume >> loadmode;
                s.ignore();
                std::getline(s, name);

                global->pLSCPServer->AddOrReplaceMIDIInstrumentMapping(
                    midiMapId, bank, prog, engine, file, index, volume,
                    MidiInstrumentMapper::mode_t(loadmode), name, false);

            } else if (type == DEFAULTMIDIMAP) {
                int oldId;
                s >> oldId;
                MidiInstrumentMapper::SetDefaultMap(oldToNewId[oldId]);

            } else { // unknown type
                // try to be forward-compatible and just skip the line
                s.ignore(std::numeric_limits<int>::max(), '\n');
            }
        }

        return true;
    }

    void Plugin::DestroyDevice(AudioOutputDevicePlugin* pDevice) {
        AudioOutputDeviceFactory::DestroyPrivate(pDevice);
    }

    void Plugin::DestroyDevice(MidiInputDevicePlugin* pDevice) {
        MidiInputDeviceFactory::DestroyPrivate(pDevice);
    }
}
