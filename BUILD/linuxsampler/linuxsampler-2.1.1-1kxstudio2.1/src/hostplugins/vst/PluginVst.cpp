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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef WIN32
#include <tchar.h>
#else
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

#include "PluginVst.h"
#include "../../drivers/midi/MidiInstrumentMapper.h"

#ifndef CHANNELS
#define CHANNELS 32
#endif

// the sampler can actually hold up a very huge amount of programs
// (2^32 [maps] * 2^16 [banks MSB + LSB] * 2^8 [programs per bank]  =  2^56 [total programs] )
// however I am not sure if we might crash some VST host with a number
// here that is too huge, however this should be enough in most cases ...
#define MAX_VST_PROGRAMS (128*128)

using namespace LinuxSampler;

namespace {

#if defined(WIN32) && CONFIG_DEBUG_LEVEL >= 2
// on windows, send debug logging to a file instead of a non-existent
// console
#include <cstdarg>
#include "../../common/Mutex.h"
    LinuxSampler::Mutex logmutex;

    void log(const char* fmt, ...) {
        LockGuard lock(logmutex);
        FILE* f = fopen((String(getenv("TEMP")) + "\\linuxsamplervst.log").c_str(), "a");
        va_list ap;
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);
        fclose(f);
    }
#undef dmsg
#define dmsg(debuglevel,x) log x;
#endif


