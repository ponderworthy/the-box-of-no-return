/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2015 Andreas Persson                             *
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

#define _DEFAULT_SOURCE 1
#define _BSD_SOURCE 1  /* for realpath() */

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include "PluginLv2.h"

#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#define NS_LS   "http://linuxsampler.org/schema#"

#define CHANNELS 32

namespace {

    PluginLv2::PluginLv2(const LV2_Descriptor* Descriptor,
                         double SampleRate, const char* BundlePath,
                         const LV2_Feature* const* Features) {
        Out = new float*[CHANNELS];
        for (int i = 0 ; i < CHANNELS ; i++) {
            Out[i] = 0;
        }
        UriMap = 0;
        MapPath = 0;
        MakePath = 0;
        for (int i = 0 ; Features[i] ; i++) {
            dmsg(2, ("linuxsampler: init feature: %s\n", Features[i]->URI));
            if (!strcmp(Features[i]->URI, LV2_URID__map)) {
                UriMap = (LV2_URID_Map*)Features[i]->data;
            } else if (!strcmp(Features[i]->URI, LV2_STATE__mapPath)) {
                MapPath = (LV2_State_Map_Path*)Features[i]->data;
            } else if (!strcmp(Features[i]->URI, LV2_STATE__makePath)) {
                MakePath = (LV2_State_Make_Path*)Features[i]->data;
            }
        }

        MidiEventType = uri_to_id(LV2_MIDI__MidiEvent);

        Init(SampleRate, 128, CHANNELS);

        InitState();

        DefaultState = GetState();
    }

    PluginLv2::~PluginLv2() {
        delete[] Out;
    }

    void PluginLv2::ConnectPort(uint32_t Port, void* DataLocation) {
        if (Port == 0) {
            MidiBuf = static_cast<LV2_Atom_Sequence*>(DataLocation);
        } else if (Port < CHANNELS + 1) {
            Out[Port - 1] = static_cast<float*>(DataLocation);
        }
    }

    void PluginLv2::Activate() {
        dmsg(2, ("linuxsampler: Activate\n"));
    }

    void PluginLv2::Run(uint32_t SampleCount) {
        int samplePos = 0;

        LV2_Atom_Event* ev = lv2_atom_sequence_begin(&MidiBuf->body);

        while (SampleCount) {
            int samples = std::min(SampleCount, 128U);

            for ( ; !lv2_atom_sequence_is_end(&MidiBuf->body,
                                              MidiBuf->atom.size, ev) ;
                  ev = lv2_atom_sequence_next(ev)) {
                if (ev->body.type == MidiEventType) {

                    int time = ev->time.frames - samplePos;
                    if (time >= samples) break;

                    uint8_t* data = reinterpret_cast<uint8_t*>(ev + 1);

                    pMidiDevice->Port()->DispatchRaw(data, time);
                }
            }
            for (int i = 0 ; i < CHANNELS ; i++) {
                pAudioDevice->Channel(i)->SetBuffer(Out[i] + samplePos);
            }
            pAudioDevice->Render(samples);

            samplePos += samples;
            SampleCount -= samples;
        }
    }

    void PluginLv2::Deactivate() {
        dmsg(2, ("linuxsampler: Deactivate\n"));
    }

    static String RealPath(const String& path)
    {
        String out   = path;
        char*  cpath = NULL;
#ifdef _WIN32
        cpath = (char*)malloc(MAX_PATH);
        GetFullPathName(path.c_str(), MAX_PATH, cpath, NULL);
#else
        cpath = realpath(path.c_str(), NULL);
#endif
        if (cpath) {
            out = cpath;
            free(cpath);
        }
        return out;
    }

    String PluginLv2::PathToState(const String& path) {
        if (MapPath) {
            char* cstr = MapPath->abstract_path(MapPath->handle, path.c_str());
            const String abstract_path(cstr);
            free(cstr);
            return abstract_path;
        }
        return path;
    }

    String PluginLv2::PathFromState(const String& path) {
        if (MapPath) {
            char* cstr = MapPath->absolute_path(MapPath->handle, path.c_str());
            // Resolve symbolic links so SFZ sample paths load correctly
            const String absolute_path(RealPath(cstr));
            free(cstr);
            return absolute_path;
        }
        return path;
    }

    void PluginLv2::SetStateFeatures(const LV2_Feature* const* Features)
    {
        for (int i = 0 ; Features[i] ; i++) {
            dmsg(2, ("linuxsampler: state feature: %s\n", Features[i]->URI));
            if (!strcmp(Features[i]->URI, LV2_STATE__mapPath)) {
                MapPath = (LV2_State_Map_Path*)Features[i]->data;
            } else if (!strcmp(Features[i]->URI, LV2_STATE__makePath)) {
                MakePath = (LV2_State_Make_Path*)Features[i]->data;
            }
        }
    }

