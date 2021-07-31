/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2009 Christian Schoenebeck                         *
 *   Copyright (C) 2009 - 2012 Christian Schoenebeck and Grigor Iliev      *
 *   Copyright (C) 2012 - 2016 Christian Schoenebeck and Andreas Persson   *
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

#include "EngineChannel.h"
#include "Engine.h"

namespace LinuxSampler { namespace sfz {
    EngineChannel::EngineChannel() {
        for(int i = 0; i < 128; i++) PressedKeys[i] = false;
        LastKey = LastKeySwitch = -1;
        AddMidiKeyboardListener(this);
    }

    EngineChannel::~EngineChannel() {
        DisconnectAudioOutputDevice();
        RemoveMidiKeyboardListener(this);
        // In case the channel was removed before the instrument was
        // fully loaded, try to give back instrument again (see bug #113)
        InstrumentChangeCmd< ::sfz::Region, ::sfz::Instrument>& cmd = ChangeInstrument(NULL);
        if (cmd.pInstrument) {
            Engine::instruments.HandBack(cmd.pInstrument, this);
        }
        ///////
    }

    AbstractEngine::Format EngineChannel::GetEngineFormat() { return AbstractEngine::SFZ; }

    /** This method is not thread safe! */
    void EngineChannel::ResetInternal(bool bResetEngine) {
        CurrentKeyDimension = 0;
        EngineChannelBase<Voice, ::sfz::Region, ::sfz::Instrument>::ResetInternal(bResetEngine);
        for(int i = 0; i < 128; i++) PressedKeys[i] = false;
    }

    /**
     *  Will be called by the MIDIIn Thread to signal that a program
     *  change should be performed. As a program change isn't
     *  real-time safe, the actual change is performed by the disk
     *  thread.
     *
     *  @param Program     - MIDI program change number
     */
    void EngineChannel::SendProgramChange(uint8_t Program) {
        SetMidiProgram(Program);
        Engine* engine = dynamic_cast<Engine*>(pEngine);
        if(engine == NULL) return;

        if(engine->GetDiskThread()) {
            uint32_t merged = (GetMidiBankMsb() << 16) | (GetMidiBankLsb() << 8) | Program;
            engine->GetDiskThread()->OrderProgramChange(merged, this);
        } else {
            // TODO: 
        }
    }

    /**
     * Load an instrument from a .sfz file. PrepareLoadInstrument() has to
     * be called first to provide the information which instrument to load.
     * This method will then actually start to load the instrument and block
     * the calling thread until loading was completed.
     *
     * @see PrepareLoadInstrument()
     */
    void EngineChannel::LoadInstrument() {
        InstrumentResourceManager* pInstrumentManager = dynamic_cast<InstrumentResourceManager*>(pEngine->GetInstrumentManager());

        // make sure we don't trigger any new notes with an old
        // instrument
        InstrumentChangeCmd< ::sfz::Region, ::sfz::Instrument>& cmd = ChangeInstrument(0);
        if (cmd.pInstrument) {
            // give old instrument back to instrument manager, but
            // keep the dimension regions and samples that are in use
            pInstrumentManager->HandBackInstrument(cmd.pInstrument, this, cmd.pRegionsInUse);
        }
        if (cmd.pScript) {
            // give old instrument script back to instrument resource manager
            cmd.pScript->resetAll();
        }
        cmd.pRegionsInUse->clear();

        // delete all key groups
        DeleteGroupEventLists();

        // request sfz instrument from instrument manager
        ::sfz::Instrument* newInstrument;
        try {
            InstrumentManager::instrument_id_t instrid;
            instrid.FileName  = InstrumentFile;
            instrid.Index     = InstrumentIdx;

            newInstrument = pInstrumentManager->Borrow(instrid, this);
            if (!newInstrument) {
                throw InstrumentManagerException("resource was not created");
            }

            // if requested by set_ccN opcode in sfz file, set initial CC values
            for (std::map<uint8_t,uint8_t>::const_iterator itCC = newInstrument->initialCCValues.begin();
                 itCC != newInstrument->initialCCValues.end(); ++itCC)
            {
                const uint8_t& cc = itCC->first;
                uint8_t value = itCC->second;
                if (cc >= CTRL_TABLE_SIZE) continue;
                if ((cc < 128 || cc == CTRL_TABLE_IDX_AFTERTOUCH) && value > 127) value = 127;
                ControllerTable[cc] = value;
            }

            if (newInstrument->scripts.size() > 1) {
                std::cerr << "WARNING: Executing more than one real-time instrument script slot is not implemented yet!\n";
            }
            ::sfz::Script* script = (!newInstrument->scripts.empty()) ? &newInstrument->scripts[0] : NULL;
            if (script) {
                String sourceCode = script->GetSourceCode();
                LoadInstrumentScript(sourceCode);
            }
        }
        catch (InstrumentManagerException e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sfz::Engine error: Failed to load instrument, cause: " + e.Message();
            throw Exception(msg);
        }
        catch (::sfz::Exception e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sfz::Engine error: Failed to load instrument, cause: " + e.Message();
            throw Exception(msg);
        }
        catch (::std::runtime_error e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sfz::Engine error: Failed to load instrument, cause: ";
            msg += e.what();
            throw Exception(msg);
        }
        catch (...) {
            InstrumentStat = -4;
            StatusChanged(true);
            throw Exception("sfz::Engine error: Failed to load instrument, cause: Unknown exception while trying to parse sfz file.");
        }

        // rebuild ActiveKeyGroups map with key groups of current instrument
        for (std::vector< ::sfz::Region*>::iterator itRegion = newInstrument->regions.begin() ;
             itRegion != newInstrument->regions.end() ; ++itRegion) {
            AddGroup((*itRegion)->group);
            AddGroup((*itRegion)->off_by);
        }

        InstrumentIdxName = newInstrument->GetName();
        InstrumentStat = 100;

        {
            InstrumentChangeCmd< ::sfz::Region, ::sfz::Instrument>& cmd =
                ChangeInstrument(newInstrument);
            if (cmd.pScript) {
                // give old instrument script back to instrument resource manager
                cmd.pScript->resetAll();
            }
        }

        StatusChanged(true);
    }

    void EngineChannel::ProcessKeySwitchChange(int key) { }

    void EngineChannel::PreProcessNoteOn(uint8_t key, uint8_t velocity) {
        if(pInstrument != NULL && pInstrument->HasKeySwitchBinding(key)) LastKeySwitch = key;
        PressedKeys[key] = true;
    }

    void EngineChannel::PostProcessNoteOn(uint8_t key, uint8_t velocity) {
        LastKey = key;
    }

    void EngineChannel::PreProcessNoteOff(uint8_t key, uint8_t velocity) {
        PressedKeys[key] = false;
    }

}} // namespace LinuxSampler::sfz
