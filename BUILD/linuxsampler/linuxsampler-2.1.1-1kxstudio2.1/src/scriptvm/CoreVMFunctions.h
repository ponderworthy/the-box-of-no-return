/*
 * Copyright (c) 2014-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_COREVMFUNCTIONS_H
#define LS_COREVMFUNCTIONS_H

#include "../common/global.h"
#include "common.h"

namespace LinuxSampler {
    
class ScriptVM;

///////////////////////////////////////////////////////////////////////////
// convenience base classes for built-in script functions ...

/**
 * An instance of this class is returned by built-in function implementations
 * which do not return a function return value.
 */
class VMEmptyResult : public VMFnResult, public VMExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call

    VMEmptyResult() : flags(STMT_SUCCESS) {}
    ExprType_t exprType() const OVERRIDE { return EMPTY_EXPR; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
};

/**
 * An instance of this class is returned by built-in function implementations
 * which return an integer value as function return value.
 */
class VMIntResult : public VMFnResult, public VMIntExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call
    int value; ///< result value of the function call

    VMIntResult() : flags(STMT_SUCCESS) {}
    int evalInt() OVERRIDE { return value; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
};

/**
 * An instance of this class is returned by built-in function implementations
 * which return a string value as function return value.
 */
class VMStringResult : public VMFnResult, public VMStringExpr {
public:
    StmtFlags_t flags; ///< general completion status (i.e. success or failure) of the function call
    String value; ///< result value of the function call

    VMStringResult() : flags(STMT_SUCCESS) {}
    String evalStr() OVERRIDE { return value; }
    VMExpr* resultValue() OVERRIDE { return this; }
    StmtFlags_t resultFlags() OVERRIDE { return flags; }
    bool isConstExpr() const OVERRIDE { return false; }
};

/**
 * Abstract base class for built-in script functions which do not return any
 * function return value (void).
 */
class VMEmptyResultFunction : public VMFunction {
protected:
    virtual ~VMEmptyResultFunction() {}
    ExprType_t returnType() OVERRIDE { return EMPTY_EXPR; }
    VMFnResult* errorResult();
    VMFnResult* successResult();
    bool modifiesArg(int iArg) const OVERRIDE { return false; }
protected:
    VMEmptyResult result;
};

/**
 * Abstract base class for built-in script functions which return an integer
 * (scalar) as their function return value.
 */
class VMIntResultFunction : public VMFunction {
protected:
    virtual ~VMIntResultFunction() {}
    ExprType_t returnType() OVERRIDE { return INT_EXPR; }
    VMFnResult* errorResult(int i = 0);
    VMFnResult* successResult(int i = 0);
    bool modifiesArg(int iArg) const OVERRIDE { return false; }
protected:
    VMIntResult result;
};

/**
 * Abstract base class for built-in script functions which return a string as
 * their function return value.
 */
class VMStringResultFunction : public VMFunction {
protected:
    virtual ~VMStringResultFunction() {}
    ExprType_t returnType() OVERRIDE { return STRING_EXPR; }
    VMFnResult* errorResult(const String& s = "");
    VMFnResult* successResult(const String& s = "");
    bool modifiesArg(int iArg) const OVERRIDE { return false; }
protected:
    VMStringResult result;
};


///////////////////////////////////////////////////////////////////////////
// implementations of core built-in script functions ...

/**
 * Implements the built-in message() script function.
 */
class CoreVMFunction_message : public VMEmptyResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const { return STRING_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in exit() script function.
 */
class CoreVMFunction_exit : public VMEmptyResultFunction {
public:
    int minRequiredArgs() const { return 0; }
    int maxAllowedArgs() const { return 0; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return false; }
    ExprType_t argType(int iArg) const { return INT_EXPR; /*whatever*/ }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in wait() script function.
 */
class CoreVMFunction_wait : public VMEmptyResultFunction {
public:
    CoreVMFunction_wait(ScriptVM* vm) : vm(vm) {}
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
protected:
    ScriptVM* vm;
};

/**
 * Implements the built-in abs() script function.
 */
class CoreVMFunction_abs : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in random() script function.
 */
class CoreVMFunction_random : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in num_elements() script function.
 */
class CoreVMFunction_num_elements : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const { return INT_ARR_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in inc() script function.
 */
class CoreVMFunction_inc : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    bool modifiesArg(int iArg) const { return true; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in dec() script function.
 */
class CoreVMFunction_dec : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 1; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    bool modifiesArg(int iArg) const { return true; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in in_range() script function.
 */
class CoreVMFunction_in_range : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 3; }
    int maxAllowedArgs() const { return 3; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in sh_left() script function.
 */
class CoreVMFunction_sh_left : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in sh_right() script function.
 */
class CoreVMFunction_sh_right : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in min() script function.
 */
class CoreVMFunction_min : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in max() script function.
 */
class CoreVMFunction_max : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_EXPR; }
    ExprType_t argType(int iArg) const { return INT_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in array_equal() script function.
 */
class CoreVMFunction_array_equal : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const { return type == INT_ARR_EXPR; }
    ExprType_t argType(int iArg) const { return INT_ARR_EXPR; }
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in search() script function.
 */
class CoreVMFunction_search : public VMIntResultFunction {
public:
    int minRequiredArgs() const { return 2; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const;
    VMFnResult* exec(VMFnArgs* args);
};

/**
 * Implements the built-in sort() script function.
 */
class CoreVMFunction_sort : public VMEmptyResultFunction {
public:
    int minRequiredArgs() const { return 1; }
    int maxAllowedArgs() const { return 2; }
    bool acceptsArgType(int iArg, ExprType_t type) const;
    ExprType_t argType(int iArg) const;
    bool modifiesArg(int iArg) const { return iArg == 0; }
    VMFnResult* exec(VMFnArgs* args);
};

} // namespace LinuxSampler

#endif // LS_COREVMFUNCTIONS_H
