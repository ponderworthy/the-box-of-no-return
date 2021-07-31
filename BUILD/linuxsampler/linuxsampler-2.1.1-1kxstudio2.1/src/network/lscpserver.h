/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2014 Christian Schoenebeck                       *
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

#ifndef __LSCPSERVER_H_
#define __LSCPSERVER_H_

#if defined(WIN32)
#include <windows.h>
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <list>

#include "lscp.h"
#include "lscpparser.h"
#include "lscpevent.h"
#include "../Sampler.h"
#include "../common/Thread.h"
#include "../common/Mutex.h"
#include "../common/Condition.h"
#include "../common/global_private.h"

#include "../drivers/midi/MidiInstrumentMapper.h"
#include "../drivers/midi/VirtualMidiDevice.h"

#if HAVE_SQLITE3
#include "../db/InstrumentsDb.h"
#endif

/// TCP Port on which the server should listen for connection requests.
#define LSCP_ADDR INADDR_ANY
#define LSCP_PORT 8888

/// try up to 3 minutes to bind server socket
#define LSCP_SERVER_BIND_TIMEOUT 180

// External references to the main scanner and parser functions
extern int yyparse(void* YYPARSE_PARAM);

namespace LinuxSampler {

extern void restart(yyparse_param_t* pparam, int& yychar);

/**
 * Network server for the LinuxSampler Control Protocol (LSCP).
 */
class LSCPServer : public Thread {
    public:
        LSCPServer(Sampler* pSampler, long int addr, short int port);
        virtual ~LSCPServer();
        int WaitUntilInitialized(long TimeoutSeconds = 0L, long TimeoutNanoSeconds = 0L);
        void RemoveListeners();

        // Methods called by the parser
        String DestroyAudioOutputDevice(uint DeviceIndex);
        String DestroyMidiInputDevice(uint DeviceIndex);
        String LoadInstrument(String Filename, uint uiInstrument, uint uiSamplerChannel, bool bBackground = false);
        String SetEngineType(String EngineName, uint uiSamplerChannel);
        String GetChannels();
        String ListChannels();
        String AddChannel();
        String RemoveChannel(uint uiSamplerChannel);
        String GetAvailableEngines();
        String ListAvailableEngines();
        String GetEngineInfo(String EngineName);
        String GetChannelInfo(uint uiSamplerChannel);
        String GetVoiceCount(uint uiSamplerChannel);
        String GetStreamCount(uint uiSamplerChannel);
        String GetBufferFill(fill_response_t ResponseType, uint uiSamplerChannel);
        String GetAvailableAudioOutputDrivers();
        String ListAvailableAudioOutputDrivers();
        String GetAvailableMidiInputDrivers();
        String ListAvailableMidiInputDrivers();
        String GetAudioOutputDriverInfo(String Driver);
        String GetMidiInputDriverInfo(String Driver);
#ifdef __GNUC__
        typedef std::map<String,String> StringMap; // nasty workaround for a GCC bug (see GCC bug #15980, #57)
        String GetAudioOutputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList = StringMap());
        String GetMidiInputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList = StringMap());
        String CreateAudioOutputDevice(String Driver, std::map<String,String> Parameters = StringMap());
        String CreateMidiInputDevice(String Driver, std::map<String,String> Parameters = StringMap());
#else
        String GetAudioOutputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList = std::map<String,String>());
        String GetMidiInputDriverParameterInfo(String Driver, String Parameter, std::map<String,String> DependencyList = std::map<String,String>());
        String CreateAudioOutputDevice(String Driver, std::map<String,String> Parameters = std::map<String,String>());
        String CreateMidiInputDevice(String Driver, std::map<String,String> Parameters = std::map<String,String>());
