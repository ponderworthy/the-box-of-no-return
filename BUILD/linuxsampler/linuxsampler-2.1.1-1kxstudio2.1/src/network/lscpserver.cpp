/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include <algorithm>
#include <string>

#include "../common/File.h"
#include "lscpserver.h"
#include "lscpresultset.h"
#include "lscpevent.h"

#if defined(WIN32)
#include <windows.h>
typedef unsigned short in_port_t;
typedef unsigned long in_addr_t;
#else
#include <fcntl.h>
#endif

#if ! HAVE_SQLITE3
#define DOESNT_HAVE_SQLITE3 "No database support. SQLITE3 was not installed when linuxsampler was built."
#endif

#include "../engines/EngineFactory.h"
#include "../engines/EngineChannelFactory.h"
#include "../drivers/audio/AudioOutputDeviceFactory.h"
#include "../drivers/midi/MidiInputDeviceFactory.h"
#include "../effects/EffectFactory.h"

namespace LinuxSampler {

String lscpParserProcessShellInteraction(String& line, yyparse_param_t* param, bool possibilities);

/**
 * Returns a copy of the given string where all special characters are
 * replaced by LSCP escape sequences ("\xHH"). This function shall be used
 * to escape LSCP response fields in case the respective response field is
 * actually defined as using escape sequences in the LSCP specs.
 *
 * @e Caution: DO NOT use this function for escaping path based responses,
 * use the Path class (src/common/Path.h) for this instead!
 */
static String _escapeLscpResponse(String txt) {
    for (int i = 0; i < txt.length(); i++) {
        const char c = txt.c_str()[i];
        if (
            !(c >= '0' && c <= '9') &&
            !(c >= 'a' && c <= 'z') &&
            !(c >= 'A' && c <= 'Z') &&
            !(c == ' ') && !(c == '!') && !(c == '#') && !(c == '$') &&
            !(c == '%') && !(c == '&') && !(c == '(') && !(c == ')') &&
            !(c == '*') && !(c == '+') && !(c == ',') && !(c == '-') &&
            !(c == '.') && !(c == '/') && !(c == ':') && !(c == ';') &&
            !(c == '<') && !(c == '=') && !(c == '>') && !(c == '?') &&
            !(c == '@') && !(c == '[') && !(c == ']') &&
            !(c == '^') && !(c == '_') && !(c == '`') && !(c == '{') &&
            !(c == '|') && !(c == '}') && !(c == '~')
        ) {
            // convert the "special" character into a "\xHH" LSCP escape sequence
            char buf[5];
            snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned char>(c));
            txt.replace(i, 1, buf);
            i += 3;
        }
    }
    return txt;
}

/**
 * Below are a few static members of the LSCPServer class.
 * The big assumption here is that LSCPServer is going to remain a singleton.
 * These members are used to support client connections.
 * Class handles multiple connections at the same time using select() and non-blocking recv()
 * Commands are processed by a single LSCPServer thread.
 * Notifications are delivered either by the thread that originated them
 * or (if the resultset is currently in progress) by the LSCPServer thread
 * after the resultset was sent out.
 * This makes sure that resultsets can not be interrupted by notifications.
 * This also makes sure that the thread sending notification is not blocked
 * by the LSCPServer thread.
 */
fd_set LSCPServer::fdSet;
int LSCPServer::currentSocket = -1;
std::vector<yyparse_param_t> LSCPServer::Sessions;
std::vector<yyparse_param_t>::iterator itCurrentSession;
std::map<int,String> LSCPServer::bufferedNotifies;
std::map<int,String> LSCPServer::bufferedCommands;
std::map< LSCPEvent::event_t, std::list<int> > LSCPServer::eventSubscriptions;
Mutex LSCPServer::NotifyMutex;
Mutex LSCPServer::NotifyBufferMutex;
Mutex LSCPServer::SubscriptionMutex;
Mutex LSCPServer::RTNotifyMutex;

LSCPServer::LSCPServer(Sampler* pSampler, long int addr, short int port) : Thread(true, false, 0, -4), eventHandler(this) {
    SocketAddress.sin_family      = AF_INET;
    SocketAddress.sin_addr.s_addr = (in_addr_t)addr;
    SocketAddress.sin_port        = (in_port_t)port;
    this->pSampler = pSampler;
    LSCPEvent::RegisterEvent(LSCPEvent::event_audio_device_count, "AUDIO_OUTPUT_DEVICE_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_audio_device_info, "AUDIO_OUTPUT_DEVICE_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_device_count, "MIDI_INPUT_DEVICE_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_device_info, "MIDI_INPUT_DEVICE_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_channel_count, "CHANNEL_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_voice_count, "VOICE_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_stream_count, "STREAM_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_buffer_fill, "BUFFER_FILL");
    LSCPEvent::RegisterEvent(LSCPEvent::event_channel_info, "CHANNEL_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_fx_send_count, "FX_SEND_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_fx_send_info, "FX_SEND_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_instr_map_count, "MIDI_INSTRUMENT_MAP_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_instr_map_info, "MIDI_INSTRUMENT_MAP_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_instr_count, "MIDI_INSTRUMENT_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_midi_instr_info, "MIDI_INSTRUMENT_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_db_instr_dir_count, "DB_INSTRUMENT_DIRECTORY_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_db_instr_dir_info, "DB_INSTRUMENT_DIRECTORY_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_db_instr_count, "DB_INSTRUMENT_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_db_instr_info, "DB_INSTRUMENT_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_db_instrs_job_info, "DB_INSTRUMENTS_JOB_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_misc, "MISCELLANEOUS");
    LSCPEvent::RegisterEvent(LSCPEvent::event_total_stream_count, "TOTAL_STREAM_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_total_voice_count, "TOTAL_VOICE_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_global_info, "GLOBAL_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_channel_midi, "CHANNEL_MIDI");
    LSCPEvent::RegisterEvent(LSCPEvent::event_device_midi, "DEVICE_MIDI");
    LSCPEvent::RegisterEvent(LSCPEvent::event_fx_instance_count, "EFFECT_INSTANCE_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_fx_instance_info, "EFFECT_INSTANCE_INFO");
    LSCPEvent::RegisterEvent(LSCPEvent::event_send_fx_chain_count, "SEND_EFFECT_CHAIN_COUNT");
    LSCPEvent::RegisterEvent(LSCPEvent::event_send_fx_chain_info, "SEND_EFFECT_CHAIN_INFO");
    hSocket = -1;
}

LSCPServer::~LSCPServer() {
    CloseAllConnections();
    InstrumentManager::StopBackgroundThread();
#if defined(WIN32)
    if (hSocket >= 0) closesocket(hSocket);
#else
    if (hSocket >= 0) close(hSocket);
#endif
}

LSCPServer::EventHandler::EventHandler(LSCPServer* pParent) {
    this->pParent = pParent;
}

LSCPServer::EventHandler::~EventHandler() {
    std::vector<midi_listener_entry> l = channelMidiListeners;
    channelMidiListeners.clear();
    for (int i = 0; i < l.size(); i++)
        delete l[i].pMidiListener;
}

void LSCPServer::EventHandler::ChannelCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_channel_count, NewCount));
}

void LSCPServer::EventHandler::ChannelAdded(SamplerChannel* pChannel) {
    pChannel->AddEngineChangeListener(this);
}

void LSCPServer::EventHandler::ChannelToBeRemoved(SamplerChannel* pChannel) {
    if (!pChannel->GetEngineChannel()) return;
    EngineToBeChanged(pChannel->Index());
}

void LSCPServer::EventHandler::EngineToBeChanged(int ChannelId) {
    SamplerChannel* pSamplerChannel =
        pParent->pSampler->GetSamplerChannel(ChannelId);
    if (!pSamplerChannel) return;
    EngineChannel* pEngineChannel =
        pSamplerChannel->GetEngineChannel();
    if (!pEngineChannel) return;
    for (std::vector<midi_listener_entry>::iterator iter = channelMidiListeners.begin(); iter != channelMidiListeners.end(); ++iter) {
        if ((*iter).pEngineChannel == pEngineChannel) {
            VirtualMidiDevice* pMidiListener = (*iter).pMidiListener;
            pEngineChannel->Disconnect(pMidiListener);
            channelMidiListeners.erase(iter);
            delete pMidiListener;
            return;
        }
    }
}

void LSCPServer::EventHandler::EngineChanged(int ChannelId) {
    SamplerChannel* pSamplerChannel =
        pParent->pSampler->GetSamplerChannel(ChannelId);
    if (!pSamplerChannel) return;
    EngineChannel* pEngineChannel =
        pSamplerChannel->GetEngineChannel();
    if (!pEngineChannel) return;
    VirtualMidiDevice* pMidiListener = new VirtualMidiDevice;
    pEngineChannel->Connect(pMidiListener);
    midi_listener_entry entry = {
        pSamplerChannel, pEngineChannel, pMidiListener
    };
    channelMidiListeners.push_back(entry);
}

void LSCPServer::EventHandler::AudioDeviceCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_audio_device_count, NewCount));
}

void LSCPServer::EventHandler::MidiDeviceCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_device_count, NewCount));
}

void LSCPServer::EventHandler::MidiDeviceToBeDestroyed(MidiInputDevice* pDevice) {
    pDevice->RemoveMidiPortCountListener(this);
    for (int i = 0; i < pDevice->PortCount(); ++i)
        MidiPortToBeRemoved(pDevice->GetPort(i));
}

void LSCPServer::EventHandler::MidiDeviceCreated(MidiInputDevice* pDevice) {
    pDevice->AddMidiPortCountListener(this);
    for (int i = 0; i < pDevice->PortCount(); ++i)
        MidiPortAdded(pDevice->GetPort(i));
}

void LSCPServer::EventHandler::MidiPortCountChanged(int NewCount) {
    // yet unused
}

void LSCPServer::EventHandler::MidiPortToBeRemoved(MidiInputPort* pPort) {
    for (std::vector<device_midi_listener_entry>::iterator iter = deviceMidiListeners.begin(); iter != deviceMidiListeners.end(); ++iter) {
        if ((*iter).pPort == pPort) {
            VirtualMidiDevice* pMidiListener = (*iter).pMidiListener;
            pPort->Disconnect(pMidiListener);
            deviceMidiListeners.erase(iter);
            delete pMidiListener;
            return;
        }
    }
}

void LSCPServer::EventHandler::MidiPortAdded(MidiInputPort* pPort) {
    // find out the device ID
    std::map<uint, MidiInputDevice*> devices =
        pParent->pSampler->GetMidiInputDevices();
    for (
        std::map<uint, MidiInputDevice*>::iterator iter = devices.begin();
        iter != devices.end(); ++iter
    ) {
        if (iter->second == pPort->GetDevice()) { // found
            VirtualMidiDevice* pMidiListener = new VirtualMidiDevice;
            pPort->Connect(pMidiListener);
            device_midi_listener_entry entry = {
                pPort, pMidiListener, iter->first
            };
            deviceMidiListeners.push_back(entry);
            return;
        }
    }
}

void LSCPServer::EventHandler::MidiInstrumentCountChanged(int MapId, int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_instr_count, MapId, NewCount));
}

void LSCPServer::EventHandler::MidiInstrumentInfoChanged(int MapId, int Bank, int Program) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_instr_info, MapId, Bank, Program));
}

void LSCPServer::EventHandler::MidiInstrumentMapCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_instr_map_count, NewCount));
}

void LSCPServer::EventHandler::MidiInstrumentMapInfoChanged(int MapId) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_instr_map_info, MapId));
}

void LSCPServer::EventHandler::FxSendCountChanged(int ChannelId, int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_count, ChannelId, NewCount));
}

void LSCPServer::EventHandler::VoiceCountChanged(int ChannelId, int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_voice_count, ChannelId, NewCount));
}

void LSCPServer::EventHandler::StreamCountChanged(int ChannelId, int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_stream_count, ChannelId, NewCount));
}

void LSCPServer::EventHandler::BufferFillChanged(int ChannelId, String FillData) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_buffer_fill, ChannelId, FillData));
}

void LSCPServer::EventHandler::TotalVoiceCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_total_voice_count, NewCount));
}

void LSCPServer::EventHandler::TotalStreamCountChanged(int NewCount) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_total_stream_count, NewCount));
}

#if HAVE_SQLITE3
void LSCPServer::DbInstrumentsEventHandler::DirectoryCountChanged(String Dir) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_dir_count, InstrumentsDb::toEscapedPath(Dir)));
}

void LSCPServer::DbInstrumentsEventHandler::DirectoryInfoChanged(String Dir) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_dir_info, InstrumentsDb::toEscapedPath(Dir)));
}

void LSCPServer::DbInstrumentsEventHandler::DirectoryNameChanged(String Dir, String NewName) {
    Dir = "'" + InstrumentsDb::toEscapedPath(Dir) + "'";
    NewName = "'" + InstrumentsDb::toEscapedPath(NewName) + "'";
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_dir_info, "NAME", Dir, NewName));
}

void LSCPServer::DbInstrumentsEventHandler::InstrumentCountChanged(String Dir) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_count, InstrumentsDb::toEscapedPath(Dir)));
}

void LSCPServer::DbInstrumentsEventHandler::InstrumentInfoChanged(String Instr) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_info, InstrumentsDb::toEscapedPath(Instr)));
}

void LSCPServer::DbInstrumentsEventHandler::InstrumentNameChanged(String Instr, String NewName) {
    Instr = "'" + InstrumentsDb::toEscapedPath(Instr) + "'";
    NewName = "'" + InstrumentsDb::toEscapedPath(NewName) + "'";
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instr_info, "NAME", Instr, NewName));
}

void LSCPServer::DbInstrumentsEventHandler::JobStatusChanged(int JobId) {
    LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_db_instrs_job_info, JobId));
}
#endif // HAVE_SQLITE3

void LSCPServer::RemoveListeners() {
    pSampler->RemoveChannelCountListener(&eventHandler);
    pSampler->RemoveAudioDeviceCountListener(&eventHandler);
    pSampler->RemoveMidiDeviceCountListener(&eventHandler);
    pSampler->RemoveVoiceCountListener(&eventHandler);
    pSampler->RemoveStreamCountListener(&eventHandler);
    pSampler->RemoveBufferFillListener(&eventHandler);
    pSampler->RemoveTotalStreamCountListener(&eventHandler);
    pSampler->RemoveTotalVoiceCountListener(&eventHandler);
    pSampler->RemoveFxSendCountListener(&eventHandler);
    MidiInstrumentMapper::RemoveMidiInstrumentCountListener(&eventHandler);
    MidiInstrumentMapper::RemoveMidiInstrumentInfoListener(&eventHandler);
    MidiInstrumentMapper::RemoveMidiInstrumentMapCountListener(&eventHandler);
    MidiInstrumentMapper::RemoveMidiInstrumentMapInfoListener(&eventHandler);
#if HAVE_SQLITE3
    InstrumentsDb::GetInstrumentsDb()->RemoveInstrumentsDbListener(&dbInstrumentsEventHandler);
#endif
}

/**
 * Blocks the calling thread until the LSCP Server is initialized and
 * accepting socket connections, if the server is already initialized then
 * this method will return immediately.
 * @param TimeoutSeconds     - optional: max. wait time in seconds
 *                             (default: 0s)
 * @param TimeoutNanoSeconds - optional: max wait time in nano seconds
 *                             (default: 0ns)
 * @returns  0 on success, a value less than 0 if timeout exceeded
 */
int LSCPServer::WaitUntilInitialized(long TimeoutSeconds, long TimeoutNanoSeconds) {
    return Initialized.WaitAndUnlockIf(false, TimeoutSeconds, TimeoutNanoSeconds);
}

int LSCPServer::Main() {
	#if defined(WIN32)
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		std::cerr << "LSCPServer: WSAStartup failed: " << iResult << "\n";
		exit(EXIT_FAILURE);
	}
	#endif
    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket < 0) {
        std::cerr << "LSCPServer: Could not create server socket." << std::endl;
        //return -1;
        exit(EXIT_FAILURE);
    }

    if (bind(hSocket, (sockaddr*) &SocketAddress, sizeof(sockaddr_in)) < 0) {
        std::cerr << "LSCPServer: Could not bind server socket, retrying for " << ToString(LSCP_SERVER_BIND_TIMEOUT) << " seconds...";
        for (int trial = 0; true; trial++) { // retry for LSCP_SERVER_BIND_TIMEOUT seconds
            if (bind(hSocket, (sockaddr*) &SocketAddress, sizeof(sockaddr_in)) < 0) {
                if (trial > LSCP_SERVER_BIND_TIMEOUT) {
                    std::cerr << "gave up!" << std::endl;
                    #if defined(WIN32)
                    closesocket(hSocket);
                    #else
                    close(hSocket);
                    #endif
                    //return -1;
                    exit(EXIT_FAILURE);
                }
                else sleep(1); // sleep 1s
            }
            else break; // success
        }
    }

    listen(hSocket, 1);
    Initialized.Set(true);

    // Registering event listeners
    pSampler->AddChannelCountListener(&eventHandler);
    pSampler->AddAudioDeviceCountListener(&eventHandler);
    pSampler->AddMidiDeviceCountListener(&eventHandler);
    pSampler->AddVoiceCountListener(&eventHandler);
    pSampler->AddStreamCountListener(&eventHandler);
    pSampler->AddBufferFillListener(&eventHandler);
    pSampler->AddTotalStreamCountListener(&eventHandler);
    pSampler->AddTotalVoiceCountListener(&eventHandler);
    pSampler->AddFxSendCountListener(&eventHandler);
    MidiInstrumentMapper::AddMidiInstrumentCountListener(&eventHandler);
    MidiInstrumentMapper::AddMidiInstrumentInfoListener(&eventHandler);
    MidiInstrumentMapper::AddMidiInstrumentMapCountListener(&eventHandler);
    MidiInstrumentMapper::AddMidiInstrumentMapInfoListener(&eventHandler);
