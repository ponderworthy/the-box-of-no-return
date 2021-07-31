/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2007 Christian Schoenebeck                       *
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

#include <strings.h>
#include <stdlib.h>

#include "DeviceParameter.h"

#include "../common/global_private.h"

namespace LinuxSampler {

    // if string is encapsulated into apostrophes or quotation marks, then remove those apostrophes / quotation marks
    static void __eliminate_quotation(String& s) {
        if (s.size()) {
            const char cBegin = s[0];
            const char cEnd   = s[s.size() - 1];
            if ( (cBegin == '\'' && cEnd == '\'') || (cBegin == '\"' && cEnd == '\"') ) {
                s = s.substr(1, s.size() - 2);
            }
        }
    }

    // like __eliminate_quotation(), but this one really kills all of 'em
    static void __eliminate_all_quotations(String& s) {
        for (int i = 0; i < s.size(); i++) {
            if (s.c_str()[i] == '\'' || s.c_str()[i] == '\"') {
                s.replace(i, 1, "");
                i--;
            }
        }
    }

    static bool __parse_bool(String val) throw (Exception) {
        __eliminate_all_quotations(val);
        int b;
        if      (val == "1" || !strcasecmp(val.c_str(),"true"))  b = true;
        else if (val == "0" || !strcasecmp(val.c_str(),"false")) b = false;
        else throw Exception("Invalid value for boolean Device parameter");
        return b;
    }

    static int __parse_int(String val) throw (Exception) {
        __eliminate_all_quotations(val);
        return atoi(val.c_str()); // TODO: format check is missing
    }

    static float __parse_float(String val) throw (Exception) {
        __eliminate_all_quotations(val);
        float x;
        std::stringstream ss(val);
        ss.imbue(std::locale::classic());
        ss >> x; // TODO: format check is missing
        return x;
    }

    static String __parse_string(String val) {
        __eliminate_quotation(val);
        return val;
    }

    static std::vector<String> __parse_strings(String val) throw (Exception) {
        std::vector<String> vS;

        // checking for empty list
        if (val.length() == 0) return vS;

        // if there's only a single value, then we also allow to give it without being encapsulated into apostrophes
        if (val.find("\'") == String::npos && val.find("\"") == String::npos) {
            vS.push_back(val);
        }
        else { // if multiple strings or a string encapsulated into apostrophes
            char* pStart = (char*) val.c_str();
            char* pC     = pStart;

            while (true) {
                if (*pC != '\'' && *pC != '\"') throw Exception("Invalid form, all individual strings should be encapsulated into apostrophes, separated by commas");

                // search for token end
                char* pTokenStart = pC + 1;
                do {
                    pC++;
                    if (*pC == '\0') throw Exception("Invalid form, all individual strings should be encapsulated into apostrophes, separated by commas");
                }
                while (*pC != '\'' && *pC != '\"');
                String token = val.substr((int)(pTokenStart - pStart), (int)(pC - pTokenStart));
                vS.push_back(token); // we found the token's end

                // now there should be either a comma or the end of the total string
                if (*(++pC) == '\0') break;
                if (*pC != ',') throw Exception("Invalid form, all individual strings should be encapsulated into apostrophes, separated by commas");
                pC++;
            }
        }

        return vS;
    }



// *************** DeviceRuntimeParameterBool ***************
// *

    DeviceRuntimeParameterBool::DeviceRuntimeParameterBool(bool bVal) {
        this->bVal = bVal;
    }

    String DeviceRuntimeParameterBool::Type() {
        return "BOOL";
    }

    bool DeviceRuntimeParameterBool::Multiplicity() {
        return false;
    }

    optional<String> DeviceRuntimeParameterBool::RangeMin() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterBool::RangeMax() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterBool::Possibilities() {
        return optional<String>::nothing;
    }

    String DeviceRuntimeParameterBool::Value() {
        return (ValueAsBool()) ? "true" : "false";
    }

    void DeviceRuntimeParameterBool::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        int b = __parse_bool(val);
        SetValue(b);
    }

    bool DeviceRuntimeParameterBool::ValueAsBool() {
        return bVal;
    }

    void DeviceRuntimeParameterBool::SetValue(bool b) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(b);
        bVal = b;
    }