// *************** LinuxSamplerEditor ***************
// *

    LinuxSamplerEditor::LinuxSamplerEditor(AudioEffect* effect) :
        AEffEditor(effect) {
        dmsg(2, ("-->LinuxSamplerEditor constructor\n"));
#ifdef WIN32
        ProcessHandle = INVALID_HANDLE_VALUE;
#else
        pid = 0;
#endif
    }

    LinuxSamplerEditor::~LinuxSamplerEditor() {
        close();
    }

    String LinuxSamplerEditor::FindFantasia() {
        String fantasia;
#ifdef WIN32
        // assume Fantasia is in the same directory or one directory above
        // the liblinuxsampler dll
        String lspath = LinuxSampler::Sampler::GetInstallDir();
        if (!lspath.empty()) {
            lspath += "\\";
            WIN32_FIND_DATA fd;
            HANDLE hFind =
                FindFirstFile((lspath + "Fantasia*.jar").c_str(), &fd);
            if (hFind == INVALID_HANDLE_VALUE) {
                lspath += "..\\";
                hFind = FindFirstFile((lspath + "Fantasia*.jar").c_str(), &fd);
            }
            if (hFind != INVALID_HANDLE_VALUE) {
                fantasia = fd.cFileName;
                FindClose(hFind);
                return lspath + fantasia;
            }
        }
#elif defined(__APPLE__)
        // look for the java application wrapper
        const char* cmd =
            "/Applications/LinuxSampler/Fantasia.app"
            "/Contents/MacOS/JavaApplicationStub";
        fantasia = String(getenv("HOME")) + cmd;
        struct stat buf;
        if (stat(fantasia.c_str(), &buf) != 0) {
            fantasia = stat(cmd, &buf) == 0 ? cmd : "";
        }
#else
        // search in <datadir>/java (default: /usr/local/share/java)
        String path(DATADIR);
        path += *(path.end() - 1) == '/' ? "java" : "/java";
        DIR* dir = opendir(path.c_str());
        if (dir) {
            while (dirent* d = readdir(dir)) {
                const char* name = d->d_name;
                if (strncmp("Fantasia", name, 8) == 0 &&
                    strcmp(name + strlen(name) - 4, ".jar") == 0) {
                    fantasia = path + "/" + name;
                }
            }
            closedir(dir);
        }
#endif
        return fantasia;
    }

    bool LinuxSamplerEditor::open(void* ptr) {
        dmsg(2, ("-->LinuxSamplerEditor::open\n"));
        AEffEditor::open(ptr);

        // try to start the JSampler Fantasia GUI as a separate process
#ifdef WIN32
        // first check if it's already running
        if (ProcessHandle != INVALID_HANDLE_VALUE) {
            DWORD exitCode;
            if (GetExitCodeProcess(ProcessHandle, &exitCode)) {
                if (exitCode == STILL_ACTIVE) return true;
            }
            free(Command);
            CloseHandle(ProcessHandle);
            ProcessHandle = INVALID_HANDLE_VALUE;
        }

        String fantasia = FindFantasia();
        if (!fantasia.empty()) {
            // start a java process
            String path; // look in PATH first
            for (int i = 0 ; i < 2 ; i++) { // two tries
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                ZeroMemory(&pi, sizeof(pi));

                Command = _tcsdup(TEXT((String("\"") + path +
                                        "javaw\" -jar \"" + fantasia +
                                        "\"").c_str()));
                if (CreateProcess(NULL, Command, NULL, NULL, FALSE, 0, NULL,
                                  NULL, &si, &pi)) {
                    ProcessHandle = pi.hProcess;
                    CloseHandle(pi.hThread);
                    break;
                } else {
                    free(Command);
                    if (path.empty()) {
                        // java wasn't found in PATH, try again
                        // with an alternative directory
                        char* windir = getenv("windir");
                        if (!windir) break;
#ifdef _WIN64
                        // LS plugin is 64 bit - look for 32 bit java
                        path = String(windir) + "\\SysWOW64\\";
#else
                        // LS plugin is 32 bit - look for 64 bit java
                        path = String(windir) + "\\Sysnative\\";
#endif
                    }
                }
            }
        }
#else
        // first check if it's already running
        if (pid && waitpid(pid, 0, WNOHANG) == 0) return true;

        String fantasia = FindFantasia();
        if (!fantasia.empty())
        {
            // start a java process
            pid = fork();
            if (pid == -1) {
                dmsg(1, ("fork failed %d %s\n", errno, strerror(errno)));
                pid = 0;
            } else if (pid == 0) {
#ifdef __APPLE__
                execl(fantasia.c_str(), fantasia.c_str(), (char*)0);
#else
                execlp("java", "java", "-jar", fantasia.c_str(), (char*)0);
#endif
                dmsg(1, ("exec failed %d %s\n", errno, strerror(errno)));

                // make sure something is executed, so the static
                // destructors copied from the parent don't run
                execl("/usr/bin/true", "/usr/bin/true", (char*)0);
            }
        }
#endif
        dmsg(2, ("<--LinuxSamplerEditor::open\n"));
        return true;
    }

    void LinuxSamplerEditor::close() {
        systemWindow = 0;
#ifdef WIN32
        if (ProcessHandle != INVALID_HANDLE_VALUE) {
            TerminateProcess(ProcessHandle, 0);
            free(Command);
            CloseHandle(ProcessHandle);
            ProcessHandle = INVALID_HANDLE_VALUE;
        }
#else
        if (pid) {
            kill(pid, SIGTERM);
            waitpid(pid, 0, 0);
            pid = 0;
        }
#endif
    }

    bool LinuxSamplerEditor::getRect(ERect** rect) {
        ERect* r = new ERect();
        r->top = 0;
        r->left = 0;
        r->bottom = 1; // 0 makes EnergyXT and Ardour on Linux crash
        r->right = 400;
        *rect = r;
        return true;
    }


