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

namespace LinuxSampler { namespace sf2 {
    EngineChannel::EngineChannel() {
        for(int i = 0; i < 128; i++) PressedKeys[i] = false;
        LastKey = LastKeySwitch = -1;
    }

    EngineChannel::~EngineChannel() {
        DisconnectAudioOutputDevice();
        // In case the channel was removed before the instrument was
        // fully loaded, try to give back instrument again (see bug #113)
        InstrumentChangeCmd< ::sf2::Region, ::sf2::Preset>& cmd = ChangeInstrument(NULL);
        if (cmd.pInstrument) {
            Engine::instruments.HandBack(cmd.pInstrument, this);
        }
        ///////
    }

    AbstractEngine::Format EngineChannel::GetEngineFormat() { return AbstractEngine::SF2; }

    /** This method is not thread safe! */
    void EngineChannel::ResetInternal(bool bResetEngine) {
        CurrentKeyDimension = 0;
        EngineChannelBase<Voice, ::sf2::Region, ::sf2::Preset>::ResetInternal(bResetEngine);
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
     * Load an instrument from a .sf2 file. PrepareLoadInstrument() has to
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
        InstrumentChangeCmd< ::sf2::Region, ::sf2::Preset>& cmd = ChangeInstrument(0);
        if (cmd.pInstrument) {
            // give old instrument back to instrument manager, but
            // keep the dimension regions and samples that are in use
            pInstrumentManager->HandBackInstrument(cmd.pInstrument, this, cmd.pRegionsInUse);
        }
        cmd.pRegionsInUse->clear();

        // delete all key groups
        DeleteGroupEventLists();

        // request sf2 instrument from instrument manager
        ::sf2::Preset* newInstrument;
        try {
            InstrumentManager::instrument_id_t instrid;
            instrid.FileName  = InstrumentFile;
            instrid.Index     = InstrumentIdx;

            newInstrument = pInstrumentManager->Borrow(instrid, this);
            if (!newInstrument) {
                throw InstrumentManagerException("resource was not created");
            }
        }
        catch (InstrumentManagerException e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sf2::Engine error: Failed to load instrument, cause: " + e.Message();
            throw Exception(msg);
        }
        catch (::sf2::Exception e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sf2::Engine error: Failed to load instrument, cause: " + e.Message;
            throw Exception(msg);
        }
        catch (::std::runtime_error e) {
            InstrumentStat = -3;
            StatusChanged(true);
            String msg = "sf2::Engine error: Failed to load instrument, cause: ";
            msg += e.what();
            throw Exception(msg);
        }
        catch (...) {
            InstrumentStat = -4;
            StatusChanged(true);
            throw Exception("sf2::Engine error: Failed to load instrument, cause: Unknown exception while trying to parse sf2 file.");
        }

        // rebuild ActiveKeyGroups map with key groups of current instrument
        for (int i = 0 ; i < newInstrument->GetRegionCount() ; i++) {
            ::sf2::Region* pRegion = newInstrument->GetRegion(i);
            for (int j = 0 ; j < pRegion->pInstrument->GetRegionCount() ; j++) {
                ::sf2::Region* pSubRegion = pRegion->pInstrument->GetRegion(j);
                AddGroup(pSubRegion->exclusiveClass);
            }
        }

        InstrumentIdxName = newInstrument->GetName();
        InstrumentStat = 100;

        ChangeInstrument(newInstrument);

        StatusChanged(true);
    }

    void EngineChannel::ProcessKeySwitchChange(int key) { }

}} // namespace LinuxSampler::sf2
