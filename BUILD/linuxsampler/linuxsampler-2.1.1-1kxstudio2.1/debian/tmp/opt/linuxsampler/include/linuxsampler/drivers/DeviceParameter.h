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

#ifndef __LS_DEVICE_PARAMETER_H__
#define __LS_DEVICE_PARAMETER_H__

#include <map>
#include <vector>

#include "../common/global.h"
#include "../common/optional.h"
#include "../common/Exception.h"
#include "Device.h"

namespace LinuxSampler {
    
    /////////////////////////////////////////////////////////////////
    // "Runtime" parameters

    // TODO: All plurar parameter classes (except for String) have to be added (namely DeviceRuntimeParameterBools, DeviceRuntimeParameterInts, DeviceRuntimeParameterFloats, DeviceCreationParameterBools, DeviceCreationParameterInts, DeviceCreationParameterFloats), I ignored them for the moment, because they were not that necessary.

    /** @brief Abstracet base class for all driver parameters of the sampler.
     * 
     * All audio / MIDI drivers for the sampler are using dynamic driver
     * parameters based on this base class. The main purpose behind this
     * concept is to be able to write GUI frontends for the sampler, which
     * don't need to know about specific drivers the sampler provides, nor the
     * exact list of parameters those drivers offer. Instead a frontend can
     * use this API to retrieve what kind of parameters the respective
     * available drivers offer at runtime. This way new drivers can be added
     * or existing ones being changed arbitrarily to the sampler at any time,
     * without a GUI frontend to be changed or recompiled.
     *
     * There are various parameter classes deriving from this base class, which
     * implement convenient handling for the various common value types like
     * bool, int, float, String. A driver would rather use one of those
     * type specialized classes, instead of this abstract base class. Because
     * the methods of this very base parameter class here are generalized being
     * encoded in String type for all parameter types.
     *
     * Besides that, there are 2 distinct sets of parameter types:
     * - "Runtime" parameters which can be set and changed by the user (i.e.
     *    with a GUI frontend) at any time.
     * - "Creation" parameters which can only be set by the user before a
     *   driver is instantiated (a driver instance is called a "device" in
     *   the sampler's terms). After the driver is instantiated, "creation"
     *   parameters are read only for the life time of a driver (device)
     *   instance. Typical example for a "creation" parameter would be an
     *   audio driver that allows to select a specific sound card.
     *
     * @see DeviceCreationParameter
     */
    class DeviceRuntimeParameter {
        public:
            /**
             * Some name reflecting the parameter's value type, like "BOOL,
             * "INT", "FLOAT", "STRING", "STRINGS". Upon the value returned
             * here, the object can be casted to the respective implementing
             * parameter class.
             */
            virtual String Type() = 0;
            
            /**
             * A human readable description, explaining the exact purpose of
             * the driver parameter. The text returned here can be used to
             * display the user in a GUI frontend some helping text, that
             * explains what the parameter actually does.
             */
            virtual String Description() = 0;
            
            /**
             * Whether the parameter is read only. Not to be confused with
             * "creation" parameters! A driver parameter which returns true
             * here can never be set or altered at any time. Not even at
             * instanciation time of the driver! A typical example would be
             * a parameter "SAMPLERATE" for a specific sound card, where the
             * specific sound card does not allow to switch the sound card's
             * sample rate in any way. Yet the value returned by the parameter
             * (read only) might be different, depending on the actual sound
             * card the user selected with the audio driver.
             */
            virtual bool Fix() = 0;
            
            /**
             * Whether the parameter only allows to set one scalar value, or
             * if @c true is returned here, the parameter allows to manage a
             * list of values instead. A typical example of multiplicity
             * parameter is i.e. a "ROUTING" parameter, that would allow a
             * user to interconnect the sampler with other apps and devices
             * with drivers that support such concepts (like JACK and ALSA MIDI
             * do).
             */
            virtual bool Multiplicity() = 0;
            
