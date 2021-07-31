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

#ifndef __LS_DEVICE_PARAMETER_FACTORY_H__
#define __LS_DEVICE_PARAMETER_FACTORY_H__

#include <map>
#include <vector>

#include "../common/global.h"
#include "../common/optional.h"
#include "../common/Exception.h"
#include "DeviceParameter.h"

namespace LinuxSampler {

	class DeviceParameterFactory {
		public:
			typedef std::map<String,String> StringMap; // nasty workaround for a GCC bug (see GCC bug #15980, #57)
			
			class InnerFactory {
				public:
					InnerFactory(DeviceParameterFactory* pParent) { this->pParent = pParent; }
					virtual ~InnerFactory() {}
					virtual DeviceCreationParameter* Create(std::map<String,String> Parameters = StringMap()) = 0;
					virtual DeviceCreationParameter* Create(String val) = 0;
				protected:
					DeviceParameterFactory* pParent;
			};
			
			template <class Parameter_T>
			class InnerFactoryTemplate : public InnerFactory {
				public:
					InnerFactoryTemplate(DeviceParameterFactory* pParent) : InnerFactory(pParent) {}
					
					/// Create parameter with respective value in map or use its default value.
					virtual DeviceCreationParameter* Create(std::map<String,String> Parameters = StringMap()) {
						const String paramName = Parameter_T::Name();
						if (Parameters.count(paramName)) {
							// parameter with this name was specified, so use that given value
							return new Parameter_T(Parameters[paramName]);
						}
						// parameter value not given, use its default value ...
						// ... but first resolve its dependencies to other parameters
						Parameter_T param;
						std::map<String,DeviceCreationParameter*> dependencies = param.DependsAsParameters();
						std::map<String,String> dependencysParams;
						for (std::map<String,DeviceCreationParameter*>::iterator iter = dependencies.begin(); iter != dependencies.end(); iter++) {
							if (Parameters.count(iter->first)) {
								// value for this dependency parameter already given
								dependencysParams[iter->first] = Parameters[iter->first];
							} else {
								// no value provided for this dependency parameter, use its default value
								// (FIXME: no sanity check for cyclic dependencies here yet)
								DeviceCreationParameter* pDependencyParam = pParent->Create(iter->first, Parameters);
								if (pDependencyParam) {
									dependencysParams[iter->first] = pDependencyParam->Value();
									delete pDependencyParam;
								}
							}
						}
						// now that we resolved all dependencies, we can finally determine parameter's default value
						optional<String> defaultValue = param.Default(dependencysParams);
						return (defaultValue) ? new Parameter_T(*defaultValue) : new Parameter_T();
					}
					/// Create parameter with given value.
					virtual DeviceCreationParameter* Create(String val) { return new Parameter_T(val); }
			};

			template <class Parameter_T>
			static void Register(DeviceParameterFactory* factory) {
				factory->InnerFactories[Parameter_T::Name()] = new InnerFactoryTemplate<Parameter_T>(factory);
			}

			std::map<String,DeviceCreationParameter*> CreateAllParams ( std::map<String,String> Parameters );
			std::map<String,DeviceCreationParameter*> CreateAllParams ();

			DeviceCreationParameter* Create(String ParameterName, std::map<String,String> Parameters = StringMap()) throw (Exception);
			DeviceCreationParameter* Create(String ParameterName, String val) throw (Exception);

			~DeviceParameterFactory();

		protected:
			std::map<String, InnerFactory*> InnerFactories;
	};

} // namespace LinuxSampler

#endif // __LS_DEVICE_PARAMETER_FACTORY_H__
