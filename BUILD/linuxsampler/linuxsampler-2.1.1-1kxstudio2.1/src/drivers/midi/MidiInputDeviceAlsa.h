/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2013 Christian Schoenebeck                       *
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

#ifndef __LS_MIDIINPUTDEVICEALSA_H__
#define __LS_MIDIINPUTDEVICEALSA_H__

#include <alsa/asoundlib.h>

#include "../../common/global_private.h"
#include "../../common/Thread.h"
#include "MidiInputDevice.h"

namespace LinuxSampler {

    /** ALSA MIDI input driver
     *
     * Implements MIDI input for the Advanced Linux Sound Architecture
     * (ALSA).
     */
    class MidiInputDeviceAlsa : public MidiInputDevice, public Thread {
        public:

            /**
             * MIDI Port implementation for the ALSA MIDI input driver.
             */
            class MidiInputPortAlsa : public MidiInputPort {
                public:
                    /** MIDI Port Parameter 'NAME'
                     *
                     * Used to assign an arbitrary name to the MIDI port.
                     */
                    class ParameterName : public MidiInputPort::ParameterName {
                        public:
                            ParameterName(MidiInputPort* pPort) throw (Exception);
                            virtual void OnSetValue(String s) throw (Exception) OVERRIDE;
                    };

                    /** MIDI Port Parameter 'ALSA_SEQ_BINDINGS'
                     *
                     * Used to connect to other Alsa sequencer clients.
                     */
                    class ParameterAlsaSeqBindings : public DeviceRuntimeParameterStrings {
                        public:
                            ParameterAlsaSeqBindings(MidiInputPortAlsa* pPort);
                            virtual String Description() OVERRIDE;
                            virtual bool Fix() OVERRIDE;
                            virtual std::vector<String> PossibilitiesAsString() OVERRIDE;
                            virtual void OnSetValue(std::vector<String> vS) throw (Exception) OVERRIDE;
                        protected:
                            MidiInputPortAlsa* pPort;
                    };

                    /** MIDI Port Parameter 'ALSA_SEQ_ID'
                     *
                     * Reflects the ALSA sequencer ID of this MIDI port,
                     * e.g. "128:0".
                     */
                    class ParameterAlsaSeqId : public DeviceRuntimeParameterString {
                        public:
                            ParameterAlsaSeqId(MidiInputPortAlsa* pPort);
                            virtual String              Description() OVERRIDE;
                            virtual bool                Fix() OVERRIDE;
                            virtual std::vector<String> PossibilitiesAsString() OVERRIDE;
                            virtual void                OnSetValue(String s) OVERRIDE;
                    };

                    void ConnectToAlsaMidiSource(const char* MidiSource);
                protected:
                    std::vector<snd_seq_port_subscribe_t*> subscriptions;

                    MidiInputPortAlsa(MidiInputDeviceAlsa* pDevice) throw (MidiInputException);
                    ~MidiInputPortAlsa();
                    friend class MidiInputDeviceAlsa;
                private:
                    MidiInputDeviceAlsa* pDevice;

                    friend class ParameterName;
                    friend class ParameterAlsaSeqBindings;
                    void UnsubscribeAll();
            };

            /** MIDI Device Parameter 'NAME'
             *
             * Used to assign an arbitrary name to the ALSA client of this
             * MIDI device.
             */
            class ParameterName : public DeviceCreationParameterString {
                public:
                    ParameterName();
                    ParameterName(String s);
                    virtual String              Description() OVERRIDE;
                    virtual bool                Fix() OVERRIDE;
                    virtual bool                Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual std::vector<String> PossibilitiesAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<String>    DefaultAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual void                OnSetValue(String s) throw (Exception) OVERRIDE;
                    static String Name();
            };

            MidiInputDeviceAlsa(std::map<String,DeviceCreationParameter*> Parameters, void* pSampler);
            ~MidiInputDeviceAlsa();

            // derived abstract methods from class 'MidiInputDevice'
            void Listen() OVERRIDE;
            void StopListen() OVERRIDE;
            virtual String Driver() OVERRIDE;
            static String Name();
            static String Description();
            static String Version();

            MidiInputPortAlsa* CreateMidiPort();
        protected:
            int Main(); ///< Implementation of virtual method from class Thread
        private:
            snd_seq_t* hAlsaSeq;
            int        hAlsaSeqClient;       ///< Alsa Sequencer client ID

            friend class MidiInputPortAlsa;
            friend class MidiInputPortAlsa::ParameterName;
            friend class MidiInputPortAlsa::ParameterAlsaSeqBindings;
    };
}

#endif // __LS_MIDIINPUTDEVICEALSA_H__