// *************** LinuxSamplerVst ***************
// *

    LinuxSamplerVst::LinuxSamplerVst(audioMasterCallback audioMaster) :
        AudioEffectX(audioMaster, MAX_VST_PROGRAMS, 0),
        StateBuf(0)
    {
        dmsg(2, ("-->constructor\n"));

        setProgram(0);
        setNumInputs(0);
        setNumOutputs(CHANNELS);
        canProcessReplacing();
        isSynth();
        programsAreChunks();
        setUniqueID(CCONST('L', 'S', 'm', 'p')); // (registred at Steinberg)
        setEditor(new LinuxSamplerEditor(this));

        suspend();
        dmsg(2, ("<--constructor\n"));
    }


    void LinuxSamplerVst::resume() {
        dmsg(2, ("-->resume\n"));

        // Ardour initially sets blockSize to zero - we postpone the
        // initialization until we have a blockSize
        if (blockSize != 0) {
            if (!pAudioDevice) {
                Init(int(sampleRate), blockSize, CHANNELS);

                if (!SavedChunk.empty()) {
                    SetState(SavedChunk);
                    SavedChunk.clear();
                } else {
                    InitState();
                }
            } else {
                Init(int(sampleRate), blockSize, CHANNELS);
            }
        }
        AudioEffectX::resume();
        dmsg(2, ("<--resume\n"));
    }

    LinuxSamplerVst::~LinuxSamplerVst() {
        dmsg(2, ("-->destructor\n"));
        if (StateBuf) free(StateBuf);
        dmsg(2, ("<--destructor\n"));
    }


    void LinuxSamplerVst::setProgram(VstInt32 program) {
        if (program < 0 || program >= MAX_VST_PROGRAMS ||
            !pMidiDevice || !pMidiDevice->Port()) return;
        
        int i = 0;
        const std::vector<int> maps = MidiInstrumentMapper::Maps();
        for (int iMap = 0; iMap < maps.size(); ++iMap) {
            const int mapID = maps[iMap];
            const std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t> mappings =
                MidiInstrumentMapper::Entries(maps[mapID]);
            for (std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t>::const_iterator iter = mappings.begin();
                 iter != mappings.end(); ++iter, ++i)
            {
                if (i == program) {
                    //TODO: switch MIDI instrument map before sending bank select and program change
                    
                    const uint iMIDIChannel = 0;
                    pMidiDevice->Port()->DispatchBankSelectMsb(iter->first.midi_bank_msb, iMIDIChannel);
                    pMidiDevice->Port()->DispatchBankSelectLsb(iter->first.midi_bank_lsb, iMIDIChannel);
                    pMidiDevice->Port()->DispatchProgramChange(iter->first.midi_prog, iMIDIChannel);
                    
                    curProgram = program;
                    
                    return;
                }
            }
        }
    }

    void LinuxSamplerVst::setProgramName(char* name) {
        int i = 0;
        const std::vector<int> maps = MidiInstrumentMapper::Maps();
        for (int iMap = 0; iMap < maps.size(); ++iMap) {
            const int mapID = maps[iMap];
            const std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t> mappings =
                MidiInstrumentMapper::Entries(maps[mapID]);
            for (std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t>::const_iterator iter = mappings.begin();
                 iter != mappings.end(); ++iter, ++i)
            {
                if (i == curProgram) {
                    char buf[kVstMaxProgNameLen + 1] = {};
                    vst_strncpy(buf, name, kVstMaxProgNameLen);
                    MidiInstrumentMapper::entry_t entry = iter->second;
                    entry.Name = buf;
                    try {
                        MidiInstrumentMapper::AddOrReplaceEntry(
                            mapID, iter->first, entry, false/*bInBackground*/
                        );
                    } catch (Exception e) {
                        e.PrintMessage();
                    }
                    return;
                }
            }
        }
    }

    void LinuxSamplerVst::getProgramName(char* name) {
        if (!getProgramNameIndexed(0 /*dont care*/, curProgram, name)) {
            vst_strncpy(name, "unknown", kVstMaxProgNameLen);
        }
    }

    bool LinuxSamplerVst::getProgramNameIndexed(VstInt32 /*category*/, VstInt32 index,
                                                char* text)
    {
        //NOTE: parameter 'category' is unused in VST 2.4
        
        int i = 0;
        const std::vector<int> maps = MidiInstrumentMapper::Maps();
        for (int iMap = 0; iMap < maps.size(); ++iMap) {
            const int mapID = maps[iMap];
            const std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t> mappings =
                MidiInstrumentMapper::Entries(maps[mapID]);
            for (std::map<midi_prog_index_t, MidiInstrumentMapper::entry_t>::const_iterator iter = mappings.begin();
                 iter != mappings.end(); ++iter, ++i)
            {
                if (i == index) {
                    vst_strncpy(text, iter->second.Name.c_str(), kVstMaxProgNameLen);
                    return true;
                }
            }
        }
        return false;
    }


    bool LinuxSamplerVst::getOutputProperties(VstInt32 index,
                                              VstPinProperties* properties) {
        if (index < CHANNELS) {
            sprintf(properties->label, "LS %d", index + 1);
            properties->flags = kVstPinIsActive | kVstPinIsStereo;
            return true;
        }
        return false;
    }


    bool LinuxSamplerVst::getEffectName(char* name) {
        vst_strncpy(name, "LinuxSampler", kVstMaxEffectNameLen);
        return true;
    }


    bool LinuxSamplerVst::getVendorString(char* text) {
        vst_strncpy(text, "linuxsampler.org", kVstMaxVendorStrLen);
        return true;
    }


    bool LinuxSamplerVst::getProductString(char* text) {
        vst_strncpy(text, "LinuxSampler VST", kVstMaxProductStrLen);
        return true;
    }


    VstInt32 LinuxSamplerVst::getVendorVersion() {
        return 1000;
    }


    VstInt32 LinuxSamplerVst::canDo(char* text) {
        dmsg(2, ("canDo %s\n", text));
        
        // supported features
        if (strcmp(text, "receiveVstEvents") == 0 ||
            strcmp(text, "receiveVstMidiEvent") == 0 ||
            strcmp(text, "midiProgramNames") == 0) return 1;
        
        // not supported features
        if (strcmp(text, "sendVstEvents") == 0 ||
            strcmp(text, "sendVstMidiEvent") == 0 ||
            strcmp(text, "receiveVstTimeInfo") == 0 ||
            strcmp(text, "offline") == 0 ||
            strcmp(text, "bypass") == 0) return -1;
        
        return 0; // "don't know", never heard of this feature
    }

    void LinuxSamplerVst::setSampleRate(float sampleRate) {
        dmsg(2, ("linuxsampler: setSampleRate %f\n", sampleRate));
        AudioEffectX::setSampleRate(sampleRate);
    }

    void LinuxSamplerVst::setBlockSize(VstInt32 blockSize) {
        dmsg(2, ("linuxsampler: setBlockSize %d\n", blockSize));
        AudioEffectX::setBlockSize(blockSize);
    }


    void LinuxSamplerVst::processReplacing(float** inputs, float** outputs,
                                           VstInt32 sampleFrames) {
        if (pAudioDevice) {
            for (int i = 0 ; i < CHANNELS ; i++) {
                pAudioDevice->Channel(i)->SetBuffer(outputs[i]);
            }
            pAudioDevice->Render(sampleFrames);
        } else {
            for (int i = 0 ; i < CHANNELS ; i++) {
                memset(outputs[i], 0, sampleFrames * sizeof(float));
            }
        }
    }


    VstInt32 LinuxSamplerVst::processEvents(VstEvents* ev) {
        if (pMidiDevice) {
            for (int i = 0; i < ev->numEvents; i++)
            {
                if (ev->events[i]->type == kVstMidiType) {
                    VstMidiEvent* event = reinterpret_cast<VstMidiEvent*>(ev->events[i]);
                    pMidiDevice->Port()->DispatchRaw(reinterpret_cast<uint8_t*>(event->midiData),
                                                     event->deltaFrames);
                }
            }
        }
        return 1;
    }


    VstInt32 LinuxSamplerVst::getChunk(void** data, bool isPreset) {
        dmsg(2, ("-->getChunk\n"));

        String state = GetState();
        dmsg(2, ("state=\"%s\"\n", state.c_str()));;
        if (StateBuf) free(StateBuf);
        StateBuf = strdup(state.c_str());
        *data = StateBuf;
        dmsg(2, ("<--getChunk\n"));
        return (VstInt32) state.length() + 1;
    }


    VstInt32 LinuxSamplerVst::setChunk(void* data, VstInt32 byteSize,
                                       bool isPreset) {
        dmsg(2, ("-->setChunk byteSize=%d isPreset=%d\n", byteSize, isPreset));

        const char* state = static_cast<const char*>(data);
        VstInt32 res = 0;
        if (byteSize > 0 && state[byteSize - 1] == '\0') {
            dmsg(2, ("state=\"%s\"\n", state));
            if (pAudioDevice) {
                res = SetState(state);
            } else {
                SavedChunk = state;
            }
        }
        dmsg(2, ("<--setChunk\n"));
        return res;
    }
}


// *************** VST main function ***************
// *

AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
    return new LinuxSamplerVst(audioMaster);
}