// *************** DeviceRuntimeParameterInt ***************
// *

    DeviceRuntimeParameterInt::DeviceRuntimeParameterInt(int iVal) {
        this->iVal = iVal;
    }

    String DeviceRuntimeParameterInt::Type() {
        return "INT";
    }

    bool DeviceRuntimeParameterInt::Multiplicity() {
        return false;
    }

    optional<String> DeviceRuntimeParameterInt::RangeMin() {
        optional<int> rangemin = RangeMinAsInt();
        if (!rangemin) return optional<String>::nothing;
        return ToString(*rangemin);
    }

    optional<String> DeviceRuntimeParameterInt::RangeMax() {
        optional<int> rangemax = RangeMaxAsInt();
        if (!rangemax) return optional<String>::nothing;
        return ToString(*rangemax);
    }

    optional<String> DeviceRuntimeParameterInt::Possibilities() {
        std::vector<int> possibilities = PossibilitiesAsInt();
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<int>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << *iter;
            iter++;
        }
        return ss.str();
    }

    String DeviceRuntimeParameterInt::Value() {
        return ToString(ValueAsInt());
    }

    void DeviceRuntimeParameterInt::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        int i = __parse_int(val);
        if (RangeMinAsInt() && i < *RangeMinAsInt()) throw Exception("Invalid device parameter value: too small");
        if (RangeMaxAsInt() && i > *RangeMaxAsInt()) throw Exception("Invalid device parameter value: too big");

        std::vector<int> possibilities = PossibilitiesAsInt();
        if (possibilities.size()) {
            bool valid = false;
            std::vector<int>::iterator iter = possibilities.begin();
            while (iter != possibilities.end()) {
                if (i == *iter) {
                    valid = true;
                    break;
                }
                iter++;
            }
            if (!valid) throw Exception("Invalid device parameter value: not in set of possible values");
        }
        SetValue(i);
    }

    int DeviceRuntimeParameterInt::ValueAsInt() {
        return iVal;
    }

    void DeviceRuntimeParameterInt::SetValue(int i) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(i);
        iVal = i;
    }



// *************** DeviceRuntimeParameterFloat ***************
// *

    DeviceRuntimeParameterFloat::DeviceRuntimeParameterFloat(float fVal) {
        this->fVal = fVal;
    }

    String DeviceRuntimeParameterFloat::Type() {
        return "FLOAT";
    }

    bool DeviceRuntimeParameterFloat::Multiplicity() {
        return false;
    }

    optional<String> DeviceRuntimeParameterFloat::RangeMin() {
        optional<float> rangemin = RangeMinAsFloat();
        if (!rangemin) return optional<String>::nothing;
        return ToString(*rangemin);
    }

    optional<String> DeviceRuntimeParameterFloat::RangeMax() {
        optional<float> rangemax = RangeMaxAsFloat();
        if (!rangemax) return optional<String>::nothing;
        return ToString(*rangemax);
    }

    optional<String> DeviceRuntimeParameterFloat::Possibilities() {
        std::vector<float> possibilities  = PossibilitiesAsFloat();
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<float>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << *iter;
            iter++;
        }
        return ss.str();
    }

    String DeviceRuntimeParameterFloat::Value() {
        return ToString(ValueAsFloat());
    }

    void DeviceRuntimeParameterFloat::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        float f = __parse_float(val);
        if (RangeMinAsFloat() && f < *RangeMinAsFloat()) throw Exception("Invalid device parameter value: too small");
        if (RangeMaxAsFloat() && f > *RangeMaxAsFloat()) throw Exception("Invalid device parameter value: too big");

        std::vector<float> possibilities = PossibilitiesAsFloat();
        if (possibilities.size()) {
            bool valid = false;
            std::vector<float>::iterator iter = possibilities.begin();
            while (iter != possibilities.end()) {
                if (f == *iter) {
                    valid = true;
                    break;
                }
                iter++;
            }
            if (!valid) throw Exception("Invalid device parameter value: not in set of possible values");
        }
        SetValue(f);
    }

    float DeviceRuntimeParameterFloat::ValueAsFloat() {
        return fVal;
    }

    void DeviceRuntimeParameterFloat::SetValue(float f) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(f);
        fVal = f;
    }