#if HAVE_SQLITE3
    InstrumentsDb::GetInstrumentsDb()->AddInstrumentsDbListener(&dbInstrumentsEventHandler);
#endif
    // now wait for client connections and handle their requests
    sockaddr_in client;
    int length = sizeof(client);
    FD_ZERO(&fdSet);
    FD_SET(hSocket, &fdSet);
    int maxSessions = hSocket;

    timeval timeout;

    while (true) {
	#if CONFIG_PTHREAD_TESTCANCEL
		TestCancel();
	#endif
        // check if some engine channel's parameter / status changed, if so notify the respective LSCP event subscribers
        {
            LockGuard lock(EngineChannelFactory::EngineChannelsMutex);
            std::set<EngineChannel*> engineChannels = EngineChannelFactory::EngineChannelInstances();
            std::set<EngineChannel*>::iterator itEngineChannel = engineChannels.begin();
            std::set<EngineChannel*>::iterator itEnd           = engineChannels.end();
            for (; itEngineChannel != itEnd; ++itEngineChannel) {
                if ((*itEngineChannel)->StatusChanged()) {
                    SendLSCPNotify(LSCPEvent(LSCPEvent::event_channel_info, (*itEngineChannel)->GetSamplerChannel()->Index()));
                }

                for (int i = 0; i < (*itEngineChannel)->GetFxSendCount(); i++) {
                    FxSend* fxs = (*itEngineChannel)->GetFxSend(i);
                    if(fxs != NULL && fxs->IsInfoChanged()) {
                        int chn = (*itEngineChannel)->GetSamplerChannel()->Index();
                        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, chn, fxs->Id()));
                        fxs->SetInfoChanged(false);
                    }
                }
            }
        }

        // check if MIDI data arrived on some engine channel
        for (int i = 0; i < eventHandler.channelMidiListeners.size(); ++i) {
            const EventHandler::midi_listener_entry entry =
                eventHandler.channelMidiListeners[i];
            VirtualMidiDevice* pMidiListener = entry.pMidiListener;
            if (pMidiListener->NotesChanged()) {
                for (int iNote = 0; iNote < 128; iNote++) {
                    if (pMidiListener->NoteChanged(iNote)) {
                        const bool bActive = pMidiListener->NoteIsActive(iNote);
                        LSCPServer::SendLSCPNotify(
                            LSCPEvent(
                                LSCPEvent::event_channel_midi,
                                entry.pSamplerChannel->Index(),
                                std::string(bActive ? "NOTE_ON" : "NOTE_OFF"),
                                iNote,
                                bActive ? pMidiListener->NoteOnVelocity(iNote)
                                        : pMidiListener->NoteOffVelocity(iNote)
                            )
                        );
                    }
                }
            }
        }

        // check if MIDI data arrived on some MIDI device
        for (int i = 0; i < eventHandler.deviceMidiListeners.size(); ++i) {
            const EventHandler::device_midi_listener_entry entry =
                eventHandler.deviceMidiListeners[i];
            VirtualMidiDevice* pMidiListener = entry.pMidiListener;
            if (pMidiListener->NotesChanged()) {
                for (int iNote = 0; iNote < 128; iNote++) {
                    if (pMidiListener->NoteChanged(iNote)) {
                        const bool bActive = pMidiListener->NoteIsActive(iNote);
                        LSCPServer::SendLSCPNotify(
                            LSCPEvent(
                                LSCPEvent::event_device_midi,
                                entry.uiDeviceID,
                                entry.pPort->GetPortNumber(),
                                std::string(bActive ? "NOTE_ON" : "NOTE_OFF"),
                                iNote,
                                bActive ? pMidiListener->NoteOnVelocity(iNote)
                                        : pMidiListener->NoteOffVelocity(iNote)
                            )
                        );
                    }
                }
            }
        }

        //Now let's deliver late notifies (if any)
        {
            LockGuard lock(NotifyBufferMutex);
            for (std::map<int,String>::iterator iterNotify = bufferedNotifies.begin(); iterNotify != bufferedNotifies.end(); iterNotify++) {
#ifdef MSG_NOSIGNAL
                send(iterNotify->first, iterNotify->second.c_str(), iterNotify->second.size(), MSG_NOSIGNAL);
#else
                send(iterNotify->first, iterNotify->second.c_str(), iterNotify->second.size(), 0);
#endif
            }
            bufferedNotifies.clear();
        }

        fd_set selectSet = fdSet;
        timeout.tv_sec  = 0;
        timeout.tv_usec = 100000;

        int retval = select(maxSessions+1, &selectSet, NULL, NULL, &timeout);

	if (retval == 0 || (retval == -1 && errno == EINTR))
		continue; //Nothing try again
	if (retval == -1) {
		std::cerr << "LSCPServer: Socket select error." << std::endl;
		#if defined(WIN32)
		closesocket(hSocket);
		#else
		close(hSocket);
		#endif
		exit(EXIT_FAILURE);
	}

	//Accept new connections now (if any)
	if (FD_ISSET(hSocket, &selectSet)) {
		int socket = accept(hSocket, (sockaddr*) &client, (socklen_t*) &length);
		if (socket < 0) {
			std::cerr << "LSCPServer: Client connection failed." << std::endl;
			exit(EXIT_FAILURE);
		}

		#if defined(WIN32)
		u_long nonblock_io = 1;
		if( ioctlsocket(socket, FIONBIO, &nonblock_io) ) {
		  std::cerr << "LSCPServer: ioctlsocket: set FIONBIO failed. Error " << WSAGetLastError() << std::endl;
		  exit(EXIT_FAILURE);
		}
        #else
                struct linger linger;
                linger.l_onoff = 1;
                linger.l_linger = 0;
                if(setsockopt(socket, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger))) {
                    std::cerr << "LSCPServer: Failed to set SO_LINGER\n";
                }

		if (fcntl(socket, F_SETFL, O_NONBLOCK)) {
			std::cerr << "LSCPServer: F_SETFL O_NONBLOCK failed." << std::endl;
			exit(EXIT_FAILURE);
		}
		#endif

                // Parser initialization
                yyparse_param_t yyparse_param;
                yyparse_param.pServer  = this;
                yyparse_param.hSession = socket;

		Sessions.push_back(yyparse_param);
		FD_SET(socket, &fdSet);
		if (socket > maxSessions)
			maxSessions = socket;
		dmsg(1,("LSCPServer: Client connection established on socket:%d.\n", socket));
		LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_misc, "Client connection established on socket", socket));
		continue; //Maybe this was the only selected socket, better select again
	}

	//Something was selected and it was not the hSocket, so it must be some command(s) coming.
	for (std::vector<yyparse_param_t>::iterator iter = Sessions.begin(); iter != Sessions.end(); iter++) {
		if (FD_ISSET((*iter).hSession, &selectSet)) {	//Was it this socket?
			currentSocket = (*iter).hSession;  //a hack
			if (GetLSCPCommand(iter)) {	//Have we read the entire command?
				dmsg(3,("LSCPServer: Got command on socket %d, calling parser.\n", currentSocket));
                                int dummy; // just a temporary hack to fulfill the restart() function prototype
                                restart(NULL, dummy); // restart the 'scanner'
				itCurrentSession = iter; // another hack
				dmsg(2,("LSCPServer: [%s]\n",bufferedCommands[currentSocket].c_str()));
                                if ((*iter).bVerbose) { // if echo mode enabled
                                    AnswerClient(bufferedCommands[currentSocket]);
                                }
				int result = yyparse(&(*iter));
				currentSocket = -1;	//continuation of a hack
				itCurrentSession = Sessions.end(); // hack as well
				dmsg(3,("LSCPServer: Done parsing on socket %d.\n", currentSocket));
				if (result == LSCP_QUIT) { //Was it a quit command by any chance?
					CloseConnection(iter);
				}
			}
			currentSocket = -1;	//continuation of a hack
			//socket may have been closed, iter may be invalid, get out of the loop for now.
			//we'll be back if there is data.
			break;
		}
	}
    }
}

void LSCPServer::CloseConnection( std::vector<yyparse_param_t>::iterator iter ) {
	int socket = (*iter).hSession;
	dmsg(1,("LSCPServer: Client connection terminated on socket:%d.\n",socket));
	LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_misc, "Client connection terminated on socket", socket));
	Sessions.erase(iter);
	FD_CLR(socket,  &fdSet);
	{
            LockGuard lock(SubscriptionMutex);
            // Must unsubscribe this socket from all events (if any)
            for (std::map< LSCPEvent::event_t, std::list<int> >::iterator iter = eventSubscriptions.begin(); iter != eventSubscriptions.end(); iter++) {
                iter->second.remove(socket);
            }
        }
	LockGuard lock(NotifyMutex);
	bufferedCommands.erase(socket);
	bufferedNotifies.erase(socket);
	#if defined(WIN32)
	closesocket(socket);
	#else
	close(socket);
	#endif
}

void LSCPServer::CloseAllConnections() {
    std::vector<yyparse_param_t>::iterator iter = Sessions.begin();
    while(iter != Sessions.end()) {
        CloseConnection(iter);
        iter = Sessions.begin();
    }
}

int LSCPServer::EventSubscribers( std::list<LSCPEvent::event_t> events ) {
	int subs = 0;
	LockGuard lock(SubscriptionMutex);
	for( std::list<LSCPEvent::event_t>::iterator iter = events.begin();
			iter != events.end(); iter++)
	{
		subs += eventSubscriptions.count(*iter);
	}
	return subs;
}

void LSCPServer::SendLSCPNotify( LSCPEvent event ) {
	LockGuard lock(SubscriptionMutex);
	if (eventSubscriptions.count(event.GetType()) == 0) {
		// Nobody is subscribed to this event
		return;
	}
	std::list<int>::iterator iter = eventSubscriptions[event.GetType()].begin();
	std::list<int>::iterator end = eventSubscriptions[event.GetType()].end();
	String notify = event.Produce();

	while (true) {
		if (NotifyMutex.Trylock()) {
			for(;iter != end; iter++)
#ifdef MSG_NOSIGNAL
				send(*iter, notify.c_str(), notify.size(), MSG_NOSIGNAL);
#else
				send(*iter, notify.c_str(), notify.size(), 0);
#endif
			NotifyMutex.Unlock();
			break;
		} else {
			if (NotifyBufferMutex.Trylock()) {
				for(;iter != end; iter++)
					bufferedNotifies[*iter] += notify;
				NotifyBufferMutex.Unlock();
				break;
			}
		}
	}
}

extern int GetLSCPCommand( void *buf, int max_size ) {
	String command = LSCPServer::bufferedCommands[LSCPServer::currentSocket];
	if (command.size() == 0) { 		//Parser wants input but we have nothing.
		strcpy((char*) buf, "\n"); 	//So give it an empty command
		return 1;			//to keep it happy.
	}

	if (max_size < command.size()) {
		std::cerr << "getLSCPCommand: Flex buffer too small, ignoring the command." << std::endl;
		return 0;	//This will never happen
	}

	strcpy((char*) buf, command.c_str());
	LSCPServer::bufferedCommands.erase(LSCPServer::currentSocket);
	return (int) command.size();
}

extern yyparse_param_t* GetCurrentYaccSession() {
    return &(*itCurrentSession);
}

/**
 * Generate the relevant LSCP documentation reference section if necessary.
 * The documentation section for the currently active command on the LSCP
 * shell's command line will be encoded in a special format, specifically for
 * the LSCP shell application.
 *
 * @param line - current LSCP command line
 * @param param - reentrant Bison parser parameters
 *
 * @return encoded reference string or empty string if nothing shall be sent
 *         to LSCP shell (client) at this point
 */
String LSCPServer::generateLSCPDocReply(const String& line, yyparse_param_t* param) {
    String result;
    lscp_ref_entry_t* ref = lscp_reference_for_command(line.c_str());
    // Pointer comparison works here, since the function above always
    // returns the same constant pointer for the respective LSCP
    // command ... Only send the LSCP reference section to the client if
    // another LSCP reference section became relevant now:
    if (ref != param->pLSCPDocRef) {
        param->pLSCPDocRef = ref;
        if (ref) { // send a new LSCP doc section to client ...
            result += "SHD:" + ToString(LSCP_SHD_MATCH) + ":" + String(ref->name) + "\n";
            result += String(ref->section) + "\n";
            result += "."; // dot line marks the end of the text for client
        } else { // inform client that no LSCP doc section matches right now ...
            result = "SHD:" + ToString(LSCP_SHD_NO_MATCH);
        }
    }
    dmsg(4,("LSCP doc reply -> '%s'\n", result.c_str()));
    return result;
}

/**
 * Will be called to try to read the command from the socket
 * If command is read, it will return true. Otherwise false is returned.
 * In any case the received portion (complete or incomplete) is saved into bufferedCommand map.
 */
bool LSCPServer::GetLSCPCommand( std::vector<yyparse_param_t>::iterator iter ) {
	int socket = (*iter).hSession;
	int result;
	char c;
	std::vector<char> input;

	// first get as many character as possible and add it to the 'input' buffer
	while (true) {
		#if defined(WIN32)
		result = (int)recv(socket, (char*)&c, 1, 0); //Read one character at a time for now
		#else
		result = (int)recv(socket, (void*)&c, 1, 0); //Read one character at a time for now
		#endif
		if (result == 1) input.push_back(c);
		else break; // end of input or some error
		if (c == '\n') break; // process line by line
	}

	// process input buffer
	for (int i = 0; i < input.size(); ++i) {
		c = input[i];
		if (c == '\r') continue; //Ignore CR
		if (c == '\n') {
			// only if the other side is the LSCP shell application:
			// check the current (incomplete) command line for syntax errors,
			// possible completions and report everything back to the shell
			if ((*iter).bShellInteract || (*iter).bShellAutoCorrect) {
				String s = lscpParserProcessShellInteraction(bufferedCommands[socket], &(*iter), false);
				if (!s.empty() && (*iter).bShellInteract) AnswerClient(s + "\n");
			}
			// if other side is LSCP shell application, send the relevant LSCP
			// documentation section of the current command line (if necessary)
			if ((*iter).bShellSendLSCPDoc && (*iter).bShellInteract) {
				String s = generateLSCPDocReply(bufferedCommands[socket], &(*iter));
				if (!s.empty()) AnswerClient(s + "\n");
			}
			LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_misc, "Received \'" + bufferedCommands[socket] + "\' on socket", socket));
			bufferedCommands[socket] += "\r\n";
			return true; //Complete command was read
		} else if (c == 2) { // custom ASCII code usage for moving cursor left (LSCP shell)
			if (iter->iCursorOffset + bufferedCommands[socket].size() > 0)
				iter->iCursorOffset--;
		} else if (c == 3) { // custom ASCII code usage for moving cursor right (LSCP shell)
			if (iter->iCursorOffset < 0) iter->iCursorOffset++;
		} else {
			ssize_t cursorPos = bufferedCommands[socket].size() + iter->iCursorOffset;
			// backspace character - should only happen with shell
			if (c == '\b') {
				if (!bufferedCommands[socket].empty() && cursorPos > 0)
					bufferedCommands[socket].erase(cursorPos - 1, 1);
			} else { // append (or insert) new character (at current cursor position) ...
				if (cursorPos >= 0)
					bufferedCommands[socket].insert(cursorPos, String(1,c)); // insert
				else
					bufferedCommands[socket] += c; // append
			}
		}
		// Only if the other side (client) is the LSCP shell application:
		// The following block takes care about automatic correction, auto
		// completion (and suggestions), LSCP reference documentation, etc.
		// The "if" statement here is for optimization reasons, so that the
		// heavy LSCP grammar evaluation algorithm is only executed once for an
		// entire command line received.
		if (i == input.size() - 1) {
			// check the current (incomplete) command line for syntax errors,
			// possible completions and report everything back to the shell
			if ((*iter).bShellInteract || (*iter).bShellAutoCorrect) {
				String s = lscpParserProcessShellInteraction(bufferedCommands[socket], &(*iter), true);
				if (!s.empty() && (*iter).bShellInteract && i == input.size() - 1)
					AnswerClient(s + "\n");
			}
			// if other side is LSCP shell application, send the relevant LSCP
			// documentation section of the current command line (if necessary)
			if ((*iter).bShellSendLSCPDoc && (*iter).bShellInteract) {
				String s = generateLSCPDocReply(bufferedCommands[socket], &(*iter));
				if (!s.empty()) AnswerClient(s + "\n");
			}
		}
	}

	// handle network errors ...
	if (result == 0) { //socket was selected, so 0 here means client has closed the connection
		CloseConnection(iter);
		return false;
	}
	#if defined(WIN32)
	if (result == SOCKET_ERROR) {
		int wsa_lasterror = WSAGetLastError();
		if (wsa_lasterror == WSAEWOULDBLOCK) //Would block, try again later.
			return false;
		dmsg(2,("LSCPScanner: Socket error after recv() Error %d.\n", wsa_lasterror));
		CloseConnection(iter);
		return false;
	}
	#else
	if (result == -1) {
		switch(errno) {
			case EBADF:
				dmsg(2,("LSCPScanner: The argument s is an invalid descriptor.\n"));
				break; // close connection
			case ECONNREFUSED:
				dmsg(2,("LSCPScanner: A remote host refused to allow the network connection (typically because it is not running the requested service).\n"));
				break; // close connection
			case ENOTCONN:
				dmsg(2,("LSCPScanner: The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).\n"));
				break; // close connection
			case ENOTSOCK:
				dmsg(2,("LSCPScanner: The argument s does not refer to a socket.\n"));
				break; // close connection
			case EAGAIN:
				dmsg(2,("LSCPScanner: The socket is marked non-blocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.\n"));
				return false; // don't close connection, try again later
			case EINTR:
				dmsg(2,("LSCPScanner: The receive was interrupted by delivery of a signal before any data were available.\n"));
				break; // close connection
			case EFAULT:
				dmsg(2,("LSCPScanner: The receive buffer pointer(s) point outside the process's address space.\n"));
				break; // close connection
			case EINVAL:
				dmsg(2,("LSCPScanner: Invalid argument passed.\n"));
				break; // close connection
			case ENOMEM:
				dmsg(2,("LSCPScanner: Could not allocate memory for recvmsg.\n"));
				break; // close connection
			default:
				dmsg(2,("LSCPScanner: Unknown recv() error.\n"));
				break; // close connection
		}
		CloseConnection(iter);
		return false;
	}
	#endif

	return false;
}

