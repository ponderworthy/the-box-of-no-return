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

#ifndef __LS_MIDI_INPUT_DEVICE_FACTORY_H__
#define __LS_MIDI_INPUT_DEVICE_FACTORY_H__

#include <map>
#include <vector>

#include "../../common/Exception.h"
#include "../DeviceParameterFactory.h"
#include "MidiInputDevice.h"
#include "../../Sampler.h"

#define REGISTER_MIDI_INPUT_DRIVER(DriverClass)  static LinuxSampler::MidiInputDeviceFactory::InnerFactoryRegistrator<DriverClass> __auto_register_midi_input_driver__##DriverClass
#define REGISTER_MIDI_INPUT_DRIVER_PARAMETER(DriverClass, ParameterClass)  static LinuxSampler::MidiInputDeviceFactory::ParameterRegistrator<DriverClass, DriverClass::ParameterClass> __auto_register_midi_input_driver_parameter__##DriverClass##ParameterClass

namespace LinuxSampler {

  class Plugin;

  class MidiInputDeviceFactory {
      public:
          class InnerFactory {
              public:
                  virtual ~InnerFactory() {}
                  virtual MidiInputDevice* Create(std::map<String,DeviceCreationParameter*>& Parameters, Sampler* pSampler) = 0;
                  virtual String Description() = 0;
                  virtual String Version() = 0;
                  virtual bool isAutonomousDriver() = 0;
          };

          template <class Driver_T>
          class InnerFactoryTemplate : public InnerFactory {
              public:
                  virtual MidiInputDevice* Create(std::map<String,DeviceCreationParameter*>& Parameters, Sampler* pSampler) { return new Driver_T(Parameters, pSampler); }
                  virtual String Description() { return Driver_T::Description(); }
                  virtual String Version()     { return Driver_T::Version();     }
                  virtual bool isAutonomousDriver() { return Driver_T::isAutonomousDriver(); }
          };

          template <class Driver_T>
          class InnerFactoryRegistrator {
              public:
                  InnerFactoryRegistrator() {
                      MidiInputDeviceFactory::InnerFactories[Driver_T::Name()] = new MidiInputDeviceFactory::InnerFactoryTemplate<Driver_T>;
                      MidiInputDeviceFactory::ParameterFactories[Driver_T::Name()] = new DeviceParameterFactory();
                  }
                  ~InnerFactoryRegistrator() {
                      std::map<String, InnerFactory*>::iterator iter = MidiInputDeviceFactory::InnerFactories.find(Driver_T::Name());
                      delete iter->second;
                      MidiInputDeviceFactory::InnerFactories.erase(iter);

                      std::map<String, DeviceParameterFactory*>::iterator iterpf = MidiInputDeviceFactory::ParameterFactories.find(Driver_T::Name());
                      delete iterpf->second;
                      MidiInputDeviceFactory::ParameterFactories.erase(iterpf);
                  }
          };

          template <class Driver_T, class Parameter_T>
          class ParameterRegistrator {
              public:
                  ParameterRegistrator() {
                      DeviceParameterFactory::Register<Parameter_T>(MidiInputDeviceFactory::ParameterFactories[Driver_T::Name()]);
                  }
          };

          static MidiInputDevice*                          Create(String DriverName, std::map<String,String> Parameters, Sampler* pSampler) throw (Exception);
          static void                                      Destroy(MidiInputDevice* pDevice) throw (Exception);
          static std::vector<String>                       AvailableDrivers();
          static String                                    AvailableDriversAsString();
          static std::map<String,DeviceCreationParameter*> GetAvailableDriverParameters(String DriverName) throw (Exception);
          static DeviceCreationParameter*                  GetDriverParameter(String DriverName, String ParameterName) throw (Exception);
          static String                                    GetDriverDescription(String DriverName) throw (Exception);
          static String                                    GetDriverVersion(String DriverName) throw (Exception);
          static std::map<uint, MidiInputDevice*>          Devices();

      protected:
          static MidiInputDevice*                          CreatePrivate(String DriverName, std::map<String,String> Parameters, Sampler* pSampler) throw (Exception);
          static void                                      DestroyPrivate(MidiInputDevice* pDevice) throw (Exception);
          friend class Plugin; // host plugin base class (e.g. for VST, AU, DSSI, LV2)

      public: /* FIXME: fields below should be protected, causes errors on gcc 2.95 though */
          static std::map<String, InnerFactory*> InnerFactories;
          static std::map<String, DeviceParameterFactory*> ParameterFactories;

      private:
          typedef std::map<uint, MidiInputDevice*> MidiInputDeviceMap;
          static MidiInputDeviceMap mMidiInputDevices; ///< contains all created MIDI input devices
  };

} // namespace LinuxSampler

#endif // __LS_AUDIO_OUTPUT_DEVICE_FACTORY_H__