// *************** DeviceRuntimeParameterString ***************
// *

    DeviceRuntimeParameterString::DeviceRuntimeParameterString(String sVal) {
        this->sVal = sVal;
    }

    String DeviceRuntimeParameterString::Type() {
        return "STRING";
    }

    bool DeviceRuntimeParameterString::Multiplicity() {
        return false;
    }

    optional<String> DeviceRuntimeParameterString::RangeMin() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterString::RangeMax() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterString::Possibilities() {
        std::vector<String> possibilities = PossibilitiesAsString();
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<String>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << "'" << *iter << "'";
            iter++;
        }
        return ss.str();
    }

    String DeviceRuntimeParameterString::Value() {
        return "\'" + ValueAsString() + "\'";
    }

    void DeviceRuntimeParameterString::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        SetValueAsString(__parse_string(val));
    }

    String DeviceRuntimeParameterString::ValueAsString() {
        return sVal;
    }

    void DeviceRuntimeParameterString::SetValueAsString(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        if (val.find("\'") != String::npos) throw Exception("Character -> \' <- not allowed");
        if (val.find("\"") != String::npos) throw Exception("Character -> \" <- not allowed");
        OnSetValue(val);
        sVal = val;
    }



// *************** DeviceRuntimeParameterStrings ***************
// *

    DeviceRuntimeParameterStrings::DeviceRuntimeParameterStrings(std::vector<String> vS) {
        this->sVals = vS;
    }

    String DeviceRuntimeParameterStrings::Type() {
        return "STRING";
    }

    bool DeviceRuntimeParameterStrings::Multiplicity() {
        return true;
    }

    optional<String> DeviceRuntimeParameterStrings::RangeMin() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterStrings::RangeMax() {
        return optional<String>::nothing;
    }

    optional<String> DeviceRuntimeParameterStrings::Possibilities() {
        std::vector<String> possibilities = PossibilitiesAsString();
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<String>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << "'" << *iter << "'";
            iter++;
        }
        return ss.str();
    }

    String DeviceRuntimeParameterStrings::Value() {
        String result;
        std::vector<String>::iterator iter = this->sVals.begin();
        while (iter != this->sVals.end()) {
            if (result != "") result += ",";
            result += "'" + *iter + "'";
            iter++;
        }
        return result;
    }

    void DeviceRuntimeParameterStrings::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        std::vector<String> vS = __parse_strings(val);
        SetValue(vS);
    }

    std::vector<String> DeviceRuntimeParameterStrings::ValueAsStrings() {
        return sVals;
    }

    void DeviceRuntimeParameterStrings::SetValue(std::vector<String> vS) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(vS);
        sVals = vS;
    }






// *************** DeviceCreationParameter ***************
// *

    optional<String> DeviceCreationParameter::Depends() {
        std::map<String,DeviceCreationParameter*> dependencies = DependsAsParameters();
        if (!dependencies.size()) return optional<String>::nothing;

        std::map<String,DeviceCreationParameter*>::iterator iter = dependencies.begin();
        String s;
        for (; iter != dependencies.end(); iter++) {
            if (s != "") s += ",";
            s += iter->first;
        }
        return s;
    }

    optional<String> DeviceCreationParameter::Default() {
        std::map<String,String> Parameters; // empty parameters vector
        return Default(Parameters);
    }

    optional<String> DeviceCreationParameter::RangeMin() {
        std::map<String,String> Parameters; // empty parameters vector
        return RangeMin(Parameters);
    }

    optional<String> DeviceCreationParameter::RangeMax() {
        std::map<String,String> Parameters; // empty parameters vector
        return RangeMax(Parameters);
    }

    optional<String> DeviceCreationParameter::Possibilities() {
        std::map<String,String> Parameters; // empty parameters vector
        return Possibilities(Parameters);
    }



