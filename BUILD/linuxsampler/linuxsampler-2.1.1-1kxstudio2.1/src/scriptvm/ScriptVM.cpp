/*
 * Copyright (c) 2014 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "ScriptVM.h"

#include <string.h>
#include <assert.h>
#include "../common/global_private.h"
#include "tree.h"
#include "CoreVMFunctions.h"
#include "CoreVMDynVars.h"
#include "editor/NkspScanner.h"

#define DEBUG_SCRIPTVM_CORE 0

/**
 * Maximum amount of VM instructions to be executed per ScriptVM::exec() call
 * in case loops are involved, before the script got automatically suspended
 * for a certain amount of time to avoid any RT instability issues.
 *
 * The following value takes a max. execution time of 300 microseconds as aimed
 * target, assuming an execution time of approximately 5 microseconds per
 * instruction this leads to the very approximate value set below.
 */
#define SCRIPTVM_MAX_INSTR_PER_CYCLE_SOFT 70

/**
 * Absolute maximum amount of VM instructions to be executed per
 * ScriptVM::exec() call (even if no loops are involved), before the script got
 * automatically suspended for a certain amount of time to avoid any RT
 * instability issues.
 *
 * A distinction between "soft" and "hard" limit is done here ATM because a
 * script author typically expects that his script might be interrupted
 * automatically if he is using while() loops, however he might not be
 * prepared that his script might also be interrupted if no loop is involved
 * (i.e. on very large scripts).
 *
 * The following value takes a max. execution time of 1000 microseconds as
 * aimed target, assuming an execution time of approximately 5 microseconds per
 * instruction this leads to the very approximate value set below.
 */
#define SCRIPTVM_MAX_INSTR_PER_CYCLE_HARD 210

/**
 * In case either SCRIPTVM_MAX_INSTR_PER_CYCLE_SOFT or
 * SCRIPTVM_MAX_INSTR_PER_CYCLE_HARD was exceeded when calling
 * ScriptVM::exec() : the amount of microseconds the respective script
 * execution instance should be automatically suspended by the VM.
 */
#define SCRIPT_VM_FORCE_SUSPENSION_MICROSECONDS 1000

int InstrScript_parse(LinuxSampler::ParserContext*);

namespace LinuxSampler {

    #if DEBUG_SCRIPTVM_CORE
    static void _printIndents(int n) {
        for (int i = 0; i < n; ++i) printf("  ");
        fflush(stdout);
    }
    #endif

    static int _requiredMaxStackSizeFor(Statement* statement, int depth = 0) {
        if (!statement) return 1;

        switch (statement->statementType()) {
            case STMT_LEAF:
                #if DEBUG_SCRIPTVM_CORE
                _printIndents(depth);
                printf("-> STMT_LEAF\n");
                #endif
                return 1;

            case STMT_LIST: {
                #if DEBUG_SCRIPTVM_CORE
                _printIndents(depth);
                printf("-> STMT_LIST\n");
                #endif
                Statements* stmts = (Statements*) statement;
                int max = 0;
                for (int i = 0; stmts->statement(i); ++i) {
                    int size = _requiredMaxStackSizeFor( stmts->statement(i), depth+1 );
                    if (max < size) max = size;
                }
                return max + 1;
            }

            case STMT_BRANCH: {
                #if DEBUG_SCRIPTVM_CORE
                _printIndents(depth);
                printf("-> STMT_BRANCH\n");
                #endif
                BranchStatement* branchStmt = (BranchStatement*) statement;
                int max = 0;
                for (int i = 0; branchStmt->branch(i); ++i) {
                    int size = _requiredMaxStackSizeFor( branchStmt->branch(i), depth+1 );
                    if (max < size) max = size;
                }
                return max + 1;
            }

            case STMT_LOOP: {
                #if DEBUG_SCRIPTVM_CORE
                _printIndents(depth);
                printf("-> STMT_LOOP\n");
                #endif
                While* whileStmt = (While*) statement;
                if (whileStmt->statements())
                    return _requiredMaxStackSizeFor( whileStmt->statements() ) + 1;
                else
                    return 1;
            }

            case STMT_SYNC: {
                #if DEBUG_SCRIPTVM_CORE
                _printIndents(depth);
                printf("-> STMT_SYNC\n");
                #endif
                SyncBlock* syncStmt = (SyncBlock*) statement;
                if (syncStmt->statements())
                    return _requiredMaxStackSizeFor( syncStmt->statements() ) + 1;
                else
                    return 1;
            }
        }

        return 1; // actually just to avoid compiler warning
    }

