/*                                                              -*- c++ -*-
 *
 * Copyright (c) 2014 - 2019 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// This header defines VM core implementation internal data types only used
// inside the parser and core VM implementation of this source directory. Not
// intended to be used in other source code parts (other source code
// directories) of the sampler.

#ifndef LS_INSTRPARSERTREE_H
#define LS_INSTRPARSERTREE_H

#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <string.h> // for memset()
#include "../common/global.h"
#include "../common/Ref.h"
#include "../common/ArrayList.h"
#include "common.h"

namespace LinuxSampler {
    
class ParserContext;
class ExecContext;

enum StmtType_t {
    STMT_LEAF,
    STMT_LIST,
    STMT_BRANCH,
    STMT_LOOP,
    STMT_SYNC,
    STMT_NOOP,
};

class Node {
public:
    Node();
    virtual ~Node();
    virtual void dump(int level = 0) = 0;
    virtual bool isPolyphonic() const = 0;
    void printIndents(int n);
};
typedef Ref<Node> NodeRef;

class Expression : virtual public VMExpr, virtual public Node {
public:
    virtual ExprType_t exprType() const = 0;
    virtual bool isConstExpr() const = 0;
    virtual String evalCastToStr() = 0;
};
typedef Ref<Expression,Node> ExpressionRef;

class IntExpr : virtual public VMIntExpr, virtual public Expression {
public:
    ExprType_t exprType() const { return INT_EXPR; }
    virtual int evalInt() = 0;
    String evalCastToStr();
};
typedef Ref<IntExpr,Node> IntExprRef;

class IntArrayExpr : virtual public VMIntArrayExpr, virtual public Expression {
public:
    ExprType_t exprType() const { return INT_ARR_EXPR; }
    String evalCastToStr();
};
typedef Ref<IntArrayExpr,Node> IntArrayExprRef;

class StringExpr : virtual public VMStringExpr, virtual public Expression {
public:
    ExprType_t exprType() const { return STRING_EXPR; }
    virtual String evalStr() = 0;
    String evalCastToStr() { return evalStr(); }
};
typedef Ref<StringExpr,Node> StringExprRef;

class IntLiteral : virtual public IntExpr {
    int value;
public:
    IntLiteral(int value) : value(value) { }
    int evalInt();
    void dump(int level = 0);
    bool isConstExpr() const { return true; }
    bool isPolyphonic() const { return false; }
};
typedef Ref<IntLiteral,Node> IntLiteralRef;

class StringLiteral : virtual public StringExpr {
public:
    String value;
    StringLiteral(const String& value) : value(value) { }
    bool isConstExpr() const { return true; }
    void dump(int level = 0);
    String evalStr() { return value; }
    bool isPolyphonic() const { return false; }
};
typedef Ref<StringLiteral,Node> StringLiteralRef;

class Args : virtual public VMFnArgs, virtual public Node {
public:
    std::vector<ExpressionRef> args;
    void add(ExpressionRef arg) { args.push_back(arg); }
    void dump(int level = 0);
    int argsCount() const { return (int) args.size(); }
    VMExpr* arg(int i) { return (i >= 0 && i < argsCount()) ? &*args.at(i) : NULL; }
    bool isPolyphonic() const;
};
typedef Ref<Args,Node> ArgsRef;

class Variable : virtual public VMVariable, virtual public Expression {
public:
    bool isConstExpr() const OVERRIDE { return bConst; }
    bool isAssignable() const OVERRIDE { return !bConst; }
    virtual void assign(Expression* expr) = 0;
    void assignExpr(VMExpr* expr) OVERRIDE { Expression* e = dynamic_cast<Expression*>(expr); if (e) assign(e); }
protected:
    Variable(ParserContext* ctx, int _memPos, bool _bConst)
        : context(ctx), memPos(_memPos), bConst(_bConst) {}

    ParserContext* context;
    int memPos;
    bool bConst;
};
typedef Ref<Variable,Node> VariableRef;

class IntVariable : public Variable, virtual public IntExpr {
    bool polyphonic;
public:
    IntVariable(ParserContext* ctx);
    void assign(Expression* expr);
    int evalInt();
    void dump(int level = 0);
    bool isPolyphonic() const { return polyphonic; }
protected:
    IntVariable(ParserContext* ctx, bool polyphonic, bool bConst, int size = 1);
};
typedef Ref<IntVariable,Node> IntVariableRef;

class ConstIntVariable : public IntVariable {
public:
    int value;

    ConstIntVariable(int value);
    //ConstIntVariable(ParserContext* ctx, int value = 0);
    void assign(Expression* expr);
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<ConstIntVariable,Node> ConstIntVariableRef;

class BuiltInIntVariable : public IntVariable {
    String name;
    VMIntRelPtr* ptr;
public:
    BuiltInIntVariable(const String& name, VMIntRelPtr* ptr);
    bool isAssignable() const OVERRIDE { return !ptr->readonly; }
    void assign(Expression* expr) OVERRIDE;
    int evalInt() OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<BuiltInIntVariable,Node> BuiltInIntVariableRef;

class PolyphonicIntVariable : public IntVariable {
public:
    PolyphonicIntVariable(ParserContext* ctx);
    void dump(int level = 0);
};
typedef Ref<PolyphonicIntVariable,Node> PolyphonicIntVariableRef;

class IntArrayVariable : public Variable, virtual public IntArrayExpr {
    ArrayList<int> values;
public:
    IntArrayVariable(ParserContext* ctx, int size);
    IntArrayVariable(ParserContext* ctx, int size, ArgsRef values, bool _bConst = false);
    void assign(Expression* expr) {} // ignore scalar assignment
    String evalCastToStr() { return ""; } // ignore scalar cast to string
    ExprType_t exprType() const { return INT_ARR_EXPR; }
    virtual int arraySize() const { return values.size(); }
    virtual int evalIntElement(uint i);
    virtual void assignIntElement(uint i, int value);
    void dump(int level = 0);
    bool isPolyphonic() const { return false; }
protected:
    IntArrayVariable(ParserContext* ctx, bool bConst);
};
typedef Ref<IntArrayVariable,Node> IntArrayVariableRef;

class BuiltInIntArrayVariable : public IntArrayVariable {
    String name;
    VMInt8Array* array;
public:
    BuiltInIntArrayVariable(const String& name, VMInt8Array* array);
    int arraySize() const OVERRIDE { return array->size; }
    int evalIntElement(uint i) OVERRIDE;
    bool isAssignable() const OVERRIDE { return !array->readonly; }
    void assignIntElement(uint i, int value) OVERRIDE;
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<BuiltInIntArrayVariable,Node> BuiltInIntArrayVariableRef;

class IntArrayElement : public IntVariable {
    IntArrayExprRef array;
    IntExprRef index;
public:
    IntArrayElement(IntArrayExprRef array, IntExprRef arrayIndex);
    void assign(Expression* expr);
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<IntArrayElement,Node> IntArrayElementRef;

class StringVariable : public Variable, virtual public StringExpr {
public:
    StringVariable(ParserContext* ctx);
    void assign(Expression* expr);
    String evalStr();
    void dump(int level = 0);
    bool isPolyphonic() const { return false; }
protected:
    StringVariable(ParserContext* ctx, bool bConst);
};
typedef Ref<StringVariable,Node> StringVariableRef;

class ConstStringVariable : public StringVariable {
public:
    String value;

    ConstStringVariable(ParserContext* ctx, String value = "");
    void assign(Expression* expr);
    String evalStr();
    void dump(int level = 0);
};
typedef Ref<ConstStringVariable,Node> ConstStringVariableRef;

class BinaryOp : virtual public Expression {
protected:
    ExpressionRef lhs;
    ExpressionRef rhs;
public:
    BinaryOp(ExpressionRef lhs, ExpressionRef rhs) : lhs(lhs), rhs(rhs) { }
    bool isConstExpr() const { return lhs->isConstExpr() && rhs->isConstExpr(); }
    bool isPolyphonic() const { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
};
typedef Ref<BinaryOp,Node> BinaryOpRef;

class Add : virtual public BinaryOp, virtual public IntExpr {
public:
    Add(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs, rhs) { }
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Add,Node> AddRef;

class Sub : virtual public BinaryOp, virtual public IntExpr {
public:
    Sub(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs, rhs) { }
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Sub,Node> SubRef;

class Mul : virtual public BinaryOp, virtual public IntExpr {
public:
    Mul(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs, rhs) { }
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Mul,Node> MulRef;

class Div : virtual public BinaryOp, virtual public IntExpr {
public:
    Div(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs, rhs) { }
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Div,Node> DivRef;

class Mod : virtual public BinaryOp, virtual public IntExpr {
public:
    Mod(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs, rhs) { }
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Mod,Node> ModRef;

class Statement : virtual public Node {
public:
    virtual StmtType_t statementType() const = 0;
};
typedef Ref<Statement,Node> StatementRef;

// Just used by parser to avoid "not a statement" parser warning, will be
// filtered out by parser. So it will not be part of the VM tree after parsing.
class NoOperation : public Statement {
public:
    NoOperation() : Statement() {}
    StmtType_t statementType() const { return STMT_NOOP; }
    void dump(int level = 0) {}
    bool isPolyphonic() const { return false; }
};
typedef Ref<NoOperation,Node> NoOperationRef;

bool isNoOperation(StatementRef statement);

class LeafStatement : public Statement {
public:
    virtual StmtFlags_t exec() = 0;
    virtual StmtType_t statementType() const { return STMT_LEAF; }
};
typedef Ref<LeafStatement,Node> LeafStatementRef;

class Statements : public Statement {
    std::vector<StatementRef> args;
public:
    void add(StatementRef arg) { args.push_back(arg); }
    void dump(int level = 0);
    StmtType_t statementType() const { return STMT_LIST; }
    virtual Statement* statement(uint i);
    bool isPolyphonic() const;
};
typedef Ref<Statements,Node> StatementsRef;

class BranchStatement : public Statement {
public:
    StmtType_t statementType() const { return STMT_BRANCH; }
    virtual int evalBranch() = 0;
    virtual Statements* branch(uint i) const = 0;
};

class DynamicVariableCall : public Variable, virtual public IntExpr, virtual public StringExpr, virtual public IntArrayExpr {
    VMDynVar* dynVar;
    String varName;
public:
    DynamicVariableCall(const String& name, ParserContext* ctx, VMDynVar* v);
    ExprType_t exprType() const OVERRIDE { return dynVar->exprType(); }
    bool isConstExpr() const OVERRIDE { return dynVar->isConstExpr(); }
    bool isAssignable() const OVERRIDE { return dynVar->isAssignable(); }
    bool isPolyphonic() const OVERRIDE { return false; }
    void assign(Expression* expr) OVERRIDE { dynVar->assignExpr(expr); }
    VMIntArrayExpr* asIntArray() const OVERRIDE { return dynVar->asIntArray(); }
    int evalInt() OVERRIDE;
    String evalStr() OVERRIDE;
    String evalCastToStr() OVERRIDE;
    int arraySize() const OVERRIDE { return dynVar->asIntArray()->arraySize(); }
    int evalIntElement(uint i) OVERRIDE { return dynVar->asIntArray()->evalIntElement(i); }
    void assignIntElement(uint i, int value) OVERRIDE { return dynVar->asIntArray()->assignIntElement(i, value); }
    void dump(int level = 0) OVERRIDE;
};
typedef Ref<DynamicVariableCall,Node> DynamicVariableCallRef;

class FunctionCall : virtual public LeafStatement, virtual public IntExpr, virtual public StringExpr {
    String functionName;
    ArgsRef args;
    VMFunction* fn;
public:
    FunctionCall(const char* function, ArgsRef args, VMFunction* fn) :
        functionName(function), args(args), fn(fn) { }
    void dump(int level = 0) OVERRIDE;
    StmtFlags_t exec() OVERRIDE;
    int evalInt() OVERRIDE;
    VMIntArrayExpr* asIntArray() const OVERRIDE;
    String evalStr() OVERRIDE;
    bool isConstExpr() const OVERRIDE { return false; }
    ExprType_t exprType() const OVERRIDE;
    String evalCastToStr() OVERRIDE;
    bool isPolyphonic() const OVERRIDE { return args->isPolyphonic(); }
protected:
    VMFnResult* execVMFn();
};
typedef Ref<FunctionCall,Node> FunctionCallRef;

class NoFunctionCall : public FunctionCall {
public:
    NoFunctionCall() : FunctionCall("nothing", new Args, NULL) {}
    StmtType_t statementType() const { return STMT_NOOP; }
};
typedef Ref<NoFunctionCall,Node> NoFunctionCallRef;

class EventHandler : virtual public Statements, virtual public VMEventHandler {
    StatementsRef statements;
    bool usingPolyphonics;
public:
    void dump(int level = 0);
    StmtFlags_t exec();
    EventHandler(StatementsRef statements);
    Statement* statement(uint i) { return statements->statement(i); }
    bool isPolyphonic() const { return usingPolyphonics; }
};
typedef Ref<EventHandler,Node> EventHandlerRef;

class OnNote : public EventHandler {
public:
    OnNote(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const { return VM_EVENT_HANDLER_NOTE; }
    String eventHandlerName() const { return "note"; }
};
typedef Ref<OnNote,Node> OnNoteRef;

class OnInit : public EventHandler {
public:
    OnInit(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const { return VM_EVENT_HANDLER_INIT; }
    String eventHandlerName() const { return "init"; }
};
typedef Ref<OnInit,Node> OnInitRef;

class OnRelease : public EventHandler {
public:
    OnRelease(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const { return VM_EVENT_HANDLER_RELEASE; }
    String eventHandlerName() const { return "release"; }
};
typedef Ref<OnRelease,Node> OnReleaseRef;

class OnController : public EventHandler {
public:
    OnController(StatementsRef statements) : EventHandler(statements) {}
    VMEventHandlerType_t eventHandlerType() const { return VM_EVENT_HANDLER_CONTROLLER; }
    String eventHandlerName() const { return "controller"; }
};
typedef Ref<OnController,Node> OnControllerRef;

class EventHandlers : virtual public Node {
    std::vector<EventHandlerRef> args;
public:
    EventHandlers();
    ~EventHandlers();
    void add(EventHandlerRef arg);
    void dump(int level = 0);
    int evalInt() { return 0; }
    EventHandler* eventHandlerByName(const String& name) const;
    EventHandler* eventHandler(uint index) const;
    inline uint size() const { return (int) args.size(); }
    bool isPolyphonic() const;
};
typedef Ref<EventHandlers,Node> EventHandlersRef;

class Assignment : public LeafStatement {
protected:
    VariableRef variable;
    ExpressionRef value;
public:
    Assignment(VariableRef variable, ExpressionRef value);
    void dump(int level = 0);
    StmtFlags_t exec();
    bool isPolyphonic() const { return (variable && variable->isPolyphonic()) || (value && value->isPolyphonic()); }
};
typedef Ref<Assignment,Node> AssignmentRef;

class If : public BranchStatement {
    IntExprRef condition;
    StatementsRef ifStatements;
    StatementsRef elseStatements;
public:
    If(IntExprRef condition, StatementsRef ifStatements, StatementsRef elseStatements) :
        condition(condition), ifStatements(ifStatements), elseStatements(elseStatements) { }
    If(IntExprRef condition, StatementsRef statements) :
        condition(condition), ifStatements(statements) { }
    void dump(int level = 0);
    int evalBranch();
    Statements* branch(uint i) const;
    bool isPolyphonic() const;
};
typedef Ref<If,Node> IfRef;

struct CaseBranch {
    IntExprRef from;
    IntExprRef to;
    StatementsRef statements;
};

typedef std::vector<CaseBranch> CaseBranches;

class SelectCase : public BranchStatement {
    IntExprRef select;
    CaseBranches branches;
public:
    SelectCase(IntExprRef select, const CaseBranches& branches) : select(select), branches(branches) { }
    void dump(int level = 0);
    int evalBranch();
    Statements* branch(uint i) const;
    //void addBranch(IntExprRef condition, StatementsRef statements);
    //void addBranch(IntExprRef from, IntExprRef to, StatementsRef statements);
    //void addBranch(CaseBranchRef branch);
    //void addBranches(CaseBranchesRef branches);
    bool isPolyphonic() const;
};
typedef Ref<SelectCase,Node> SelectCaseRef;

class While : public Statement {
    IntExprRef m_condition;
    StatementsRef m_statements;
public:
    While(IntExprRef condition, StatementsRef statements) :
        m_condition(condition), m_statements(statements) {}
    StmtType_t statementType() const { return STMT_LOOP; }
    void dump(int level = 0);
    bool evalLoopStartCondition();
    Statements* statements() const;
    bool isPolyphonic() const { return m_condition->isPolyphonic() || m_statements->isPolyphonic(); }
};

class SyncBlock : public Statement {
    StatementsRef m_statements;
public:
    SyncBlock(StatementsRef statements) : m_statements(statements) {}
    StmtType_t statementType() const { return STMT_SYNC; }
    void dump(int level = 0);
    Statements* statements() const;
    bool isPolyphonic() const { return m_statements->isPolyphonic(); }
};
typedef Ref<SyncBlock,Node> SyncBlockRef;

class Neg : public IntExpr {
    IntExprRef expr;
public:
    Neg(IntExprRef expr) : expr(expr) { }
    int evalInt() { return (expr) ? -expr->evalInt() : 0; }
    void dump(int level = 0);
    bool isConstExpr() const { return expr->isConstExpr(); }
    bool isPolyphonic() const { return expr->isPolyphonic(); }
};
typedef Ref<Neg,Node> NegRef;

class ConcatString : public StringExpr {
    ExpressionRef lhs;
    ExpressionRef rhs;
public:
    ConcatString(ExpressionRef lhs, ExpressionRef rhs) : lhs(lhs), rhs(rhs) {}
    String evalStr();
    void dump(int level = 0);
    bool isConstExpr() const;
    bool isPolyphonic() const { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
};
typedef Ref<ConcatString,Node> ConcatStringRef;

class Relation : public IntExpr {
public:
    enum Type {
        LESS_THAN,
        GREATER_THAN,
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL,
        EQUAL,
        NOT_EQUAL
    };
    Relation(IntExprRef lhs, Type type, IntExprRef rhs) :
        lhs(lhs), rhs(rhs), type(type) {}
    int evalInt();
    void dump(int level = 0);
    bool isConstExpr() const;
    bool isPolyphonic() const { return lhs->isPolyphonic() || rhs->isPolyphonic(); }
private:
    IntExprRef lhs;
    IntExprRef rhs;
    Type type;
};
typedef Ref<Relation,Node> RelationRef;

class Or : virtual public BinaryOp, virtual public IntExpr {
public:
    Or(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs,rhs) {}
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<Or,Node> OrRef;

class BitwiseOr : virtual public BinaryOp, virtual public IntExpr {
public:
    BitwiseOr(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs,rhs) {}
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<BitwiseOr,Node> BitwiseOrRef;

class And : virtual public BinaryOp, virtual public IntExpr {
public:
    And(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs,rhs) {}
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<And,Node> AndRef;

class BitwiseAnd : virtual public BinaryOp, virtual public IntExpr {
public:
    BitwiseAnd(IntExprRef lhs, IntExprRef rhs) : BinaryOp(lhs,rhs) {}
    int evalInt();
    void dump(int level = 0);
};
typedef Ref<BitwiseAnd,Node> BitwiseAndRef;

class Not : virtual public IntExpr {
    IntExprRef expr;
public:
    Not(IntExprRef expr) : expr(expr) {}
    int evalInt() { return !expr->evalInt(); }
    void dump(int level = 0);
    bool isConstExpr() const { return expr->isConstExpr(); }
    bool isPolyphonic() const { return expr->isPolyphonic(); }
};
typedef Ref<Not,Node> NotRef;

class BitwiseNot : virtual public IntExpr {
    IntExprRef expr;
public:
    BitwiseNot(IntExprRef expr) : expr(expr) {}
    int evalInt() { return ~expr->evalInt(); }
    void dump(int level = 0);
    bool isConstExpr() const { return expr->isConstExpr(); }
    bool isPolyphonic() const { return expr->isPolyphonic(); }
};
typedef Ref<BitwiseNot,Node> BitwiseNotRef;

class ParserContext : public VMParserContext {
public:
    struct Error {
        String txt;
        int line;
    };
    typedef Error Warning;

    void* scanner;
    std::istream* is;
    std::vector<ParserIssue> vErrors;
    std::vector<ParserIssue> vWarnings;
    std::vector<ParserIssue> vIssues;
    std::vector<CodeBlock>   vPreprocessorComments;

    std::set<String> builtinPreprocessorConditions;
    std::set<String> userPreprocessorConditions;

    std::map<String,VariableRef> vartable;
    std::map<String,StatementsRef> userFnTable;
    int globalIntVarCount;
    int globalStrVarCount;
    int polyphonicIntVarCount;

    EventHandlersRef handlers;

    OnInitRef onInit;
    OnNoteRef onNote;
    OnReleaseRef onRelease;
    OnControllerRef onController;

    ArrayList<int>* globalIntMemory;
    ArrayList<String>* globalStrMemory;
    int requiredMaxStackSize;

    VMFunctionProvider* functionProvider;

    ExecContext* execContext;

    ParserContext(VMFunctionProvider* parent) :
        scanner(NULL), is(NULL),
        globalIntVarCount(0), globalStrVarCount(0), polyphonicIntVarCount(0),
        globalIntMemory(NULL), globalStrMemory(NULL), requiredMaxStackSize(-1),
        functionProvider(parent), execContext(NULL)
    {
    }
    virtual ~ParserContext();
    VariableRef globalVar(const String& name);
    IntVariableRef globalIntVar(const String& name);
    StringVariableRef globalStrVar(const String& name);
    VariableRef variableByName(const String& name);
    StatementsRef userFunctionByName(const String& name);
    void addErr(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt);
    void addWrn(int firstLine, int lastLine, int firstColumn, int lastColumn, const char* txt);
    void addPreprocessorComment(int firstLine, int lastLine, int firstColumn, int lastColumn);
    void createScanner(std::istream* is);
    void destroyScanner();
    bool setPreprocessorCondition(const char* name);
    bool resetPreprocessorCondition(const char* name);
    bool isPreprocessorConditionSet(const char* name);
    std::vector<ParserIssue> issues() const OVERRIDE;
    std::vector<ParserIssue> errors() const OVERRIDE;
    std::vector<ParserIssue> warnings() const OVERRIDE;
    std::vector<CodeBlock> preprocessorComments() const OVERRIDE;
    VMEventHandler* eventHandler(uint index) OVERRIDE;
    VMEventHandler* eventHandlerByName(const String& name) OVERRIDE;
    void registerBuiltInConstIntVariables(const std::map<String,int>& vars);
    void registerBuiltInIntVariables(const std::map<String,VMIntRelPtr*>& vars);
    void registerBuiltInIntArrayVariables(const std::map<String,VMInt8Array*>& vars);
    void registerBuiltInDynVariables(const std::map<String,VMDynVar*>& vars);
};

class ExecContext : public VMExecContext {
public:
    struct StackFrame {
        Statement* statement;
        int subindex;

        StackFrame() {
            statement = NULL;
            subindex  = -1;
        }
    };

    ArrayList<int> polyphonicIntMemory;
    VMExecStatus_t status;
    StmtFlags_t flags;
    ArrayList<StackFrame> stack;
    int stackFrame;
    int suspendMicroseconds;
    size_t instructionsCount;

    ExecContext() :
        status(VM_EXEC_NOT_RUNNING), flags(STMT_SUCCESS), stackFrame(-1),
        suspendMicroseconds(0), instructionsCount(0) {}

    virtual ~ExecContext() {}

    inline void pushStack(Statement* stmt) {
        stackFrame++;
        //printf("pushStack() -> %d\n", stackFrame);
        if (stackFrame >= stack.size()) return;
        stack[stackFrame].statement = stmt;
        stack[stackFrame].subindex  = 0;
    }

    inline void popStack() {
        stack[stackFrame].statement = NULL;
        stack[stackFrame].subindex  = -1;
        stackFrame--;
        //printf("popStack() -> %d\n", stackFrame);
    }

    inline void reset() {
        stack[0].statement = NULL;
        stack[0].subindex  = -1;
        stackFrame = -1;
        flags = STMT_SUCCESS;
    }

    int suspensionTimeMicroseconds() const OVERRIDE {
        return suspendMicroseconds;
    }

    void resetPolyphonicData() OVERRIDE {
        if (polyphonicIntMemory.empty()) return;
        memset(&polyphonicIntMemory[0], 0, polyphonicIntMemory.size() * sizeof(int));
    }

    size_t instructionsPerformed() const OVERRIDE {
        return instructionsCount;
    }

    void signalAbort() OVERRIDE {
        flags = StmtFlags_t(flags | STMT_ABORT_SIGNALLED);
    }

    void forkTo(VMExecContext* ectx) const OVERRIDE;
};

} // namespace LinuxSampler

#endif // LS_INSTRPARSERTREE_H
