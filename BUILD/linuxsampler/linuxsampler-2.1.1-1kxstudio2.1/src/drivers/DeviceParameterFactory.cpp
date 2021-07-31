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

#include "DeviceParameterFactory.h"

namespace LinuxSampler {

    DeviceCreationParameter* DeviceParameterFactory::Create(String ParameterName, String val) throw (Exception) {
        if (!InnerFactories.count(ParameterName)) throw Exception("No such parameter: '" + ParameterName + "'.");
        return InnerFactories[ParameterName]->Create(val);
    }

    DeviceCreationParameter* DeviceParameterFactory::Create(String ParameterName, std::map<String,String> Parameters) throw (Exception) {
        if (!InnerFactories.count(ParameterName)) throw Exception("No such parameter: '" + ParameterName + "'.");
        return InnerFactories[ParameterName]->Create(Parameters);
    }

    /* This method is used by the device factory to create parameters _before_ creating a device
     * Input parameter map is used to set parameters that device supports
     * If extra parameters are present in the input map they are ignored. Only supported parameters are created.
     * If parameter is not present in the input map it is created with a default setting
     **/
    std::map<String,DeviceCreationParameter*> DeviceParameterFactory::CreateAllParams (std::map<String,String> Parameters) {
	std::map<String,DeviceCreationParameter*> result;
	for (std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin(); iter != InnerFactories.end(); iter++) {
		DeviceCreationParameter* param;
		String paramName = iter->first;
		if (Parameters.count(paramName)) { //Parameter with this name was specified
			param = iter->second->Create(Parameters[paramName]);
		} else { //Not specified, create default
			param = iter->second->Create(Parameters);
		}
		result[paramName] = param;
	}
	return result;
    }

    std::map<String,DeviceCreationParameter*> DeviceParameterFactory::CreateAllParams () {
	std::map<String,DeviceCreationParameter*> result;
	for (std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin(); iter != InnerFactories.end(); iter++) {
		result[iter->first] = iter->second->Create();
	}
	return result;
    }

    DeviceParameterFactory::~DeviceParameterFactory() {
        for (std::map<String, InnerFactory*>::iterator iter = InnerFactories.begin(); iter != InnerFactories.end(); iter++) {
            delete iter->second;
        }
    }

} // namespace LinuxSampler