            /**
             * The driver parameter might (optionally) return a minimum value
             * for the parameter. If some actual value is returned here, the
             * sampler automatically performs bounds checking of parameter
             * values to be set for such a parameter and a GUI frontend might
             * display a spin box in such a case to the user, honoring the
             * returned minimum value.
             *
             * You probably don't want to call this method directly, but instead
             * cast this object to the respective deriving parameter class like
             * DeviceRuntimeParameterInt, and use its RangeMinAsInt() method
             * instead, which conveniently returns a value in its value type.
             * So you don't need to parse this return value here.
             */
            virtual optional<String> RangeMin() = 0;
            
            /**
             * The driver parameter might (optionally) return a maximum value
             * for the parameter. If some actual value is returned here, the
             * sampler automatically performs bounds checking of parameter
             * values to be set for such a parameter and a GUI frontend might
             * display a spin box in such a case to the user, honoring the
             * returned maximum value.
             *
             * You probably don't want to call this method directly, but instead
             * cast this object to the respective deriving parameter class like
             * DeviceRuntimeParameterInt, and use its RangeMaxAsInt() method
             * instead, which conveniently returns a value in its value type.
             * So you don't need to parse this return value here.
             */
            virtual optional<String> RangeMax() = 0;
            
            /**
             * The driver parameter might (optionally) return a list of
             * possible values for this parameter, encoded as comma separated
             * list. For example an audio driver might return "44100,96000" for
             * a "SAMPLERATE" parameter for a specific sound card.
             *
             * You probably don't want to call this method directly, but instead
             * cast this object to the respective deriving parameter class like
             * DeviceRuntimeParameterInt, and use its PossibilitiesAsInt() method
             * instead, which conveniently returns a vector in its value type.
             * So you don't need to parse this return value here.
             */
            virtual optional<String> Possibilities() = 0;
            
            /**
             * The current value of this parameter (encoded as String).
             * You might want to cast to the respective deriving parameter
             * class like DeviceRuntimeParameterInt and use its method
             * ValueAsInt() for not being forced to parse the String here.
             */
            virtual String Value() = 0;
            
            /**
             * Alter the parameter with the value given by @a val. The
             * respective deriving parameter class automatically parses the
             * String value supplied here, and converts it into its native
             * value type like int, float or String vector ("Strings").
             *
             * @param - new parameter value encoded as string
             * @throws Exception - if @a val is out of bounds, not encoded
             *                     correctly in its string representation or
             *                     any other reason the driver might not want
             *                     to accept the given value.
             */
            virtual void SetValue(String val) throw (Exception) = 0;
            
            /** @brief Destructor.
             * 
             * Virtual base destructor which enforces that all destructors of
             * all deriving classes are called automatically upon object
             * destruction.
             */
            virtual ~DeviceRuntimeParameter(){};
    };

    /** @brief Abstract base class for driver parameters of type @c bool.
     *
     * Implements a "runtime" driver parameter for value type @c bool.
     * A driver offering a parameter of type @c bool would derive its
     * parameter class from this class and implement the abstract method
     * OnSetValue() to react on a parameter value being set.
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceCreationParameterBool
     */
    class DeviceRuntimeParameterBool : public DeviceRuntimeParameter {
        public:
            /** @brief Constructor for value type @c bool.
             *
             * Sets an initial value for the parameter.
             *
             * @param bVal - initial boolean value
             */
            DeviceRuntimeParameterBool(bool bVal);
            
            /////////////////////////////////////////////////////////////////
            // derived methods, implementing type "BOOL"
            //     (usually not to be overriden by descendant)
            
            virtual String           Type() OVERRIDE;
            virtual bool             Multiplicity() OVERRIDE;
            virtual optional<String> RangeMin() OVERRIDE;
            virtual optional<String> RangeMax() OVERRIDE;
            virtual optional<String> Possibilities() OVERRIDE;
            virtual String           Value() OVERRIDE;
            virtual void             SetValue(String val) throw (Exception) OVERRIDE;
            
            /////////////////////////////////////////////////////////////////
            // convenience methods for type "BOOL"
            //     (usually not to be overriden by descendant)

