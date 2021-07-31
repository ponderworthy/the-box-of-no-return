/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 Andreas Persson                                    *
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

#ifndef LS_PLUGIN_H
#define LS_PLUGIN_H

#include "../Sampler.h"
#include "../common/Thread.h"
#include "../network/lscpserver.h"
#include "audio/AudioOutputDevicePlugin.h"
#include "midi/MidiInputDevicePlugin.h"

namespace LinuxSampler {

    class EventThread;

    class PluginGlobal {
    public:
        PluginGlobal();
        virtual ~PluginGlobal();

        Sampler* pSampler;
        int RefCount;
        LSCPServer* pLSCPServer;
    private:
        EventThread* pEventThread;
    };

    class EventThread : public Thread {
    public:
        EventThread(Sampler* pSampler);
        int Main();
    private:
        Sampler* pSampler;
    };


    /** @brief Base class for LinuxSampler plugin implementations
     *
     * When LinuxSampler is run as a plugin in a sequencer, the
     * classes that implement the different type of plugin interfaces
     * (VST, DSSI, LV2, etc) can inherit from this class.
     *
     * The plugin gets access to audio and midi devices that it can
     * use to let the engine render the audio. Methods to get and set
     * the state of the sampler is also provided.
     */
    class Plugin {
    protected:
        Plugin(bool bDoPreInit = true);
        virtual ~Plugin();
        void PreInit();
        void Init(int SampleRate, int FragmentSize, int Channels = -1);

        virtual String PathToState(const String& string);
        virtual String PathFromState(const String& string);
        
        void InitState();
        String GetState();
        bool SetState(String State);
        void RemoveChannels();
        void DestroyDevice(AudioOutputDevicePlugin* pDevice);
        void DestroyDevice(MidiInputDevicePlugin* pDevice);

        AudioOutputDevicePlugin* pAudioDevice;
        MidiInputDevicePlugin* pMidiDevice;
        static PluginGlobal* global;

    private:
        bool bPreInitDone;
    };
}

#endif