    static int _requiredMaxStackSizeFor(EventHandlers* handlers) {
        int max = 1;
        for (int i = 0; i < handlers->size(); ++i) {
            int size = _requiredMaxStackSizeFor(handlers->eventHandler(i));
            if (max < size) max = size;
        }
        return max;
    }

    ScriptVM::ScriptVM() : m_eventHandler(NULL), m_parserContext(NULL), m_autoSuspend(true) {
        m_fnMessage = new CoreVMFunction_message;
        m_fnExit = new CoreVMFunction_exit;
        m_fnWait = new CoreVMFunction_wait(this);
        m_fnAbs = new CoreVMFunction_abs;
        m_fnRandom = new CoreVMFunction_random;
        m_fnNumElements = new CoreVMFunction_num_elements;
        m_fnInc = new CoreVMFunction_inc;
        m_fnDec = new CoreVMFunction_dec;
        m_fnInRange = new CoreVMFunction_in_range;
        m_varRealTimer = new CoreVMDynVar_NKSP_REAL_TIMER;
        m_varPerfTimer = new CoreVMDynVar_NKSP_PERF_TIMER;
        m_fnShLeft = new CoreVMFunction_sh_left;
        m_fnShRight = new CoreVMFunction_sh_right;
        m_fnMin = new CoreVMFunction_min;
        m_fnMax = new CoreVMFunction_max;
        m_fnArrayEqual = new CoreVMFunction_array_equal;
        m_fnSearch = new CoreVMFunction_search;
        m_fnSort = new CoreVMFunction_sort;
    }

    ScriptVM::~ScriptVM() {
        delete m_fnMessage;
        delete m_fnExit;
        delete m_fnWait;
        delete m_fnAbs;
        delete m_fnRandom;
        delete m_fnNumElements;
        delete m_fnInc;
        delete m_fnDec;
        delete m_fnInRange;
        delete m_fnShLeft;
        delete m_fnShRight;
        delete m_fnMin;
        delete m_fnMax;
        delete m_fnArrayEqual;
        delete m_fnSearch;
        delete m_fnSort;
        delete m_varRealTimer;
        delete m_varPerfTimer;
    }

    VMParserContext* ScriptVM::loadScript(const String& s) {
        std::istringstream iss(s);
        return loadScript(&iss);
    }
    
    VMParserContext* ScriptVM::loadScript(std::istream* is) {
        ParserContext* context = new ParserContext(this);
        //printf("parserCtx=0x%lx\n", (uint64_t)context);

        context->registerBuiltInConstIntVariables( builtInConstIntVariables() );
        context->registerBuiltInIntVariables( builtInIntVariables() );
        context->registerBuiltInIntArrayVariables( builtInIntArrayVariables() );
        context->registerBuiltInDynVariables( builtInDynamicVariables() );

        context->createScanner(is);

        InstrScript_parse(context);
        dmsg(2,("Allocating %ld bytes of global int VM memory.\n", long(context->globalIntVarCount * sizeof(int))));
        dmsg(2,("Allocating %d of global VM string variables.\n", context->globalStrVarCount));
        if (!context->globalIntMemory)
            context->globalIntMemory = new ArrayList<int>();
        if (!context->globalStrMemory)
            context->globalStrMemory = new ArrayList<String>();
        context->globalIntMemory->resize(context->globalIntVarCount);
        memset(&((*context->globalIntMemory)[0]), 0, context->globalIntVarCount * sizeof(int));
        
        context->globalStrMemory->resize(context->globalStrVarCount);

        context->destroyScanner();

        return context;
    }