// *************** DeviceCreationParameterBool ***************
// *

    DeviceCreationParameterBool::DeviceCreationParameterBool(bool bVal) : DeviceCreationParameter() {
        this->bVal = bVal;
    }

    DeviceCreationParameterBool::DeviceCreationParameterBool(String val) throw (Exception) {
        this->bVal = __parse_bool(val);
    }

    void DeviceCreationParameterBool::InitWithDefault() {
        std::map<String,String> Parameters; // empty parameters vector
        optional<bool> defaultval = DefaultAsBool(Parameters);
        this->bVal = (defaultval) ? *defaultval : false;
    }

    String DeviceCreationParameterBool::Type() {
        return "BOOL";
    }

    bool DeviceCreationParameterBool::Multiplicity() {
        return false;
    }

    optional<String> DeviceCreationParameterBool::Default(std::map<String,String> Parameters) {
        optional<bool> defaultval = DefaultAsBool(Parameters);
        if (!defaultval) return optional<String>::nothing;
        return (*defaultval) ? "true" : "false";
    }

    optional<String> DeviceCreationParameterBool::RangeMin(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterBool::RangeMax(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterBool::Possibilities(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    String DeviceCreationParameterBool::Value() {
        return (ValueAsBool()) ? "true" : "false";
    }

    void DeviceCreationParameterBool::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        int b = __parse_bool(val);
        SetValue(b);
    }

    bool DeviceCreationParameterBool::ValueAsBool() {
        return bVal;
    }

    void DeviceCreationParameterBool::SetValue(bool b) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(b);
        bVal = b;
    }



// *************** DeviceCreationParameterInt ***************
// *

    DeviceCreationParameterInt::DeviceCreationParameterInt(int iVal) : DeviceCreationParameter() {
        this->iVal = iVal;
    }

    DeviceCreationParameterInt::DeviceCreationParameterInt(String val) throw (Exception) {
        this->iVal = __parse_int(val);
    }

    void DeviceCreationParameterInt::InitWithDefault() {
        std::map<String,String> Parameters; // empty parameters vector
        optional<int> i = DefaultAsInt(Parameters);
        this->iVal = (i) ? *i : 0;
    }

    String DeviceCreationParameterInt::Type() {
        return "INT";
    }

    bool DeviceCreationParameterInt::Multiplicity() {
        return false;
    }

    optional<String> DeviceCreationParameterInt::Default(std::map<String,String> Parameters) {
        optional<int> defaultval = DefaultAsInt(Parameters);
        if (!defaultval) return optional<String>::nothing;
        return ToString(*defaultval);
    }

    optional<String> DeviceCreationParameterInt::RangeMin(std::map<String,String> Parameters) {
        optional<int> rangemin = RangeMinAsInt(Parameters);
        if (!rangemin) return optional<String>::nothing;
        return ToString(*rangemin);
    }

    optional<String> DeviceCreationParameterInt::RangeMax(std::map<String,String> Parameters) {
        optional<int> rangemax = RangeMaxAsInt(Parameters);
        if (!rangemax) return optional<String>::nothing;
        return ToString(*rangemax);
    }

    optional<String> DeviceCreationParameterInt::Possibilities(std::map<String,String> Parameters) {
        std::vector<int> possibilities = PossibilitiesAsInt(Parameters);
        if (possibilities.empty()) return optional<String>::nothing;

        std::vector<int>::iterator iter = possibilities.begin();
        std::stringstream ss;
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << *iter;
            iter++;
        }
        return ss.str();
    }

    String DeviceCreationParameterInt::Value() {
        return ToString(ValueAsInt());
    }

    void DeviceCreationParameterInt::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        int i = __parse_int(val);

        std::map<String,String> emptyMap;
        if (RangeMinAsInt(emptyMap) && i < *RangeMinAsInt(emptyMap)) {
            throw Exception("Invalid device parameter value: too small");
        }
        if (RangeMaxAsInt(emptyMap) && i > *RangeMaxAsInt(emptyMap)) {
            throw Exception("Invalid device parameter value: too big");
        }

        if (PossibilitiesAsInt(emptyMap).size()) {
            bool valid = false;
            std::vector<int> possibilities = PossibilitiesAsInt(emptyMap);
            std::vector<int>::iterator iter  = possibilities.begin();
            while (iter != possibilities.end()) {
                if (i == *iter) {
                    valid = true;
                    break;
                }
                iter++;
            }
            if (!valid) throw Exception("Invalid Device parameter value: not in set of possible values");
        }
        SetValue(i);
    }

    int DeviceCreationParameterInt::ValueAsInt() {
        return iVal;
    }

    void DeviceCreationParameterInt::SetValue(int i) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(i);
        iVal = i;
    }



