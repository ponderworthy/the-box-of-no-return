/*
 * Copyright (c) 2014 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "InstrumentScriptVM.h"
#include "../../common/global_private.h"
#include "EngineChannel.h"

namespace LinuxSampler { namespace sfz {

    InstrumentScriptVM::InstrumentScriptVM() :
        LinuxSampler::InstrumentScriptVM()
    {
    }

    /*std::map<String,int> InstrumentScriptVM::builtInConstIntVariables() {
        // first get buil-in integer variables of derived VM class
        std::map<String,int> m =
            ::LinuxSampler::InstrumentScriptVM::builtInConstIntVariables();

        // add own built-in script constants
        m["$GIG_DIM_CHANNEL"] = ::gig::dimension_samplechannel;
        m["$GIG_DIM_LAYER"] = ::gig::dimension_layer;
        m["$GIG_DIM_VELOCITY"] = ::gig::dimension_velocity;
        m["$GIG_DIM_AFTERTOUCH"] = ::gig::dimension_channelaftertouch;
        m["$GIG_DIM_RELEASE"] = ::gig::dimension_releasetrigger;
        m["$GIG_DIM_KEYBOARD"] = ::gig::dimension_keyboard;
        m["$GIG_DIM_ROUNDROBIN"] = ::gig::dimension_roundrobin;
        m["$GIG_DIM_RANDOM"] = ::gig::dimension_random;
        m["$GIG_DIM_SMARTMIDI"] = ::gig::dimension_smartmidi;
        m["$GIG_DIM_ROUNDROBINKEY"] = ::gig::dimension_roundrobinkeyboard;
        m["$GIG_DIM_MODWHEEL"] = ::gig::dimension_modwheel;
        m["$GIG_DIM_BREATH"] = ::gig::dimension_breath;
        m["$GIG_DIM_FOOT"] = ::gig::dimension_foot;
        m["$GIG_DIM_PORTAMENTOTIME"] = ::gig::dimension_portamentotime;
        m["$GIG_DIM_EFFECT1"] = ::gig::dimension_effect1;
        m["$GIG_DIM_EFFECT2"] = ::gig::dimension_effect2;
        m["$GIG_DIM_GENPURPOSE1"] = ::gig::dimension_genpurpose1;
        m["$GIG_DIM_GENPURPOSE2"] = ::gig::dimension_genpurpose2;
        m["$GIG_DIM_GENPURPOSE3"] = ::gig::dimension_genpurpose3;
        m["$GIG_DIM_GENPURPOSE4"] = ::gig::dimension_genpurpose4;
        m["$GIG_DIM_SUSTAIN"] = ::gig::dimension_sustainpedal;
        m["$GIG_DIM_PORTAMENTO"] = ::gig::dimension_portamento;
        m["$GIG_DIM_SOSTENUTO"] = ::gig::dimension_sostenutopedal;
        m["$GIG_DIM_SOFT"] = ::gig::dimension_softpedal;
        m["$GIG_DIM_GENPURPOSE5"] = ::gig::dimension_genpurpose5;
        m["$GIG_DIM_GENPURPOSE6"] = ::gig::dimension_genpurpose6;
        m["$GIG_DIM_GENPURPOSE7"] = ::gig::dimension_genpurpose7;
        m["$GIG_DIM_GENPURPOSE8"] = ::gig::dimension_genpurpose8;
        m["$GIG_DIM_EFFECT1DEPTH"] = ::gig::dimension_effect1depth;
        m["$GIG_DIM_EFFECT2DEPTH"] = ::gig::dimension_effect2depth;
        m["$GIG_DIM_EFFECT3DEPTH"] = ::gig::dimension_effect3depth;
        m["$GIG_DIM_EFFECT4DEPTH"] = ::gig::dimension_effect4depth;
        m["$GIG_DIM_EFFECT5DEPTH"] = ::gig::dimension_effect5depth;

        return m;
    }*/

    /*VMFunction* InstrumentScriptVM::functionByName(const String& name) {
        // built-in script functions of this class
        if (name == "gig_set_dim_zone") return &m_fnGigSetDimZone;

        // built-in script functions of derived VM class
        return ::LinuxSampler::InstrumentScriptVM::functionByName(name);
    }*/

}} // namespace LinuxSampler::sfz
