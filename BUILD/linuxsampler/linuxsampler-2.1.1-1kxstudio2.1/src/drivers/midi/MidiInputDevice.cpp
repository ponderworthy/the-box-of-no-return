/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#include "MidiInputDevice.h"

#include "../../common/global_private.h"
#include "../../Sampler.h"
#include "MidiInputDeviceFactory.h"

namespace LinuxSampler {

// *************** ParameterActive ***************
// *

    MidiInputDevice::ParameterActive::ParameterActive() : DeviceCreationParameterBool() {
        InitWithDefault();
    }

    MidiInputDevice::ParameterActive::ParameterActive(String active) : DeviceCreationParameterBool(active) {
    }

    String MidiInputDevice::ParameterActive::Description() {
        return "Enable / disable device";
    }

    bool MidiInputDevice::ParameterActive::Fix() {
        return false;
    }

    bool MidiInputDevice::ParameterActive::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDevice::ParameterActive::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<bool> MidiInputDevice::ParameterActive::DefaultAsBool(std::map<String,String> Parameters) {
        return true;
    }

    void MidiInputDevice::ParameterActive::OnSetValue(bool b) throw (Exception) {
        if (b) ((MidiInputDevice*)pDevice)->Listen();
        else ((MidiInputDevice*)pDevice)->StopListen();
    }

    String MidiInputDevice::ParameterActive::Name() {
        return "ACTIVE";
    }



// *************** ParameterPorts ***************
// *

    MidiInputDevice::ParameterPorts::ParameterPorts() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    MidiInputDevice::ParameterPorts::ParameterPorts(String val) : DeviceCreationParameterInt(val) {
    }

    String MidiInputDevice::ParameterPorts::Description() {
        return "Number of ports";
    }

    bool MidiInputDevice::ParameterPorts::Fix() {
        return false;
    }

    bool MidiInputDevice::ParameterPorts::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDevice::ParameterPorts::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    optional<int> MidiInputDevice::ParameterPorts::DefaultAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    optional<int> MidiInputDevice::ParameterPorts::RangeMinAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    optional<int> MidiInputDevice::ParameterPorts::RangeMaxAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    std::vector<int> MidiInputDevice::ParameterPorts::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void MidiInputDevice::ParameterPorts::OnSetValue(int i) throw (Exception) {
        MidiInputDevice* dev = static_cast<MidiInputDevice*> (pDevice);
        Sampler* s = static_cast<Sampler*> (dev->pSampler);
        std::map<uint, SamplerChannel*> channels = s->GetSamplerChannels();
        std::map<uint, SamplerChannel*>::iterator iter = channels.begin();
        for (; iter != channels.end(); iter++) {
            SamplerChannel* chn = iter->second;
            std::vector<MidiInputPort*> vPorts = chn->GetMidiInputPorts();
            for (int k = 0; k < vPorts.size(); ++k) {
                if (vPorts[k]->GetDevice() != pDevice)
                    continue;
                int port = vPorts[k]->GetPortNumber();
                if (port >= i) {
                    String err = "Sampler channel " + ToString(iter->first);
                    err += " is still connected to MIDI port " + ToString(port);
                    throw Exception(err);
                }
            }
        }

        ((MidiInputDevice*)pDevice)->AcquirePorts(i);
    }

    String MidiInputDevice::ParameterPorts::Name() {
        return "PORTS";
    }



// *************** MidiInputDevice ***************
// *

    MidiInputDevice::MidiInputDevice(std::map<String,DeviceCreationParameter*> DriverParameters, void* pSampler) {
        this->Parameters = DriverParameters;
        this->pSampler   = pSampler;
    }

    MidiInputDevice::~MidiInputDevice() {
        std::map<String,DeviceCreationParameter*>::iterator iter = Parameters.begin();
        while (iter != Parameters.end()) {
            delete iter->second;
            iter++;
        }
        Parameters.clear();
    }

    MidiInputPort* MidiInputDevice::GetPort(uint iPort) throw (MidiInputException) {
        if (iPort >= Ports.size()) throw MidiInputException("There is no port " + ToString(iPort));
        return Ports[iPort];
    }

    uint MidiInputDevice::PortCount() {
        return (uint) Ports.size();
    }

    std::map<String,DeviceCreationParameter*> MidiInputDevice::DeviceParameters() {
        return Parameters;
    }

    int MidiInputDevice::MidiInputDeviceID() {
        std::map<uint, MidiInputDevice*> mDevices = MidiInputDeviceFactory::Devices();
        for (std::map<uint, MidiInputDevice*>::const_iterator it = mDevices.begin(); it != mDevices.end(); ++it) {
            if (it->second == this) {
                return it->first;
            }
        }
        return -1;
    }

    void MidiInputDevice::AddMidiPortCountListener(MidiPortCountListener* l) {
        portCountListeners.AddListener(l);
    }

    void MidiInputDevice::RemoveMidiPortCountListener(MidiPortCountListener* l) {
        portCountListeners.RemoveListener(l);
    }

    void MidiInputDevice::fireMidiPortCountChanged(int NewCount) {
        for (int i = 0; i < portCountListeners.GetListenerCount(); i++) {
            portCountListeners.GetListener(i)->MidiPortCountChanged(NewCount);
        }
    }

    void MidiInputDevice::fireMidiPortToBeRemoved(MidiInputPort* pPort) {
        for (int i = 0; i < portCountListeners.GetListenerCount(); i++) {
            portCountListeners.GetListener(i)->MidiPortToBeRemoved(pPort);
        }
    }

    void MidiInputDevice::fireMidiPortAdded(MidiInputPort* pPort) {
        for (int i = 0; i < portCountListeners.GetListenerCount(); i++) {
            portCountListeners.GetListener(i)->MidiPortAdded(pPort);
        }
    }

    void MidiInputDevice::AcquirePorts(uint newPorts) {
        //FIXME: hooo, this looks scary, no synchronization AT ALL yet!
        int diff = int(this->Ports.size() - newPorts);
        if (!diff)
            return; // number of ports matches already, nothing to do

        while (diff != 0) {
            if (diff > 0) { // we've got too many ports, remove one
                std::map<int,MidiInputPort*>::iterator portsIter = Ports.end();
                --portsIter;

                fireMidiPortToBeRemoved(portsIter->second);
                delete portsIter->second;
                Ports.erase(portsIter);
                diff--;
            }
            if (diff < 0) { // we don't have enough ports, create one
                MidiInputPort* midiPort = this->CreateMidiPort();
                Ports[midiPort->portNumber] = midiPort;
                diff++;
                fireMidiPortAdded(midiPort);
            }
        }
        fireMidiPortCountChanged((int)Ports.size());
    }

} // namespace LinuxSampler
