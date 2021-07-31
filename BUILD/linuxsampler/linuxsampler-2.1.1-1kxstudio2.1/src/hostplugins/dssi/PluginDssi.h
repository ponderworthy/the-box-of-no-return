/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2012 Andreas Persson                             *
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

#ifndef LS_PLUGINDSSI_H
#define LS_PLUGINDSSI_H

#include <ladspa.h>
#include <dssi.h>

#include "../../drivers/Plugin.h"

namespace {

    class PluginDssi : public LinuxSampler::Plugin {
    public:
        PluginDssi(unsigned long SampleRate);
    private:
        int RefCount;
        friend class PluginInstance;
    };

    class PluginInstance {
    public:
        PluginInstance(unsigned long SampleRate);
        ~PluginInstance();
        void ConnectPort(unsigned long Port, LADSPA_Data* DataLocation);
        char* Configure(const char* Key, const char* Value);
        void Activate();
        static void RunMultipleSynths(unsigned long InstanceCount,
                                      LADSPA_Handle* Instances,
                                      unsigned long SampleCount,
                                      snd_seq_event_t** Events,
                                      unsigned long* EventCounts);
    private:
        static PluginDssi* plugin;
        LinuxSampler::SamplerChannel* pChannel;
        LinuxSampler::MidiInputPort* pPort;
        LinuxSampler::AudioChannel* pChannelLeft;
        LinuxSampler::AudioChannel* pChannelRight;

        LADSPA_Data* Out[2];
    };

    class PluginInfo {
    public:
        static const LADSPA_Descriptor* LadspaDescriptor() {
            return &Instance.Ladspa;
        }
        static const DSSI_Descriptor* DssiDescriptor() {
            return &Instance.Dssi;
        }
    private:
        LADSPA_Descriptor Ladspa;
        DSSI_Descriptor Dssi;
        LADSPA_PortDescriptor PortDescriptors[2];
        LADSPA_PortRangeHint PortRangeHints[2];
        const char* PortNames[2];

        PluginInfo();
        static PluginInfo Instance;
    };

    extern "C" {
        // LADSPA interface
        static LADSPA_Handle instantiate(const LADSPA_Descriptor* Descriptor,
                                         unsigned long SampleRate);
        static void connect_port(LADSPA_Handle Instance, unsigned long Port,
                                 LADSPA_Data* DataLocation);
        static void activate(LADSPA_Handle Instance);
        static void run(LADSPA_Handle Instance, unsigned long SampleCount);
        static void cleanup(LADSPA_Handle Instance);

        // DSSI interface
        static char* configure(LADSPA_Handle Instance,
                               const char* Key,
                               const char* Value);
        static void run_multiple_synths(unsigned long InstanceCount,
                                        LADSPA_Handle* Instances,
                                        unsigned long SampleCount,
                                        snd_seq_event_t** Events,
                                        unsigned long* EventCounts);
    }
}

#endif
