/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2009 Christian Schoenebeck                       *
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

#include "MidiInputDeviceFactory.h"

#include "../../common/global_private.h"

#if HAVE_ALSA
# include "MidiInputDeviceAlsa.h"
#endif // HAVE_ALSA

#if HAVE_COREMIDI
# include "MidiInputDeviceCoreMidi.h"
#endif // HAVE_CORE_MIDI

#if HAVE_MIDISHARE
# include "MidiInputDeviceMidiShare.h"
#endif // HAVE_MIDISHARE

#if HAVE_MME_MIDI
# include "MidiInputDeviceMme.h"
#endif // HAVE_MME_MIDI

#if HAVE_JACK_MIDI
# include "MidiInputDeviceJack.h"
#endif // HAVE_JACK_MIDI

namespace LinuxSampler {

    std::map<String, MidiInputDeviceFactory::InnerFactory*> MidiInputDeviceFactory::InnerFactories;
    std::map<String, DeviceParameterFactory*> MidiInputDeviceFactory::ParameterFactories;

#if HAVE_ALSA
    REGISTER_MIDI_INPUT_DRIVER(MidiInputDeviceAlsa);
    /* Common parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceAlsa, ParameterActive);
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceAlsa, ParameterPorts);
    /* Driver specific parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceAlsa, ParameterName);
#endif // HAVE_ALSA

#if HAVE_COREMIDI
    REGISTER_MIDI_INPUT_DRIVER(MidiInputDeviceCoreMidi);
    /* Common parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceCoreMidi, ParameterActive);
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceCoreMidi, ParameterPorts);
#endif // HAVE_COREMIDI

#if HAVE_MIDISHARE
    REGISTER_MIDI_INPUT_DRIVER(MidiInputDeviceMidiShare);
    /* Common parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceMidiShare, ParameterActive);
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceMidiShare, ParameterPorts);
#endif // HAVE_MIDISHARE

#if HAVE_MME_MIDI
    REGISTER_MIDI_INPUT_DRIVER(MidiInputDeviceMme);
    /* Common parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceMme, ParameterActive);
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceMme, ParameterPorts);
#endif // HAVE_MME_MIDI

#if HAVE_JACK_MIDI
    REGISTER_MIDI_INPUT_DRIVER(MidiInputDeviceJack);
    /* Common parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceJack, ParameterActive);
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceJack, ParameterPorts);
    /* Driver specific parameters */
    REGISTER_MIDI_INPUT_DRIVER_PARAMETER(MidiInputDeviceJack, ParameterName);
#endif // HAVE_JACK_MIDI

    MidiInputDeviceFactory::MidiInputDeviceMap MidiInputDeviceFactory::mMidiInputDevices;

    /**
     * Creates a new MIDI input device for the given driver name and
     * parameters. Note, this method will also check whether it is actually
     * allowed to create an instance of the given driver on its own and throw
     * an Exception in case it is not allowed (this is the case for host plugin
     * drivers like VST, AU, DSSI, LV2).
     *
     * @param DriverName - name of the driver of which a new device shall be created
     * @param Parameters - device creation parameters which shall be passed to the driver's constructor
     * @param pSampler - the sampler instance
     *
     * @returns pointer to the new device instance
     * @throws Exception - in case the device could not be created
     *
     * @see CreatePrivate()
     */
    MidiInputDevice* MidiInputDeviceFactory::Create(String DriverName, std::map<String,String> Parameters, Sampler* pSampler) throw (Exception) {
        if (!InnerFactories.count(DriverName))
            throw Exception("There is no midi input driver '" + DriverName + "'.");
        if (!InnerFactories[DriverName]->isAutonomousDriver())
            throw Exception("You cannot directly create a new MIDI input device of the '" + DriverName + "' driver!");

        return CreatePrivate(DriverName, Parameters, pSampler);
    }

