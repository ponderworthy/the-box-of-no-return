/*
 * Copyright (c) 2016 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_INSTRSCRIPTVMDYNVARS_H
#define LS_INSTRSCRIPTVMDYNVARS_H

#include "../../common/global.h"
#include "../../scriptvm/CoreVMDynVars.h"
#include "Event.h"

namespace LinuxSampler {

    class InstrumentScriptVM;

    /**
     * Implements the built-in $ENGINE_UPTIME script variable.
     */
    class InstrumentScriptVMDynVar_ENGINE_UPTIME : public VMDynIntVar {
    public:
        InstrumentScriptVMDynVar_ENGINE_UPTIME(InstrumentScriptVM* parent) : m_vm(parent) {}
        bool isAssignable() const OVERRIDE { return false; }
        int evalInt() OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    /**
     * Implements the built-in $NI_CALLBACK_ID script variable.
     */
    class InstrumentScriptVMDynVar_NI_CALLBACK_ID : public VMDynIntVar {
    public:
        InstrumentScriptVMDynVar_NI_CALLBACK_ID(InstrumentScriptVM* parent) : m_vm(parent) {}
        bool isAssignable() const OVERRIDE { return false; }
        int evalInt() OVERRIDE;
    protected:
        InstrumentScriptVM* m_vm;
    };

    /**
     * Implements the built-in array %NKSP_CALLBACK_CHILD_ID[] script variable.
     */
    class InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID : public VMDynIntArrayVar {
    public:
        InstrumentScriptVMDynVar_NKSP_CALLBACK_CHILD_ID(InstrumentScriptVM* parent);
        VMIntArrayExpr* asIntArray() const OVERRIDE;
        int arraySize() const OVERRIDE;
        bool isAssignable() const OVERRIDE { return false; }
        int evalIntElement(uint i) OVERRIDE;
        void assignIntElement(uint i, int value) OVERRIDE {}
    protected:
        InstrumentScriptVM* m_vm;
    };

    /**
     * Implements the built-in %ALL_EVENTS script array variable.
     */
    class InstrumentScriptVMDynVar_ALL_EVENTS : public VMDynIntArrayVar {
    public:
        InstrumentScriptVMDynVar_ALL_EVENTS(InstrumentScriptVM* parent);
        virtual ~InstrumentScriptVMDynVar_ALL_EVENTS();
        VMIntArrayExpr* asIntArray() const OVERRIDE;
        int arraySize() const OVERRIDE;
        bool isAssignable() const OVERRIDE { return false; }
        int evalIntElement(uint i) OVERRIDE;
        void assignIntElement(uint i, int value) OVERRIDE {}
    protected:
        void updateNoteIDs();
    private:
        InstrumentScriptVM* m_vm;
        note_id_t* m_ids;
        uint m_numIDs;
    };

} // namespace LinuxSampler

#endif // LS_INSTRSCRIPTVMDYNVARS_H
