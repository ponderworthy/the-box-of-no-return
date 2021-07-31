/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
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

namespace LinuxSampler { namespace gig {
    EngineChannel::EngineChannel() {
        CurrentGigScript = NULL;
    }

    EngineChannel::~EngineChannel() {
        DisconnectAudioOutputDevice();
        // In case the channel was removed before the instrument was
        // fully loaded, try to give back instrument again (see bug #113)
        InstrumentChangeCmd< ::gig::DimensionRegion, ::gig::Instrument>& cmd = ChangeInstrument(NULL);
        if (cmd.pInstrument) {
            Engine::instruments.HandBack(cmd.pInstrument, this);
        }
        ///////
    }

    AbstractEngine::Format EngineChannel::GetEngineFormat() { return AbstractEngine::GIG; }

    /** This method is not thread safe! */
    void EngineChannel::ResetInternal(bool bResetEngine) {
        CurrentKeyDimension = 0;
        EngineChannelBase<Voice, ::gig::DimensionRegion, ::gig::Instrument>::ResetInternal(bResetEngine);
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
     * Load an instrument from a .gig file. PrepareLoadInstrument() has to
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
        InstrumentChangeCmd< ::gig::DimensionRegion, ::gig::Instrument>& cmd = ChangeInstrument(NULL);
        if (cmd.pInstrument) {
            // give old instrument back to instrument manager, but
            // keep the dimension regions and samples that are in use
            pInstrumentManager->HandBackInstrument(cmd.pInstrument, this, cmd.pRegionsInUse);
        }
        if (cmd.pScript) {
            // give old instrument script back to instrument resource manager
            cmd.pScript->resetAll();
            CurrentGigScript = NULL;
        }
        cmd.pRegionsInUse->clear();

        // delete all key groups
        DeleteGroupEventLists();

        // request gig instrument from instrument manager
        ::gig::Instrument* newInstrument;
        try {
            InstrumentManager::instrument_id_t instrid;
            instrid.FileName  = InstrumentFile;
            instrid.Index     = InstrumentIdx;

            newInstrument = pInstrumentManager->Borrow(instrid, this);
            if (!newInstrument) {
                throw InstrumentManagerException("resource was not created");
            }

            if (newInstrument->ScriptSlotCount() > 1) {
                std::cerr << "WARNING: Executing more than one real-time instrument script slot is not implemented yet!\n";
            }
            ::gig::Script* script = newInstrument->GetScriptOfSlot(0);
            if (script) {
                String sourceCode = script->GetScriptAsText();
                LoadInstrumentScript(sourceCode);
            }
            CurrentGigScript = script;
        }
        catch (RIFF::Exception e) {
            InstrumentStat = -2;
            StatusChanged(true);
            String msg = "gig::Engine error: Failed to load instrument, cause: " + e.Message;
            throw Exception(msg);
        }
        catch (InstrumentManagerException e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "gig::Engine error: Failed to load instrument, cause: " + e.Message();
            throw Exception(msg);
        }
        catch (...) {
            InstrumentStat = -4;
            StatusChanged(true);
            throw Exception("gig::Engine error: Failed to load instrument, cause: Unknown exception while trying to parse gig file.");
        }

        RoundRobinIndex = 0;
        for (int i = 0 ; i < 128 ; i++) pMIDIKeyInfo[i].pRoundRobinIndex = NULL;

        // rebuild ActiveKeyGroups map with key groups of current
        // instrument and set the round robin pointers to use one
        // counter for each region
        int region = 0;
        for (::gig::Region* pRegion = newInstrument->GetFirstRegion(); pRegion; pRegion = newInstrument->GetNextRegion()) {
            AddGroup(pRegion->KeyGroup);

            RoundRobinIndexes[region] = 0;
            for (int iKey = pRegion->KeyRange.low; iKey <= pRegion->KeyRange.high; iKey++) {
                pMIDIKeyInfo[iKey].pRoundRobinIndex = &RoundRobinIndexes[region];
            }
            region++;
        }

        InstrumentIdxName = newInstrument->pInfo->Name;
        InstrumentStat = 100;

        {
            InstrumentChangeCmd< ::gig::DimensionRegion, ::gig::Instrument>& cmd =
                ChangeInstrument(newInstrument);
            if (cmd.pScript) {
                // give old instrument script back to instrument resource manager
                cmd.pScript->resetAll();
            }
        }

        StatusChanged(true);
    }

    /**
     * Called by instrument editor API to inform this engine channel that the
     * passed @a script has been modified by the instrument editor and that
     * this engine channel should thus reload the instrument script (that is
     * that it should re-parse the scripts new source code).
     */
    void EngineChannel::reloadScript(::gig::Script* script) {
        dmsg(3,("gig::EngineChannel::realoadScript()\n"));
        // make sure this engine channel is actually using the requested
        // modified gig::Script object (because the internal script resource
        // manager will i.e. provide the same VM object for different
        // ::gig::Script objects with same source code though.
        if (!script || CurrentGigScript != script) return;

        //TODO: reloading the entire instrument is currently a very lazy (and slow) solution just for reloading the instrument script
        try {
            LoadInstrument();
        } catch (Exception e) {
            std::cerr << e.Message() << std::endl;
        } catch (...) {
            String msg = "gig::Engine error: Failed to reload instrument script, cause: Unknown exception while trying to reload gig file.";
            std::cerr << msg << std::endl;
        }
    }

    void EngineChannel::ProcessKeySwitchChange(int key) {
        // Change key dimension value if key is in keyswitching area
        {
            if (key >= pInstrument->DimensionKeyRange.low && key <= pInstrument->DimensionKeyRange.high)
                CurrentKeyDimension = float(key - pInstrument->DimensionKeyRange.low) /
                    (pInstrument->DimensionKeyRange.high - pInstrument->DimensionKeyRange.low + 1);
        }
    }
    
    String EngineChannel::InstrumentFileName() {
        return EngineChannelBase<Voice, ::gig::DimensionRegion, ::gig::Instrument>::InstrumentFileName();
    }
    
    String EngineChannel::InstrumentFileName(int index) {
        if (index == 0) return InstrumentFileName();
        if (!pInstrument || !pInstrument->GetParent()) return "";
        DLS::File* pMainFile = dynamic_cast<DLS::File*>(pInstrument->GetParent());
        if (!pMainFile) return "";
        RIFF::File* pExtensionFile = pMainFile->GetExtensionFile(index);
        return (pExtensionFile) ? pExtensionFile->GetFileName() : "";
    }

}} // namespace LinuxSampler::gig