    void ScriptVM::dumpParsedScript(VMParserContext* context) {
        ParserContext* ctx = dynamic_cast<ParserContext*>(context);
        if (!ctx) {
            std::cerr << "No VM context. So nothing to dump.\n";
            return;
        }
        if (!ctx->handlers) {
            std::cerr << "No event handlers defined in script. So nothing to dump.\n";
            return;
        }
        if (!ctx->globalIntMemory) {
            std::cerr << "Internal error: no global memory assigend to script VM.\n";
            return;
        }
        ctx->handlers->dump();
    }

    VMExecContext* ScriptVM::createExecContext(VMParserContext* parserContext) {
        ParserContext* parserCtx = dynamic_cast<ParserContext*>(parserContext);
        ExecContext* execCtx = new ExecContext();
        
        if (parserCtx->requiredMaxStackSize < 0) {
             parserCtx->requiredMaxStackSize =
                _requiredMaxStackSizeFor(&*parserCtx->handlers);
        }
        execCtx->stack.resize(parserCtx->requiredMaxStackSize);
        dmsg(2,("Created VM exec context with %ld bytes VM stack size.\n",
                long(parserCtx->requiredMaxStackSize * sizeof(ExecContext::StackFrame))));
        //printf("execCtx=0x%lx\n", (uint64_t)execCtx);
        const int polySize = parserCtx->polyphonicIntVarCount;
        execCtx->polyphonicIntMemory.resize(polySize);
        memset(&execCtx->polyphonicIntMemory[0], 0, polySize * sizeof(int));

        dmsg(2,("Allocated %ld bytes polyphonic memory.\n", long(polySize * sizeof(int))));
        return execCtx;
    }

    std::vector<VMSourceToken> ScriptVM::syntaxHighlighting(const String& s) {
        std::istringstream iss(s);
        return syntaxHighlighting(&iss);
    }

    std::vector<VMSourceToken> ScriptVM::syntaxHighlighting(std::istream* is) {
        try {
            NkspScanner scanner(is);
            std::vector<SourceToken> tokens = scanner.tokens();
            std::vector<VMSourceToken> result;
            result.resize(tokens.size());
            for (int i = 0; i < tokens.size(); ++i) {
                SourceToken* st = new SourceToken;
                *st = tokens[i];
                result[i] = VMSourceToken(st);
            }
            return result;
        } catch (...) {
            return std::vector<VMSourceToken>();
        }
    }

    VMFunction* ScriptVM::functionByName(const String& name) {
        if (name == "message") return m_fnMessage;
        else if (name == "exit") return m_fnExit;
        else if (name == "wait") return m_fnWait;
        else if (name == "abs") return m_fnAbs;
        else if (name == "random") return m_fnRandom;
        else if (name == "num_elements") return m_fnNumElements;
        else if (name == "inc") return m_fnInc;
        else if (name == "dec") return m_fnDec;
        else if (name == "in_range") return m_fnInRange;
        else if (name == "sh_left") return m_fnShLeft;
        else if (name == "sh_right") return m_fnShRight;
        else if (name == "min") return m_fnMin;
        else if (name == "max") return m_fnMax;
        else if (name == "array_equal") return m_fnArrayEqual;
        else if (name == "search") return m_fnSearch;
        else if (name == "sort") return m_fnSort;
        return NULL;
    }

    bool ScriptVM::isFunctionDisabled(VMFunction* fn, VMParserContext* ctx) {
        ParserContext* parserCtx = dynamic_cast<ParserContext*>(ctx);
        if (!parserCtx) return false;

        if (fn == m_fnMessage && parserCtx->userPreprocessorConditions.count("NKSP_NO_MESSAGE"))
            return true;

        return false;
    }

