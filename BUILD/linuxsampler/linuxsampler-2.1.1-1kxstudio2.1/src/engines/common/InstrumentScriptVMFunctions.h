/*
 * Copyright (c) 2014 - 2019 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_INSTRSCRIPTVMFUNCTIONS_H
#define LS_INSTRSCRIPTVMFUNCTIONS_H

#include "../../common/global.h"
#include "../../scriptvm/CoreVMFunctions.h"
#include "Note.h"

namespace LinuxSampler {

    class EventGroup;
    class InstrumentScriptVM;

    class InstrumentScriptVMFunction_play_note : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_play_note(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 1; }
        int maxAllowedArgs() const { return 4; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_controller : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_set_controller(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_ignore_event : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_ignore_event(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 0; }
        int maxAllowedArgs() const { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_ignore_controller : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_ignore_controller(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 0; }
        int maxAllowedArgs() const { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_note_off : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_note_off(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 1; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_event_mark : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_set_event_mark(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_delete_event_mark : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_delete_event_mark(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_by_marks : public VMFunction {
    public:
        InstrumentScriptVMFunction_by_marks(InstrumentScriptVM* parent);
        int minRequiredArgs() const OVERRIDE { return 1; }
        int maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR;}
        bool modifiesArg(int iArg) const OVERRIDE { return false; }
        ExprType_t argType(int iArg) const OVERRIDE { return INT_EXPR; }
        ExprType_t returnType() OVERRIDE { return INT_ARR_EXPR; }
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
        class Result : public VMFnResult, public VMIntArrayExpr {
        public:
            StmtFlags_t flags;
            EventGroup* eventGroup;

            int arraySize() const OVERRIDE;
            int evalIntElement(uint i) OVERRIDE;
            void assignIntElement(uint i, int value) OVERRIDE {} // ignore assignment
            VMExpr* resultValue() OVERRIDE { return this; }
            StmtFlags_t resultFlags() OVERRIDE { return flags; }
            bool isConstExpr() const OVERRIDE { return false; }
        } m_result;

        VMFnResult* errorResult();
        VMFnResult* successResult(EventGroup* eventGroup);
    };

    class InstrumentScriptVMFunction_change_vol : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_vol(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 3; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_tune : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_tune(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 3; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_pan : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_pan(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 3; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_cutoff : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_reso : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_reso(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };
    
    class InstrumentScriptVMFunction_change_attack : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_attack(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_decay : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_decay(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };
    
    class InstrumentScriptVMFunction_change_release : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_release(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class VMChangeSynthParamFunction : public VMEmptyResultFunction {
    public:
        VMChangeSynthParamFunction(InstrumentScriptVM* parent) : m_vm(parent) {}
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }

        template<float NoteBase::_Override::*T_noteParam, int T_synthParam,
                 bool T_isNormalizedParam, int T_maxValue, int T_minValue>
        VMFnResult* execTemplate(VMFnArgs* args, const char* functionName);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_sustain : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_sustain(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_attack : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_attack(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_decay : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_decay(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_sustain : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_sustain(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_release : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_release(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_amp_lfo_depth : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_amp_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_amp_lfo_freq : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_amp_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_lfo_depth : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_cutoff_lfo_freq : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_cutoff_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_pitch_lfo_depth : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pitch_lfo_depth(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_pitch_lfo_freq : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pitch_lfo_freq(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_vol_time : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_vol_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_tune_time : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_tune_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_pan_time : public VMChangeSynthParamFunction {
    public:
        InstrumentScriptVMFunction_change_pan_time(InstrumentScriptVM* parent) : VMChangeSynthParamFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class VMChangeFadeCurveFunction : public VMEmptyResultFunction {
    public:
        VMChangeFadeCurveFunction(InstrumentScriptVM* parent) : m_vm(parent) {}
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }

        template<fade_curve_t NoteBase::_Override::*T_noteParam, int T_synthParam>
        VMFnResult* execTemplate(VMFnArgs* args, const char* functionName);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_vol_curve : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_vol_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_tune_curve : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_tune_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_change_pan_curve : public VMChangeFadeCurveFunction {
    public:
        InstrumentScriptVMFunction_change_pan_curve(InstrumentScriptVM* parent) : VMChangeFadeCurveFunction(parent) {}
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_fade_in : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_fade_in(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_fade_out : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_fade_out(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 3; }
        bool acceptsArgType(int iArg, ExprType_t type) const;
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_get_event_par : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_get_event_par(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_set_event_par : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_set_event_par(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 3; }
        int maxAllowedArgs() const { return 3; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_note : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_note(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_velo : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_velo(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_change_play_pos : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_change_play_pos(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 2; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_event_status : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_event_status(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 1; }
        int maxAllowedArgs() const { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_callback_status : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_callback_status(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 1; }
        int maxAllowedArgs() const { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    // overrides core wait() implementation
    class InstrumentScriptVMFunction_wait : public CoreVMFunction_wait {
    public:
        InstrumentScriptVMFunction_wait(InstrumentScriptVM* parent);
        VMFnResult* exec(VMFnArgs* args);
    };

    class InstrumentScriptVMFunction_stop_wait : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_stop_wait(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 1; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_abort : public VMEmptyResultFunction {
    public:
        InstrumentScriptVMFunction_abort(InstrumentScriptVM* parent);
        int minRequiredArgs() const OVERRIDE { return 1; }
        int maxAllowedArgs() const OVERRIDE { return 1; }
        bool acceptsArgType(int iArg, ExprType_t type) const OVERRIDE { return type == INT_EXPR; }
        ExprType_t argType(int iArg) const OVERRIDE { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args) OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    class InstrumentScriptVMFunction_fork : public VMIntResultFunction {
    public:
        InstrumentScriptVMFunction_fork(InstrumentScriptVM* parent);
        int minRequiredArgs() const { return 0; }
        int maxAllowedArgs() const { return 2; }
        bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR;}
        ExprType_t argType(int iArg) const { return INT_EXPR; }
        VMFnResult* exec(VMFnArgs* args);
    protected:
        InstrumentScriptVM* m_vm;
    };

} // namespace LinuxSampler

#endif // LS_INSTRSCRIPTVMFUNCTIONS_H
