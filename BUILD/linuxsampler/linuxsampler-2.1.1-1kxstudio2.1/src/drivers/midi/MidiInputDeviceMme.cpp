/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
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

#include "MidiInputDeviceMme.h"
#include "MidiInputDeviceFactory.h"




namespace LinuxSampler {

void CALLBACK MidiInputDeviceMme::MidiInputPortMme::win32_midiin_callback(HMIDIIN handle, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    MidiInputDeviceMme::MidiInputPortMme* p = (MidiInputDeviceMme::MidiInputPortMme*)dwInstance;
    p->MmeCallbackDispatcher(handle, uMsg, dwParam1, dwParam2);
}



// *************** ParameterPorts ***************
// *

// *************** ParameterPorts ***************
// *

    MidiInputDeviceMme::ParameterPorts::ParameterPorts() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    MidiInputDeviceMme::ParameterPorts::ParameterPorts(String val) : DeviceCreationParameterInt(val) {
    }

    String MidiInputDeviceMme::ParameterPorts::Description() {
        return "Number of ports";
    }

    bool MidiInputDeviceMme::ParameterPorts::Fix() {
        return true;
    }

    bool MidiInputDeviceMme::ParameterPorts::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> MidiInputDeviceMme::ParameterPorts::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>();
    }

    // the MME driver supports only one port so to manage multiple MME MIDI ports the user just creates several MME drivers and connects each one to the desired MME port
    optional<int> MidiInputDeviceMme::ParameterPorts::DefaultAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    optional<int> MidiInputDeviceMme::ParameterPorts::RangeMinAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    optional<int> MidiInputDeviceMme::ParameterPorts::RangeMaxAsInt(std::map<String,String> Parameters) {
        return 1;
    }

    std::vector<int> MidiInputDeviceMme::ParameterPorts::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

   void MidiInputDeviceMme::ParameterPorts::OnSetValue(int i) throw (Exception) {
        if (i != 1) throw Exception("MME only supports one MIDI port per device");
    }

    String MidiInputDeviceMme::ParameterPorts::Name() {
        return "PORTS";
    }

    // the MME driver supports only one port so to manage multiple MME MIDI ports the user just creates several MME drivers and connects each one to the desired MME port



// *************** ParameterPort ***************
// *

    MidiInputDeviceMme::MidiInputPortMme::ParameterPort::ParameterPort(MidiInputPortMme* pPort)
        : DeviceRuntimeParameterString("") {
        this->pPort = pPort;
    }

    String MidiInputDeviceMme::MidiInputPortMme::ParameterPort::Description() {
        return "MME Destination MIDI Port";
    }

    bool MidiInputDeviceMme::MidiInputPortMme::ParameterPort::Fix() {
        return false;
    }

    std::vector<String> MidiInputDeviceMme::MidiInputPortMme::ParameterPort::PossibilitiesAsString() {
        // returns a list of the available MME Input MIDI ports you can connect to
        std::vector<String> ports;
        MIDIINCAPS midiincaps;

        int NumDevs = midiInGetNumDevs();

        for(int i=0;i<NumDevs;i++) {
            int res = midiInGetDevCaps(i, &midiincaps, sizeof(MIDIINCAPS));
            if(res == MMSYSERR_NOERROR) {
                ports.push_back( (String)midiincaps.szPname);
            }
        }

        return ports;
}

    void MidiInputDeviceMme::MidiInputPortMme::ParameterPort::OnSetValue(String s) {
        pPort->ConnectToMmeMidiSource(s.c_str());
    }



// *************** MidiInputPortMme ***************
// *

    MidiInputDeviceMme::MidiInputPortMme::MidiInputPortMme(MidiInputDeviceMme* pDevice) throw (MidiInputException)
        : MidiInputPort(pDevice, ((DeviceCreationParameterInt*)pDevice->Parameters["PORTS"])->ValueAsInt() - 1)
    {
        this->pDevice = pDevice;
        MidiInOpened = false;
        SysExBuf = new char[MME_MAX_SYSEX_BUF_SIZE];
        TmpSysExBuf = new char[MME_MAX_SYSEX_BUF_SIZE];
        ExitFlag = false;
        FirstSysExBlock = true;
        SysExMsgComplete = false;
        dmsg(3,("created MME port %d\n", this->portNumber));
        Parameters["PORT"] = new ParameterPort(this);
    }

    MidiInputDeviceMme::MidiInputPortMme::~MidiInputPortMme() {
        CloseMmeMidiPort();
        delete[] TmpSysExBuf;
        delete[] SysExBuf;
    }


    /***
     * Closes the MME MIDI Input port in a safe way
     */
void MidiInputDeviceMme::MidiInputPortMme::CloseMmeMidiPort(void) {
    int res;
    if (MidiInOpened == true) {
        ExitFlag = true;
        midiInReset(MidiInHandle);
        while ((res = midiInClose(MidiInHandle)) == MIDIERR_STILLPLAYING) Sleep(100);
        midiInUnprepareHeader(MidiInHandle, &midiHdr, sizeof(MIDIHDR));
        MidiInOpened = false;
    }
}

    /**
     * Connects the port with the MME Midi Input port
     *
     * @param Name of the MME Midi Input port  e.g. "MAUDIO MIDI IN"
     * @throws MidiInputException  if connection cannot be established
     */