    std::map<String,VMIntRelPtr*> ScriptVM::builtInIntVariables() {
        return std::map<String,VMIntRelPtr*>();
    }

    std::map<String,VMInt8Array*> ScriptVM::builtInIntArrayVariables() {
        return std::map<String,VMInt8Array*>();
    }

    std::map<String,VMDynVar*> ScriptVM::builtInDynamicVariables() {
        std::map<String,VMDynVar*> m;

        m["$NKSP_PERF_TIMER"] = m_varPerfTimer;
        m["$NKSP_REAL_TIMER"] = m_varRealTimer;
        m["$KSP_TIMER"] = m_varRealTimer;

        return m;
    }

    std::map<String,int> ScriptVM::builtInConstIntVariables() {
        std::map<String,int> m;

        m["$NI_CB_TYPE_INIT"] = VM_EVENT_HANDLER_INIT;
        m["$NI_CB_TYPE_NOTE"] = VM_EVENT_HANDLER_NOTE;
        m["$NI_CB_TYPE_RELEASE"] = VM_EVENT_HANDLER_RELEASE;
        m["$NI_CB_TYPE_CONTROLLER"] = VM_EVENT_HANDLER_CONTROLLER;

        return m;
    }

    VMEventHandler* ScriptVM::currentVMEventHandler() {
        return m_eventHandler;
    }

    VMParserContext* ScriptVM::currentVMParserContext() {
        return m_parserContext;
    }

    VMExecContext* ScriptVM::currentVMExecContext() {
        if (!m_parserContext) return NULL;
        return m_parserContext->execContext;
    }

    void ScriptVM::setAutoSuspendEnabled(bool b) {
        m_autoSuspend = b;
    }

    bool ScriptVM::isAutoSuspendEnabled() const {
        return m_autoSuspend;
    }