/**
 * Will be called by the parser whenever it wants to send an answer to the
 * client / frontend.
 *
 * @param ReturnMessage - message that will be send to the client
 */
void LSCPServer::AnswerClient(String ReturnMessage) {
    dmsg(2,("LSCPServer::AnswerClient(ReturnMessage='%s')", ReturnMessage.c_str()));
    if (currentSocket != -1) {
	    LockGuard lock(NotifyMutex);

        // just if other side is LSCP shell: in case respose is a multi-line
        // one, then inform client about it before sending the actual mult-line
        // response
        if (GetCurrentYaccSession()->bShellInteract) {
            // check if this is a multi-line response
            int n = 0;
            for (int i = 0; i < ReturnMessage.size(); ++i)
                if (ReturnMessage[i] == '\n') ++n;
            if (n >= 2) {
                dmsg(2,("LSCP Shell <- expect mult-line response\n"));
                String s = LSCP_SHK_EXPECT_MULTI_LINE "\r\n";
#ifdef MSG_NOSIGNAL
                send(currentSocket, s.c_str(), s.size(), MSG_NOSIGNAL);
#else
                send(currentSocket, s.c_str(), s.size(), 0);
#endif                
            }
        }

#ifdef MSG_NOSIGNAL
	    send(currentSocket, ReturnMessage.c_str(), ReturnMessage.size(), MSG_NOSIGNAL);
#else
	    send(currentSocket, ReturnMessage.c_str(), ReturnMessage.size(), 0);
#endif
    }
}

/**
 * Find a created audio output device index.
 */
int LSCPServer::GetAudioOutputDeviceIndex ( AudioOutputDevice *pDevice )
{
    // Search for the created device to get its index
    std::map<uint, AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
    std::map<uint, AudioOutputDevice*>::iterator iter = devices.begin();
    for (; iter != devices.end(); iter++) {
        if (iter->second == pDevice)
            return iter->first;
    }
    // Not found.
    return -1;
}

/**
 * Find a created midi input device index.
 */
int LSCPServer::GetMidiInputDeviceIndex ( MidiInputDevice *pDevice )
{
    // Search for the created device to get its index
    std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
    std::map<uint, MidiInputDevice*>::iterator iter = devices.begin();
    for (; iter != devices.end(); iter++) {
        if (iter->second == pDevice)
            return iter->first;
    }
    // Not found.
    return -1;
}