void MidiInputDeviceMme::MidiInputPortMme::ConnectToMmeMidiSource(const char* MidiSource) {

    // close the old MIDI Input port if it was already opened
    CloseMmeMidiPort();

    MIDIINCAPS midiincaps;
    int NumDevs = midiInGetNumDevs();

    int FoundMidiInDeviceId = -1;
    for(int i=0;i<NumDevs;i++) {
        int res = midiInGetDevCaps(i, &midiincaps, sizeof(MIDIINCAPS));
        if(res == MMSYSERR_NOERROR) {
            if(!strcmp(midiincaps.szPname, MidiSource)) {
                FoundMidiInDeviceId = i;
                break;
            }
        }
    }

    if(FoundMidiInDeviceId == -1) throw MidiInputException("MIDI port connect failed");

    int res;
    res = midiInOpen(&MidiInHandle, FoundMidiInDeviceId, (DWORD_PTR)win32_midiin_callback, (DWORD_PTR)this, CALLBACK_FUNCTION);
    if(res != MMSYSERR_NOERROR) {
        throw MidiInputException("MIDI port connect failed. midiInOpen error");
    }

    MidiInOpened = true;

    /* Store pointer to our input buffer for System Exclusive messages in MIDIHDR */
    midiHdr.lpData = &TmpSysExBuf[0];

    /* Store its size in the MIDIHDR */
    midiHdr.dwBufferLength = MME_MAX_SYSEX_BUF_SIZE;

    /* Flags must be set to 0 */
    midiHdr.dwFlags = 0;

    /* Prepare the buffer and MIDIHDR */
    res = midiInPrepareHeader(MidiInHandle, &midiHdr, sizeof(MIDIHDR));
    if(res != MMSYSERR_NOERROR) {
        CloseMmeMidiPort();
        throw MidiInputException("MIDI port connect failed. midiInPrepareHeader error");
    }

    /* Queue MIDI input buffer */
    res = midiInAddBuffer(MidiInHandle, &midiHdr, sizeof(MIDIHDR));
    if(res != MMSYSERR_NOERROR) {
        CloseMmeMidiPort();
        throw MidiInputException("MIDI port connect failed. midiInAddBuffer error");
    }


    res = midiInStart(MidiInHandle);
    if(res != MMSYSERR_NOERROR) {
        CloseMmeMidiPort();
        throw MidiInputException("MIDI port connect failed, midiInStart failed.");
    }

}

void MidiInputDeviceMme::MidiInputPortMme::MmeCallbackDispatcher(HMIDIIN handle, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {

    unsigned char *DataPtr; // pointer to SysEx data
    unsigned char *data;  // pointer to standard MIDI messages which are not sysex data(max 3 bytes long)

    data = (unsigned char *)&dwParam1;

    switch(uMsg) {
        case MIM_DATA: {
            //FIXME: passing timeStamp this way here does not work, since the DispatchRaw() expects it to be in period position, not miliseconds, requires additional code in RTMath to be able to transform the value for this purpose here
            //int32_t timeStamp = dwParam2;
            //DispatchRaw(data, timeStamp);
            DispatchRaw(data);
            break;
        }

        case MIM_LONGDATA:
            if(!ExitFlag) {
                LPMIDIHDR lpMIDIHeader = (LPMIDIHDR)dwParam1;
                DataPtr = (unsigned char *)(lpMIDIHeader->lpData);
                if(lpMIDIHeader->dwBytesRecorded == 0) break;

                if(FirstSysExBlock) {
                    SysExOffset = 0;
                    FirstSysExBlock = false;
                }

                if( DataPtr[lpMIDIHeader->dwBytesRecorded - 1] == 0xF7) {
                        SysExMsgComplete = true;
                        FirstSysExBlock = true;
                }
                else {
                        SysExMsgComplete = false;
                }

                if(SysExOffset + lpMIDIHeader->dwBytesRecorded <= MME_MAX_SYSEX_BUF_SIZE) {
                    memcpy(&SysExBuf[SysExOffset], DataPtr, lpMIDIHeader->dwBytesRecorded);
                    SysExOffset += lpMIDIHeader->dwBytesRecorded;
                }

                if(SysExMsgComplete) DispatchSysex(SysExBuf, SysExOffset);

                /* Queue the MIDIHDR for more input, only if ExitFlag was not set otherwise we risk an infinite loop
                because when we call midiInReset() below, Windows will send a final  MIM_LONGDATA message to that callback.
    */
                midiInAddBuffer(MidiInHandle, lpMIDIHeader, sizeof(MIDIHDR));
            }
            break;
    }

}




// *************** MidiInputDeviceMme ***************
// *

    MidiInputDeviceMme::MidiInputDeviceMme(std::map<String,DeviceCreationParameter*> Parameters, void* pSampler) : MidiInputDevice(Parameters, pSampler) {
        AcquirePorts(((DeviceCreationParameterInt*)Parameters["PORTS"])->ValueAsInt());
        if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) {
            Listen();
        }
    }

    MidiInputDeviceMme::~MidiInputDeviceMme() {
        // free the midi ports (we can't let the base class do this,
        // as the MidiInputPortMme destructors need access to
        // hAlsaSeq)
        for (std::map<int,MidiInputPort*>::iterator iter = Ports.begin(); iter != Ports.end() ; iter++) {
            delete static_cast<MidiInputPortMme*>(iter->second);
        }
        Ports.clear();
    }

    MidiInputDeviceMme::MidiInputPortMme* MidiInputDeviceMme::CreateMidiPort() {
        return new MidiInputPortMme(this);
    }

    String MidiInputDeviceMme::Name() {
        return "MME";
    }

    String MidiInputDeviceMme::Driver() {
        return Name();
    }

    void MidiInputDeviceMme::Listen() {
        //TODO: ...
    }

    void MidiInputDeviceMme::StopListen() {
        //TODO: ...
    }

    String MidiInputDeviceMme::Description() {
        return "MultiMedia Extensions";
    }

    String MidiInputDeviceMme::Version() {
        String s = "$Revision: 2494 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }



} // namespace LinuxSampler