    VMExecStatus_t ScriptVM::exec(VMParserContext* parserContext, VMExecContext* execContex, VMEventHandler* handler) {
        m_parserContext = dynamic_cast<ParserContext*>(parserContext);
        if (!m_parserContext) {
            std::cerr << "No VM parser context provided. Did you load a script?.\n";
            return VMExecStatus_t(VM_EXEC_NOT_RUNNING | VM_EXEC_ERROR);
        }

        // a ParserContext object is always tied to exactly one ScriptVM object
        assert(m_parserContext->functionProvider == this);

        ExecContext* ctx = dynamic_cast<ExecContext*>(execContex);
        if (!ctx) {
            std::cerr << "Invalid VM exec context.\n";
            return VMExecStatus_t(VM_EXEC_NOT_RUNNING | VM_EXEC_ERROR);
        }
        EventHandler* h = dynamic_cast<EventHandler*>(handler);
        if (!h) return VM_EXEC_NOT_RUNNING;
        m_eventHandler = handler;

        m_parserContext->execContext = ctx;

        ctx->status = VM_EXEC_RUNNING;
        ctx->instructionsCount = 0;
        StmtFlags_t& flags = ctx->flags;
        int instructionsCounter = 0;
        int synced = m_autoSuspend ? 0 : 1;

        int& frameIdx = ctx->stackFrame;
        if (frameIdx < 0) { // start condition ...
            frameIdx = -1;
            ctx->pushStack(h);
        }

        while (flags == STMT_SUCCESS && frameIdx >= 0) {
            if (frameIdx >= ctx->stack.size()) { // should never happen, otherwise it's a bug ...
                std::cerr << "CRITICAL: VM stack overflow! (" << frameIdx << ")\n";
                flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
                break;
            }

            ExecContext::StackFrame& frame = ctx->stack[frameIdx];
            switch (frame.statement->statementType()) {
                case STMT_LEAF: {
                    #if DEBUG_SCRIPTVM_CORE
                    _printIndents(frameIdx);
                    printf("-> STMT_LEAF\n");
                    #endif
                    LeafStatement* leaf = (LeafStatement*) frame.statement;
                    flags = leaf->exec();
                    ctx->popStack();
                    break;
                }

                case STMT_LIST: {
                    #if DEBUG_SCRIPTVM_CORE
                    _printIndents(frameIdx);
                    printf("-> STMT_LIST subidx=%d\n", frame.subindex);
                    #endif
                    Statements* stmts = (Statements*) frame.statement;
                    if (stmts->statement(frame.subindex)) {
                        ctx->pushStack(
                            stmts->statement(frame.subindex++)
                        );
                    } else {
                        #if DEBUG_SCRIPTVM_CORE
                        _printIndents(frameIdx);
                        printf("[END OF LIST] subidx=%d\n", frame.subindex);
                        #endif
                        ctx->popStack();
                    }
                    break;
                }

                case STMT_BRANCH: {
                    #if DEBUG_SCRIPTVM_CORE
                    _printIndents(frameIdx);
                    printf("-> STMT_BRANCH\n");
                    #endif
                    if (frame.subindex < 0) ctx->popStack();
                    else {
                        BranchStatement* branchStmt = (BranchStatement*) frame.statement;
                        frame.subindex = branchStmt->evalBranch();
                        if (frame.subindex >= 0) {
                            ctx->pushStack(
                                branchStmt->branch(frame.subindex)
                            );
                            frame.subindex = -1;
                        } else ctx->popStack();
                    }
                    break;
                }

                case STMT_LOOP: {
                    #if DEBUG_SCRIPTVM_CORE
                    _printIndents(frameIdx);
                    printf("-> STMT_LOOP\n");
                    #endif
                    While* whileStmt = (While*) frame.statement;
                    if (whileStmt->evalLoopStartCondition() && whileStmt->statements()) {
                        ctx->pushStack(
                            whileStmt->statements()
                        );
                        if (flags == STMT_SUCCESS && !synced &&
                            instructionsCounter > SCRIPTVM_MAX_INSTR_PER_CYCLE_SOFT)
                        {
                            flags = StmtFlags_t(STMT_SUSPEND_SIGNALLED);
                            ctx->suspendMicroseconds = SCRIPT_VM_FORCE_SUSPENSION_MICROSECONDS;
                        }
                    } else ctx->popStack();
                    break;
                }

                case STMT_SYNC: {
                    #if DEBUG_SCRIPTVM_CORE
                    _printIndents(frameIdx);
                    printf("-> STMT_SYNC\n");
                    #endif
                    SyncBlock* syncStmt = (SyncBlock*) frame.statement;
                    if (!frame.subindex++ && syncStmt->statements()) {
                        ++synced;
                        ctx->pushStack(
                            syncStmt->statements()
                        );
                    } else {
                        ctx->popStack();
                        --synced;
                    }
                    break;
                }
            }

            if (flags == STMT_SUCCESS && !synced &&
                instructionsCounter > SCRIPTVM_MAX_INSTR_PER_CYCLE_HARD)
            {
                flags = StmtFlags_t(STMT_SUSPEND_SIGNALLED);
                ctx->suspendMicroseconds = SCRIPT_VM_FORCE_SUSPENSION_MICROSECONDS;
            }

            ++instructionsCounter;
        }

        if ((flags & STMT_SUSPEND_SIGNALLED) && !(flags & STMT_ABORT_SIGNALLED)) {
            ctx->status = VM_EXEC_SUSPENDED;
            ctx->flags  = STMT_SUCCESS;
        } else {
            ctx->status = VM_EXEC_NOT_RUNNING;
            if (flags & STMT_ERROR_OCCURRED)
                ctx->status = VM_EXEC_ERROR;
            ctx->reset();
        }

        ctx->instructionsCount = instructionsCounter;

        m_eventHandler = NULL;
        m_parserContext->execContext = NULL;
        m_parserContext = NULL;
        return ctx->status;
    }

} // namespace LinuxSampler
