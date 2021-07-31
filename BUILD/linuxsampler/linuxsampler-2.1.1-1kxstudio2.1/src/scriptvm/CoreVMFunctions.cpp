/*
 * Copyright (c) 2014-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "CoreVMFunctions.h"

#include <iostream>
#include <algorithm> // for std::sort()
#include <math.h>
#include <stdlib.h>
#include "tree.h"
#include "ScriptVM.h"
#include "../common/RTMath.h"

namespace LinuxSampler {

///////////////////////////////////////////////////////////////////////////
// class VMEmptyResultFunction

VMFnResult* VMEmptyResultFunction::errorResult() {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    return &result;
}

VMFnResult* VMEmptyResultFunction::successResult() {
    result.flags = STMT_SUCCESS;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMIntResultFunction

VMFnResult* VMIntResultFunction::errorResult(int i) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = i;
    return &result;
}

VMFnResult* VMIntResultFunction::successResult(int i) {
    result.flags = STMT_SUCCESS;
    result.value = i;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// class VMStringResultFunction

VMFnResult* VMStringResultFunction::errorResult(const String& s) {
    result.flags = StmtFlags_t(STMT_ABORT_SIGNALLED | STMT_ERROR_OCCURRED);
    result.value = s;
    return &result;
}

VMFnResult* VMStringResultFunction::successResult(const String& s) {
    result.flags = STMT_SUCCESS;
    result.value = s;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  message()

bool CoreVMFunction_message::acceptsArgType(int iArg, ExprType_t type) const {
    return type == INT_EXPR || type == STRING_EXPR;
}

VMFnResult* CoreVMFunction_message::exec(VMFnArgs* args) {
    if (!args->argsCount()) return errorResult();

    uint64_t usecs = RTMath::unsafeMicroSeconds(RTMath::real_clock);

    VMStringExpr* strExpr = dynamic_cast<VMStringExpr*>(args->arg(0));
    if (strExpr) {
        printf("[ScriptVM %.3f] %s\n", usecs/1000000.f, strExpr->evalStr().c_str());
        return successResult();
    }

    VMIntExpr* intExpr = dynamic_cast<VMIntExpr*>(args->arg(0));
    if (intExpr) {
        printf("[ScriptVM %.3f] %d\n", usecs/1000000.f, intExpr->evalInt());
        return successResult();
    }

    return errorResult();
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  exit()

VMFnResult* CoreVMFunction_exit::exec(VMFnArgs* args) {
    this->result.flags = STMT_ABORT_SIGNALLED;
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  wait()

VMFnResult* CoreVMFunction_wait::exec(VMFnArgs* args) {
    ExecContext* ctx = dynamic_cast<ExecContext*>(vm->currentVMExecContext());
    VMIntExpr* expr = dynamic_cast<VMIntExpr*>(args->arg(0));
    int us = expr->evalInt();
    if (us < 0) {
        wrnMsg("wait(): argument may not be negative! Aborting script!");
        this->result.flags = STMT_ABORT_SIGNALLED;
    } else if (us == 0) {
        wrnMsg("wait(): argument may not be zero! Aborting script!");
        this->result.flags = STMT_ABORT_SIGNALLED;
    } else {
        ctx->suspendMicroseconds = us;
        this->result.flags = STMT_SUSPEND_SIGNALLED;
    }
    return &result;
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  abs()

bool CoreVMFunction_abs::acceptsArgType(int iArg, ExprType_t type) const {
    return type == INT_EXPR;
}

VMFnResult* CoreVMFunction_abs::exec(VMFnArgs* args) {
    return successResult( ::abs(args->arg(0)->asInt()->evalInt()) );
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  random()

bool CoreVMFunction_random::acceptsArgType(int iArg, ExprType_t type) const {
    return type == INT_EXPR;
}

VMFnResult* CoreVMFunction_random::exec(VMFnArgs* args) {
    int iMin = args->arg(0)->asInt()->evalInt();
    int iMax = args->arg(1)->asInt()->evalInt();
    float f = float(::rand()) / float(RAND_MAX);
    return successResult(
        iMin + roundf( f * float(iMax - iMin) )
    );
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  num_elements()

bool CoreVMFunction_num_elements::acceptsArgType(int iArg, ExprType_t type) const {
    return type == INT_ARR_EXPR;
}

VMFnResult* CoreVMFunction_num_elements::exec(VMFnArgs* args) {
    return successResult( args->arg(0)->asIntArray()->arraySize() );
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  inc()

VMFnResult* CoreVMFunction_inc::exec(VMFnArgs* args) {
    VMExpr* arg = args->arg(0);
    VMIntExpr* in = dynamic_cast<VMIntExpr*>(arg);
    VMVariable* out = dynamic_cast<VMVariable*>(arg);
    if (!in || !out) successResult(0);
    int i = in->evalInt() + 1;
    IntLiteral tmp(i);
    out->assignExpr(&tmp);
    return successResult(i);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  dec()

VMFnResult* CoreVMFunction_dec::exec(VMFnArgs* args) {
    VMExpr* arg = args->arg(0);
    VMIntExpr* in = dynamic_cast<VMIntExpr*>(arg);
    VMVariable* out = dynamic_cast<VMVariable*>(arg);
    if (!in || !out) successResult(0);
    int i = in->evalInt() - 1;
    IntLiteral tmp(i);
    out->assignExpr(&tmp);
    return successResult(i);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  in_range()

VMFnResult* CoreVMFunction_in_range::exec(VMFnArgs* args) {
    int i  = args->arg(0)->asInt()->evalInt();
    int lo = args->arg(1)->asInt()->evalInt();
    int hi = args->arg(2)->asInt()->evalInt();
    if (lo > hi) { // swap lo and hi
        int tmp = lo;
        lo = hi;
        hi = tmp;
    }
    return successResult(i >= lo && i <= hi);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sh_left()

VMFnResult* CoreVMFunction_sh_left::exec(VMFnArgs* args) {
    int i = args->arg(0)->asInt()->evalInt();
    int n = args->arg(1)->asInt()->evalInt();
    return successResult(i << n);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sh_right()

VMFnResult* CoreVMFunction_sh_right::exec(VMFnArgs* args) {
    int i = args->arg(0)->asInt()->evalInt();
    int n = args->arg(1)->asInt()->evalInt();
    return successResult(i >> n);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  min()

VMFnResult* CoreVMFunction_min::exec(VMFnArgs* args) {
    int l = args->arg(0)->asInt()->evalInt();
    int r = args->arg(1)->asInt()->evalInt();
    return successResult(l < r ? l : r);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  max()

VMFnResult* CoreVMFunction_max::exec(VMFnArgs* args) {
    int l = args->arg(0)->asInt()->evalInt();
    int r = args->arg(1)->asInt()->evalInt();
    return successResult(l > r ? l : r);
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  array_equal()

VMFnResult* CoreVMFunction_array_equal::exec(VMFnArgs* args) {
    VMIntArrayExpr* l = args->arg(0)->asIntArray();
    VMIntArrayExpr* r = args->arg(1)->asIntArray();
    if (l->arraySize() != r->arraySize()) {
        wrnMsg("array_equal(): the two arrays differ in size");
        return successResult(0); // false
    }
    const int n = l->arraySize();
    for (int i = 0; i < n; ++i)
        if (l->evalIntElement(i) != r->evalIntElement(i))
            return successResult(0); // false
    return successResult(1); // true
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  search()

ExprType_t CoreVMFunction_search::argType(int iArg) const {
    return (iArg == 0) ? INT_ARR_EXPR : INT_EXPR;
}

bool CoreVMFunction_search::acceptsArgType(int iArg, ExprType_t type) const {
    if (iArg == 0)
        return type == INT_ARR_EXPR;
    else
        return type == INT_EXPR;
}

VMFnResult* CoreVMFunction_search::exec(VMFnArgs* args) {
    VMIntArrayExpr* a = args->arg(0)->asIntArray();
    const int needle = args->arg(1)->asInt()->evalInt();
    const int n = a->arraySize();
    for (int i = 0; i < n; ++i)
        if (a->evalIntElement(i) == needle)
            return successResult(i);
    return successResult(-1); // not found
}

///////////////////////////////////////////////////////////////////////////
// built-in script function:  sort()

ExprType_t CoreVMFunction_sort::argType(int iArg) const {
    return (iArg == 0) ? INT_ARR_EXPR : INT_EXPR;
}

bool CoreVMFunction_sort::acceptsArgType(int iArg, ExprType_t type) const {
    if (iArg == 0)
        return type == INT_ARR_EXPR;
    else
        return type == INT_EXPR;
}

struct ArrElemPOD {
    VMIntArrayExpr* m_array;
    int m_index;
};

static inline void swap(class ArrElemRef a, class ArrElemRef b);

class ArrElemRef : protected ArrElemPOD {
public:
    ArrElemRef() {
        m_array = NULL;
        m_index = 0;
    }
    ArrElemRef(VMIntArrayExpr* a, int index) {
        m_array = a;
        m_index = index;
    }
    inline ArrElemRef& operator=(const ArrElemRef& e) {
        setValue(e.getValue());
        return *this;
    }
    inline ArrElemRef& operator=(int val) {
        setValue(val);
        return *this;
    }
    inline bool operator==(const ArrElemRef& e) const {
        if (m_index == e.m_index)
            return true;
        return getValue() == e.getValue();
    }
    inline bool operator==(int val) const {
        return getValue() == val;
    }
    inline bool operator!=(const ArrElemRef& e) const {
        return !(operator==(e));
    }
    inline bool operator!=(int val) const {
        return !(operator==(val));
    }
    inline bool operator<(const ArrElemRef& e) const {
        if (m_index == e.m_index)
            return false;
        return getValue() < e.getValue();
    }
    inline bool operator<(int val) const {
        return getValue() < val;
    }
    inline bool operator>(const ArrElemRef& e) const {
        if (m_index == e.m_index)
            return false;
        return getValue() > e.getValue();
    }
    inline bool operator>(int val) const {
        return getValue() > val;
    }
    inline bool operator<=(const ArrElemRef& e) const {
        if (m_index == e.m_index)
            return true;
        return getValue() <= e.getValue();
    }
    inline bool operator<=(int val) const {
        return getValue() <= val;
    }
    inline bool operator>=(const ArrElemRef& e) const {
        if (m_index == e.m_index)
            return true;
        return getValue() >= e.getValue();
    }
    inline bool operator>=(int val) const {
        return getValue() >= val;
    }
    inline operator int() const {
        return getValue();
    }
protected:
    inline int getValue() const {
        return m_array->evalIntElement(m_index);
    }
    inline void setValue(int value) {
        m_array->assignIntElement(m_index, value);
    }

    friend void swap(class ArrElemRef a, class ArrElemRef b);
};

class ArrElemPtr : protected ArrElemPOD {
public:
    ArrElemPtr() {
        m_array = NULL;
        m_index = 0;
    }
    ArrElemPtr(VMIntArrayExpr* a, int index) {
        m_array = a;
        m_index = index;
    }
    inline ArrElemRef operator*() {
        return *(ArrElemRef*)this;
    }
};

static inline void swap(ArrElemRef a, ArrElemRef b) {
    int valueA = a.getValue();
    int valueB = b.getValue();
    a.setValue(valueB);
    b.setValue(valueA);
}

class ArrExprIter : public ArrElemPOD {
public:
    typedef std::random_access_iterator_tag iterator_category;
    typedef int value_type;
    typedef ssize_t difference_type;
    typedef ArrElemPtr pointer;
    typedef ArrElemRef reference;

    ArrExprIter(VMIntArrayExpr* a, int index) {
        m_array = a;
        m_index = index;
    }
    inline ArrElemRef operator*() {
        return *(ArrElemRef*)this;
    }
    inline ArrExprIter& operator++() { // prefix increment
        ++m_index;
        return *this;
    }
    inline ArrExprIter& operator--() { // prefix decrement
        --m_index;
        return *this;
    }
    inline ArrExprIter operator++(int) { // postfix increment
        ArrExprIter it = *this;
        ++m_index;
        return it;
    }
    inline ArrExprIter operator--(int) { // postfix decrement
        ArrExprIter it = *this;
        --m_index;
        return it;
    }
    inline ArrExprIter& operator+=(difference_type d) {
        m_index += d;
        return *this;
    }
    inline ArrExprIter& operator-=(difference_type d) {
        m_index -= d;
        return *this;
    }
    inline bool operator==(const ArrExprIter& other) const {
        return m_index == other.m_index;
    }
    inline bool operator!=(const ArrExprIter& other) const {
        return m_index != other.m_index;
    }
    inline bool operator<(const ArrExprIter& other) const {
        return m_index < other.m_index;
    }
    inline bool operator>(const ArrExprIter& other) const {
        return m_index > other.m_index;
    }
    inline bool operator<=(const ArrExprIter& other) const {
        return m_index <= other.m_index;
    }
    inline bool operator>=(const ArrExprIter& other) const {
        return m_index >= other.m_index;
    }
    inline difference_type operator+(const ArrExprIter& other) const {
        return m_index + other.m_index;
    }
    inline difference_type operator-(const ArrExprIter& other) const {
        return m_index - other.m_index;
    }
    inline ArrExprIter operator-(difference_type d) const {
        return ArrExprIter(m_array, m_index - d);
    }
    inline ArrExprIter operator+(difference_type d) const {
        return ArrExprIter(m_array, m_index + d);
    }
    inline ArrExprIter operator*(difference_type factor) const {
        return ArrExprIter(m_array, m_index * factor);
    }
};

struct DescArrExprSorter {
    inline bool operator()(const int& a, const int& b) const {
        return a > b;
    }
};

VMFnResult* CoreVMFunction_sort::exec(VMFnArgs* args) {
    VMIntArrayExpr* a = args->arg(0)->asIntArray();
    bool bAscending =
        (args->argsCount() < 2) ? true : !args->arg(1)->asInt()->evalInt();
    int n = a->arraySize();
    ArrExprIter itBegin(a, 0);
    ArrExprIter itEnd(a, n);
    if (bAscending) {
        std::sort(itBegin, itEnd);
    } else {
        DescArrExprSorter sorter;
        std::sort(itBegin, itEnd, sorter);
    }
    return successResult();
}

} // namespace LinuxSampler