    LV2_State_Status PluginLv2::Save(
        LV2_State_Store_Function store, LV2_State_Handle handle,
        uint32_t flags, const LV2_Feature* const* features)
    {
        LV2_State_Map_Path*  OldMapPath  = MapPath;
        LV2_State_Make_Path* OldMakePath = MakePath;
        SetStateFeatures(features);

        if (MakePath && MapPath) {
            char* abs_path = MakePath->path(MakePath->handle, "linuxsampler");
            dmsg(2, ("saving to file %s\n", abs_path));

            std::ofstream out(abs_path);
            out << GetState();

            char* path = MapPath->abstract_path(MapPath->handle, abs_path);

            store(handle,
                  uri_to_id(NS_LS "state-file"),
                  path,
                  strlen(path) + 1,
                  uri_to_id(LV2_ATOM__Path),
                  LV2_STATE_IS_PORTABLE);

            free(path);
            free(abs_path);
        } else {
            dmsg(2, ("saving to string\n"));

            std::ostringstream out;
            out << GetState();

            store(handle,
                  uri_to_id(NS_LS "state-string"),
                  out.str().c_str(),
                  out.str().length() + 1,
                  uri_to_id(LV2_ATOM__String),
                  LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
        }
        dmsg(2, ("saving done\n"));

        MapPath  = OldMapPath;
        MakePath = OldMakePath;

        return LV2_STATE_SUCCESS;
    }

    LV2_State_Status PluginLv2::Restore(
        LV2_State_Retrieve_Function retrieve, LV2_State_Handle handle,
        uint32_t rflags, const LV2_Feature* const* features)
    {
        LV2_State_Map_Path*  OldMapPath  = MapPath;
        LV2_State_Make_Path* OldMakePath = MakePath;
        SetStateFeatures(features);

        size_t   size;
        uint32_t type;
        uint32_t flags;

        const void* value = retrieve(
            handle,
            uri_to_id(NS_LS "state-file"),
            &size, &type, &flags);
        if (value) {
            // Restore from state-file
            assert(type == uri_to_id(LV2_ATOM__Path));
            const String path((const char*)value);
            dmsg(2, ("linuxsampler: restoring from file %s\n", path.c_str()));
            std::ifstream in(path.c_str());
            String state;
            std::getline(in, state, '\0');
            SetState(state);
        } else if ((value = retrieve(handle,
                                     uri_to_id(NS_LS "state-string"),
                                     &size, &type, &flags))) {
            // Restore from state-string
            dmsg(2, ("linuxsampler: restoring from string\n"));
            assert(type == uri_to_id(LV2_ATOM__String));
            String state((const char*)value);
            SetState(state);
        } else {
            // No valid state found, reset to default state
            dmsg(2, ("linuxsampler: restoring default state\n"));
            SetState(DefaultState);
        }

        MapPath  = OldMapPath;
        MakePath = OldMakePath;

        return LV2_STATE_SUCCESS;
    }

    LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                           double sample_rate, const char* bundle_path,
                           const LV2_Feature* const* features) {
        return new PluginLv2(descriptor, sample_rate, bundle_path, features);
    }

    void connect_port(LV2_Handle instance, uint32_t port, void* data_location) {
        static_cast<PluginLv2*>(instance)->ConnectPort(port, data_location);
    }

    void activate(LV2_Handle instance) {
        static_cast<PluginLv2*>(instance)->Activate();
    }

    void run(LV2_Handle instance, uint32_t sample_count) {
        static_cast<PluginLv2*>(instance)->Run(sample_count);
    }

    void deactivate(LV2_Handle instance) {
        static_cast<PluginLv2*>(instance)->Deactivate();
    }

    void cleanup(LV2_Handle instance) {
        delete static_cast<PluginLv2*>(instance);
    }

    LV2_State_Status save(LV2_Handle handle, LV2_State_Store_Function store,
                          LV2_State_Handle state,
                          uint32_t flags, const LV2_Feature* const* features) {
        return static_cast<PluginLv2*>(handle)->Save(
            store, state, flags, features);
    }

    LV2_State_Status restore(LV2_Handle handle, LV2_State_Retrieve_Function retrieve,
                             LV2_State_Handle state,
                             uint32_t flags, const LV2_Feature* const* features) {
        return static_cast<PluginLv2*>(handle)->Restore(
            retrieve, state, flags, features);
    }

    PluginInfo PluginInfo::Instance;

    PluginInfo::PluginInfo() {
        Lv2.URI = "http://linuxsampler.org/plugins/linuxsampler";
        Lv2.activate = activate;
        Lv2.cleanup = cleanup;
        Lv2.connect_port = connect_port;
        Lv2.deactivate = deactivate;
        Lv2.instantiate = instantiate;
        Lv2.run = run;
        Lv2.extension_data = extension_data;
        StateInterface.save = save;
        StateInterface.restore = restore;
    }


    const void* extension_data(const char* uri) {
        dmsg(2, ("linuxsampler: extension_data %s\n", uri));
        if (strcmp(uri, LV2_STATE__interface) == 0) {
            return PluginInfo::Lv2StateInterface();
        }
        return 0;
    }
}


extern "C" {
    LV2_SYMBOL_EXPORT
    const LV2_Descriptor* lv2_descriptor(uint32_t index) {
        return index == 0 ? PluginInfo::Lv2Descriptor() : 0;
    }
}
