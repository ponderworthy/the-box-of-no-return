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

#ifndef LS_PLUGINVST_H
#define LS_PLUGINVST_H

#include "../../drivers/Plugin.h"

#include <audioeffectx.h>
#include <aeffeditor.h>


namespace {

    class LinuxSamplerEditor : public AEffEditor {
    public:
        LinuxSamplerEditor(AudioEffect* effect);
        ~LinuxSamplerEditor();
        bool open(void* ptr);
        void close();
        bool getRect(ERect** rect);
    private:
#ifdef WIN32
        HANDLE ProcessHandle;
        LPTSTR Command;
#else
        pid_t pid;
#endif
        static String FindFantasia();
    };


    class LinuxSamplerVst : public AudioEffectX, LinuxSampler::Plugin {
    public:
        LinuxSamplerVst(audioMasterCallback audioMaster);
        ~LinuxSamplerVst();

        void processReplacing(float** inputs, float** outputs,
                              VstInt32 sampleframes);
        VstInt32 processEvents(VstEvents* events);

        void resume();
        void setProgram(VstInt32 program);
        void setProgramName(char* name);
        void getProgramName(char* name);
        bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text);

        void setSampleRate(float sampleRate);
        void setBlockSize(VstInt32 blockSize);

        bool getOutputProperties(VstInt32 index, VstPinProperties* properties);

        bool getEffectName(char* name);
        bool getVendorString(char* text);
        bool getProductString(char* text);
        VstInt32 getVendorVersion();
        VstInt32 canDo(char* text);
        VstInt32 getChunk(void** data, bool isPreset);
        VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset);

    private:
        char* StateBuf;
        String SavedChunk;
    };
}

#endif