            virtual bool ValueAsBool();
            virtual void SetValue(bool b) throw (Exception);

            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)
            
            /**
             * Must be implemented be a driver's parameter class to react on
             * the parameter value being set / altered.
             *
             * @param b - new parameter value set by the user
             * @throws Exception - might be thrown by the driver in case it
             *                     cannot handle the supplied parameter value
             *                     for whatever reason
             */
            virtual void OnSetValue(bool b) throw (Exception) = 0;
        protected:
            bool bVal;
    };

    /** @brief Abstract base class for driver parameters of type @c int.
     *
     * Implements a "runtime" driver parameter for value type @c int.
     * A driver offering a parameter of type @c int would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue(), RangeMinAsInt(), RangeMaxAsInt(), PossibilitiesAsInt().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceCreationParameterInt
     */
    class DeviceRuntimeParameterInt : public DeviceRuntimeParameter {
        public:
            /** @brief Constructor for value type @c int.
             *
             * Sets an initial value for the parameter.
             *
             * @param iVal - initial integer value
             */
            DeviceRuntimeParameterInt(int iVal);
            
            /////////////////////////////////////////////////////////////////
            // derived methods, implementing type "INT"
            //     (usually not to be overriden by descendant)
            
            virtual String           Type() OVERRIDE;
            virtual bool             Multiplicity() OVERRIDE;
            virtual optional<String> RangeMin() OVERRIDE;
            virtual optional<String> RangeMax() OVERRIDE;
            virtual optional<String> Possibilities() OVERRIDE;
            virtual String           Value() OVERRIDE;
            virtual void             SetValue(String val) throw (Exception) OVERRIDE;
            
            /////////////////////////////////////////////////////////////////
            // convenience methods for type "INT"
            //     (usually not to be overriden by descendant)

            virtual int  ValueAsInt();
            virtual void SetValue(int i) throw (Exception);
            
            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Must be implemented by descendant, returning a minimum int value
             * for the parameter. If a minimum value does not make sense for
             * the specific driver parameter, then the implementation should
             * return @c optional<int>::nothing.
             */
            virtual optional<int> RangeMinAsInt() = 0;
            
            /**
             * Must be implemented by descendant, returning a maximum int value
             * for the parameter. If a maximum value does not make sense for
             * the specific driver parameter, then the implementation should
             * return @c optional<int>::nothing.
             */
            virtual optional<int> RangeMaxAsInt() = 0;
            
            /**
             * Must be implemented by descendant, returning a list of possible
             * int values for the parameter. If a list of possible values does
             * not make sense, the implementation should return an empty
             * vector.
             */
            virtual std::vector<int> PossibilitiesAsInt() = 0;
            
            /**
             * Must be implemented be a driver's parameter class to react on
             * the parameter value being set / altered.
             *
             * @param i - new parameter value set by the user
             * @throws Exception - might be thrown by the driver in case it
             *                     cannot handle the supplied parameter value
             *                     for whatever reason
             */
            virtual void OnSetValue(int i) throw (Exception) = 0;
        protected:
            int iVal;
    };

    /** @brief Abstract base class for driver parameters of type @c float.
     *
     * Implements a "runtime" driver parameter for value type @c float.
     * A driver offering a parameter of type @c float would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue(), RangeMinAsFloat(), RangeMaxAsFloat() and
     * PossibilitiesAsFloat().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceCreationParameterFloat
     */
    class DeviceRuntimeParameterFloat : public DeviceRuntimeParameter {
        public:
            /** @brief Constructor for value type @c float.
             *
             * Sets an initial value for the parameter.
             *
             * @param fVal - initial float value
             */
            DeviceRuntimeParameterFloat(float fVal);
            
            /////////////////////////////////////////////////////////////////
            // derived methods, implementing type "FLOAT"
            //     (usually not to be overriden by descendant)
            
            virtual String           Type() OVERRIDE;
            virtual bool             Multiplicity() OVERRIDE;
            virtual optional<String> RangeMin() OVERRIDE;
            virtual optional<String> RangeMax() OVERRIDE;
            virtual optional<String> Possibilities() OVERRIDE;
            virtual String           Value() OVERRIDE;
            virtual void             SetValue(String val) throw (Exception) OVERRIDE;

            /////////////////////////////////////////////////////////////////
            // convenience methods for type "FLOAT"
            //     (usually not to be overriden by descendant)
            
            virtual float ValueAsFloat();
            virtual void  SetValue(float f) throw (Exception);
            
            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Must be implemented by descendant, returning a minimum float
             * value for the parameter. If a minimum value does not make sense
             * for the specific driver parameter, then the implementation
             * should return @c optional<float>::nothing.
             */
            virtual optional<float> RangeMinAsFloat() = 0;
            
            /**
             * Must be implemented by descendant, returning a maximum float
             * value for the parameter. If a maximum value does not make sense
             * for the specific driver parameter, then the implementation
             * should return @c optional<float>::nothing.
             */
            virtual optional<float> RangeMaxAsFloat() = 0;
            
            /**
             * Must be implemented by descendant, returning a list of possible
             * float values for the parameter. If a list of possible values
             * does not make sense, the implementation should return an empty
             * vector.
             */
            virtual std::vector<float> PossibilitiesAsFloat() = 0;
            
            /**
             * Must be implemented be a driver's parameter class to react on
             * the parameter value being set / altered.
             *
             * @param f - new parameter value set by the user
             * @throws Exception - might be thrown by the driver in case it
             *                     cannot handle the supplied parameter value
             *                     for whatever reason
             */
            virtual void OnSetValue(float f) = 0;
        protected:
            float fVal;
    };

    /** @brief Abstract base class for driver parameters of type @c String.
     *
     * Implements a "runtime" driver parameter for value type @c String.
     * A driver offering a parameter of type @c String would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue() and PossibilitiesAsString().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceCreationParameterString
     */
    class DeviceRuntimeParameterString : public DeviceRuntimeParameter {
        public:
            /** @brief Constructor for value type @c String.
             *
             * Sets an initial value for the parameter.
             *
             * @param sVal - initial String value
             */
            DeviceRuntimeParameterString(String sVal);
            
            /// @brief Destructor.
            virtual ~DeviceRuntimeParameterString(){}
            
            /////////////////////////////////////////////////////////////////
            // derived methods, implementing type "STRING"
            //     (usually not to be overriden by descendant)
            
            virtual String           Type() OVERRIDE;
            virtual bool             Multiplicity() OVERRIDE;
            virtual optional<String> RangeMin() OVERRIDE;
            virtual optional<String> RangeMax() OVERRIDE;
            virtual optional<String> Possibilities() OVERRIDE;
            virtual String           Value() OVERRIDE;
            virtual void             SetValue(String val) throw (Exception) OVERRIDE;
            
            /////////////////////////////////////////////////////////////////
            // convenience methods for type "STRING"
            //     (usually not to be overriden by descendant)

            virtual String ValueAsString();
            virtual void   SetValueAsString(String s) throw (Exception);
            
            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Must be implemented by descendant, returning a list of possible
             * String values for the parameter. If a list of possible values
             * does not make sense, the implementation should return an empty
             * vector.
             */
            virtual std::vector<String> PossibilitiesAsString() = 0;
            
            /**
             * Must be implemented be a driver's parameter class to react on
             * the parameter value being set / altered.
             *
             * @param s - new parameter value set by the user
             * @throws Exception - might be thrown by the driver in case it
             *                     cannot handle the supplied parameter value
             *                     for whatever reason
             */
            virtual void OnSetValue(String s) = 0;
        protected:
            String sVal;
    };

    /** @brief Abstract base class for driver parameters of a @c String list type.
     *
     * Implements a "runtime" driver parameter for multiple values of type
     * @c String. A driver offering a parameter of a @c String list type would
     * derive its parameter class from this class and implement the abstract
     * methods OnSetValue() and PossibilitiesAsString().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceCreationParameterStrings
     */
    class DeviceRuntimeParameterStrings : public DeviceRuntimeParameter {
        public:
            /** @brief Constructor for value type @c String.
             *
             * Sets an initial list of values for the parameter.
             *
             * @param sVal - initial String value
             */
            DeviceRuntimeParameterStrings(std::vector<String> vS);
            
            /// @brief Destructor.
            virtual ~DeviceRuntimeParameterStrings(){}
            
            /////////////////////////////////////////////////////////////////
            // derived methods, implementing type "STRINGS"
            //     (usually not to be overriden by descendant)
            
            virtual String           Type() OVERRIDE;
            virtual bool             Multiplicity() OVERRIDE;
            virtual optional<String> RangeMin() OVERRIDE;
            virtual optional<String> RangeMax() OVERRIDE;
            virtual optional<String> Possibilities() OVERRIDE;
            virtual String           Value() OVERRIDE;
            virtual void             SetValue(String val) throw (Exception) OVERRIDE;
            
            /////////////////////////////////////////////////////////////////
            // convenience methods for type "STRINGS"
            //     (usually not to be overriden by descendant)

            virtual std::vector<String> ValueAsStrings();
            virtual void                SetValue(std::vector<String> vS) throw (Exception);
            
            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Must be implemented by descendant, returning a list of possible
             * String values for the parameter. If a list of possible values
             * does not make sense, the implementation should return an empty
             * vector.
             */
            virtual std::vector<String> PossibilitiesAsString() = 0;
            
            /**
             * Must be implemented be a driver's parameter class to react on
             * the parameter value being set / altered.
             *
             * @param vS - new parameter values set by the user
             * @throws Exception - might be thrown by the driver in case it
             *                     cannot handle the supplied parameter values
             *                     for whatever reason
             */
            virtual void OnSetValue(std::vector<String> vS) = 0;
        protected:
            std::vector<String> sVals;
    };

    
    /////////////////////////////////////////////////////////////////
    // "Creation" parameters

    /** @brief Abstract base class for parameters at driver instanciation time.
     *
     * Device "creation" parameters are special parameters, that are meant to be only
     * set when a device (driver instance) is created. After device creation those
     * parameters act as read only parameters. See DeviceRuntimeParameter for a
     * discussion about this topic.
     *
     * In addition to "runtime" parameters, "creation" parameters also handle
     * additional meta informations required in the specific situations of
     * creating a device (driver instance). For example "creation" parameters
     * might indicate whether they MUST be provided explicitly
     * ( @c Mandatory() ) by the user (i.e. a parameter "CARD" selecting a
     * specific sound card) for being able to create an instance of the driver.
     * And "creation" parameters might indicate being dependent to other
     * parameters, i.e. a parameter "SAMPLERATE" might depend on a parameter
     * "CARD", to be able to actually provide a list of meaningful
     * possibilities for sample rates the respective sound card supports.
     */
    class DeviceCreationParameter : public DeviceRuntimeParameter {
        public:
            /// @brief Constructor.
            DeviceCreationParameter ( void ) { pDevice = NULL; }
            
            /**
             * Whether the parameter must be supplied by the user at device
             * creation time. If this method return @c false, then the
             * parameter is optional
             */
            virtual bool Mandatory() = 0;
            
            /**
             * Might return a comma separated list of parameter names this
             * parameter depends on. See this class's introduction about a
             * discussion of parameters with dependencies. If this method
             * returns @c optional<String>::nothing, then it does not have any
             * dependencies to other parameters.
             *
             * This method already provides an implementation, which usually is
             * not overridden by descendants. A descendant would rather
             * implement DependsAsParameters() instead.
             *
             * @see DependsAsParameters()
             */
            virtual optional<String> Depends();
            
            /**
             * Might return a unique key-value pair list (map) reflecting the
             * dependencies of this parameter to other parameters. Each entry
             * in the map consists of a key-value pair, the key being the
             * parameter name of the respective dependent parameter, and the
             * value being an instance of the dependency parameter of that name.
             *
             * A descendant MUST implement this method, informing about its
             * dependencies. If the parameter does not have any dependencies it
             * should return an empty map.
             *
             * See this class's introduction about a discussion of parameters
             * with dependencies.
             */
            virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() = 0;
            
            /**
             * Might return a default value for this parameter. The default
             * value will be used if the user does not provide a specific value
             * for this parameter at device creation time. If the parameter
             * does not have a reasonable default value, it will return
             * @c optional<String>::nothing.
             *
             * This method already provides an implementation, which usually is
             * not overridden by descendants. A descendant would rather
             * implement the other Default() method taking arguments.
             */
            virtual optional<String> Default();
            
            /**
             * Might return a default value for this parameter. The default
             * value will be used if the user does not provide a specific value
             * for this parameter at device creation time.
             *
             * This method must be implemented by descendants. If the parameter
             * does not have a reasonable default value, it should return
             * @c optional<String>::nothing.
             *
             * As arguments, the parameters are passed to this method
             * automatically, which the user already provided. For example a
             * parameter "CARD" the user already did set for selecting a
             * specific sound card. So such arguments resolve dependencies
             * of this parameter.
             *
             * @param Parameters - other parameters which the user already
             *                     supplied
             */
            virtual optional<String> Default(std::map<String,String> Parameters) = 0;
            
            /**
             * Might return a minimum value for this parameter. If the
             * parameter does not have a reasonable minimum value, it will
             * return @c optional<String>::nothing.
             *
             * This method already provides an implementation, which usually is
             * not overridden by descendants. A descendant would rather
             * implement the other RangeMin() method taking arguments.
             */
            virtual optional<String> RangeMin();
            
            /**
             * Might return a minimum value for this parameter.
             *
             * This method must be implemented by descendants. If the parameter
             * does not have a reasonable minimum value, it should return
             * @c optional<String>::nothing.
             *
             * As arguments, the parameters are passed to this method
             * automatically, which the user already provided. For example a
             * parameter "CARD" the user already did set for selecting a
             * specific sound card. So such arguments resolve dependencies
             * of this parameter.
             *
             * @param Parameters - other parameters which the user already
             *                     supplied
             */
            virtual optional<String> RangeMin(std::map<String,String> Parameters) = 0;
            
            /**
             * Might return a maximum value for this parameter. If the
             * parameter does not have a reasonable maximum value, it will
             * return @c optional<String>::nothing.
             *
             * This method already provides an implementation, which usually is
             * not overridden by descendants. A descendant would rather
             * implement the other RangeMax() method taking arguments.
             */
            virtual optional<String> RangeMax();
            
            /**
             * Might return a maximum value for this parameter.
             *
             * This method must be implemented by descendants. If the parameter
             * does not have a reasonable maximum value, it should return
             * @c optional<String>::nothing.
             *
             * As arguments, the parameters are passed to this method
             * automatically, which the user already provided. For example a
             * parameter "CARD" the user already did set for selecting a
             * specific sound card. So such arguments resolve dependencies
             * of this parameter.
             *
             * @param Parameters - other parameters which the user already
             *                     supplied
             */
            virtual optional<String> RangeMax(std::map<String,String> Parameters) = 0;
            
            /**
             * Might return a comma separated list as String with possible
             * values for this parameter. If the parameter does not have
             * reasonable, specific possible values, it will return
             * @c optional<String>::nothing.
             *
             * This method already provides an implementation, which usually is
             * not overridden by descendants. A descendant would rather
             * implement the other Possibilities() method taking arguments.
             */
            virtual optional<String> Possibilities();
            
            /**
             * Might return a comma separated list as String with possible
             * values for this parameter.
             *
             * This method must be implemented by descendants. If the parameter
             * does not have reasonable, specific possible values, it should
             * return @c optional<String>::nothing.
             *
             * As arguments, the parameters are passed to this method
             * automatically, which the user already provided. For example a
             * parameter "CARD" the user already did set for selecting a
             * specific sound card. So such arguments resolve dependencies
             * of this parameter.
             *
             * @param Parameters - other parameters which the user already
             *                     supplied
             */
            virtual optional<String> Possibilities(std::map<String,String> Parameters) = 0;
            
            /**
             * Sets the internal device pointer to a specific device (driver
             * instance) for this parameter object. Descendants usually use
             * that internal pointer to interact with their specific driver
             * implementation class.
             */
            void Attach(Device* pDevice) { this->pDevice = pDevice; }
	protected:
	    Device* pDevice;
    };

    /** @brief Abstract base class for driver parameters of type @c Bool.
     *
     * Implements a "creation" driver parameter for value type @c bool.
     * A driver offering a parameter of type @c bool would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue() and DefaultAsBool().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceRuntimeParameterBool
     */
    class DeviceCreationParameterBool : public DeviceCreationParameter {
        public:
            DeviceCreationParameterBool(bool bVal = false);
            DeviceCreationParameterBool(String val) throw (Exception);
            virtual String Type() OVERRIDE;
            virtual bool   Multiplicity() OVERRIDE;
            virtual optional<String> Default(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMin(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMax(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> Possibilities(std::map<String,String> Parameters) OVERRIDE;
            virtual String Value() OVERRIDE;
            virtual void   SetValue(String val) throw (Exception) OVERRIDE;

            virtual bool ValueAsBool();
            virtual void SetValue(bool b) throw (Exception);

            virtual optional<bool> DefaultAsBool(std::map<String,String> Parameters) = 0;
            virtual void OnSetValue(bool b) throw (Exception)  = 0;
        protected:
            bool bVal;
            void InitWithDefault();
        private:
    };

    /** @brief Abstract base class for driver parameters of type @c int.
     *
     * Implements a "creation" driver parameter for value type @c int.
     * A driver offering a parameter of type @c int would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue(), PossibilitiesAsInt(), RangeMinAsInt(), RangeMaxAsInt()
     * and DefaultAsInt().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceRuntimeParameterInt
     */
    class DeviceCreationParameterInt : public DeviceCreationParameter {
        public:
            DeviceCreationParameterInt(int iVal = 0);
            DeviceCreationParameterInt(String val) throw (Exception);
            virtual String Type() OVERRIDE;
            virtual bool   Multiplicity() OVERRIDE;
            virtual optional<String> Default(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMin(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMax(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> Possibilities(std::map<String,String> Parameters) OVERRIDE;
            virtual String Value() OVERRIDE;
            virtual void   SetValue(String val) throw (Exception) OVERRIDE;

            virtual int  ValueAsInt();
            virtual void SetValue(int i) throw (Exception);

            virtual optional<int>    DefaultAsInt(std::map<String,String> Parameters)  = 0;
            virtual optional<int>    RangeMinAsInt(std::map<String,String> Parameters) = 0;
            virtual optional<int>    RangeMaxAsInt(std::map<String,String> Parameters) = 0;
            virtual std::vector<int> PossibilitiesAsInt(std::map<String,String> Parameters) = 0;
            virtual void             OnSetValue(int i) throw (Exception)  = 0;
        protected:
            int iVal;
            void InitWithDefault();
        private:
    };

    /** @brief Abstract base class for driver parameters of type @c float.
     *
     * Implements a "creation" driver parameter for value type @c float.
     * A driver offering a parameter of type @c float would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue(), PossibilitiesAsFloat(), RangeMinAsFloat(),
     * RangeMaxAsFloat() and DefaultAsFloat().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceRuntimeParameterFloat
     */
    class DeviceCreationParameterFloat : public DeviceCreationParameter {
        public:
            DeviceCreationParameterFloat(float fVal = 0.0);
            DeviceCreationParameterFloat(String val) throw (Exception);
            virtual String Type() OVERRIDE;
            virtual bool   Multiplicity() OVERRIDE;
            virtual optional<String> Default(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMin(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMax(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> Possibilities(std::map<String,String> Parameters) OVERRIDE;
            virtual String Value() OVERRIDE;
            virtual void   SetValue(String val) throw (Exception) OVERRIDE;

            virtual float ValueAsFloat();
            virtual void  SetValue(float f) throw (Exception);

            virtual optional<float>    DefaultAsFloat(std::map<String,String> Parameters)  = 0;
            virtual optional<float>    RangeMinAsFloat(std::map<String,String> Parameters) = 0;
            virtual optional<float>    RangeMaxAsFloat(std::map<String,String> Parameters) = 0;
            virtual std::vector<float> PossibilitiesAsFloat(std::map<String,String> Parameters) = 0;
            virtual void OnSetValue(float f) throw (Exception)  = 0;
        protected:
            float fVal;
            void InitWithDefault();
        private:
    };

    /** @brief Abstract base class for driver parameters of type @c String.
     *
     * Implements a "creation" driver parameter for value type @c String.
     * A driver offering a parameter of type @c String would derive its
     * parameter class from this class and implement the abstract methods
     * OnSetValue(), PossibilitiesAsString() and DefaultAsString().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceRuntimeParameterString
     */
    class DeviceCreationParameterString : public DeviceCreationParameter {
        public:
            DeviceCreationParameterString(String sVal = String());
            virtual ~DeviceCreationParameterString(){}
            virtual String Type() OVERRIDE;
            virtual bool   Multiplicity() OVERRIDE;
            virtual optional<String> Default(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMin(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMax(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> Possibilities(std::map<String,String> Parameters) OVERRIDE;
            virtual String Value() OVERRIDE;
            virtual void   SetValue(String val) throw (Exception) OVERRIDE;

            virtual String ValueAsString();
            virtual void   SetValueAsString(String s) throw (Exception);

            virtual optional<String>    DefaultAsString(std::map<String,String> Parameters) = 0;
            virtual std::vector<String> PossibilitiesAsString(std::map<String,String> Parameters) = 0;
            virtual void OnSetValue(String s) throw (Exception) = 0;
        protected:
            String sVal;
            void InitWithDefault();
        private:
    };

    /** @brief Abstract base class for driver parameters of a @c String list type.
     *
     * Implements a "creation" driver parameter for @c String value types.
     * A driver offering a parameter allowing to set multiple values of type
     * @c String would derive its parameter class from this class and implement
     * the abstract methods OnSetValue(), PossibilitiesAsString() and
     * DefaultAsStrings().
     *
     * See DeviceCreationParameter and DeviceRuntimeParameter for a discussion
     * about "runtime" parameters vs. "creation" parameters.
     *
     * @see DeviceRuntimeParameterStrings
     */
    class DeviceCreationParameterStrings : public DeviceCreationParameter {
        public:
            DeviceCreationParameterStrings();
            DeviceCreationParameterStrings(std::vector<String> sVals);
            DeviceCreationParameterStrings(String val) throw (Exception);
            virtual ~DeviceCreationParameterStrings(){}
            virtual String Type() OVERRIDE;
            virtual bool   Multiplicity() OVERRIDE;
            virtual optional<String> Default(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMin(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> RangeMax(std::map<String,String> Parameters) OVERRIDE;
            virtual optional<String> Possibilities(std::map<String,String> Parameters) OVERRIDE;
            virtual String Value() OVERRIDE;
            virtual void   SetValue(String val) throw (Exception) OVERRIDE;

            virtual std::vector<String> ValueAsStrings();
            virtual void                SetValue(std::vector<String> vS) throw (Exception);

            virtual std::vector<String> DefaultAsStrings(std::map<String,String> Parameters) = 0;
            virtual std::vector<String> PossibilitiesAsString(std::map<String,String> Parameters) = 0;
            virtual void OnSetValue(std::vector<String> vS) throw (Exception) = 0;
        protected:
            std::vector<String> sVals;
            void InitWithDefault();
        private:
    };

} // namespace LinuxSampler

#endif // __LS_DEVICE_PARAMETER_H__