    /**
     * Same as Create(), but this method won't check whether it is allowed to
     * create an instance of the given driver on its own. This method is
     * usually called by host plugins (e.g. VST, AU, DSSI, LV2) to actually
     * create their respective MIDI input devices for the sampler. Usually one
     * shouldn't call this method directly, but call Create() instead.
     */
    MidiInputDevice* MidiInputDeviceFactory::CreatePrivate(String DriverName, std::map<String,String> Parameters, Sampler* pSampler) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no midi input driver '" + DriverName + "'.");
        // let's see if we need to create parameters
        std::map<String,DeviceCreationParameter*> thisDeviceParams;
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            thisDeviceParams = pParamFactory->CreateAllParams(Parameters);
        } else {
            // no parameters are registered by the driver. Throw if any parameters were specified.
            if (Parameters.size() != 0) throw Exception("Driver '" + DriverName + "' does not have any parameters.");
        }

        // get a free device id
        int iDeviceId = -1;
        for (int i = 0; i >= 0; i++) { // seek for a free place starting from the beginning
            if (!mMidiInputDevices[i]) {
                iDeviceId = i;
                mMidiInputDevices.erase(i);
                break;
            }
        }
        if (iDeviceId < 0)
            throw Exception("Could not retrieve free device ID!");

        // now create the device using those parameters
        MidiInputDevice* pDevice = InnerFactories[DriverName]->Create(thisDeviceParams, pSampler);
        pDevice->setDeviceId(iDeviceId);
        // now attach all parameters to the newly created device.
        for (std::map<String,DeviceCreationParameter*>::iterator iter = thisDeviceParams.begin(); iter != thisDeviceParams.end(); iter++) {
            iter->second->Attach(pDevice);
        }

        // add new audio device to the audio device list
        mMidiInputDevices[iDeviceId] = pDevice;

        return pDevice;
    }

    std::vector<String> MidiInputDeviceFactory::AvailableDrivers() {
        std::vector<String> result;
        std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin();
        while (iter != InnerFactories.end()) {
            result.push_back(iter->first);
            iter++;
        }
        return result;
    }

    String MidiInputDeviceFactory::AvailableDriversAsString() {
        std::vector<String> drivers = AvailableDrivers();
        String result;
        std::vector<String>::iterator iter = drivers.begin();
        for (; iter != drivers.end(); iter++) {
            if (result != "") result += ",";
            result += *iter;
        }
        return result;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDeviceFactory::GetAvailableDriverParameters(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no midi input driver '" + DriverName + "'.");
        std::map<String,DeviceCreationParameter*> thisDeviceParams;
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            thisDeviceParams = pParamFactory->CreateAllParams();
        }
        return thisDeviceParams;
    }

    DeviceCreationParameter* MidiInputDeviceFactory::GetDriverParameter(String DriverName, String ParameterName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no midi input driver '" + DriverName + "'.");
        DeviceParameterFactory* pParamFactory = ParameterFactories[DriverName];
        if (pParamFactory) {
            try { return pParamFactory->Create(ParameterName); }
            catch(Exception e) { }
        }
        throw Exception("Midi input driver '" + DriverName + "' does not have a parameter '" + ParameterName + "'.");
    }

    String MidiInputDeviceFactory::GetDriverDescription(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no midi input driver '" + DriverName + "'.");
        return InnerFactories[DriverName]->Description();
    }

    String MidiInputDeviceFactory::GetDriverVersion(String DriverName) throw (Exception) {
        if (!InnerFactories.count(DriverName)) throw Exception("There is no midi input driver '" + DriverName + "'.");
        return InnerFactories[DriverName]->Version();
    }

    std::map<uint, MidiInputDevice*> MidiInputDeviceFactory::Devices() {
        return mMidiInputDevices;
    }

    /**
     * Destroys the given device. Usually this method shouldn't be called
     * directly, Sampler::DestroyMidiInputDevice should be called instead,
     * since it also takes care whether some sampler channel is still using
     * the device, etc.
     */
    void MidiInputDeviceFactory::Destroy(MidiInputDevice* pDevice) throw (Exception) {
        if (pDevice && !pDevice->isAutonomousDevice())
            throw Exception("You cannot directly destroy this '" + pDevice->Driver() + "' device!");

        DestroyPrivate(pDevice);
    }

    void MidiInputDeviceFactory::DestroyPrivate(MidiInputDevice* pDevice) throw (Exception) {
        MidiInputDeviceMap::iterator iter = mMidiInputDevices.begin();
        for (; iter != mMidiInputDevices.end(); iter++) {
            if (iter->second == pDevice) {
                // disable device
                pDevice->StopListen();

                // remove device from the device list
                mMidiInputDevices.erase(iter);

                // destroy and free device from memory
                delete pDevice;

                break;
            }
        }
    }

} // namespace LinuxSampler