String LSCPServer::CreateAudioOutputDevice(String Driver, std::map<String,String> Parameters) {
    dmsg(2,("LSCPServer: CreateAudioOutputDevice(Driver=%s)\n", Driver.c_str()));
    LSCPResultSet result;
    try {
        AudioOutputDevice* pDevice = pSampler->CreateAudioOutputDevice(Driver, Parameters);
        // search for the created device to get its index
        int index = GetAudioOutputDeviceIndex(pDevice);
        if (index == -1) throw Exception("Internal error: could not find created audio output device.");
        result = index; // success
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::CreateMidiInputDevice(String Driver, std::map<String,String> Parameters) {
    dmsg(2,("LSCPServer: CreateMidiInputDevice(Driver=%s)\n", Driver.c_str()));
    LSCPResultSet result;
    try {
        MidiInputDevice* pDevice = pSampler->CreateMidiInputDevice(Driver, Parameters);
        // search for the created device to get its index
        int index = GetMidiInputDeviceIndex(pDevice);
        if (index == -1) throw Exception("Internal error: could not find created midi input device.");
        result = index; // success
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::DestroyAudioOutputDevice(uint DeviceIndex) {
    dmsg(2,("LSCPServer: DestroyAudioOutputDevice(DeviceIndex=%d)\n", DeviceIndex));
    LSCPResultSet result;
    try {
        std::map<uint, AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no audio output device with index " + ToString(DeviceIndex) + ".");
        AudioOutputDevice* pDevice = devices[DeviceIndex];
        pSampler->DestroyAudioOutputDevice(pDevice);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::DestroyMidiInputDevice(uint DeviceIndex) {
    dmsg(2,("LSCPServer: DestroyMidiInputDevice(DeviceIndex=%d)\n", DeviceIndex));
    LSCPResultSet result;
    try {
        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no audio output device with index " + ToString(DeviceIndex) + ".");
        MidiInputDevice* pDevice = devices[DeviceIndex];
        pSampler->DestroyMidiInputDevice(pDevice);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

EngineChannel* LSCPServer::GetEngineChannel(uint uiSamplerChannel) {
    SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
    if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));

    EngineChannel* pEngineChannel = pSamplerChannel->GetEngineChannel();
    if (!pEngineChannel) throw Exception("There is no engine deployed on this sampler channel yet");

    return pEngineChannel;
}

/**
 * Will be called by the parser to load an instrument.
 */
String LSCPServer::LoadInstrument(String Filename, uint uiInstrument, uint uiSamplerChannel, bool bBackground) {
    dmsg(2,("LSCPServer: LoadInstrument(Filename=%s,Instrument=%d,SamplerChannel=%d)\n", Filename.c_str(), uiInstrument, uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        EngineChannel* pEngineChannel = pSamplerChannel->GetEngineChannel();
        if (!pEngineChannel) throw Exception("No engine type assigned to sampler channel yet");
        if (!pSamplerChannel->GetAudioOutputDevice())
            throw Exception("No audio output device connected to sampler channel");
        if (bBackground) {
            InstrumentManager::instrument_id_t id;
            id.FileName = Filename;
            id.Index    = uiInstrument;
            InstrumentManager::LoadInstrumentInBackground(id, pEngineChannel);
        }
        else {
            // tell the engine channel which instrument to load
            pEngineChannel->PrepareLoadInstrument(Filename.c_str(), uiInstrument);
            // actually start to load the instrument (blocks until completed)
            pEngineChannel->LoadInstrument();
        }
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to assign a sampler engine type to a
 * sampler channel.
 */
String LSCPServer::SetEngineType(String EngineName, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetEngineType(EngineName=%s,uiSamplerChannel=%d)\n", EngineName.c_str(), uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
	LockGuard lock(RTNotifyMutex);
        pSamplerChannel->SetEngineType(EngineName);
        if(HasSoloChannel()) pSamplerChannel->GetEngineChannel()->SetMute(-1);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get the amount of sampler channels.
 */
String LSCPServer::GetChannels() {
    dmsg(2,("LSCPServer: GetChannels()\n"));
    LSCPResultSet result;
    result.Add(pSampler->SamplerChannels());
    return result.Produce();
}

/**
 * Will be called by the parser to get the list of sampler channels.
 */
String LSCPServer::ListChannels() {
    dmsg(2,("LSCPServer: ListChannels()\n"));
    String list;
    std::map<uint,SamplerChannel*> channels = pSampler->GetSamplerChannels();
    std::map<uint,SamplerChannel*>::iterator iter = channels.begin();
    for (; iter != channels.end(); iter++) {
        if (list != "") list += ",";
        list += ToString(iter->first);
    }
    LSCPResultSet result;
    result.Add(list);
    return result.Produce();
}

/**
 * Will be called by the parser to add a sampler channel.
 */
String LSCPServer::AddChannel() {
    dmsg(2,("LSCPServer: AddChannel()\n"));
    SamplerChannel* pSamplerChannel;
    {
        LockGuard lock(RTNotifyMutex);
        pSamplerChannel = pSampler->AddSamplerChannel();
    }
    LSCPResultSet result(pSamplerChannel->Index());
    return result.Produce();
}

/**
 * Will be called by the parser to remove a sampler channel.
 */
String LSCPServer::RemoveChannel(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: RemoveChannel(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    {
        LockGuard lock(RTNotifyMutex);
        pSampler->RemoveSamplerChannel(uiSamplerChannel);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get the amount of all available engines.
 */
String LSCPServer::GetAvailableEngines() {
    dmsg(2,("LSCPServer: GetAvailableEngines()\n"));
    LSCPResultSet result;
    try {
        int n = (int)EngineFactory::AvailableEngineTypes().size();
        result.Add(n);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get a list of all available engines.
 */
String LSCPServer::ListAvailableEngines() {
    dmsg(2,("LSCPServer: ListAvailableEngines()\n"));
    LSCPResultSet result;
    try {
        String s = EngineFactory::AvailableEngineTypesAsString();
        result.Add(s);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get descriptions for a particular
 * sampler engine.
 */
String LSCPServer::GetEngineInfo(String EngineName) {
    dmsg(2,("LSCPServer: GetEngineInfo(EngineName=%s)\n", EngineName.c_str()));
    LSCPResultSet result;
    {
        LockGuard lock(RTNotifyMutex);
        try {
            Engine* pEngine = EngineFactory::Create(EngineName);
            result.Add("DESCRIPTION", _escapeLscpResponse(pEngine->Description()));
            result.Add("VERSION",     pEngine->Version());
            EngineFactory::Destroy(pEngine);
        }
        catch (Exception e) {
            result.Error(e);
        }
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get informations about a particular
 * sampler channel.
 */
String LSCPServer::GetChannelInfo(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: GetChannelInfo(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        EngineChannel* pEngineChannel = pSamplerChannel->GetEngineChannel();

	//Defaults values
	String EngineName = "NONE";
        float Volume = 0.0f;
	String InstrumentFileName = "NONE";
	String InstrumentName = "NONE";
	int InstrumentIndex = -1;
	int InstrumentStatus = -1;
        int AudioOutputChannels = 0;
        String AudioRouting;
        int Mute = 0;
        bool Solo = false;
        String MidiInstrumentMap = "NONE";

        if (pEngineChannel) {
            EngineName          = pEngineChannel->EngineName();
            AudioOutputChannels = pEngineChannel->Channels();
            Volume              = pEngineChannel->Volume();
            InstrumentStatus    = pEngineChannel->InstrumentStatus();
            InstrumentIndex     = pEngineChannel->InstrumentIndex();
            if (InstrumentIndex != -1) {
                InstrumentFileName = pEngineChannel->InstrumentFileName();
                InstrumentName     = pEngineChannel->InstrumentName();
            }
            for (int chan = 0; chan < pEngineChannel->Channels(); chan++) {
                if (AudioRouting != "") AudioRouting += ",";
                AudioRouting += ToString(pEngineChannel->OutputChannel(chan));
            }
            Mute = pEngineChannel->GetMute();
            Solo = pEngineChannel->GetSolo();
            if (pEngineChannel->UsesNoMidiInstrumentMap())
                MidiInstrumentMap = "NONE";
            else if (pEngineChannel->UsesDefaultMidiInstrumentMap())
                MidiInstrumentMap = "DEFAULT";
            else
                MidiInstrumentMap = ToString(pEngineChannel->GetMidiInstrumentMap());
	}

        result.Add("ENGINE_NAME", EngineName);
        result.Add("VOLUME", Volume);

	//Some not-so-hardcoded stuff to make GUI look good
        result.Add("AUDIO_OUTPUT_DEVICE", GetAudioOutputDeviceIndex(pSamplerChannel->GetAudioOutputDevice()));
        result.Add("AUDIO_OUTPUT_CHANNELS", AudioOutputChannels);
        result.Add("AUDIO_OUTPUT_ROUTING", AudioRouting);

        result.Add("MIDI_INPUT_DEVICE", GetMidiInputDeviceIndex(pSamplerChannel->GetMidiInputDevice()));
        result.Add("MIDI_INPUT_PORT", pSamplerChannel->GetMidiInputPort());
        if (pSamplerChannel->GetMidiInputChannel() == midi_chan_all) result.Add("MIDI_INPUT_CHANNEL", "ALL");
        else result.Add("MIDI_INPUT_CHANNEL", pSamplerChannel->GetMidiInputChannel());

        // convert the filename into the correct encoding as defined for LSCP
        // (especially in terms of special characters -> escape sequences)
        if (InstrumentFileName != "NONE" && InstrumentFileName != "") {
#if WIN32
            InstrumentFileName = Path::fromWindows(InstrumentFileName).toLscp();
#else
            // assuming POSIX
            InstrumentFileName = Path::fromPosix(InstrumentFileName).toLscp();
#endif
        }

        result.Add("INSTRUMENT_FILE", InstrumentFileName);
        result.Add("INSTRUMENT_NR", InstrumentIndex);
        result.Add("INSTRUMENT_NAME", _escapeLscpResponse(InstrumentName));
        result.Add("INSTRUMENT_STATUS", InstrumentStatus);
        result.Add("MUTE", Mute == -1 ? "MUTED_BY_SOLO" : (Mute ? "true" : "false"));
        result.Add("SOLO", Solo);
        result.Add("MIDI_INSTRUMENT_MAP", MidiInstrumentMap);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get the amount of active voices on a
 * particular sampler channel.
 */
String LSCPServer::GetVoiceCount(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: GetVoiceCount(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        if (!pEngineChannel->GetEngine()) throw Exception("No audio output device connected to sampler channel");
	result.Add(pEngineChannel->GetEngine()->VoiceCount());
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get the amount of active disk streams on a
 * particular sampler channel.
 */
String LSCPServer::GetStreamCount(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: GetStreamCount(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        if (!pEngineChannel->GetEngine()) throw Exception("No audio output device connected to sampler channel");
        result.Add(pEngineChannel->GetEngine()->DiskStreamCount());
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to get the buffer fill states of all disk
 * streams on a particular sampler channel.
 */
String LSCPServer::GetBufferFill(fill_response_t ResponseType, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: GetBufferFill(ResponseType=%d, SamplerChannel=%d)\n", ResponseType, uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        if (!pEngineChannel->GetEngine()) throw Exception("No audio output device connected to sampler channel");
        if (!pEngineChannel->GetEngine()->DiskStreamSupported()) result.Add("NA");
        else {
            switch (ResponseType) {
                case fill_response_bytes:
                    result.Add(pEngineChannel->GetEngine()->DiskStreamBufferFillBytes());
                    break;
                case fill_response_percentage:
                    result.Add(pEngineChannel->GetEngine()->DiskStreamBufferFillPercentage());
                    break;
                default:
                    throw Exception("Unknown fill response type");
            }
	}
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAvailableAudioOutputDrivers() {
    dmsg(2,("LSCPServer: GetAvailableAudioOutputDrivers()\n"));
    LSCPResultSet result;
    try {
        int n = (int) AudioOutputDeviceFactory::AvailableDrivers().size();
        result.Add(n);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListAvailableAudioOutputDrivers() {
    dmsg(2,("LSCPServer: ListAvailableAudioOutputDrivers()\n"));
    LSCPResultSet result;
    try {
        String s = AudioOutputDeviceFactory::AvailableDriversAsString();
        result.Add(s);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAvailableMidiInputDrivers() {
    dmsg(2,("LSCPServer: GetAvailableMidiInputDrivers()\n"));
    LSCPResultSet result;
    try {
        int n = (int)MidiInputDeviceFactory::AvailableDrivers().size();
        result.Add(n);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListAvailableMidiInputDrivers() {
    dmsg(2,("LSCPServer: ListAvailableMidiInputDrivers()\n"));
    LSCPResultSet result;
    try {
        String s = MidiInputDeviceFactory::AvailableDriversAsString();
        result.Add(s);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputDriverInfo(String Driver) {
    dmsg(2,("LSCPServer: GetMidiInputDriverInfo(Driver=%s)\n",Driver.c_str()));
    LSCPResultSet result;
    try {
        result.Add("DESCRIPTION", MidiInputDeviceFactory::GetDriverDescription(Driver));
        result.Add("VERSION",     MidiInputDeviceFactory::GetDriverVersion(Driver));

        std::map<String,DeviceCreationParameter*> parameters = MidiInputDeviceFactory::GetAvailableDriverParameters(Driver);
        if (parameters.size()) { // if there are parameters defined for this driver
            String s;
            std::map<String,DeviceCreationParameter*>::iterator iter = parameters.begin();
            for (;iter != parameters.end(); iter++) {
                if (s != "") s += ",";
                s += iter->first;
                delete iter->second;
            }
            result.Add("PARAMETERS", s);
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputDriverInfo(String Driver) {
    dmsg(2,("LSCPServer: GetAudioOutputDriverInfo(Driver=%s)\n",Driver.c_str()));
    LSCPResultSet result;
    try {
        result.Add("DESCRIPTION", AudioOutputDeviceFactory::GetDriverDescription(Driver));
        result.Add("VERSION",     AudioOutputDeviceFactory::GetDriverVersion(Driver));

        std::map<String,DeviceCreationParameter*> parameters = AudioOutputDeviceFactory::GetAvailableDriverParameters(Driver);
        if (parameters.size()) { // if there are parameters defined for this driver
            String s;
            std::map<String,DeviceCreationParameter*>::iterator iter = parameters.begin();
            for (;iter != parameters.end(); iter++) {
                if (s != "") s += ",";
                s += iter->first;
                delete iter->second;
            }
            result.Add("PARAMETERS", s);
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList) {
    dmsg(2,("LSCPServer: GetMidiInputDriverParameterInfo(Driver=%s,Parameter=%s,DependencyListSize=%d)\n",Driver.c_str(),Parameter.c_str(),int(DependencyList.size())));
    LSCPResultSet result;
    try {
        DeviceCreationParameter* pParameter = MidiInputDeviceFactory::GetDriverParameter(Driver, Parameter);
        result.Add("TYPE",         pParameter->Type());
        result.Add("DESCRIPTION",  pParameter->Description());
        result.Add("MANDATORY",    pParameter->Mandatory());
        result.Add("FIX",          pParameter->Fix());
        result.Add("MULTIPLICITY", pParameter->Multiplicity());
        optional<String> oDepends       = pParameter->Depends();
        optional<String> oDefault       = pParameter->Default(DependencyList);
        optional<String> oRangeMin      = pParameter->RangeMin(DependencyList);
        optional<String> oRangeMax      = pParameter->RangeMax(DependencyList);
        optional<String> oPossibilities = pParameter->Possibilities(DependencyList);
        if (oDepends)       result.Add("DEPENDS",       *oDepends);
        if (oDefault)       result.Add("DEFAULT",       *oDefault);
        if (oRangeMin)      result.Add("RANGE_MIN",     *oRangeMin);
        if (oRangeMax)      result.Add("RANGE_MAX",     *oRangeMax);
        if (oPossibilities) result.Add("POSSIBILITIES", *oPossibilities);
        delete pParameter;
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList) {
    dmsg(2,("LSCPServer: GetAudioOutputDriverParameterInfo(Driver=%s,Parameter=%s,DependencyListSize=%d)\n",Driver.c_str(),Parameter.c_str(),int(DependencyList.size())));
    LSCPResultSet result;
    try {
        DeviceCreationParameter* pParameter = AudioOutputDeviceFactory::GetDriverParameter(Driver, Parameter);
        result.Add("TYPE",         pParameter->Type());
        result.Add("DESCRIPTION",  pParameter->Description());
        result.Add("MANDATORY",    pParameter->Mandatory());
        result.Add("FIX",          pParameter->Fix());
        result.Add("MULTIPLICITY", pParameter->Multiplicity());
        optional<String> oDepends       = pParameter->Depends();
        optional<String> oDefault       = pParameter->Default(DependencyList);
        optional<String> oRangeMin      = pParameter->RangeMin(DependencyList);
        optional<String> oRangeMax      = pParameter->RangeMax(DependencyList);
        optional<String> oPossibilities = pParameter->Possibilities(DependencyList);
        if (oDepends)       result.Add("DEPENDS",       *oDepends);
        if (oDefault)       result.Add("DEFAULT",       *oDefault);
        if (oRangeMin)      result.Add("RANGE_MIN",     *oRangeMin);
        if (oRangeMax)      result.Add("RANGE_MAX",     *oRangeMax);
        if (oPossibilities) result.Add("POSSIBILITIES", *oPossibilities);
        delete pParameter;
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputDeviceCount() {
    dmsg(2,("LSCPServer: GetAudioOutputDeviceCount()\n"));
    LSCPResultSet result;
    try {
        uint count = pSampler->AudioOutputDevices();
        result.Add(count); // success
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputDeviceCount() {
    dmsg(2,("LSCPServer: GetMidiInputDeviceCount()\n"));
    LSCPResultSet result;
    try {
        uint count = pSampler->MidiInputDevices();
        result.Add(count); // success
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputDevices() {
    dmsg(2,("LSCPServer: GetAudioOutputDevices()\n"));
    LSCPResultSet result;
    try {
        String s;
        std::map<uint, AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        std::map<uint, AudioOutputDevice*>::iterator iter = devices.begin();
        for (; iter != devices.end(); iter++) {
            if (s != "") s += ",";
            s += ToString(iter->first);
        }
        result.Add(s);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputDevices() {
    dmsg(2,("LSCPServer: GetMidiInputDevices()\n"));
    LSCPResultSet result;
    try {
        String s;
        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        std::map<uint, MidiInputDevice*>::iterator iter = devices.begin();
        for (; iter != devices.end(); iter++) {
            if (s != "") s += ",";
            s += ToString(iter->first);
        }
        result.Add(s);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputDeviceInfo(uint DeviceIndex) {
    dmsg(2,("LSCPServer: GetAudioOutputDeviceInfo(DeviceIndex=%d)\n",DeviceIndex));
    LSCPResultSet result;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no audio output device with index " + ToString(DeviceIndex) + ".");
        AudioOutputDevice* pDevice = devices[DeviceIndex];
        result.Add("DRIVER", pDevice->Driver());
        std::map<String,DeviceCreationParameter*> parameters = pDevice->DeviceParameters();
        std::map<String,DeviceCreationParameter*>::iterator iter = parameters.begin();
        for (; iter != parameters.end(); iter++) {
            result.Add(iter->first, iter->second->Value());
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputDeviceInfo(uint DeviceIndex) {
    dmsg(2,("LSCPServer: GetMidiInputDeviceInfo(DeviceIndex=%d)\n",DeviceIndex));
    LSCPResultSet result;
    try {
        std::map<uint,MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no MIDI input device with index " + ToString(DeviceIndex) + ".");
        MidiInputDevice* pDevice = devices[DeviceIndex];
        result.Add("DRIVER", pDevice->Driver());
        std::map<String,DeviceCreationParameter*> parameters = pDevice->DeviceParameters();
        std::map<String,DeviceCreationParameter*>::iterator iter = parameters.begin();
        for (; iter != parameters.end(); iter++) {
            result.Add(iter->first, iter->second->Value());
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}
String LSCPServer::GetMidiInputPortInfo(uint DeviceIndex, uint PortIndex) {
    dmsg(2,("LSCPServer: GetMidiInputPortInfo(DeviceIndex=%d, PortIndex=%d)\n",DeviceIndex, PortIndex));
    LSCPResultSet result;
    try {
        // get MIDI input device
        std::map<uint,MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no MIDI input device with index " + ToString(DeviceIndex) + ".");
        MidiInputDevice* pDevice = devices[DeviceIndex];

        // get MIDI port
        MidiInputPort* pMidiInputPort = pDevice->GetPort(PortIndex);
        if (!pMidiInputPort) throw Exception("There is no MIDI input port with index " + ToString(PortIndex) + ".");

        // return the values of all MIDI port parameters
        std::map<String,DeviceRuntimeParameter*> parameters = pMidiInputPort->PortParameters();
        std::map<String,DeviceRuntimeParameter*>::iterator iter = parameters.begin();
        for (; iter != parameters.end(); iter++) {
            result.Add(iter->first, iter->second->Value());
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputChannelInfo(uint DeviceId, uint ChannelId) {
    dmsg(2,("LSCPServer: GetAudioOutputChannelInfo(DeviceId=%u,ChannelId=%u)\n",DeviceId,ChannelId));
    LSCPResultSet result;
    try {
        // get audio output device
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceId)) throw Exception("There is no audio output device with index " + ToString(DeviceId) + ".");
        AudioOutputDevice* pDevice = devices[DeviceId];

        // get audio channel
        AudioChannel* pChannel = pDevice->Channel(ChannelId);
        if (!pChannel) throw Exception("Audio output device does not have audio channel " + ToString(ChannelId) + ".");

        // return the values of all audio channel parameters
        std::map<String,DeviceRuntimeParameter*> parameters = pChannel->ChannelParameters();
        std::map<String,DeviceRuntimeParameter*>::iterator iter = parameters.begin();
        for (; iter != parameters.end(); iter++) {
            result.Add(iter->first, iter->second->Value());
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInputPortParameterInfo(uint DeviceId, uint PortId, String ParameterName) {
    dmsg(2,("LSCPServer: GetMidiInputPortParameterInfo(DeviceId=%d,PortId=%d,ParameterName=%s)\n",DeviceId,PortId,ParameterName.c_str()));
    LSCPResultSet result;
    try {
        // get MIDI input device
        std::map<uint,MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceId)) throw Exception("There is no midi input device with index " + ToString(DeviceId) + ".");
        MidiInputDevice* pDevice = devices[DeviceId];

        // get midi port
        MidiInputPort* pPort = pDevice->GetPort(PortId);
        if (!pPort) throw Exception("Midi input device does not have port " + ToString(PortId) + ".");

        // get desired port parameter
        std::map<String,DeviceRuntimeParameter*> parameters = pPort->PortParameters();
        if (!parameters.count(ParameterName)) throw Exception("Midi port does not provide a parameter '" + ParameterName + "'.");
        DeviceRuntimeParameter* pParameter = parameters[ParameterName];

        // return all fields of this audio channel parameter
        result.Add("TYPE",         pParameter->Type());
        result.Add("DESCRIPTION",  pParameter->Description());
        result.Add("FIX",          pParameter->Fix());
        result.Add("MULTIPLICITY", pParameter->Multiplicity());
        if (pParameter->RangeMin())      result.Add("RANGE_MIN",     *pParameter->RangeMin());
        if (pParameter->RangeMax())      result.Add("RANGE_MAX",     *pParameter->RangeMax());
        if (pParameter->Possibilities()) result.Add("POSSIBILITIES", *pParameter->Possibilities());
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAudioOutputChannelParameterInfo(uint DeviceId, uint ChannelId, String ParameterName) {
    dmsg(2,("LSCPServer: GetAudioOutputChannelParameterInfo(DeviceId=%d,ChannelId=%d,ParameterName=%s)\n",DeviceId,ChannelId,ParameterName.c_str()));
    LSCPResultSet result;
    try {
        // get audio output device
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceId)) throw Exception("There is no audio output device with index " + ToString(DeviceId) + ".");
        AudioOutputDevice* pDevice = devices[DeviceId];

        // get audio channel
        AudioChannel* pChannel = pDevice->Channel(ChannelId);
        if (!pChannel) throw Exception("Audio output device does not have audio channel " + ToString(ChannelId) + ".");

        // get desired audio channel parameter
        std::map<String,DeviceRuntimeParameter*> parameters = pChannel->ChannelParameters();
        if (!parameters.count(ParameterName)) throw Exception("Audio channel does not provide a parameter '" + ParameterName + "'.");
        DeviceRuntimeParameter* pParameter = parameters[ParameterName];

        // return all fields of this audio channel parameter
        result.Add("TYPE",         pParameter->Type());
        result.Add("DESCRIPTION",  pParameter->Description());
        result.Add("FIX",          pParameter->Fix());
        result.Add("MULTIPLICITY", pParameter->Multiplicity());
        if (pParameter->RangeMin())      result.Add("RANGE_MIN",     *pParameter->RangeMin());
        if (pParameter->RangeMax())      result.Add("RANGE_MAX",     *pParameter->RangeMax());
        if (pParameter->Possibilities()) result.Add("POSSIBILITIES", *pParameter->Possibilities());
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetAudioOutputChannelParameter(uint DeviceId, uint ChannelId, String ParamKey, String ParamVal) {
    dmsg(2,("LSCPServer: SetAudioOutputChannelParameter(DeviceId=%d,ChannelId=%d,ParamKey=%s,ParamVal=%s)\n",DeviceId,ChannelId,ParamKey.c_str(),ParamVal.c_str()));
    LSCPResultSet result;
    try {
        // get audio output device
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceId)) throw Exception("There is no audio output device with index " + ToString(DeviceId) + ".");
        AudioOutputDevice* pDevice = devices[DeviceId];

        // get audio channel
        AudioChannel* pChannel = pDevice->Channel(ChannelId);
        if (!pChannel) throw Exception("Audio output device does not have audio channel " + ToString(ChannelId) + ".");

        // get desired audio channel parameter
        std::map<String,DeviceRuntimeParameter*> parameters = pChannel->ChannelParameters();
        if (!parameters.count(ParamKey)) throw Exception("Audio channel does not provide a parameter '" + ParamKey + "'.");
        DeviceRuntimeParameter* pParameter = parameters[ParamKey];

        // set new channel parameter value
        pParameter->SetValue(ParamVal);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_audio_device_info, DeviceId));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetAudioOutputDeviceParameter(uint DeviceIndex, String ParamKey, String ParamVal) {
    dmsg(2,("LSCPServer: SetAudioOutputDeviceParameter(DeviceIndex=%d,ParamKey=%s,ParamVal=%s)\n",DeviceIndex,ParamKey.c_str(),ParamVal.c_str()));
    LSCPResultSet result;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no audio output device with index " + ToString(DeviceIndex) + ".");
        AudioOutputDevice* pDevice = devices[DeviceIndex];
        std::map<String,DeviceCreationParameter*> parameters = pDevice->DeviceParameters();
        if (!parameters.count(ParamKey)) throw Exception("Audio output device " + ToString(DeviceIndex) + " does not have a device parameter '" + ParamKey + "'");
        parameters[ParamKey]->SetValue(ParamVal);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_audio_device_info, DeviceIndex));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMidiInputDeviceParameter(uint DeviceIndex, String ParamKey, String ParamVal) {
    dmsg(2,("LSCPServer: SetMidiOutputDeviceParameter(DeviceIndex=%d,ParamKey=%s,ParamVal=%s)\n",DeviceIndex,ParamKey.c_str(),ParamVal.c_str()));
    LSCPResultSet result;
    try {
        std::map<uint,MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no MIDI input device with index " + ToString(DeviceIndex) + ".");
        MidiInputDevice* pDevice = devices[DeviceIndex];
        std::map<String,DeviceCreationParameter*> parameters = pDevice->DeviceParameters();
        if (!parameters.count(ParamKey)) throw Exception("MIDI input device " + ToString(DeviceIndex) + " does not have a device parameter '" + ParamKey + "'");
        parameters[ParamKey]->SetValue(ParamVal);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_device_info, DeviceIndex));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMidiInputPortParameter(uint DeviceIndex, uint PortIndex, String ParamKey, String ParamVal) {
    dmsg(2,("LSCPServer: SetMidiOutputDeviceParameter(DeviceIndex=%d,ParamKey=%s,ParamVal=%s)\n",DeviceIndex,ParamKey.c_str(),ParamVal.c_str()));
    LSCPResultSet result;
    try {
        // get MIDI input device
        std::map<uint,MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(DeviceIndex)) throw Exception("There is no MIDI input device with index " + ToString(DeviceIndex) + ".");
        MidiInputDevice* pDevice = devices[DeviceIndex];

        // get MIDI port
        MidiInputPort* pMidiInputPort = pDevice->GetPort(PortIndex);
        if (!pMidiInputPort) throw Exception("There is no MIDI input port with index " + ToString(PortIndex) + ".");

        // set port parameter value
        std::map<String,DeviceRuntimeParameter*> parameters = pMidiInputPort->PortParameters();
        if (!parameters.count(ParamKey)) throw Exception("MIDI input device " + ToString(PortIndex) + " does not have a parameter '" + ParamKey + "'");
        parameters[ParamKey]->SetValue(ParamVal);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_midi_device_info, DeviceIndex));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to change the audio output channel for
 * playback on a particular sampler channel.
 */
String LSCPServer::SetAudioOutputChannel(uint ChannelAudioOutputChannel, uint AudioOutputDeviceInputChannel, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetAudioOutputChannel(ChannelAudioOutputChannel=%d, AudioOutputDeviceInputChannel=%d, SamplerChannel=%d)\n",ChannelAudioOutputChannel,AudioOutputDeviceInputChannel,uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        EngineChannel* pEngineChannel = pSamplerChannel->GetEngineChannel();
        if (!pEngineChannel) throw Exception("No engine type yet assigned to sampler channel " + ToString(uiSamplerChannel));
        if (!pSamplerChannel->GetAudioOutputDevice()) throw Exception("No audio output device connected to sampler channel " + ToString(uiSamplerChannel));
        pEngineChannel->SetOutputChannel(ChannelAudioOutputChannel, AudioOutputDeviceInputChannel);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetAudioOutputDevice(uint AudioDeviceId, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetAudiotOutputDevice(AudioDeviceId=%d, SamplerChannel=%d)\n",AudioDeviceId,uiSamplerChannel));
    LSCPResultSet result;
    {
        LockGuard lock(RTNotifyMutex);
        try {
            SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
            if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
            std::map<uint, AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
            if (!devices.count(AudioDeviceId)) throw Exception("There is no audio output device with index " + ToString(AudioDeviceId));
            AudioOutputDevice* pDevice = devices[AudioDeviceId];
            pSamplerChannel->SetAudioOutputDevice(pDevice);
        }
        catch (Exception e) {
            result.Error(e);
        }
    }
    return result.Produce();
}

String LSCPServer::SetAudioOutputType(String AudioOutputDriver, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetAudioOutputType(String AudioOutputDriver=%s, SamplerChannel=%d)\n",AudioOutputDriver.c_str(),uiSamplerChannel));
    LSCPResultSet result;
    {
        LockGuard lock(RTNotifyMutex);
        try {
            SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
            if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
            // Driver type name aliasing...
            if (AudioOutputDriver == "Alsa") AudioOutputDriver = "ALSA";
            if (AudioOutputDriver == "Jack") AudioOutputDriver = "JACK";
            // Check if there's one audio output device already created
            // for the intended audio driver type (AudioOutputDriver)...
            AudioOutputDevice *pDevice = NULL;
            std::map<uint, AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
            std::map<uint, AudioOutputDevice*>::iterator iter = devices.begin();
            for (; iter != devices.end(); iter++) {
                if ((iter->second)->Driver() == AudioOutputDriver) {
                    pDevice = iter->second;
                    break;
                }
            }
            // If it doesn't exist, create a new one with default parameters...
            if (pDevice == NULL) {
                std::map<String,String> params;
                pDevice = pSampler->CreateAudioOutputDevice(AudioOutputDriver, params);
            }
            // Must have a device...
            if (pDevice == NULL)
                throw Exception("Internal error: could not create audio output device.");
            // Set it as the current channel device...
            pSamplerChannel->SetAudioOutputDevice(pDevice);
        }
        catch (Exception e) {
            result.Error(e);
        }
    }
    return result.Produce();
}

String LSCPServer::AddChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId, uint MIDIPort) {
    dmsg(2,("LSCPServer: AddChannelMidiInput(uiSamplerChannel=%d, MIDIDeviceId=%d, MIDIPort=%d)\n",uiSamplerChannel,MIDIDeviceId,MIDIPort));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));

        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(MIDIDeviceId)) throw Exception("There is no MIDI input device with index " + ToString(MIDIDeviceId));
        MidiInputDevice* pDevice = devices[MIDIDeviceId];

        MidiInputPort* pPort = pDevice->GetPort(MIDIPort);
        if (!pPort) throw Exception("There is no MIDI input port with index " + ToString(MIDIPort) + " on MIDI input device with index " + ToString(MIDIDeviceId));

        pSamplerChannel->Connect(pPort);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveChannelMidiInput(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: RemoveChannelMidiInput(uiSamplerChannel=%d)\n",uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        pSamplerChannel->DisconnectAllMidiInputPorts();
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId) {
    dmsg(2,("LSCPServer: RemoveChannelMidiInput(uiSamplerChannel=%d, MIDIDeviceId=%d)\n",uiSamplerChannel,MIDIDeviceId));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));

        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(MIDIDeviceId)) throw Exception("There is no MIDI input device with index " + ToString(MIDIDeviceId));
        MidiInputDevice* pDevice = devices[MIDIDeviceId];
        
        std::vector<MidiInputPort*> vPorts = pSamplerChannel->GetMidiInputPorts();
        for (int i = 0; i < vPorts.size(); ++i)
            if (vPorts[i]->GetDevice() == pDevice)
                pSamplerChannel->Disconnect(vPorts[i]);

    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId, uint MIDIPort) {
    dmsg(2,("LSCPServer: RemoveChannelMidiInput(uiSamplerChannel=%d, MIDIDeviceId=%d, MIDIPort=%d)\n",uiSamplerChannel,MIDIDeviceId,MIDIPort));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));

        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(MIDIDeviceId)) throw Exception("There is no MIDI input device with index " + ToString(MIDIDeviceId));
        MidiInputDevice* pDevice = devices[MIDIDeviceId];

        MidiInputPort* pPort = pDevice->GetPort(MIDIPort);
        if (!pPort) throw Exception("There is no MIDI input port with index " + ToString(MIDIPort) + " on MIDI input device with index " + ToString(MIDIDeviceId));

        pSamplerChannel->Disconnect(pPort);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListChannelMidiInputs(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: ListChannelMidiInputs(uiSamplerChannel=%d)\n",uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        std::vector<MidiInputPort*> vPorts = pSamplerChannel->GetMidiInputPorts();

        String s;
        for (int i = 0; i < vPorts.size(); ++i) {
            const int iDeviceID = vPorts[i]->GetDevice()->MidiInputDeviceID();
            const int iPortNr   = vPorts[i]->GetPortNumber();
            if (s.size()) s += ",";
            s += "{" + ToString(iDeviceID) + ","
                     + ToString(iPortNr) + "}";
        }
        result.Add(s);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMIDIInputPort(uint MIDIPort, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetMIDIInputPort(MIDIPort=%d, SamplerChannel=%d)\n",MIDIPort,uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        pSamplerChannel->SetMidiInputPort(MIDIPort);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMIDIInputChannel(uint MIDIChannel, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetMIDIInputChannel(MIDIChannel=%d, SamplerChannel=%d)\n",MIDIChannel,uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        pSamplerChannel->SetMidiInputChannel((midi_chan_t) MIDIChannel);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMIDIInputDevice(uint MIDIDeviceId, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetMIDIInputDevice(MIDIDeviceId=%d, SamplerChannel=%d)\n",MIDIDeviceId,uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        if (!devices.count(MIDIDeviceId)) throw Exception("There is no MIDI input device with index " + ToString(MIDIDeviceId));
        MidiInputDevice* pDevice = devices[MIDIDeviceId];
        pSamplerChannel->SetMidiInputDevice(pDevice);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMIDIInputType(String MidiInputDriver, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetMIDIInputType(String MidiInputDriver=%s, SamplerChannel=%d)\n",MidiInputDriver.c_str(),uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        // Driver type name aliasing...
        if (MidiInputDriver == "Alsa") MidiInputDriver = "ALSA";
        // Check if there's one MIDI input device already created
        // for the intended MIDI driver type (MidiInputDriver)...
        MidiInputDevice *pDevice = NULL;
        std::map<uint, MidiInputDevice*> devices = pSampler->GetMidiInputDevices();
        std::map<uint, MidiInputDevice*>::iterator iter = devices.begin();
        for (; iter != devices.end(); iter++) {
            if ((iter->second)->Driver() == MidiInputDriver) {
                pDevice = iter->second;
                break;
            }
        }
        // If it doesn't exist, create a new one with default parameters...
        if (pDevice == NULL) {
            std::map<String,String> params;
            pDevice = pSampler->CreateMidiInputDevice(MidiInputDriver, params);
            // Make it with at least one initial port.
            std::map<String,DeviceCreationParameter*> parameters = pDevice->DeviceParameters();
        }
        // Must have a device...
        if (pDevice == NULL)
            throw Exception("Internal error: could not create MIDI input device.");
        // Set it as the current channel device...
        pSamplerChannel->SetMidiInputDevice(pDevice);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to change the MIDI input device, port and channel on which
 * engine of a particular sampler channel should listen to.
 */
String LSCPServer::SetMIDIInput(uint MIDIDeviceId, uint MIDIPort, uint MIDIChannel, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetMIDIInput(MIDIDeviceId=%d, MIDIPort=%d, MIDIChannel=%d, SamplerChannel=%d)\n", MIDIDeviceId, MIDIPort, MIDIChannel, uiSamplerChannel));
    LSCPResultSet result;
    try {
        SamplerChannel* pSamplerChannel = pSampler->GetSamplerChannel(uiSamplerChannel);
        if (!pSamplerChannel) throw Exception("Invalid sampler channel number " + ToString(uiSamplerChannel));
        std::map<uint, MidiInputDevice*> devices =  pSampler->GetMidiInputDevices();
        if (!devices.count(MIDIDeviceId)) throw Exception("There is no MIDI input device with index " + ToString(MIDIDeviceId));
        MidiInputDevice* pDevice = devices[MIDIDeviceId];
        pSamplerChannel->SetMidiInput(pDevice, MIDIPort, (midi_chan_t) MIDIChannel);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to change the global volume factor on a
 * particular sampler channel.
 */
String LSCPServer::SetVolume(double dVolume, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetVolume(Volume=%f, SamplerChannel=%d)\n", dVolume, uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        pEngineChannel->Volume(dVolume);
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to mute/unmute particular sampler channel.
 */
String LSCPServer::SetChannelMute(bool bMute, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetChannelMute(bMute=%d,uiSamplerChannel=%d)\n",bMute,uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        if(!bMute) pEngineChannel->SetMute((HasSoloChannel() && !pEngineChannel->GetSolo()) ? -1 : 0);
        else pEngineChannel->SetMute(1);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to solo particular sampler channel.
 */
String LSCPServer::SetChannelSolo(bool bSolo, uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: SetChannelSolo(bSolo=%d,uiSamplerChannel=%d)\n",bSolo,uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        bool oldSolo = pEngineChannel->GetSolo();
        bool hadSoloChannel = HasSoloChannel();

        pEngineChannel->SetSolo(bSolo);

        if(!oldSolo && bSolo) {
            if(pEngineChannel->GetMute() == -1) pEngineChannel->SetMute(0);
            if(!hadSoloChannel) MuteNonSoloChannels();
        }

        if(oldSolo && !bSolo) {
            if(!HasSoloChannel()) UnmuteChannels();
            else if(!pEngineChannel->GetMute()) pEngineChannel->SetMute(-1);
        }
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Determines whether there is at least one solo channel in the channel list.
 *
 * @returns true if there is at least one solo channel in the channel list,
 * false otherwise.
 */
bool LSCPServer::HasSoloChannel() {
    std::map<uint,SamplerChannel*> channels = pSampler->GetSamplerChannels();
    std::map<uint,SamplerChannel*>::iterator iter = channels.begin();
    for (; iter != channels.end(); iter++) {
        EngineChannel* c = iter->second->GetEngineChannel();
        if(c && c->GetSolo()) return true;
    }

    return false;
}

/**
 * Mutes all unmuted non-solo channels. Notice that the channels are muted
 * with -1 which indicates that they are muted because of the presence
 * of a solo channel(s). Channels muted with -1 will be automatically unmuted
 * when there are no solo channels left.
 */
void LSCPServer::MuteNonSoloChannels() {
    dmsg(2,("LSCPServer: MuteNonSoloChannels()\n"));
    std::map<uint,SamplerChannel*> channels = pSampler->GetSamplerChannels();
    std::map<uint,SamplerChannel*>::iterator iter = channels.begin();
    for (; iter != channels.end(); iter++) {
        EngineChannel* c = iter->second->GetEngineChannel();
        if(c && !c->GetSolo() && !c->GetMute()) c->SetMute(-1);
    }
}

/**
 * Unmutes all channels that are muted because of the presence
 * of a solo channel(s).
 */
void  LSCPServer::UnmuteChannels() {
    dmsg(2,("LSCPServer: UnmuteChannels()\n"));
    std::map<uint,SamplerChannel*> channels = pSampler->GetSamplerChannels();
    std::map<uint,SamplerChannel*>::iterator iter = channels.begin();
    for (; iter != channels.end(); iter++) {
        EngineChannel* c = iter->second->GetEngineChannel();
        if(c && c->GetMute() == -1) c->SetMute(0);
    }
}

String LSCPServer::AddOrReplaceMIDIInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg, String EngineType, String InstrumentFile, uint InstrumentIndex, float Volume, MidiInstrumentMapper::mode_t LoadMode, String Name, bool bModal) {
    dmsg(2,("LSCPServer: AddOrReplaceMIDIInstrumentMapping()\n"));

    midi_prog_index_t idx;
    idx.midi_bank_msb = (MidiBank >> 7) & 0x7f;
    idx.midi_bank_lsb = MidiBank & 0x7f;
    idx.midi_prog     = MidiProg;

    MidiInstrumentMapper::entry_t entry;
    entry.EngineName      = EngineType;
    entry.InstrumentFile  = InstrumentFile;
    entry.InstrumentIndex = InstrumentIndex;
    entry.LoadMode        = LoadMode;
    entry.Volume          = Volume;
    entry.Name            = Name;

    LSCPResultSet result;
    try {
        // PERSISTENT mapping commands might block for a long time, so in
        // that case we add/replace the mapping in another thread in case
        // the NON_MODAL argument was supplied, non persistent mappings
        // should return immediately, so we don't need to do that for them
        bool bInBackground = (entry.LoadMode == MidiInstrumentMapper::PERSISTENT && !bModal);
        MidiInstrumentMapper::AddOrReplaceEntry(MidiMapID, idx, entry, bInBackground);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveMIDIInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg) {
    dmsg(2,("LSCPServer: RemoveMIDIInstrumentMapping()\n"));

    midi_prog_index_t idx;
    idx.midi_bank_msb = (MidiBank >> 7) & 0x7f;
    idx.midi_bank_lsb = MidiBank & 0x7f;
    idx.midi_prog     = MidiProg;

    LSCPResultSet result;
    try {
        MidiInstrumentMapper::RemoveEntry(MidiMapID, idx);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInstrumentMappings(uint MidiMapID) {
    dmsg(2,("LSCPServer: GetMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        result.Add(MidiInstrumentMapper::GetInstrumentCount(MidiMapID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}


String LSCPServer::GetAllMidiInstrumentMappings() {
    dmsg(2,("LSCPServer: GetAllMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        result.Add(MidiInstrumentMapper::GetInstrumentCount());
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg) {
    dmsg(2,("LSCPServer: GetMidiIstrumentMapping()\n"));
    LSCPResultSet result;
    try {
        MidiInstrumentMapper::entry_t entry = MidiInstrumentMapper::GetEntry(MidiMapID, MidiBank, MidiProg);
        // convert the filename into the correct encoding as defined for LSCP
        // (especially in terms of special characters -> escape sequences)
#if WIN32
        const String instrumentFileName = Path::fromWindows(entry.InstrumentFile).toLscp();
#else
        // assuming POSIX
        const String instrumentFileName = Path::fromPosix(entry.InstrumentFile).toLscp();
#endif

        result.Add("NAME", _escapeLscpResponse(entry.Name));
        result.Add("ENGINE_NAME", entry.EngineName);
        result.Add("INSTRUMENT_FILE", instrumentFileName);
        result.Add("INSTRUMENT_NR", (int) entry.InstrumentIndex);
        String instrumentName;
        Engine* pEngine = EngineFactory::Create(entry.EngineName);
        if (pEngine) {
            if (pEngine->GetInstrumentManager()) {
                InstrumentManager::instrument_id_t instrID;
                instrID.FileName = entry.InstrumentFile;
                instrID.Index    = entry.InstrumentIndex;
                instrumentName = pEngine->GetInstrumentManager()->GetInstrumentName(instrID);
            }
            EngineFactory::Destroy(pEngine);
        }
        result.Add("INSTRUMENT_NAME", _escapeLscpResponse(instrumentName));
        switch (entry.LoadMode) {
            case MidiInstrumentMapper::ON_DEMAND:
                result.Add("LOAD_MODE", "ON_DEMAND");
                break;
            case MidiInstrumentMapper::ON_DEMAND_HOLD:
                result.Add("LOAD_MODE", "ON_DEMAND_HOLD");
                break;
            case MidiInstrumentMapper::PERSISTENT:
                result.Add("LOAD_MODE", "PERSISTENT");
                break;
            default:
                throw Exception("entry reflects invalid LOAD_MODE, consider this as a bug!");
        }
        result.Add("VOLUME", entry.Volume);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListMidiInstrumentMappings(uint MidiMapID) {
    dmsg(2,("LSCPServer: ListMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        String s;
        std::map<midi_prog_index_t,MidiInstrumentMapper::entry_t> mappings = MidiInstrumentMapper::Entries(MidiMapID);
        std::map<midi_prog_index_t,MidiInstrumentMapper::entry_t>::iterator iter = mappings.begin();
        for (; iter != mappings.end(); iter++) {
            if (s.size()) s += ",";
            s += "{" + ToString(MidiMapID) + ","
                     + ToString((int(iter->first.midi_bank_msb) << 7) | int(iter->first.midi_bank_lsb)) + ","
                     + ToString(int(iter->first.midi_prog)) + "}";
        }
        result.Add(s);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListAllMidiInstrumentMappings() {
    dmsg(2,("LSCPServer: ListAllMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        std::vector<int> maps = MidiInstrumentMapper::Maps();
        String s;
        for (int i = 0; i < maps.size(); i++) {
            std::map<midi_prog_index_t,MidiInstrumentMapper::entry_t> mappings = MidiInstrumentMapper::Entries(maps[i]);
            std::map<midi_prog_index_t,MidiInstrumentMapper::entry_t>::iterator iter = mappings.begin();
            for (; iter != mappings.end(); iter++) {
                if (s.size()) s += ",";
                s += "{" + ToString(maps[i]) + ","
                         + ToString((int(iter->first.midi_bank_msb) << 7) | int(iter->first.midi_bank_lsb)) + ","
                         + ToString(int(iter->first.midi_prog)) + "}";
            }
        }
        result.Add(s);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ClearMidiInstrumentMappings(uint MidiMapID) {
    dmsg(2,("LSCPServer: ClearMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        MidiInstrumentMapper::RemoveAllEntries(MidiMapID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ClearAllMidiInstrumentMappings() {
    dmsg(2,("LSCPServer: ClearAllMidiInstrumentMappings()\n"));
    LSCPResultSet result;
    try {
        std::vector<int> maps = MidiInstrumentMapper::Maps();
        for (int i = 0; i < maps.size(); i++)
            MidiInstrumentMapper::RemoveAllEntries(maps[i]);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::AddMidiInstrumentMap(String MapName) {
    dmsg(2,("LSCPServer: AddMidiInstrumentMap()\n"));
    LSCPResultSet result;
    try {
        int MapID = MidiInstrumentMapper::AddMap(MapName);
        result = LSCPResultSet(MapID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveMidiInstrumentMap(uint MidiMapID) {
    dmsg(2,("LSCPServer: RemoveMidiInstrumentMap()\n"));
    LSCPResultSet result;
    try {
        MidiInstrumentMapper::RemoveMap(MidiMapID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveAllMidiInstrumentMaps() {
    dmsg(2,("LSCPServer: RemoveAllMidiInstrumentMaps()\n"));
    LSCPResultSet result;
    try {
        MidiInstrumentMapper::RemoveAllMaps();
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInstrumentMaps() {
    dmsg(2,("LSCPServer: GetMidiInstrumentMaps()\n"));
    LSCPResultSet result;
    try {
        result.Add(int(MidiInstrumentMapper::Maps().size()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListMidiInstrumentMaps() {
    dmsg(2,("LSCPServer: ListMidiInstrumentMaps()\n"));
    LSCPResultSet result;
    try {
        std::vector<int> maps = MidiInstrumentMapper::Maps();
        String sList;
        for (int i = 0; i < maps.size(); i++) {
            if (sList != "") sList += ",";
            sList += ToString(maps[i]);
        }
        result.Add(sList);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetMidiInstrumentMap(uint MidiMapID) {
    dmsg(2,("LSCPServer: GetMidiInstrumentMap()\n"));
    LSCPResultSet result;
    try {
        result.Add("NAME", _escapeLscpResponse(MidiInstrumentMapper::MapName(MidiMapID)));
        result.Add("DEFAULT", MidiInstrumentMapper::GetDefaultMap() == MidiMapID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetMidiInstrumentMapName(uint MidiMapID, String NewName) {
    dmsg(2,("LSCPServer: SetMidiInstrumentMapName()\n"));
    LSCPResultSet result;
    try {
        MidiInstrumentMapper::RenameMap(MidiMapID, NewName);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Set the MIDI instrument map the given sampler channel shall use for
 * handling MIDI program change messages. There are the following two
 * special (negative) values:
 *
 *    - (-1) :  set to NONE (ignore program changes)
 *    - (-2) :  set to DEFAULT map
 */
String LSCPServer::SetChannelMap(uint uiSamplerChannel, int MidiMapID) {
    dmsg(2,("LSCPServer: SetChannelMap()\n"));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        if      (MidiMapID == -1) pEngineChannel->SetMidiInstrumentMapToNone();
        else if (MidiMapID == -2) pEngineChannel->SetMidiInstrumentMapToDefault();
        else                      pEngineChannel->SetMidiInstrumentMap(MidiMapID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::CreateFxSend(uint uiSamplerChannel, uint MidiCtrl, String Name) {
    dmsg(2,("LSCPServer: CreateFxSend()\n"));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        FxSend* pFxSend = pEngineChannel->AddFxSend(MidiCtrl, Name);
        if (!pFxSend) throw Exception("Could not add FxSend, don't ask, I don't know why (probably a bug)");

        result = LSCPResultSet(pFxSend->Id()); // success
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::DestroyFxSend(uint uiSamplerChannel, uint FxSendID) {
    dmsg(2,("LSCPServer: DestroyFxSend()\n"));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        FxSend* pFxSend = NULL;
        for (int i = 0; i < pEngineChannel->GetFxSendCount(); i++) {
            if (pEngineChannel->GetFxSend(i)->Id() == FxSendID) {
                pFxSend = pEngineChannel->GetFxSend(i);
                break;
            }
        }
        if (!pFxSend) throw Exception("There is no FxSend with that ID on the given sampler channel");
        pEngineChannel->RemoveFxSend(pFxSend);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetFxSends(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: GetFxSends()\n"));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        result.Add(pEngineChannel->GetFxSendCount());
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListFxSends(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: ListFxSends()\n"));
    LSCPResultSet result;
    String list;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        for (int i = 0; i < pEngineChannel->GetFxSendCount(); i++) {
            FxSend* pFxSend = pEngineChannel->GetFxSend(i);
            if (list != "") list += ",";
            list += ToString(pFxSend->Id());
        }
        result.Add(list);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

FxSend* LSCPServer::GetFxSend(uint uiSamplerChannel, uint FxSendID) {
    EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

    FxSend* pFxSend = NULL;
    for (int i = 0; i < pEngineChannel->GetFxSendCount(); i++) {
        if (pEngineChannel->GetFxSend(i)->Id() == FxSendID) {
            pFxSend = pEngineChannel->GetFxSend(i);
            break;
        }
    }
    if (!pFxSend) throw Exception("There is no FxSend with that ID on the given sampler channel");
    return pFxSend;
}

String LSCPServer::GetFxSendInfo(uint uiSamplerChannel, uint FxSendID) {
    dmsg(2,("LSCPServer: GetFxSendInfo()\n"));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        // gather audio routing informations
        String AudioRouting;
        for (int chan = 0; chan < pEngineChannel->Channels(); chan++) {
            if (AudioRouting != "") AudioRouting += ",";
            AudioRouting += ToString(pFxSend->DestinationChannel(chan));
        }

        const String sEffectRouting =
            (pFxSend->DestinationEffectChain() >= 0 && pFxSend->DestinationEffectChainPosition() >= 0)
                ? ToString(pFxSend->DestinationEffectChain()) + "," + ToString(pFxSend->DestinationEffectChainPosition())
                : "NONE";

        // success
        result.Add("NAME", _escapeLscpResponse(pFxSend->Name()));
        result.Add("MIDI_CONTROLLER", pFxSend->MidiController());
        result.Add("LEVEL", ToString(pFxSend->Level()));
        result.Add("AUDIO_OUTPUT_ROUTING", AudioRouting);
        result.Add("EFFECT", sEffectRouting);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetFxSendName(uint uiSamplerChannel, uint FxSendID, String Name) {
    dmsg(2,("LSCPServer: SetFxSendName()\n"));
    LSCPResultSet result;
    try {
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        pFxSend->SetName(Name);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, uiSamplerChannel, FxSendID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetFxSendAudioOutputChannel(uint uiSamplerChannel, uint FxSendID, uint FxSendChannel, uint DeviceChannel) {
    dmsg(2,("LSCPServer: SetFxSendAudioOutputChannel()\n"));
    LSCPResultSet result;
    try {
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        pFxSend->SetDestinationChannel(FxSendChannel, DeviceChannel);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, uiSamplerChannel, FxSendID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetFxSendMidiController(uint uiSamplerChannel, uint FxSendID, uint MidiController) {
    dmsg(2,("LSCPServer: SetFxSendMidiController()\n"));
    LSCPResultSet result;
    try {
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        pFxSend->SetMidiController(MidiController);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, uiSamplerChannel, FxSendID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetFxSendLevel(uint uiSamplerChannel, uint FxSendID, double dLevel) {
    dmsg(2,("LSCPServer: SetFxSendLevel()\n"));
    LSCPResultSet result;
    try {
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        pFxSend->SetLevel((float)dLevel);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, uiSamplerChannel, FxSendID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetFxSendEffect(uint uiSamplerChannel, uint FxSendID, int iSendEffectChain, int iEffectChainPosition) {
    dmsg(2,("LSCPServer: SetFxSendEffect(%d,%d)\n", iSendEffectChain, iEffectChainPosition));
    LSCPResultSet result;
    try {
        FxSend* pFxSend = GetFxSend(uiSamplerChannel, FxSendID);

        pFxSend->SetDestinationEffect(iSendEffectChain, iEffectChainPosition);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_send_info, uiSamplerChannel, FxSendID));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetAvailableEffects() {
    dmsg(2,("LSCPServer: GetAvailableEffects()\n"));
    LSCPResultSet result;
    try {
        int n = EffectFactory::AvailableEffectsCount();
        result.Add(n);
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListAvailableEffects() {
    dmsg(2,("LSCPServer: ListAvailableEffects()\n"));
    LSCPResultSet result;
    String list;
    try {
        //FIXME: for now we simply enumerate from 0 .. EffectFactory::AvailableEffectsCount() here, in future we should use unique IDs for effects during the whole sampler session. This issue comes into game when the user forces a reload of available effect plugins
        int n = EffectFactory::AvailableEffectsCount();
        for (int i = 0; i < n; i++) {
            if (i) list += ",";
            list += ToString(i);
        }
    }
    catch (Exception e) {
        result.Error(e);
    }
    result.Add(list);
    return result.Produce();
}

String LSCPServer::GetEffectInfo(int iEffectIndex) {
    dmsg(2,("LSCPServer: GetEffectInfo(%d)\n", iEffectIndex));
    LSCPResultSet result;
    try {
        EffectInfo* pEffectInfo = EffectFactory::GetEffectInfo(iEffectIndex);
        if (!pEffectInfo)
            throw Exception("There is no effect with index " + ToString(iEffectIndex));

        // convert the filename into the correct encoding as defined for LSCP
        // (especially in terms of special characters -> escape sequences)
#if WIN32
        const String dllFileName = Path::fromWindows(pEffectInfo->Module()).toLscp();
#else
        // assuming POSIX
        const String dllFileName = Path::fromPosix(pEffectInfo->Module()).toLscp();
#endif

        result.Add("SYSTEM", pEffectInfo->EffectSystem());
        result.Add("MODULE", dllFileName);
        result.Add("NAME", _escapeLscpResponse(pEffectInfo->Name()));
        result.Add("DESCRIPTION", _escapeLscpResponse(pEffectInfo->Description()));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();    
}

String LSCPServer::GetEffectInstanceInfo(int iEffectInstance) {
    dmsg(2,("LSCPServer: GetEffectInstanceInfo(%d)\n", iEffectInstance));
    LSCPResultSet result;
    try {
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with ID " + ToString(iEffectInstance));

        EffectInfo* pEffectInfo = pEffect->GetEffectInfo();

        // convert the filename into the correct encoding as defined for LSCP
        // (especially in terms of special characters -> escape sequences)
#if WIN32
        const String dllFileName = Path::fromWindows(pEffectInfo->Module()).toLscp();
#else
        // assuming POSIX
        const String dllFileName = Path::fromPosix(pEffectInfo->Module()).toLscp();
#endif

        result.Add("SYSTEM", pEffectInfo->EffectSystem());
        result.Add("MODULE", dllFileName);
        result.Add("NAME", _escapeLscpResponse(pEffectInfo->Name()));
        result.Add("DESCRIPTION", _escapeLscpResponse(pEffectInfo->Description()));
        result.Add("INPUT_CONTROLS", ToString(pEffect->InputControlCount()));
    }
    catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetEffectInstanceInputControlInfo(int iEffectInstance, int iInputControlIndex) {
    dmsg(2,("LSCPServer: GetEffectInstanceInputControlInfo(%d,%d)\n", iEffectInstance, iInputControlIndex));
    LSCPResultSet result;
    try {
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with ID " + ToString(iEffectInstance));

        EffectControl* pEffectControl = pEffect->InputControl(iInputControlIndex);
        if (!pEffectControl)
            throw Exception(
                "Effect instance " + ToString(iEffectInstance) +
                " does not have an input control with index " +
                ToString(iInputControlIndex)
            );

        result.Add("DESCRIPTION", _escapeLscpResponse(pEffectControl->Description()));
        result.Add("VALUE", pEffectControl->Value());
        if (pEffectControl->MinValue())
             result.Add("RANGE_MIN", *pEffectControl->MinValue());
        if (pEffectControl->MaxValue())
             result.Add("RANGE_MAX", *pEffectControl->MaxValue());
        if (!pEffectControl->Possibilities().empty())
             result.Add("POSSIBILITIES", pEffectControl->Possibilities());
        if (pEffectControl->DefaultValue())
             result.Add("DEFAULT", *pEffectControl->DefaultValue());
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetEffectInstanceInputControlValue(int iEffectInstance, int iInputControlIndex, double dValue) {
    dmsg(2,("LSCPServer: SetEffectInstanceInputControlValue(%d,%d,%f)\n", iEffectInstance, iInputControlIndex, dValue));
    LSCPResultSet result;
    try {
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with ID " + ToString(iEffectInstance));

        EffectControl* pEffectControl = pEffect->InputControl(iInputControlIndex);
        if (!pEffectControl)
            throw Exception(
                "Effect instance " + ToString(iEffectInstance) +
                " does not have an input control with index " +
                ToString(iInputControlIndex)
            );

        pEffectControl->SetValue(dValue);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_instance_info, iEffectInstance));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::CreateEffectInstance(int iEffectIndex) {
    dmsg(2,("LSCPServer: CreateEffectInstance(%d)\n", iEffectIndex));
    LSCPResultSet result;
    try {
        EffectInfo* pEffectInfo = EffectFactory::GetEffectInfo(iEffectIndex);
        if (!pEffectInfo)
            throw Exception("There is no effect with index " + ToString(iEffectIndex));
        Effect* pEffect = EffectFactory::Create(pEffectInfo);
        result = pEffect->ID(); // success
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_instance_count, EffectFactory::EffectInstancesCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::CreateEffectInstance(String effectSystem, String module, String effectName) {
    dmsg(2,("LSCPServer: CreateEffectInstance('%s','%s','%s')\n", effectSystem.c_str(), module.c_str(), effectName.c_str()));
    LSCPResultSet result;
    try {
        // to allow loading the same LSCP session file on different systems
        // successfully, probably with different effect plugin DLL paths or even
        // running completely different operating systems, we do the following
        // for finding the right effect:
        //
        // first try to search for an exact match of the effect plugin DLL
        // (a.k.a 'module'), to avoid picking the wrong DLL with the same
        // effect name ...
        EffectInfo* pEffectInfo = EffectFactory::GetEffectInfo(effectSystem, module, effectName, EffectFactory::MODULE_MATCH_EXACTLY);
        // ... if no effect with exactly matchin DLL filename was found, then
        // try to lower the restrictions of matching the effect plugin DLL
        // filename and try again and again ...
        if (!pEffectInfo) {
            dmsg(2,("no exact module match, trying MODULE_IGNORE_PATH\n"));
            pEffectInfo = EffectFactory::GetEffectInfo(effectSystem, module, effectName, EffectFactory::MODULE_IGNORE_PATH);
        }
        if (!pEffectInfo) {
            dmsg(2,("no module match, trying MODULE_IGNORE_PATH | MODULE_IGNORE_CASE\n"));
            pEffectInfo = EffectFactory::GetEffectInfo(effectSystem, module, effectName, EffectFactory::MODULE_IGNORE_PATH | EffectFactory::MODULE_IGNORE_CASE);
        }
        if (!pEffectInfo) {
            dmsg(2,("no module match, trying MODULE_IGNORE_PATH | MODULE_IGNORE_CASE | MODULE_IGNORE_EXTENSION\n"));
            pEffectInfo = EffectFactory::GetEffectInfo(effectSystem, module, effectName, EffectFactory::MODULE_IGNORE_PATH | EffectFactory::MODULE_IGNORE_CASE | EffectFactory::MODULE_IGNORE_EXTENSION);
        }
        // ... if there was still no effect found, then completely ignore the
        // DLL plugin filename argument and just search for the matching effect
        // system type and effect name
        if (!pEffectInfo) {
            dmsg(2,("no module match, trying MODULE_IGNORE_ALL\n"));
            pEffectInfo = EffectFactory::GetEffectInfo(effectSystem, module, effectName, EffectFactory::MODULE_IGNORE_ALL);
        }
        if (!pEffectInfo)
            throw Exception("There is no such effect '" + effectSystem + "' '" + module + "' '" + effectName + "'");

        Effect* pEffect = EffectFactory::Create(pEffectInfo);
        result = LSCPResultSet(pEffect->ID());
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_instance_count, EffectFactory::EffectInstancesCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::DestroyEffectInstance(int iEffectInstance) {
    dmsg(2,("LSCPServer: DestroyEffectInstance(%d)\n", iEffectInstance));
    LSCPResultSet result;
    try {
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with ID " + ToString(iEffectInstance));
        EffectFactory::Destroy(pEffect);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_fx_instance_count, EffectFactory::EffectInstancesCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetEffectInstances() {
    dmsg(2,("LSCPServer: GetEffectInstances()\n"));
    LSCPResultSet result;
    try {
        int n = EffectFactory::EffectInstancesCount();
        result.Add(n);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListEffectInstances() {
    dmsg(2,("LSCPServer: ListEffectInstances()\n"));
    LSCPResultSet result;
    String list;
    try {
        int n = EffectFactory::EffectInstancesCount();
        for (int i = 0; i < n; i++) {
            Effect* pEffect = EffectFactory::GetEffectInstance(i);
            if (i) list += ",";
            list += ToString(pEffect->ID());
        }
    } catch (Exception e) {
        result.Error(e);
    }
    result.Add(list);
    return result.Produce();
}

String LSCPServer::GetSendEffectChains(int iAudioOutputDevice) {
    dmsg(2,("LSCPServer: GetSendEffectChains(%d)\n", iAudioOutputDevice));
    LSCPResultSet result;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(iAudioOutputDevice))
            throw Exception("There is no audio output device with index " + ToString(iAudioOutputDevice) + ".");
        AudioOutputDevice* pDevice = devices[iAudioOutputDevice];
        int n = pDevice->SendEffectChainCount();
        result.Add(n);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::ListSendEffectChains(int iAudioOutputDevice) {
    dmsg(2,("LSCPServer: ListSendEffectChains(%d)\n", iAudioOutputDevice));
    LSCPResultSet result;
    String list;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(iAudioOutputDevice))
            throw Exception("There is no audio output device with index " + ToString(iAudioOutputDevice) + ".");
        AudioOutputDevice* pDevice = devices[iAudioOutputDevice];
        int n = pDevice->SendEffectChainCount();
        for (int i = 0; i < n; i++) {
            EffectChain* pEffectChain = pDevice->SendEffectChain(i);
            if (i) list += ",";
            list += ToString(pEffectChain->ID());
        }
    } catch (Exception e) {
        result.Error(e);
    }
    result.Add(list);
    return result.Produce();
}

String LSCPServer::AddSendEffectChain(int iAudioOutputDevice) {
    dmsg(2,("LSCPServer: AddSendEffectChain(%d)\n", iAudioOutputDevice));
    LSCPResultSet result;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(iAudioOutputDevice))
            throw Exception("There is no audio output device with index " + ToString(iAudioOutputDevice) + ".");
        AudioOutputDevice* pDevice = devices[iAudioOutputDevice];
        EffectChain* pEffectChain = pDevice->AddSendEffectChain();
        result = pEffectChain->ID();
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_send_fx_chain_count, iAudioOutputDevice, pDevice->SendEffectChainCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveSendEffectChain(int iAudioOutputDevice, int iSendEffectChain) {
    dmsg(2,("LSCPServer: RemoveSendEffectChain(%d,%d)\n", iAudioOutputDevice, iSendEffectChain));
    LSCPResultSet result;
    try {
        std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
        if (!devices.count(iAudioOutputDevice))
            throw Exception("There is no audio output device with index " + ToString(iAudioOutputDevice) + ".");

        std::set<EngineChannel*> engineChannels = EngineChannelFactory::EngineChannelInstances();
        std::set<EngineChannel*>::iterator itEngineChannel = engineChannels.begin();
        std::set<EngineChannel*>::iterator itEnd           = engineChannels.end();
        for (; itEngineChannel != itEnd; ++itEngineChannel) {
            AudioOutputDevice* pDev = (*itEngineChannel)->GetAudioOutputDevice();
            if (pDev != NULL && pDev->deviceId() == iAudioOutputDevice) {
                for (int i = 0; i < (*itEngineChannel)->GetFxSendCount(); i++) {
                    FxSend* fxs = (*itEngineChannel)->GetFxSend(i);
                    if(fxs != NULL && fxs->DestinationEffectChain() == iSendEffectChain) {
                        throw Exception("The effect chain is still in use by channel " + ToString((*itEngineChannel)->GetSamplerChannel()->Index()));
                    }
                }
            }
        }

        AudioOutputDevice* pDevice = devices[iAudioOutputDevice];
        for (int i = 0; i < pDevice->SendEffectChainCount(); i++) {
            EffectChain* pEffectChain = pDevice->SendEffectChain(i);
            if (pEffectChain->ID() == iSendEffectChain) {
                pDevice->RemoveSendEffectChain(i);
                LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_send_fx_chain_count, iAudioOutputDevice, pDevice->SendEffectChainCount()));
                return result.Produce();
            }
        }
        throw Exception(
            "There is no send effect chain with ID " +
            ToString(iSendEffectChain) + " for audio output device " +
            ToString(iAudioOutputDevice) + "."
        );
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

static EffectChain* _getSendEffectChain(Sampler* pSampler, int iAudioOutputDevice, int iSendEffectChain) throw (Exception) {
    std::map<uint,AudioOutputDevice*> devices = pSampler->GetAudioOutputDevices();
    if (!devices.count(iAudioOutputDevice))
        throw Exception(
            "There is no audio output device with index " +
            ToString(iAudioOutputDevice) + "."
        );
    AudioOutputDevice* pDevice = devices[iAudioOutputDevice];
    EffectChain* pEffectChain = pDevice->SendEffectChainByID(iSendEffectChain);
    if(pEffectChain != NULL) return pEffectChain;
    throw Exception(
        "There is no send effect chain with ID " +
        ToString(iSendEffectChain) + " for audio output device " +
        ToString(iAudioOutputDevice) + "."
    );
}

String LSCPServer::GetSendEffectChainInfo(int iAudioOutputDevice, int iSendEffectChain) {
    dmsg(2,("LSCPServer: GetSendEffectChainInfo(%d,%d)\n", iAudioOutputDevice, iSendEffectChain));
    LSCPResultSet result;
    try {
        EffectChain* pEffectChain =
            _getSendEffectChain(pSampler, iAudioOutputDevice, iSendEffectChain);
        String sEffectSequence;
        for (int i = 0; i < pEffectChain->EffectCount(); i++) {
            if (i) sEffectSequence += ",";
            sEffectSequence += ToString(pEffectChain->GetEffect(i)->ID());
        }
        result.Add("EFFECT_COUNT", pEffectChain->EffectCount());
        result.Add("EFFECT_SEQUENCE", sEffectSequence);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::AppendSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectInstance) {
    dmsg(2,("LSCPServer: AppendSendEffectChainEffect(%d,%d,%d)\n", iAudioOutputDevice, iSendEffectChain, iEffectInstance));
    LSCPResultSet result;
    try {
        EffectChain* pEffectChain =
            _getSendEffectChain(pSampler, iAudioOutputDevice, iSendEffectChain);
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with ID " + ToString(iEffectInstance));
        pEffectChain->AppendEffect(pEffect);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_send_fx_chain_info, iAudioOutputDevice, iSendEffectChain, pEffectChain->EffectCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::InsertSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectChainPosition, int iEffectInstance) {
    dmsg(2,("LSCPServer: InsertSendEffectChainEffect(%d,%d,%d,%d)\n", iAudioOutputDevice, iSendEffectChain, iEffectChainPosition, iEffectInstance));
    LSCPResultSet result;
    try {
        EffectChain* pEffectChain =
            _getSendEffectChain(pSampler, iAudioOutputDevice, iSendEffectChain);
        Effect* pEffect = EffectFactory::GetEffectInstanceByID(iEffectInstance);
        if (!pEffect)
            throw Exception("There is no effect instance with index " + ToString(iEffectInstance));
        pEffectChain->InsertEffect(pEffect, iEffectChainPosition);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_send_fx_chain_info, iAudioOutputDevice, iSendEffectChain, pEffectChain->EffectCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::RemoveSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectChainPosition) {
    dmsg(2,("LSCPServer: RemoveSendEffectChainEffect(%d,%d,%d)\n", iAudioOutputDevice, iSendEffectChain, iEffectChainPosition));
    LSCPResultSet result;
    try {
        EffectChain* pEffectChain =
            _getSendEffectChain(pSampler, iAudioOutputDevice, iSendEffectChain);

        std::set<EngineChannel*> engineChannels = EngineChannelFactory::EngineChannelInstances();
        std::set<EngineChannel*>::iterator itEngineChannel = engineChannels.begin();
        std::set<EngineChannel*>::iterator itEnd           = engineChannels.end();
        for (; itEngineChannel != itEnd; ++itEngineChannel) {
            AudioOutputDevice* pDev = (*itEngineChannel)->GetAudioOutputDevice();
            if (pDev != NULL && pDev->deviceId() == iAudioOutputDevice) {
                for (int i = 0; i < (*itEngineChannel)->GetFxSendCount(); i++) {
                    FxSend* fxs = (*itEngineChannel)->GetFxSend(i);
                    if(fxs != NULL && fxs->DestinationEffectChain() == iSendEffectChain && fxs->DestinationEffectChainPosition() == iEffectChainPosition) {
                        throw Exception("The effect instance is still in use by channel " + ToString((*itEngineChannel)->GetSamplerChannel()->Index()));
                    }
                }
            }
        }

        pEffectChain->RemoveEffect(iEffectChainPosition);
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_send_fx_chain_info, iAudioOutputDevice, iSendEffectChain, pEffectChain->EffectCount()));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::EditSamplerChannelInstrument(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: EditSamplerChannelInstrument(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        if (pEngineChannel->InstrumentStatus() < 0) throw Exception("No instrument loaded to sampler channel");
        Engine* pEngine = pEngineChannel->GetEngine();
        InstrumentManager* pInstrumentManager = pEngine->GetInstrumentManager();
        if (!pInstrumentManager) throw Exception("Engine does not provide an instrument manager");
        InstrumentManager::instrument_id_t instrumentID;
        instrumentID.FileName = pEngineChannel->InstrumentFileName();
        instrumentID.Index    = pEngineChannel->InstrumentIndex();
        pInstrumentManager->LaunchInstrumentEditor(pEngineChannel, instrumentID);
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SendChannelMidiData(String MidiMsg, uint uiSamplerChannel, uint Arg1, uint Arg2) {
    dmsg(2,("LSCPServer: SendChannelMidiData(MidiMsg=%s,uiSamplerChannel=%d,Arg1=%d,Arg2=%d)\n", MidiMsg.c_str(), uiSamplerChannel, Arg1, Arg2));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);

        if (Arg1 > 127 || Arg2 > 127) {
            throw Exception("Invalid MIDI message");
        }

        VirtualMidiDevice* pMidiDevice = NULL;
        std::vector<EventHandler::midi_listener_entry>::iterator iter = eventHandler.channelMidiListeners.begin();
        for (; iter != eventHandler.channelMidiListeners.end(); ++iter) {
            if ((*iter).pEngineChannel == pEngineChannel) {
                pMidiDevice = (*iter).pMidiListener;
                break;
            }
        }
        
        if(pMidiDevice == NULL) throw Exception("Couldn't find virtual MIDI device");

        if (MidiMsg == "NOTE_ON") {
            pMidiDevice->SendNoteOnToDevice(Arg1, Arg2);
            bool b = pMidiDevice->SendNoteOnToSampler(Arg1, Arg2);
            if (!b) throw Exception("MIDI event failed: " + MidiMsg + " " + ToString(Arg1) + " " + ToString(Arg2));
        } else if (MidiMsg == "NOTE_OFF") {
            pMidiDevice->SendNoteOffToDevice(Arg1, Arg2);
            bool b = pMidiDevice->SendNoteOffToSampler(Arg1, Arg2);
            if (!b) throw Exception("MIDI event failed: " + MidiMsg + " " + ToString(Arg1) + " " + ToString(Arg2));
        } else if (MidiMsg == "CC") {
            pMidiDevice->SendCCToDevice(Arg1, Arg2);
            bool b = pMidiDevice->SendCCToSampler(Arg1, Arg2);
            if (!b) throw Exception("MIDI event failed: " + MidiMsg + " " + ToString(Arg1) + " " + ToString(Arg2));
        } else {
            throw Exception("Unknown MIDI message type: " + MidiMsg);
        }
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to reset a particular sampler channel.
 */
String LSCPServer::ResetChannel(uint uiSamplerChannel) {
    dmsg(2,("LSCPServer: ResetChannel(SamplerChannel=%d)\n", uiSamplerChannel));
    LSCPResultSet result;
    try {
        EngineChannel* pEngineChannel = GetEngineChannel(uiSamplerChannel);
        pEngineChannel->Reset();
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to reset the whole sampler.
 */
String LSCPServer::ResetSampler() {
    dmsg(2,("LSCPServer: ResetSampler()\n"));
    pSampler->Reset();
    LSCPResultSet result;
    return result.Produce();
}

/**
 * Will be called by the parser to return general informations about this
 * sampler.
 */
String LSCPServer::GetServerInfo() {
    dmsg(2,("LSCPServer: GetServerInfo()\n"));
    const std::string description =
        _escapeLscpResponse("LinuxSampler - modular, streaming capable sampler");
    LSCPResultSet result;
    result.Add("DESCRIPTION", description);
    result.Add("VERSION", VERSION);
    result.Add("PROTOCOL_VERSION", ToString(LSCP_RELEASE_MAJOR) + "." + ToString(LSCP_RELEASE_MINOR));
#if HAVE_SQLITE3
    result.Add("INSTRUMENTS_DB_SUPPORT", "yes");
#else
    result.Add("INSTRUMENTS_DB_SUPPORT", "no");
#endif

    return result.Produce();
}

/**
 * Will be called by the parser to return the current number of all active streams.
 */
String LSCPServer::GetTotalStreamCount() {
    dmsg(2,("LSCPServer: GetTotalStreamCount()\n"));
    LSCPResultSet result;
    result.Add(pSampler->GetDiskStreamCount());
    return result.Produce();
}

/**
 * Will be called by the parser to return the current number of all active voices.
 */
String LSCPServer::GetTotalVoiceCount() {
    dmsg(2,("LSCPServer: GetTotalVoiceCount()\n"));
    LSCPResultSet result;
    result.Add(pSampler->GetVoiceCount());
    return result.Produce();
}

/**
 * Will be called by the parser to return the maximum number of voices.
 */
String LSCPServer::GetTotalVoiceCountMax() {
    dmsg(2,("LSCPServer: GetTotalVoiceCountMax()\n"));
    LSCPResultSet result;
    result.Add(int(EngineFactory::EngineInstances().size() * pSampler->GetGlobalMaxVoices()));
    return result.Produce();
}

/**
 * Will be called by the parser to return the sampler global maximum
 * allowed number of voices.
 */
String LSCPServer::GetGlobalMaxVoices() {
    dmsg(2,("LSCPServer: GetGlobalMaxVoices()\n"));
    LSCPResultSet result;
    result.Add(pSampler->GetGlobalMaxVoices());
    return result.Produce();
}

/**
 * Will be called by the parser to set the sampler global maximum number of
 * voices.
 */
String LSCPServer::SetGlobalMaxVoices(int iVoices) {
    dmsg(2,("LSCPServer: SetGlobalMaxVoices(%d)\n", iVoices));
    LSCPResultSet result;
    try {
        pSampler->SetGlobalMaxVoices(iVoices);
        LSCPServer::SendLSCPNotify(
            LSCPEvent(LSCPEvent::event_global_info, "VOICES", pSampler->GetGlobalMaxVoices())
        );
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to return the sampler global maximum
 * allowed number of disk streams.
 */
String LSCPServer::GetGlobalMaxStreams() {
    dmsg(2,("LSCPServer: GetGlobalMaxStreams()\n"));
    LSCPResultSet result;
    result.Add(pSampler->GetGlobalMaxStreams());
    return result.Produce();
}

/**
 * Will be called by the parser to set the sampler global maximum number of
 * disk streams.
 */
String LSCPServer::SetGlobalMaxStreams(int iStreams) {
    dmsg(2,("LSCPServer: SetGlobalMaxStreams(%d)\n", iStreams));
    LSCPResultSet result;
    try {
        pSampler->SetGlobalMaxStreams(iStreams);
        LSCPServer::SendLSCPNotify(
            LSCPEvent(LSCPEvent::event_global_info, "STREAMS", pSampler->GetGlobalMaxStreams())
        );
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetGlobalVolume() {
    LSCPResultSet result;
    result.Add(ToString(GLOBAL_VOLUME)); // see common/global.cpp
    return result.Produce();
}

String LSCPServer::SetGlobalVolume(double dVolume) {
    LSCPResultSet result;
    try {
        if (dVolume < 0) throw Exception("Volume may not be negative");
        GLOBAL_VOLUME = dVolume; // see common/global_private.cpp
        LSCPServer::SendLSCPNotify(LSCPEvent(LSCPEvent::event_global_info, "VOLUME", GLOBAL_VOLUME));
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::GetFileInstruments(String Filename) {
    dmsg(2,("LSCPServer: GetFileInstruments(String Filename=%s)\n",Filename.c_str()));
    LSCPResultSet result;
    try {
        VerifyFile(Filename);
    } catch (Exception e) {
        result.Error(e);
        return result.Produce();
    }
    // try to find a sampler engine that can handle the file
    bool bFound = false;
    std::vector<String> engineTypes = EngineFactory::AvailableEngineTypes();
    for (int i = 0; !bFound && i < engineTypes.size(); i++) {
        Engine* pEngine = NULL;
        try {
            pEngine = EngineFactory::Create(engineTypes[i]);
            if (!pEngine) throw Exception("Internal error: could not create '" + engineTypes[i] + "' engine");
            InstrumentManager* pManager = pEngine->GetInstrumentManager();
            if (pManager) {
                std::vector<InstrumentManager::instrument_id_t> IDs =
                    pManager->GetInstrumentFileContent(Filename);
                // return the amount of instruments in the file
                result.Add((int)IDs.size());
                // no more need to ask other engine types
                bFound = true;
            } else dmsg(1,("Warning: engine '%s' does not provide an instrument manager\n", engineTypes[i].c_str()));
        } catch (Exception e) {
            // NOOP, as exception is thrown if engine doesn't support file
        }
        if (pEngine) EngineFactory::Destroy(pEngine);
    }

    if (!bFound) result.Error("Unknown file format");
    return result.Produce();
}

String LSCPServer::ListFileInstruments(String Filename) {
    dmsg(2,("LSCPServer: ListFileInstruments(String Filename=%s)\n",Filename.c_str()));
    LSCPResultSet result;
    try {
        VerifyFile(Filename);
    } catch (Exception e) {
        result.Error(e);
        return result.Produce();
    }
    // try to find a sampler engine that can handle the file
    bool bFound = false;
    std::vector<String> engineTypes = EngineFactory::AvailableEngineTypes();
    for (int i = 0; !bFound && i < engineTypes.size(); i++) {
        Engine* pEngine = NULL;
        try {
            pEngine = EngineFactory::Create(engineTypes[i]);
            if (!pEngine) throw Exception("Internal error: could not create '" + engineTypes[i] + "' engine");
            InstrumentManager* pManager = pEngine->GetInstrumentManager();
            if (pManager) {
                std::vector<InstrumentManager::instrument_id_t> IDs =
                    pManager->GetInstrumentFileContent(Filename);
                // return a list of IDs of the instruments in the file
                String s;
                for (int j = 0; j < IDs.size(); j++) {
                    if (s.size()) s += ",";
                    s += ToString(IDs[j].Index);
                }
                result.Add(s);
                // no more need to ask other engine types
                bFound = true;
            } else dmsg(1,("Warning: engine '%s' does not provide an instrument manager\n", engineTypes[i].c_str()));
        } catch (Exception e) {
            // NOOP, as exception is thrown if engine doesn't support file
        }
        if (pEngine) EngineFactory::Destroy(pEngine);
    }

    if (!bFound) result.Error("Unknown file format");
    return result.Produce();
}

String LSCPServer::GetFileInstrumentInfo(String Filename, uint InstrumentID) {
    dmsg(2,("LSCPServer: GetFileInstrumentInfo(String Filename=%s, InstrumentID=%d)\n",Filename.c_str(),InstrumentID));
    LSCPResultSet result;
    try {
        VerifyFile(Filename);
    } catch (Exception e) {
        result.Error(e);
        return result.Produce();
    }
    InstrumentManager::instrument_id_t id;
    id.FileName = Filename;
    id.Index    = InstrumentID;
    // try to find a sampler engine that can handle the file
    bool bFound = false;
    bool bFatalErr = false;
    std::vector<String> engineTypes = EngineFactory::AvailableEngineTypes();
    for (int i = 0; !bFound && !bFatalErr && i < engineTypes.size(); i++) {
        Engine* pEngine = NULL;
        try {
            pEngine = EngineFactory::Create(engineTypes[i]);
            if (!pEngine) throw Exception("Internal error: could not create '" + engineTypes[i] + "' engine");
            InstrumentManager* pManager = pEngine->GetInstrumentManager();
            if (pManager) {
                // check if the instrument index is valid
                // FIXME: this won't work if an engine only supports parts of the instrument file
                std::vector<InstrumentManager::instrument_id_t> IDs =
                    pManager->GetInstrumentFileContent(Filename);
                if (std::find(IDs.begin(), IDs.end(), id) == IDs.end()) {
                    std::stringstream ss;
                    ss << "Invalid instrument index " << InstrumentID << " for instrument file '" << Filename << "'";
                    bFatalErr = true;
                    throw Exception(ss.str());
                }
                // get the info of the requested instrument
                InstrumentManager::instrument_info_t info =
                    pManager->GetInstrumentInfo(id);
                // return detailed informations about the file
                result.Add("NAME", info.InstrumentName);
                result.Add("FORMAT_FAMILY", engineTypes[i]);
                result.Add("FORMAT_VERSION", info.FormatVersion);
                result.Add("PRODUCT", info.Product);
                result.Add("ARTISTS", info.Artists);

                std::stringstream ss;
                bool b = false;
                for (int i = 0; i < 128; i++) {
                    if (info.KeyBindings[i]) {
                        if (b) ss << ',';
                        ss << i; b = true;
                    }
                }
                result.Add("KEY_BINDINGS", ss.str());

                b = false;
                std::stringstream ss2;
                for (int i = 0; i < 128; i++) {
                    if (info.KeySwitchBindings[i]) {
                        if (b) ss2 << ',';
                        ss2 << i; b = true;
                    }
                }
                result.Add("KEYSWITCH_BINDINGS", ss2.str());
                // no more need to ask other engine types
                bFound = true;
            } else dmsg(1,("Warning: engine '%s' does not provide an instrument manager\n", engineTypes[i].c_str()));
        } catch (Exception e) {
            // usually NOOP, as exception is thrown if engine doesn't support file
            if (bFatalErr) result.Error(e);
        }
        if (pEngine) EngineFactory::Destroy(pEngine);
    }

    if (!bFound && !bFatalErr) result.Error("Unknown file format");
    return result.Produce();
}

void LSCPServer::VerifyFile(String Filename) {
    #if WIN32
    WIN32_FIND_DATA win32FileAttributeData;
    BOOL res = GetFileAttributesEx( Filename.c_str(), GetFileExInfoStandard, &win32FileAttributeData );
    if (!res) {
        std::stringstream ss;
        ss << "File does not exist, GetFileAttributesEx failed `" << Filename << "`: Error " << GetLastError();
        throw Exception(ss.str());
    }
    if ( win32FileAttributeData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        throw Exception("Directory is specified");
    }
    #else
    File f(Filename);
    if(!f.Exist()) throw Exception(f.GetErrorMsg());
    if (f.IsDirectory()) throw Exception("Directory is specified");
    #endif
}

/**
 * Will be called by the parser to subscribe a client (frontend) on the
 * server for receiving event messages.
 */
String LSCPServer::SubscribeNotification(LSCPEvent::event_t type) {
    dmsg(2,("LSCPServer: SubscribeNotification(Event=%s)\n", LSCPEvent::Name(type).c_str()));
    LSCPResultSet result;
    {
        LockGuard lock(SubscriptionMutex);
        eventSubscriptions[type].push_back(currentSocket);
    }
    return result.Produce();
}

/**
 * Will be called by the parser to unsubscribe a client on the server
 * for not receiving further event messages.
 */
String LSCPServer::UnsubscribeNotification(LSCPEvent::event_t type) {
    dmsg(2,("LSCPServer: UnsubscribeNotification(Event=%s)\n", LSCPEvent::Name(type).c_str()));
    LSCPResultSet result;
    {
        LockGuard lock(SubscriptionMutex);
        eventSubscriptions[type].remove(currentSocket);
    }
    return result.Produce();
}

String LSCPServer::AddDbInstrumentDirectory(String Dir) {
    dmsg(2,("LSCPServer: AddDbInstrumentDirectory(Dir=%s)\n", Dir.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->AddDirectory(Dir);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::RemoveDbInstrumentDirectory(String Dir, bool Force) {
    dmsg(2,("LSCPServer: RemoveDbInstrumentDirectory(Dir=%s,Force=%d)\n", Dir.c_str(), Force));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->RemoveDirectory(Dir, Force);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentDirectoryCount(String Dir, bool Recursive) {
    dmsg(2,("LSCPServer: GetDbInstrumentDirectoryCount(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        result.Add(InstrumentsDb::GetInstrumentsDb()->GetDirectoryCount(Dir, Recursive));
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentDirectories(String Dir, bool Recursive) {
    dmsg(2,("LSCPServer: GetDbInstrumentDirectories(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        String list;
        StringListPtr dirs = InstrumentsDb::GetInstrumentsDb()->GetDirectories(Dir, Recursive);

        for (int i = 0; i < dirs->size(); i++) {
            if (list != "") list += ",";
            list += "'" + InstrumentsDb::toEscapedPath(dirs->at(i)) + "'";
        }

        result.Add(list);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentDirectoryInfo(String Dir) {
    dmsg(2,("LSCPServer: GetDbInstrumentDirectoryInfo(Dir=%s)\n", Dir.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        DbDirectory info = InstrumentsDb::GetInstrumentsDb()->GetDirectoryInfo(Dir);

        result.Add("DESCRIPTION", _escapeLscpResponse(info.Description));
        result.Add("CREATED", info.Created);
        result.Add("MODIFIED", info.Modified);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::SetDbInstrumentDirectoryName(String Dir, String Name) {
    dmsg(2,("LSCPServer: SetDbInstrumentDirectoryName(Dir=%s,Name=%s)\n", Dir.c_str(), Name.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->RenameDirectory(Dir, Name);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::MoveDbInstrumentDirectory(String Dir, String Dst) {
    dmsg(2,("LSCPServer: MoveDbInstrumentDirectory(Dir=%s,Dst=%s)\n", Dir.c_str(), Dst.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->MoveDirectory(Dir, Dst);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::CopyDbInstrumentDirectory(String Dir, String Dst) {
    dmsg(2,("LSCPServer: CopyDbInstrumentDirectory(Dir=%s,Dst=%s)\n", Dir.c_str(), Dst.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->CopyDirectory(Dir, Dst);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::SetDbInstrumentDirectoryDescription(String Dir, String Desc) {
    dmsg(2,("LSCPServer: SetDbInstrumentDirectoryDescription(Dir=%s,Desc=%s)\n", Dir.c_str(), Desc.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->SetDirectoryDescription(Dir, Desc);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::AddDbInstruments(String DbDir, String FilePath, int Index, bool bBackground) {
    dmsg(2,("LSCPServer: AddDbInstruments(DbDir=%s,FilePath=%s,Index=%d,bBackground=%d)\n", DbDir.c_str(), FilePath.c_str(), Index, bBackground));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        int id;
        InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();
        id = db->AddInstruments(DbDir, FilePath, Index, bBackground);
        if (bBackground) result = id;
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::AddDbInstruments(String ScanMode, String DbDir, String FsDir, bool bBackground, bool insDir) {
    dmsg(2,("LSCPServer: AddDbInstruments(ScanMode=%s,DbDir=%s,FsDir=%s,bBackground=%d,insDir=%d)\n", ScanMode.c_str(), DbDir.c_str(), FsDir.c_str(), bBackground, insDir));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        int id;
        InstrumentsDb* db = InstrumentsDb::GetInstrumentsDb();
        if (ScanMode.compare("RECURSIVE") == 0) {
            id = db->AddInstruments(RECURSIVE, DbDir, FsDir, bBackground, insDir);
        } else if (ScanMode.compare("NON_RECURSIVE") == 0) {
            id = db->AddInstruments(NON_RECURSIVE, DbDir, FsDir, bBackground, insDir);
        } else if (ScanMode.compare("FLAT") == 0) {
            id = db->AddInstruments(FLAT, DbDir, FsDir, bBackground, insDir);
        } else {
            throw Exception("Unknown scan mode: " + ScanMode);
        }

        if (bBackground) result = id;
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::RemoveDbInstrument(String Instr) {
    dmsg(2,("LSCPServer: RemoveDbInstrument(Instr=%s)\n", Instr.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->RemoveInstrument(Instr);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentCount(String Dir, bool Recursive) {
    dmsg(2,("LSCPServer: GetDbInstrumentCount(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        result.Add(InstrumentsDb::GetInstrumentsDb()->GetInstrumentCount(Dir, Recursive));
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstruments(String Dir, bool Recursive) {
    dmsg(2,("LSCPServer: GetDbInstruments(Dir=%s,Recursive=%d)\n", Dir.c_str(), Recursive));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        String list;
        StringListPtr instrs = InstrumentsDb::GetInstrumentsDb()->GetInstruments(Dir, Recursive);

        for (int i = 0; i < instrs->size(); i++) {
            if (list != "") list += ",";
            list += "'" + InstrumentsDb::toEscapedPath(instrs->at(i)) + "'";
        }

        result.Add(list);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentInfo(String Instr) {
    dmsg(2,("LSCPServer: GetDbInstrumentInfo(Instr=%s)\n", Instr.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        DbInstrument info = InstrumentsDb::GetInstrumentsDb()->GetInstrumentInfo(Instr);

        result.Add("INSTRUMENT_FILE", info.InstrFile);
        result.Add("INSTRUMENT_NR", info.InstrNr);
        result.Add("FORMAT_FAMILY", info.FormatFamily);
        result.Add("FORMAT_VERSION", info.FormatVersion);
        result.Add("SIZE", (int)info.Size);
        result.Add("CREATED", info.Created);
        result.Add("MODIFIED", info.Modified);
        result.Add("DESCRIPTION", _escapeLscpResponse(info.Description));
        result.Add("IS_DRUM", info.IsDrum);
        result.Add("PRODUCT", _escapeLscpResponse(info.Product));
        result.Add("ARTISTS", _escapeLscpResponse(info.Artists));
        result.Add("KEYWORDS", _escapeLscpResponse(info.Keywords));
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::GetDbInstrumentsJobInfo(int JobId) {
    dmsg(2,("LSCPServer: GetDbInstrumentsJobInfo(JobId=%d)\n", JobId));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        ScanJob job = InstrumentsDb::GetInstrumentsDb()->Jobs.GetJobById(JobId);

        result.Add("FILES_TOTAL", job.FilesTotal);
        result.Add("FILES_SCANNED", job.FilesScanned);
        result.Add("SCANNING", job.Scanning);
        result.Add("STATUS", job.Status);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::SetDbInstrumentName(String Instr, String Name) {
    dmsg(2,("LSCPServer: SetDbInstrumentName(Instr=%s,Name=%s)\n", Instr.c_str(), Name.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->RenameInstrument(Instr, Name);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::MoveDbInstrument(String Instr, String Dst) {
    dmsg(2,("LSCPServer: MoveDbInstrument(Instr=%s,Dst=%s)\n", Instr.c_str(), Dst.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->MoveInstrument(Instr, Dst);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::CopyDbInstrument(String Instr, String Dst) {
    dmsg(2,("LSCPServer: CopyDbInstrument(Instr=%s,Dst=%s)\n", Instr.c_str(), Dst.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->CopyInstrument(Instr, Dst);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::SetDbInstrumentDescription(String Instr, String Desc) {
    dmsg(2,("LSCPServer: SetDbInstrumentDescription(Instr=%s,Desc=%s)\n", Instr.c_str(), Desc.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->SetInstrumentDescription(Instr, Desc);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::SetDbInstrumentFilePath(String OldPath, String NewPath) {
    dmsg(2,("LSCPServer: SetDbInstrumentFilePath(OldPath=%s,NewPath=%s)\n", OldPath.c_str(), NewPath.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->SetInstrumentFilePath(OldPath, NewPath);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::FindLostDbInstrumentFiles() {
    dmsg(2,("LSCPServer: FindLostDbInstrumentFiles()\n"));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        String list;
        StringListPtr pLostFiles = InstrumentsDb::GetInstrumentsDb()->FindLostInstrumentFiles();

        for (int i = 0; i < pLostFiles->size(); i++) {
            if (list != "") list += ",";
            list += "'" + pLostFiles->at(i) + "'";
        }

        result.Add(list);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::FindDbInstrumentDirectories(String Dir, std::map<String,String> Parameters, bool Recursive) {
    dmsg(2,("LSCPServer: FindDbInstrumentDirectories(Dir=%s)\n", Dir.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        SearchQuery Query;
        std::map<String,String>::iterator iter;
        for (iter = Parameters.begin(); iter != Parameters.end(); iter++) {
            if (iter->first.compare("NAME") == 0) {
                Query.Name = iter->second;
            } else if (iter->first.compare("CREATED") == 0) {
                Query.SetCreated(iter->second);
            } else if (iter->first.compare("MODIFIED") == 0) {
                Query.SetModified(iter->second);
            } else if (iter->first.compare("DESCRIPTION") == 0) {
                Query.Description = iter->second;
            } else {
                throw Exception("Unknown search criteria: " + iter->first);
            }
        }

        String list;
        StringListPtr pDirectories =
            InstrumentsDb::GetInstrumentsDb()->FindDirectories(Dir, &Query, Recursive);

        for (int i = 0; i < pDirectories->size(); i++) {
            if (list != "") list += ",";
            list += "'" + InstrumentsDb::toEscapedPath(pDirectories->at(i)) + "'";
        }

        result.Add(list);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::FindDbInstruments(String Dir, std::map<String,String> Parameters, bool Recursive) {
    dmsg(2,("LSCPServer: FindDbInstruments(Dir=%s)\n", Dir.c_str()));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        SearchQuery Query;
        std::map<String,String>::iterator iter;
        for (iter = Parameters.begin(); iter != Parameters.end(); iter++) {
            if (iter->first.compare("NAME") == 0) {
                Query.Name = iter->second;
            } else if (iter->first.compare("FORMAT_FAMILIES") == 0) {
                Query.SetFormatFamilies(iter->second);
            } else if (iter->first.compare("SIZE") == 0) {
                Query.SetSize(iter->second);
            } else if (iter->first.compare("CREATED") == 0) {
                Query.SetCreated(iter->second);
            } else if (iter->first.compare("MODIFIED") == 0) {
                Query.SetModified(iter->second);
            } else if (iter->first.compare("DESCRIPTION") == 0) {
                Query.Description = iter->second;
            } else if (iter->first.compare("IS_DRUM") == 0) {
                if (!strcasecmp(iter->second.c_str(), "true")) {
                    Query.InstrType = SearchQuery::DRUM;
                } else {
                    Query.InstrType = SearchQuery::CHROMATIC;
                }
            } else if (iter->first.compare("PRODUCT") == 0) {
                 Query.Product = iter->second;
            } else if (iter->first.compare("ARTISTS") == 0) {
                 Query.Artists = iter->second;
            } else if (iter->first.compare("KEYWORDS") == 0) {
                 Query.Keywords = iter->second;
            } else {
                throw Exception("Unknown search criteria: " + iter->first);
            }
        }

        String list;
        StringListPtr pInstruments =
            InstrumentsDb::GetInstrumentsDb()->FindInstruments(Dir, &Query, Recursive);

        for (int i = 0; i < pInstruments->size(); i++) {
            if (list != "") list += ",";
            list += "'" + InstrumentsDb::toEscapedPath(pInstruments->at(i)) + "'";
        }

        result.Add(list);
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}

String LSCPServer::FormatInstrumentsDb() {
    dmsg(2,("LSCPServer: FormatInstrumentsDb()\n"));
    LSCPResultSet result;
#if HAVE_SQLITE3
    try {
        InstrumentsDb::GetInstrumentsDb()->Format();
    } catch (Exception e) {
         result.Error(e);
    }
#else
    result.Error(String(DOESNT_HAVE_SQLITE3), 0);
#endif
    return result.Produce();
}


/**
 * Will be called by the parser to enable or disable echo mode; if echo
 * mode is enabled, all commands from the client will (immediately) be
 * echoed back to the client.
 */
String LSCPServer::SetEcho(yyparse_param_t* pSession, double boolean_value) {
    dmsg(2,("LSCPServer: SetEcho(val=%f)\n", boolean_value));
    LSCPResultSet result;
    try {
        if      (boolean_value == 0) pSession->bVerbose = false;
        else if (boolean_value == 1) pSession->bVerbose = true;
        else throw Exception("Not a boolean value, must either be 0 or 1");
    }
    catch (Exception e) {
         result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetShellInteract(yyparse_param_t* pSession, double boolean_value) {
    dmsg(2,("LSCPServer: SetShellInteract(val=%f)\n", boolean_value));
    LSCPResultSet result;
    try {
        if      (boolean_value == 0) pSession->bShellInteract = false;
        else if (boolean_value == 1) pSession->bShellInteract = true;
        else throw Exception("Not a boolean value, must either be 0 or 1");
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetShellAutoCorrect(yyparse_param_t* pSession, double boolean_value) {
    dmsg(2,("LSCPServer: SetShellAutoCorrect(val=%f)\n", boolean_value));
    LSCPResultSet result;
    try {
        if      (boolean_value == 0) pSession->bShellAutoCorrect = false;
        else if (boolean_value == 1) pSession->bShellAutoCorrect = true;
        else throw Exception("Not a boolean value, must either be 0 or 1");
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

String LSCPServer::SetShellDoc(yyparse_param_t* pSession, double boolean_value) {
    dmsg(2,("LSCPServer: SetShellDoc(val=%f)\n", boolean_value));
    LSCPResultSet result;
    try {
        if      (boolean_value == 0) pSession->bShellSendLSCPDoc = false;
        else if (boolean_value == 1) pSession->bShellSendLSCPDoc = true;
        else throw Exception("Not a boolean value, must either be 0 or 1");
    } catch (Exception e) {
        result.Error(e);
    }
    return result.Produce();
}

}