// *************** DeviceCreationParameterFloat ***************
// *

    DeviceCreationParameterFloat::DeviceCreationParameterFloat(float fVal) : DeviceCreationParameter() {
        this->fVal = fVal;
    }

    DeviceCreationParameterFloat::DeviceCreationParameterFloat(String val) throw (Exception) {
        this->fVal = __parse_float(val);
    }

    void DeviceCreationParameterFloat::InitWithDefault() {
        std::map<String,String> Parameters; // empty parameters vector
        optional<float> f = DefaultAsFloat(Parameters);
        this->fVal = (f) ? *f : 0.0f;
    }

    String DeviceCreationParameterFloat::Type() {
        return "FLOAT";
    }

    bool DeviceCreationParameterFloat::Multiplicity() {
        return false;
    }

    optional<String> DeviceCreationParameterFloat::Default(std::map<String,String> Parameters) {
        optional<float> defaultval = DefaultAsFloat(Parameters);
        if (!defaultval) return optional<String>::nothing;
        return ToString(*defaultval);
    }

    optional<String> DeviceCreationParameterFloat::RangeMin(std::map<String,String> Parameters) {
        optional<float> rangemin = RangeMinAsFloat(Parameters);
        if (!rangemin) return optional<String>::nothing;
        return ToString(*rangemin);
    }

    optional<String> DeviceCreationParameterFloat::RangeMax(std::map<String,String> Parameters) {
        optional<float> rangemax = RangeMaxAsFloat(Parameters);
        if (!rangemax) return optional<String>::nothing;
        return ToString(*rangemax);
    }

    optional<String> DeviceCreationParameterFloat::Possibilities(std::map<String,String> Parameters) {
        std::vector<float> possibilities = PossibilitiesAsFloat(Parameters);
        if (possibilities.empty()) return optional<String>::nothing;

        std::vector<float>::iterator iter = possibilities.begin();
        std::stringstream ss;
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << *iter;
            iter++;
        }
        return ss.str();
    }

    String DeviceCreationParameterFloat::Value() {
        return ToString(ValueAsFloat());
    }

    void DeviceCreationParameterFloat::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        float f = __parse_float(val);
        //if (RangeMinAsFloat() && i < *RangeMinAsFloat()) throw Exception("Invalid device parameter value: too small");
        //if (RangeMaxAsFloat() && i > *RangeMaxAsFloat()) throw Exception("Invalid device parameter value: too big");
        /*if (PossibilitiesAsFloat()) {
            bool valid = false;
            std::vector<float>* pPossibilities = PossibilitiesAsFloat();
            std::vector<float>::iterator iter  = pPossibilities->begin();
            while (iter != pPossibilities->end()) {
                if (f == *iter) {
                    valid = true;
                    break;
                }
                iter++;
            }
            if (!valid) throw Exception("Invalid Device parameter value: not in set of possible values");
        }*/
        SetValue(f);
    }

    float DeviceCreationParameterFloat::ValueAsFloat() {
        return fVal;
    }

    void DeviceCreationParameterFloat::SetValue(float f) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(f);
        fVal = f;
    }



