/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2007 - 2017 Christian Schoenebeck                       *
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

#include "InstrumentEditor.h"

#include "../common/global_private.h"

#include <algorithm>
#include <functional>

namespace LinuxSampler {

    InstrumentEditor::InstrumentEditor() : Thread(false, false, -1, 0) {
        pInstrument = NULL;
        pUserData   = NULL;
        pEngineChannel = NULL;
    }

    InstrumentEditor::~InstrumentEditor() {
    }

    void InstrumentEditor::Launch(EngineChannel* pEngineChannel, void* pInstrument, String sTypeName, String sTypeVersion, void* pUserData) {
        dmsg(1,("InstrumentEditor::Launch(instr=%p,type=%s,version=%s)\n", pInstrument, sTypeName.c_str(), sTypeVersion.c_str()));
        // prepare the editor's mandatory parameters
        this->pInstrument  = pInstrument;
        this->sTypeName    = sTypeName;
        this->sTypeVersion = sTypeVersion;
        this->pUserData    = pUserData;
        this->pEngineChannel = pEngineChannel;
        // start the editor in its own thread
        StartThread();
    }

    int InstrumentEditor::Main() {
        dmsg(1,("InstrumentEditor::Main()\n"));
        // run the editor's main loop
        int iResult = Main(pInstrument, sTypeName, sTypeVersion, pUserData);
        // reset editor parameters
        this->pInstrument  = NULL;
        this->sTypeName    = "";
        this->sTypeVersion = "";
        this->pUserData    = NULL;
        dmsg(1,("Instrument editor '%s' returned with exit status %d\n", Name().c_str(), iResult));
        // notify all registered listeners
        std::for_each(
            listeners.begin(), listeners.end(),
            std::bind2nd(std::mem_fun(&InstrumentEditorListener::OnInstrumentEditorQuit), this)
        );
        // done
        StopThread();
        return iResult;
    }

    EngineChannel* InstrumentEditor::GetEngineChannel() {
        return pEngineChannel;
    }

    void InstrumentEditor::AddListener(InstrumentEditorListener* pListener) {
        listeners.insert(pListener);
    }

    void InstrumentEditor::RemoveListener(InstrumentEditorListener* pListener) {
        listeners.erase(pListener);
    }

    void InstrumentEditor::NotifySamplesToBeRemoved(std::set<void*> Samples) {
        for ( // notify all registered listeners
            std::set<InstrumentEditorListener*>::iterator iter = listeners.begin();
            iter != listeners.end(); iter++
        ) (*iter)->OnSamplesToBeRemoved(Samples, this);
    }

    void InstrumentEditor::NotifySamplesRemoved() {
        for ( // notify all registered listeners
            std::set<InstrumentEditorListener*>::iterator iter = listeners.begin();
            iter != listeners.end(); iter++
        ) (*iter)->OnSamplesRemoved(this);
    }

    void InstrumentEditor::NotifyDataStructureToBeChanged(void* pStruct, String sStructType) {
        for ( // notify all registered listeners
            std::set<InstrumentEditorListener*>::iterator iter = listeners.begin();
            iter != listeners.end(); iter++
        ) (*iter)->OnDataStructureToBeChanged(pStruct, sStructType, this);
    }

    void InstrumentEditor::NotifyDataStructureChanged(void* pStruct, String sStructType) {
        for ( // notify all registered listeners
            std::set<InstrumentEditorListener*>::iterator iter = listeners.begin();
            iter != listeners.end(); iter++
        ) (*iter)->OnDataStructureChanged(pStruct, sStructType, this);
    }

    void InstrumentEditor::NotifySampleReferenceChanged(void* pOldSample, void* pNewSample) {
        for ( // notify all registered listeners
            std::set<InstrumentEditorListener*>::iterator iter = listeners.begin();
            iter != listeners.end(); iter++
        ) (*iter)->OnSampleReferenceChanged(pOldSample, pNewSample, this);
    }

} // namespace LinuxSampler
