/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2014 Christian Schoenebeck                       *
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

#ifndef __LS_MIDIINPUTDEVICE_H__
#define __LS_MIDIINPUTDEVICE_H__

#include <stdexcept>
#include <set>
#include <map>
#include <vector>

#include "../../common/global.h"
#include "../../common/Exception.h"
#include "../DeviceParameter.h"
#include "MidiInputPort.h"
#include "../../engines/Engine.h"
#include "../../EventListeners.h"

namespace LinuxSampler {

    // just symbol prototyping
    class MidiInputPort;
    class Engine;
    class MidiInputDeviceFactory;

    /**
     * Midi input exception that should be thrown by the MidiInputDevice
     * descendants in case initialization of the MIDI input system failed
     * (which should be done in the constructor of the MidiInputDevice
     * descendant).
     */
    class MidiInputException : public Exception {
        public:
            MidiInputException(const std::string& msg) : Exception(msg) {}
    };

    /** Abstract base class for MIDI input drivers in LinuxSampler
     *
     * This class will be derived by specialized classes which implement the
     * connection to a specific MIDI input system (e.g. Alsa Sequencer,
     * CoreMIDI). The MidiInputDevice desendant should just call the
     * appropriate (protected) Dispatch* method here when an MIDI event
     * occured. The dispatch* methods here will automatically forward the
     * MIDI event to the appropriate, connected sampler engines.
     */
    class MidiInputDevice : public Device {
        public:

            /////////////////////////////////////////////////////////////////
            // type definitions

            /** Device Parameter 'ACTIVE'
             *
             * Used to activate / deactivate the MIDI input device.
             */
            class ParameterActive : public DeviceCreationParameterBool {
                public:
                    ParameterActive();
                    ParameterActive(String active);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<bool> DefaultAsBool(std::map<String,String> Parameters) OVERRIDE;
                    virtual void OnSetValue(bool b) throw (Exception) OVERRIDE;
                    static String Name();
            };

            /** Device Parameter 'PORTS'
             *
             * Used to increase / decrease the number of MIDI ports of the
             * MIDI input device.
             */
            class ParameterPorts : public DeviceCreationParameterInt {
                public:
                    ParameterPorts();
                    ParameterPorts(String val);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<int>    DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMinAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMaxAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual std::vector<int> PossibilitiesAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual void             OnSetValue(int i) throw (Exception) OVERRIDE;
                    static String Name();
            };



            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Start listen to MIDI input events on the MIDI input port.
             * The MIDIInputPort descendant should forward all MIDI input
             * events by calling the appropriate (protected) Dispatch*
             * method of class MidiInputPort.
             */
            virtual void Listen() = 0;

            /**
             * Stop to listen to MIDI input events on the MIDI input port.
             * After this method was called, the MidiInputPort descendant
             * should ignore all MIDI input events.
             */
            virtual void StopListen() = 0;

            /**
             * Return device driver name
             */
            virtual String Driver() = 0;

            /**
             * Create new Midi port
             * This will be called by AcquirePorts
             * Each individual device must implement this.
             */
            virtual MidiInputPort* CreateMidiPort() = 0;



            /////////////////////////////////////////////////////////////////
            // normal methods
            //     (usually not to be overriden by descendant)

            /**
             * Return midi port \a iPort.
             *
             * @throws MidiInputException  if index out of bounds
             */
            MidiInputPort* GetPort(uint iPort) throw (MidiInputException);

            /**
             * Returns amount of MIDI ports this MIDI input device currently
             * provides.
             */
            uint PortCount();

            /**
             * Return all device parameter settings.
             */
            std::map<String,DeviceCreationParameter*> DeviceParameters();

            /**
             * Returns the unique ID number associated with this MIDIInputDevice
             * instance. This ID number is unique among all MIDIInputDevice
             * instances of the same Sampler instance and during the whole
             * lifetime of the Sampler instance.
             *
             * @returns a value equal or larger than 0, a negative value only
             *          on severe internal problems
             */
            int MidiInputDeviceID();

            /**
             * Registers the specified listener to be notified
             * when the number of MIDI input ports is changed.
             */
            void AddMidiPortCountListener(MidiPortCountListener* l);

            /**
             * Removes the specified listener, to stop being notified of
             * further MIDI input port count chances.
             */
            void RemoveMidiPortCountListener(MidiPortCountListener* l);

        protected:
            std::map<String,DeviceCreationParameter*> Parameters;  ///< All device parameters.
            std::map<int,MidiInputPort*> Ports;                    ///< All MIDI ports.
            void* pSampler;                                        ///< Sampler instance. FIXME: should actually be of type Sampler*
            ListenerList<MidiPortCountListener*> portCountListeners;

            /**
             * Constructor
             *
             * FIXME: the pointer argument \a pSapmler should actually be of type Sampler*.
             * Unfortunately the bidirectional relationship between this
             * header and Sampler.h would clash on header file inclusion,
             * so that's why I had to make it of type void* here. This is
             * an annoying constraint of C++.
             */
            MidiInputDevice(std::map<String,DeviceCreationParameter*> DriverParameters, void* pSampler);

            /**
             * Destructor
             */
            virtual ~MidiInputDevice();

            /**
             * Notifies listeners that the amount of MIDI inpurt ports have
             * been changed.
             * @param NewCount The new number of MIDI input ports.
             */
            void fireMidiPortCountChanged(int NewCount);

            /**
             * Notifies listeners that the supplied MIDI input port is
             * going to be removed soon.
             * @param pPort The MIDI input port that is going to be removed.
             */
            void fireMidiPortToBeRemoved(MidiInputPort* pPort);

            /**
             * Notifies listeners that the supplied MIDI input port has
             * just been added.
             * @param pPort The MIDI input port that has been added.
             */
            void fireMidiPortAdded(MidiInputPort* pPort);

            /**
             * Set number of MIDI ports required by the engine
             * This can either do nothing, create more ports
             * or destroy ports depenging on the parameter
             * and how many ports already exist on this driver.
             *
             * @param Ports - number of ports to be left on this driver after this call.
             */
            void AcquirePorts(uint Ports);

            friend class ParameterActive;
            friend class ParameterPorts;
            friend class MidiInputDeviceFactory; // allow MidiInputDeviceFactory class to destroy midi devices
            friend class MidiInputPort; // allow MidiInputPort to access pSampler
    };
}

#endif // __LS_MIDIINPUTDEVICE_H__