// *************** DeviceCreationParameterString ***************
// *

    DeviceCreationParameterString::DeviceCreationParameterString(String sVal) : DeviceCreationParameter() {
        this->sVal = __parse_string(sVal);
    }

    void DeviceCreationParameterString::InitWithDefault() {
        std::map<String,String> Parameters; // empty parameters vector
        optional<String> defaulval = DefaultAsString(Parameters);
        if (defaulval) this->sVal = *defaulval;
        else           this->sVal = "";
    }

    String DeviceCreationParameterString::Type() {
        return "STRING";
    }

    bool DeviceCreationParameterString::Multiplicity() {
        return false;
    }

    optional<String> DeviceCreationParameterString::Default(std::map<String,String> Parameters) {
        optional<String> defaultval = DefaultAsString(Parameters);
        if (!defaultval) return optional<String>::nothing;
        return "'" + *defaultval + "'";
    }

    optional<String> DeviceCreationParameterString::RangeMin(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterString::RangeMax(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterString::Possibilities(std::map<String,String> Parameters) {
        std::vector<String> possibilities = PossibilitiesAsString(Parameters);
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<String>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << "'" << *iter << "'";
            iter++;
        }
        return ss.str();
    }

    String DeviceCreationParameterString::Value() {
        return "\'" + ValueAsString() + "\'";
    }

    void DeviceCreationParameterString::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        SetValueAsString(__parse_string(val));
    }

    String DeviceCreationParameterString::ValueAsString() {
        return sVal;
    }

    void DeviceCreationParameterString::SetValueAsString(String val) throw (Exception) {
        if (val.find("\'") != String::npos) throw Exception("Character -> \' <- not allowed");
        if (val.find("\"") != String::npos) throw Exception("Character -> \" <- not allowed");
        OnSetValue(val);
        sVal = val;
    }



// *************** DeviceCreationParameterStrings ***************
// *

    DeviceCreationParameterStrings::DeviceCreationParameterStrings(std::vector<String> sVals) : DeviceCreationParameter() {
        this->sVals = sVals;
    }

    DeviceCreationParameterStrings::DeviceCreationParameterStrings(String val) throw (Exception) {
        this->sVals = __parse_strings(val);
    }

    void DeviceCreationParameterStrings::InitWithDefault() {
        std::map<String,String> Parameters; // empty parameters vector
        optional<std::vector<String> > defaultval = DefaultAsStrings(Parameters);
        this->sVals = (defaultval) ? *defaultval : std::vector<String>();
    }

    String DeviceCreationParameterStrings::Type() {
        return "STRING";
    }

    bool DeviceCreationParameterStrings::Multiplicity() {
        return true;
    }

    optional<String> DeviceCreationParameterStrings::Default(std::map<String,String> Parameters) {
        std::vector<String> defaultval = DefaultAsStrings(Parameters);
        if (defaultval.empty()) return optional<String>::nothing;
        String result;
        std::vector<String>::iterator iter = defaultval.begin();
        for (; iter != defaultval.end(); iter++) {
            if (result != "") result += ",";
            result += ("'" + *iter + "'");
        }
        return result;
    }

    optional<String> DeviceCreationParameterStrings::RangeMin(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterStrings::RangeMax(std::map<String,String> Parameters) {
        return optional<String>::nothing;
    }

    optional<String> DeviceCreationParameterStrings::Possibilities(std::map<String,String> Parameters) {
        std::vector<String> possibilities = PossibilitiesAsString(Parameters);
        if (possibilities.empty()) return optional<String>::nothing;

        std::stringstream ss;
        std::vector<String>::iterator iter = possibilities.begin();
        while (iter != possibilities.end()) {
            if (ss.str() != "") ss << ",";
            ss << "'" << *iter << "'";
            iter++;
        }
        return ss.str();
    }

    String DeviceCreationParameterStrings::Value() {
        String result;
        std::vector<String>::iterator iter = this->sVals.begin();
        while (iter != this->sVals.end()) {
            if (result != "") result += ",";
            result += "'" + *iter + "'";
            iter++;
        }
        return result;
    }

    void DeviceCreationParameterStrings::SetValue(String val) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        std::vector<String> vS = __parse_strings(val);
        SetValue(vS);
    }

    std::vector<String> DeviceCreationParameterStrings::ValueAsStrings() {
        return sVals;
    }

    void DeviceCreationParameterStrings::SetValue(std::vector<String> vS) throw (Exception) {
        if (Fix()) throw Exception("Device parameter is read only");
        OnSetValue(vS);
        sVals = vS;
    }

} // namespace LinuxSampler