#endif // __GNUC__
        String GetAudioOutputDeviceCount();
        String GetMidiInputDeviceCount();
        String GetAudioOutputDevices();
        String GetMidiInputDevices();
        String GetAudioOutputDeviceInfo(uint DeviceIndex);
        String GetMidiInputDeviceInfo(uint DeviceIndex);
        String GetMidiInputPortInfo(uint DeviceIndex, uint PortIndex);
        String GetMidiInputPortParameterInfo(uint DeviceId, uint PortId, String ParameterName);
        String GetAudioOutputChannelInfo(uint DeviceId, uint ChannelId);
        String GetAudioOutputChannelParameterInfo(uint DeviceId, uint ChannelId, String ParameterName);
        String SetAudioOutputChannelParameter(uint DeviceId, uint ChannelId, String ParamKey, String ParamVal);
        String SetAudioOutputDeviceParameter(uint DeviceIndex, String ParamKey, String ParamVal);
        String SetMidiInputDeviceParameter(uint DeviceIndex, String ParamKey, String ParamVal);
        String SetMidiInputPortParameter(uint DeviceIndex, uint PortIndex, String ParamKey, String ParamVal);
        String SetAudioOutputChannel(uint ChannelAudioOutputChannel, uint AudioOutputDeviceInputChannel, uint uiSamplerChannel);
        String SetAudioOutputDevice(uint AudioDeviceId, uint SamplerChannel);
        String SetAudioOutputType(String AudioOutputDriver, uint uiSamplerChannel);
        String AddChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId, uint MIDIPort = 0);
        String RemoveChannelMidiInput(uint uiSamplerChannel);
        String RemoveChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId);
        String RemoveChannelMidiInput(uint uiSamplerChannel, uint MIDIDeviceId, uint MIDIPort);
        String ListChannelMidiInputs(uint uiSamplerChannel);
        String SetMIDIInputPort(uint MIDIPort, uint uiSamplerChannel);
        String SetMIDIInputChannel(uint MIDIChannel, uint uiSamplerChannel);
        String SetMIDIInputDevice(uint MIDIDeviceId, uint uiSamplerChannel);
        String SetMIDIInputType(String MidiInputDriver, uint uiSamplerChannel);
        String SetMIDIInput(uint MIDIDeviceId, uint MIDIPort, uint MIDIChannel, uint uiSamplerChannel);
        String SetVolume(double dVolume, uint uiSamplerChannel);
        String SetChannelMute(bool bMute, uint uiSamplerChannel);
        String SetChannelSolo(bool bSolo, uint uiSamplerChannel);
        String AddOrReplaceMIDIInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg, String EngineType, String InstrumentFile, uint InstrumentIndex, float Volume, MidiInstrumentMapper::mode_t LoadMode, String Name, bool bModal);
        String RemoveMIDIInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg);
        String GetMidiInstrumentMappings(uint MidiMapID);
        String GetAllMidiInstrumentMappings();
        String GetMidiInstrumentMapping(uint MidiMapID, uint MidiBank, uint MidiProg);
        String ListMidiInstrumentMappings(uint MidiMapID);
        String ListAllMidiInstrumentMappings();
        String ClearMidiInstrumentMappings(uint MidiMapID);
        String ClearAllMidiInstrumentMappings();
        String AddMidiInstrumentMap(String MapName = "");
        String RemoveMidiInstrumentMap(uint MidiMapID);
        String RemoveAllMidiInstrumentMaps();
        String GetMidiInstrumentMaps();
        String ListMidiInstrumentMaps();
        String GetMidiInstrumentMap(uint MidiMapID);
        String SetMidiInstrumentMapName(uint MidiMapID, String NewName);
        String SetChannelMap(uint uiSamplerChannel, int MidiMapID);
        String CreateFxSend(uint uiSamplerChannel, uint MidiCtrl, String Name = "");
        String DestroyFxSend(uint uiSamplerChannel, uint FxSendID);
        String GetFxSends(uint uiSamplerChannel);
        String ListFxSends(uint uiSamplerChannel);
        String GetFxSendInfo(uint uiSamplerChannel, uint FxSendID);
        String SetFxSendName(uint uiSamplerChannel, uint FxSendID, String Name);
        String SetFxSendAudioOutputChannel(uint uiSamplerChannel, uint FxSendID, uint FxSendChannel, uint DeviceChannel);
        String SetFxSendMidiController(uint uiSamplerChannel, uint FxSendID, uint MidiController);
        String SetFxSendLevel(uint uiSamplerChannel, uint FxSendID, double dLevel);
        String SetFxSendEffect(uint uiSamplerChannel, uint FxSendID, int iSendEffectChain, int iEffectChainPosition);

        // effect commands
        String GetAvailableEffects();
        String ListAvailableEffects();
        String GetEffectInfo(int iEffectIndex);
        String CreateEffectInstance(int iEffectIndex);
        String CreateEffectInstance(String effectSystem, String module, String effectName);
        String DestroyEffectInstance(int iEffectInstance);
        String GetEffectInstances();
        String ListEffectInstances();
        String GetEffectInstanceInfo(int iEffectInstance);
        String GetEffectInstanceInputControlInfo(int iEffectInstance, int iInputControlIndex);
        String SetEffectInstanceInputControlValue(int iEffectInstance, int iInputControlIndex, double dValue);
        String GetSendEffectChains(int iAudioOutputDevice);
        String ListSendEffectChains(int iAudioOutputDevice);
        String AddSendEffectChain(int iAudioOutputDevice);
        String RemoveSendEffectChain(int iAudioOutputDevice, int iSendEffectChain);
        String GetSendEffectChainInfo(int iAudioOutputDevice, int iSendEffectChain);
        String AppendSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectInstance);
        String InsertSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectChainPosition, int iEffectInstance);
        String RemoveSendEffectChainEffect(int iAudioOutputDevice, int iSendEffectChain, int iEffectChainPosition);

        String AddDbInstrumentDirectory(String Dir);
        String RemoveDbInstrumentDirectory(String Dir, bool Force = false);
        String GetDbInstrumentDirectoryCount(String Dir, bool Recursive = false);
        String GetDbInstrumentDirectories(String Dir, bool Recursive = false);
        String GetDbInstrumentDirectoryInfo(String Dir);
        String SetDbInstrumentDirectoryName(String Dir, String Name);
        String MoveDbInstrumentDirectory(String Dir, String Dst);
        String CopyDbInstrumentDirectory(String Dir, String Dst);
        String SetDbInstrumentDirectoryDescription(String Dir, String Desc);
        String FindDbInstrumentDirectories(String Dir, std::map<String,String> Parameters, bool Recursive = true);
        String AddDbInstruments(String DbDir, String FilePath, int Index = -1, bool bBackground = false);
        String AddDbInstruments(String ScanMode, String DbDir, String FsDir, bool bBackground = false, bool insDir = false);
        String RemoveDbInstrument(String Instr);
        String GetDbInstrumentCount(String Dir, bool Recursive = false);
        String GetDbInstruments(String Dir, bool Recursive = false);
        String GetDbInstrumentInfo(String Instr);
        String SetDbInstrumentName(String Instr, String Name);
        String MoveDbInstrument(String Instr, String Dst);
        String CopyDbInstrument(String Instr, String Dst);
        String SetDbInstrumentDescription(String Instr, String Desc);
        String SetDbInstrumentFilePath(String OldPath, String NewPath);
        String FindLostDbInstrumentFiles();
        String FindDbInstruments(String Dir, std::map<String,String> Parameters, bool Recursive = true);
        String FormatInstrumentsDb();
        String EditSamplerChannelInstrument(uint uiSamplerChannel);
        String GetDbInstrumentsJobInfo(int JobId);
        String ResetChannel(uint uiSamplerChannel);
        String ResetSampler();
        String GetServerInfo();
        String GetTotalStreamCount();
        String GetTotalVoiceCount();
        String GetTotalVoiceCountMax();
        String GetGlobalMaxVoices();
        String SetGlobalMaxVoices(int iVoices);
        String GetGlobalMaxStreams();
        String SetGlobalMaxStreams(int iStreams);
        String GetGlobalVolume();
        String SetGlobalVolume(double dVolume);
        String GetFileInstruments(String Filename);
        String ListFileInstruments(String Filename);
        String GetFileInstrumentInfo(String Filename, uint InstrumentID);
        String SendChannelMidiData(String MidiMsg, uint uiSamplerChannel, uint Arg1, uint Arg2);
        String SubscribeNotification(LSCPEvent::event_t);
        String UnsubscribeNotification(LSCPEvent::event_t);
        String SetEcho(yyparse_param_t* pSession, double boolean_value);
        String SetShellInteract(yyparse_param_t* pSession, double boolean_value);
        String SetShellDoc(yyparse_param_t* pSession, double boolean_value);
        String SetShellAutoCorrect(yyparse_param_t* pSession, double boolean_value);
        void   AnswerClient(String ReturnMessage);
        void   CloseAllConnections();

	static int currentSocket;
	static std::map<int,String> bufferedCommands;

	static void SendLSCPNotify( LSCPEvent Event );
	static int EventSubscribers( std::list<LSCPEvent::event_t> events );
	static String FilterEndlines(String s);

	//Protect main thread that generates real time notify messages
	//like voice count, stream count and buffer fill
	//from LSCP server removing engines and channels from underneath
	static Mutex RTNotifyMutex;

    protected:
        int            hSocket;
        sockaddr_in    SocketAddress;
        Sampler*       pSampler;
        Condition      Initialized;

        int Main(); ///< Implementation of virtual method from class Thread

    private:

        /**
         * Find a created audio output device index.
         */
        int GetAudioOutputDeviceIndex (AudioOutputDevice *pDevice);

        /**
         * Find a created midi input device index.
         */
        int GetMidiInputDeviceIndex (MidiInputDevice *pDevice);

        EngineChannel* GetEngineChannel(uint uiSamplerChannel);

		/**
		 * Gets the specified effect send on the specified sampler channel.
		 */
		FxSend* GetFxSend(uint uiSamplerChannel, uint FxSendID);

        bool HasSoloChannel();
        void MuteNonSoloChannels();
        void UnmuteChannels();

        /**
         * Throws an exception if the specified file is not found or
         * if directory is specified.
         */
        static void VerifyFile(String Filename);

	static std::map<int,String> bufferedNotifies;
	static Mutex NotifyMutex;
	static Mutex NotifyBufferMutex;
	String generateLSCPDocReply(const String& line, yyparse_param_t* param);
	bool GetLSCPCommand( std::vector<yyparse_param_t>::iterator iter );
	static void CloseConnection( std::vector<yyparse_param_t>::iterator iter );
	static std::vector<yyparse_param_t> Sessions;
	static Mutex SubscriptionMutex;
	static std::map< LSCPEvent::event_t, std::list<int> > eventSubscriptions;
	static fd_set fdSet;

        class EventHandler : public ChannelCountListener, public AudioDeviceCountListener,
            public MidiDeviceCountListener, public MidiInstrumentCountListener,
            public MidiInstrumentInfoListener, public MidiInstrumentMapCountListener,
            public MidiInstrumentMapInfoListener, public FxSendCountListener,
            public VoiceCountListener, public StreamCountListener, public BufferFillListener,
            public TotalStreamCountListener, public TotalVoiceCountListener,
            public EngineChangeListener, public MidiPortCountListener {

            public:
                EventHandler(LSCPServer* pParent);

                /**
                 * Invoked when the number of sampler channels has changed.
                 * @param NewCount The new number of sampler channels.
                 */
                virtual void ChannelCountChanged(int NewCount);
                virtual void ChannelAdded(SamplerChannel* pChannel);
                virtual void ChannelToBeRemoved(SamplerChannel* pChannel);

                /**
                 * Invoked when the number of audio output devices has changed.
                 * @param NewCount The new number of audio output devices.
                 */
                virtual void AudioDeviceCountChanged(int NewCount);

                /**
                 * Invoked when the number of MIDI input devices has changed.
                 * @param NewCount The new number of MIDI input devices.
                 */
                virtual void MidiDeviceCountChanged(int NewCount);

                /**
                 * Invoked right before the supplied MIDI input device is going
                 * to be destroyed.
                 * @param pDevice MidiInputDevice to be deleted
                 */
                virtual void MidiDeviceToBeDestroyed(MidiInputDevice* pDevice);

                /**
                 * Invoked to inform that a new MidiInputDevice has just been
                 * created.
                 * @param pDevice newly created MidiInputDevice
                 */
                virtual void MidiDeviceCreated(MidiInputDevice* pDevice);

                /**
                 * Invoked when the number of MIDI input ports has changed.
                 * @param NewCount The new number of MIDI input ports.
                 */
                virtual void MidiPortCountChanged(int NewCount);

                /**
                 * Invoked right before the supplied MIDI input port is going
                 * to be destroyed.
                 * @param pPort MidiInputPort to be deleted
                 */
                virtual void MidiPortToBeRemoved(MidiInputPort* pPort);

                /**
                 * Invoked to inform that a new MidiInputPort has just been
                 * added.
                 * @param pPort newly created MidiInputPort
                 */
                virtual void MidiPortAdded(MidiInputPort* pPort);

                /**
                 * Invoked when the number of MIDI instruments has changed.
                 * @param MapId The numerical ID of the MIDI instrument map.
                 * @param NewCount The new number of MIDI instruments.
                 */
                virtual void MidiInstrumentCountChanged(int MapId, int NewCount);

                /**
                 * Invoked when a MIDI instrument in a MIDI instrument map is changed.
                 * @param MapId The numerical ID of the MIDI instrument map.
                 * @param Bank The index of the MIDI bank, containing the instrument.
                 * @param Program The MIDI program number of the instrument.
                 */
                virtual void MidiInstrumentInfoChanged(int MapId, int Bank, int Program);

                /**
                 * Invoked when the number of MIDI instrument maps has changed.
                 * @param NewCount The new number of MIDI instruments.
                 */
                virtual void MidiInstrumentMapCountChanged(int NewCount);

                /**
                 * Invoked when the settings of a MIDI instrument map are changed.
                 * @param MapId The numerical ID of the MIDI instrument map.
                 */
                virtual void MidiInstrumentMapInfoChanged(int MapId);

                /**
                 * Invoked when the number of effect sends
                 * on the specified sampler channel has changed.
                 * @param ChannelId The numerical ID of the sampler channel.
                 * @param NewCount The new number of effect sends.
                 */
                virtual void FxSendCountChanged(int ChannelId, int NewCount);

                /**
                 * Invoked when the number of active voices
                 * on the specified sampler channel has changed.
                 * @param ChannelId The numerical ID of the sampler channel.
                 * @param NewCount The new number of active voices.
                 */
                virtual void VoiceCountChanged(int ChannelId, int NewCount);

                /**
                 * Invoked when the number of active disk streams
                 * on the specified sampler channel has changed.
                 * @param ChannelId The numerical ID of the sampler channel.
                 * @param NewCount The new number of active disk streams.
                 */
                virtual void StreamCountChanged(int ChannelId, int NewCount);

                /**
                 * Invoked when the fill state of the disk stream
                 * buffers on the specified sampler channel is changed.
                 * @param ChannelId The numerical ID of the sampler channel.
                 * @param FillData The buffer fill data for the specified sampler channel.
                 */
                virtual void BufferFillChanged(int ChannelId, String FillData);

                /**
                 * Invoked when the total number of active voices is changed.
                 * @param NewCount The new number of active voices.
                 */
                virtual void TotalVoiceCountChanged(int NewCount);
                virtual void TotalStreamCountChanged(int NewCount);

                virtual void EngineToBeChanged(int ChannelId);
                virtual void EngineChanged(int ChannelId);

                virtual ~EventHandler();

                struct midi_listener_entry {
                    SamplerChannel* pSamplerChannel;
                    EngineChannel* pEngineChannel;
                    VirtualMidiDevice* pMidiListener;
                };

                std::vector<midi_listener_entry> channelMidiListeners;

                struct device_midi_listener_entry {
                    MidiInputPort* pPort;
                    VirtualMidiDevice* pMidiListener;
                    uint uiDeviceID;
                };

                std::vector<device_midi_listener_entry> deviceMidiListeners;

            private:
                LSCPServer* pParent;
        } eventHandler;

#if HAVE_SQLITE3
        class DbInstrumentsEventHandler : public InstrumentsDb::Listener {
            public:
                virtual void DirectoryCountChanged(String Dir);
                virtual void DirectoryInfoChanged(String Dir);
                virtual void DirectoryNameChanged(String Dir, String NewName);
                virtual void InstrumentCountChanged(String Dir);
                virtual void InstrumentInfoChanged(String Instr);
                virtual void InstrumentNameChanged(String Instr, String NewName);
                virtual void JobStatusChanged(int JobId);
        } dbInstrumentsEventHandler;
#endif // HAVE_SQLITE3
};

}

#endif // __LSCPSERVER_H_
