/*
 * Copyright (c) 2014-2017 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */
 
/* Parser for NKSP real-time instrument script language. */

%{
    #define YYERROR_VERBOSE 1
    #include "parser_shared.h"
    #include <string>
    #include <map>
    using namespace LinuxSampler;

    void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err);
    void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt);
    int InstrScript_tnamerr(char* yyres, const char* yystr);
    int InstrScript_lex(YYSTYPE* lvalp, YYLTYPE* llocp, void* scanner);
    #define scanner context->scanner
    #define PARSE_ERR(loc,txt)  yyerror(&loc, context, txt)
    #define PARSE_WRN(loc,txt)  InstrScript_warning(&loc, context, txt)
    #define PARSE_DROP(loc)     context->addPreprocessorComment(loc.first_line, loc.last_line, loc.first_column+1, loc.last_column+1);
    #define yytnamerr(res,str)  InstrScript_tnamerr(res, str)
%}

// generate reentrant safe parser
%pure-parser
%parse-param { LinuxSampler::ParserContext* context }
%lex-param { void* scanner }
// avoid symbol collision with other (i.e. future) auto generated (f)lex scanners
// (NOTE: "=" is deprecated here with Bison 3.x, however removing it would cause an error with Bison 2.x)
%name-prefix="InstrScript_"
%locations
%defines
%error-verbose

%token <iValue> INTEGER "integer literal"
%token <sValue> STRING "string literal"
%token <sValue> IDENTIFIER "function name"
%token <sValue> VARIABLE "variable name"
%token ON "keyword 'on'"
%token END "keyword 'end'"
%token INIT "keyword 'init'"
%token NOTE "keyword 'note'"
%token RELEASE "keyword 'release'"
%token CONTROLLER "keyword 'controller'"
%token DECLARE "keyword 'declare'"
%token ASSIGNMENT "operator ':='"
%token CONST_ "keyword 'const'"
%token POLYPHONIC "keyword 'polyphonic'"
%token WHILE "keyword 'while'"
%token SYNCHRONIZED "keyword 'synchronized'"
%token IF "keyword 'if'"
%token ELSE "keyword 'else'"
%token SELECT "keyword 'select'"
%token CASE "keyword 'case'"
%token TO "keyword 'to'"
%token OR "operator 'or'"
%token AND "operator 'and'"
%token NOT "operator 'not'"
%token BITWISE_OR "bitwise operator '.or.'"
%token BITWISE_AND "bitwise operator '.and.'"
%token BITWISE_NOT "bitwise operator '.not.'"
%token FUNCTION "keyword 'function'"
%token CALL "keyword 'call'"
%token MOD "operator 'mod'"
%token LE "operator '<='"
%token GE "operator '>='"
%token END_OF_FILE 0 "end of file"
%token UNKNOWN_CHAR "unknown character"

%type <nEventHandlers> script sections
%type <nEventHandler> section eventhandler
%type <nStatements> statements opt_statements userfunctioncall
%type <nStatement> statement assignment
%type <nFunctionCall> functioncall
%type <nArgs> args
%type <nExpression> arg expr logical_or_expr logical_and_expr bitwise_or_expr bitwise_and_expr rel_expr add_expr mul_expr unary_expr concat_expr
%type <nCaseBranch> caseclause
%type <nCaseBranches> caseclauses

%start script

%%

script:
    sections  {
        $$ = context->handlers = $1;
    }

sections:
    section  {
        $$ = new EventHandlers();
        if ($1) $$->add($1);
    }
    | sections section  {
        $$ = $1;
        if ($2) $$->add($2);
    }

section:
    function_declaration  {
        $$ = EventHandlerRef();
    }
    | eventhandler  {
        $$ = $1;
    }

eventhandler:
    ON NOTE opt_statements END ON  {
        if (context->onNote)
            PARSE_ERR(@2, "Redeclaration of 'note' event handler.");
        context->onNote = new OnNote($3);
        $$ = context->onNote;
    }
    | ON INIT opt_statements END ON  {
        if (context->onInit)
            PARSE_ERR(@2, "Redeclaration of 'init' event handler.");
        context->onInit = new OnInit($3);
        $$ = context->onInit;
    }
    | ON RELEASE opt_statements END ON  {
        if (context->onRelease)
            PARSE_ERR(@2, "Redeclaration of 'release' event handler.");
        context->onRelease = new OnRelease($3);
        $$ = context->onRelease;
    }
    | ON CONTROLLER opt_statements END ON  {
        if (context->onController)
            PARSE_ERR(@2, "Redeclaration of 'controller' event handler.");
        context->onController = new OnController($3);
        $$ = context->onController;
    }

function_declaration:
    FUNCTION IDENTIFIER opt_statements END FUNCTION  {
        const char* name = $2;
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR(@2, (String("There is already a built-in function with name '") + name + "'.").c_str());
        } else if (context->userFunctionByName(name)) {
            PARSE_ERR(@2, (String("There is already a user defined function with name '") + name + "'.").c_str());
        } else {
            context->userFnTable[name] = $3;
        }
    }

opt_statements:
    /* epsilon (empty argument) */  {
        $$ = new Statements();
    }
    | statements  {
        $$ = $1;
    }

statements:
    statement  {
        $$ = new Statements();
        if ($1) {
            if (!isNoOperation($1)) $$->add($1); // filter out NoOperation statements
        } else 
            PARSE_WRN(@1, "Not a statement.");
    }
    | statements statement  {
        $$ = $1;
        if ($2) {
            if (!isNoOperation($2)) $$->add($2); // filter out NoOperation statements
        } else
            PARSE_WRN(@2, "Not a statement.");
    }

statement:
    functioncall  {
        $$ = $1;
    }
    | userfunctioncall  {
        $$ = $1;
    }
    | DECLARE VARIABLE  {
        const char* name = $2;
        //printf("declared var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
        if (name[0] == '@') {
            context->vartable[name] = new StringVariable(context);
            $$ = new NoOperation;
        } else {
            context->vartable[name] = new IntVariable(context);
            $$ = new NoOperation;
        }
    }
    | DECLARE POLYPHONIC VARIABLE  {
        const char* name = $3;
        //printf("declared polyphonic var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR(@3, (String("Redeclaration of variable '") + name + "'.").c_str());
        if (name[0] != '$') {
            PARSE_ERR(@3, "Polyphonic variables may only be declared as integers.");
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            context->vartable[name] = new PolyphonicIntVariable(context);
            $$ = new NoOperation;
        }
    }
    | DECLARE VARIABLE ASSIGNMENT expr  {
        const char* name = $2;
        //printf("declared assign var '%s'\n", name);
        if (context->variableByName(name))
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
        if ($4->exprType() == STRING_EXPR) {
            if (name[0] == '$')
                PARSE_WRN(@2, (String("Variable '") + name + "' declared as integer, string expression assigned though.").c_str());
            StringExprRef expr = $4;
            if (expr->isConstExpr()) {
                const String s = expr->evalStr();
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, new StringLiteral(s));
            } else {
                StringVariableRef var = new StringVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, expr);
            }
        } else {
            if (name[0] == '@')
                PARSE_WRN(@2, (String("Variable '") + name + "' declared as string, integer expression assigned though.").c_str());
            IntExprRef expr = $4;
            if (expr->isConstExpr()) {
                const int i = expr->evalInt();
                IntVariableRef var = new IntVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, new IntLiteral(i));
            } else {
                IntVariableRef var = new IntVariable(context);
                context->vartable[name] = var;
                $$ = new Assignment(var, expr);
            }
        }
    }
    | DECLARE VARIABLE '[' expr ']'  {
        //printf("declare array without args\n");
        const char* name = $2;
        if (!$4->isConstExpr()) {
            PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ($4->exprType() != INT_EXPR) {
            PARSE_ERR(@4, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef expr = $4;
            int size = expr->evalInt();
            if (size <= 0) {
                PARSE_ERR(@4, (String("Array variable '") + name + "' declared with array size " + ToString(size) + ".").c_str());
                $$ = new FunctionCall("nothing", new Args, NULL); // whatever
            } else {
                context->vartable[name] = new IntArrayVariable(context, size);
                $$ = new NoOperation;
            }
        }
    }
    | DECLARE VARIABLE '[' expr ']' ASSIGNMENT '(' args ')'  {
        const char* name = $2;
        if (!$4->isConstExpr()) {
            PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ($4->exprType() != INT_EXPR) {
            PARSE_ERR(@4, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR(@2, (String("Redeclaration of variable '") + name + "'.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef sizeExpr = $4;
            ArgsRef args = $8;
            int size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR(@4, (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
                $$ = new FunctionCall("nothing", new Args, NULL); // whatever
            } else if (args->argsCount() > size) {
                PARSE_ERR(@8, (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
                $$ = new FunctionCall("nothing", new Args, NULL); // whatever           
            } else {
                bool argsOK = true;
                for (int i = 0; i < args->argsCount(); ++i) {
                    if (args->arg(i)->exprType() != INT_EXPR) {
                        PARSE_ERR(
                            @8, 
                            (String("Array variable '") + name +
                            "' declared with invalid assignment values. Assigned element " +
                            ToString(i+1) + " is not an integer expression.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                }
                if (argsOK) {
                    context->vartable[name] = new IntArrayVariable(context, size, args);
                    $$ = new NoOperation;
                } else
                    $$ = new FunctionCall("nothing", new Args, NULL); // whatever
            }
        }
    }
    | DECLARE CONST_ VARIABLE '[' expr ']' ASSIGNMENT '(' args ')'  {
        const char* name = $3;
        if (!$5->isConstExpr()) {
            PARSE_ERR(@5, (String("Array variable '") + name + "' must be declared with constant array size.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if ($5->exprType() != INT_EXPR) {
            PARSE_ERR(@5, (String("Size of array variable '") + name + "' declared with non integer expression.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else if (context->variableByName(name)) {
            PARSE_ERR(@3, (String("Redeclaration of variable '") + name + "'.").c_str());
            $$ = new FunctionCall("nothing", new Args, NULL); // whatever
        } else {
            IntExprRef sizeExpr = $5;
            ArgsRef args = $9;
            int size = sizeExpr->evalInt();
            if (size <= 0) {
                PARSE_ERR(@5, (String("Array variable '") + name + "' must be declared with positive array size.").c_str());
                $$ = new FunctionCall("nothing", new Args, NULL); // whatever
            } else if (args->argsCount() > size) {
                PARSE_ERR(@9, (String("Array variable '") + name +
                          "' was declared with size " + ToString(size) +
                          " but " + ToString(args->argsCount()) +
                          " values were assigned." ).c_str());
                $$ = new FunctionCall("nothing", new Args, NULL); // whatever           
            } else {
                bool argsOK = true;
                for (int i = 0; i < args->argsCount(); ++i) {
                    if (args->arg(i)->exprType() != INT_EXPR) {
                        PARSE_ERR(
                            @9,
                            (String("Array variable '") + name +
                            "' declared with invalid assignment values. Assigned element " +
                            ToString(i+1) + " is not an integer expression.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                    if (!args->arg(i)->isConstExpr()) {
                        PARSE_ERR(
                            @9,
                            (String("const array variable '") + name +
                            "' must be defined with const values. Assigned element " +
                            ToString(i+1) + " is not a const expression though.").c_str()
                        );
                        argsOK = false;
                        break;
                    }
                }
                if (argsOK) {
                    context->vartable[name] = new IntArrayVariable(context, size, args, true);
                    $$ = new NoOperation;
                } else
                    $$ = new FunctionCall("nothing", new Args, NULL); // whatever
            }
        }
    }
    | DECLARE CONST_ VARIABLE ASSIGNMENT expr  {
        const char* name = $3;
        if ($5->exprType() == STRING_EXPR) {
            if (name[0] == '$')
                PARSE_WRN(@5, "Variable declared as integer, string expression assigned though.");
            String s;
            StringExprRef expr = $5;
            if (expr->isConstExpr())
                s = expr->evalStr();
            else
                PARSE_ERR(@5, (String("Assignment to const string variable '") + name + "' requires const expression.").c_str());
            ConstStringVariableRef var = new ConstStringVariable(context, s);
            context->vartable[name] = var;
            //$$ = new Assignment(var, new StringLiteral(s));
            $$ = new NoOperation();
        } else {
            if (name[0] == '@')
                PARSE_WRN(@5, "Variable declared as string, integer expression assigned though.");
            int i = 0;
            IntExprRef expr = $5;
            if (expr->isConstExpr())
                i = expr->evalInt();
            else
                PARSE_ERR(@5, (String("Assignment to const integer variable '") + name + "' requires const expression.").c_str());
            ConstIntVariableRef var = new ConstIntVariable(i);
            context->vartable[name] = var;
            //$$ = new Assignment(var, new IntLiteral(i));
            $$ = new NoOperation();
        }
    }
    | assignment  {
        $$ = $1;
    }
    | WHILE '(' expr ')' opt_statements END WHILE  {
        if ($3->exprType() == INT_EXPR) {
            $$ = new While($3, $5);
        } else {
            PARSE_ERR(@3, "Condition for 'while' loops must be integer expression.");
            $$ = new While(new IntLiteral(0), $5);
        }
    }
    | SYNCHRONIZED opt_statements END SYNCHRONIZED  {
        $$ = new SyncBlock($2);
    }
    | IF '(' expr ')' opt_statements ELSE opt_statements END IF  {
        $$ = new If($3, $5, $7);
    }
    | IF '(' expr ')' opt_statements END IF  {
        $$ = new If($3, $5);
    }
    | SELECT expr caseclauses END SELECT  {
        if ($2->exprType() == INT_EXPR) {
            $$ = new SelectCase($2, $3);
        } else {
            PARSE_ERR(@2, "Statement 'select' can only by applied to integer expressions.");
            $$ = new SelectCase(new IntLiteral(0), $3);
        }
    }

caseclauses:
    caseclause  {
        $$ = CaseBranches();
        $$.push_back($1);
    }
    | caseclauses caseclause  {
        $$ = $1;
        $$.push_back($2);
    }

caseclause:
    CASE INTEGER opt_statements  {
        $$ = CaseBranch();
        $$.from = new IntLiteral($2);
        $$.statements = $3;
    }
    | CASE INTEGER TO INTEGER opt_statements  {
        $$ = CaseBranch();
        $$.from = new IntLiteral($2);
        $$.to   = new IntLiteral($4);
        $$.statements = $5;
    }

userfunctioncall:
    CALL IDENTIFIER  {
        const char* name = $2;
        StatementsRef fn = context->userFunctionByName(name);
        if (context->functionProvider->functionByName(name)) {
            PARSE_ERR(@1, (String("Keyword 'call' must only be used for user defined functions, not for any built-in function like '") + name + "'.").c_str());
            $$ = StatementsRef();
        } else if (!fn) {
            PARSE_ERR(@2, (String("No user defined function with name '") + name + "'.").c_str());
            $$ = StatementsRef();
        } else {
            $$ = fn;
        }
    }

functioncall:
    IDENTIFIER '(' args ')'  {
        const char* name = $1;
        //printf("function call of '%s' with args\n", name);
        ArgsRef args = $3;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (args->argsCount() < fn->minRequiredArgs()) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (args->argsCount() > fn->maxAllowedArgs()) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' accepts max. " + ToString(fn->maxAllowedArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            bool argsOK = true;
            for (int i = 0; i < args->argsCount(); ++i) {
                if (args->arg(i)->exprType() != fn->argType(i) && !fn->acceptsArgType(i, args->arg(i)->exprType())) {
                    PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects " + typeStr(fn->argType(i)) + " type, but type " + typeStr(args->arg(i)->exprType()) + " was given instead.").c_str());
                    argsOK = false;
                    break;
                } else if (fn->modifiesArg(i) && !args->arg(i)->isModifyable()) {
                    PARSE_ERR(@3, (String("Argument ") + ToString(i+1) + " of built-in function '" + name + "' expects an assignable variable.").c_str());
                    argsOK = false;
                    break;
                }
            }
            $$ = new FunctionCall(name, args, argsOK ? fn : NULL);
        }
    }
    | IDENTIFIER '(' ')' {
        const char* name = $1;
        //printf("function call of '%s' (with empty args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR(@3, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            $$ = new FunctionCall(name, args, fn);
        }
    }
    | IDENTIFIER  {
        const char* name = $1;
        //printf("function call of '%s' (without args)\n", name);
        ArgsRef args = new Args;
        VMFunction* fn = context->functionProvider->functionByName(name);
        if (context->userFunctionByName(name)) {
            PARSE_ERR(@1, (String("Missing 'call' keyword before user defined function name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (!fn) {
            PARSE_ERR(@1, (String("No built-in function with name '") + name + "'.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else if (context->functionProvider->isFunctionDisabled(fn,context)) {
            PARSE_DROP(@$);
            $$ = new NoFunctionCall;
        } else if (fn->minRequiredArgs() > 0) {
            PARSE_ERR(@1, (String("Built-in function '") + name + "' requires at least " + ToString(fn->minRequiredArgs()) + " arguments.").c_str());
            $$ = new FunctionCall(name, args, NULL);
        } else {
            $$ = new FunctionCall(name, args, fn);
        }
    }

args:
    arg  {
        $$ = new Args();
        $$->add($1);
    }
    | args ',' arg  {
        $$ = $1;
        $$->add($3);
    }

arg:
    expr

assignment:
    VARIABLE ASSIGNMENT expr  {
        //printf("variable lookup with name '%s' as assignment expr\n", $1);
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR(@1, (String("Variable assignment: No variable declared with name '") + name + "'.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR(@2, (String("Variable assignment: Cannot modify const variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR(@2, (String("Variable assignment: Variable '") + name + "' is not assignable.").c_str());
        else if (var->exprType() != $3->exprType())
            PARSE_ERR(@3, (String("Variable assignment: Variable '") + name + "' is of type " + typeStr(var->exprType()) + ", assignment is of type " + typeStr($3->exprType()) + " though.").c_str());
        $$ = new Assignment(var, $3);
    }
    | VARIABLE '[' expr ']' ASSIGNMENT expr  {
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var)
            PARSE_ERR(@1, (String("No variable declared with name '") + name + "'.").c_str());
        else if (var->exprType() != INT_ARR_EXPR)
            PARSE_ERR(@2, (String("Variable '") + name + "' is not an array variable.").c_str());
        else if (var->isConstExpr())
            PARSE_ERR(@5, (String("Variable assignment: Cannot modify const array variable '") + name + "'.").c_str());
        else if (!var->isAssignable())
            PARSE_ERR(@5, (String("Variable assignment: Array variable '") + name + "' is not assignable.").c_str());
        else if ($3->exprType() != INT_EXPR)
            PARSE_ERR(@3, (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
        else if ($6->exprType() != INT_EXPR)
            PARSE_ERR(@5, (String("Value assigned to array variable '") + name + "' must be an integer expression.").c_str());
        else if ($3->isConstExpr() && $3->asInt()->evalInt() >= ((IntArrayVariableRef)var)->arraySize())
            PARSE_WRN(@3, (String("Index ") + ToString($3->asInt()->evalInt()) +
                          " exceeds size of array variable '" + name +
                          "' which was declared with size " +
                          ToString(((IntArrayVariableRef)var)->arraySize()) + ".").c_str());
        IntArrayElementRef element = new IntArrayElement(var, $3);
        $$ = new Assignment(element, $6);
    }

unary_expr:
    INTEGER  {
        $$ = new IntLiteral($1);
    }
    | STRING    {
        $$ = new StringLiteral($1);
    }
    | VARIABLE  {
        //printf("variable lookup with name '%s' as unary expr\n", $1);
        VariableRef var = context->variableByName($1);
        if (var)
            $$ = var;
        else {
            PARSE_ERR(@1, (String("No variable declared with name '") + $1 + "'.").c_str());
            $$ = new IntLiteral(0);
        }
    }
    | VARIABLE '[' expr ']'  {
        const char* name = $1;
        VariableRef var = context->variableByName(name);
        if (!var) {
            PARSE_ERR(@1, (String("No variable declared with name '") + name + "'.").c_str());
            $$ = new IntLiteral(0);
        } else if (var->exprType() != INT_ARR_EXPR) {
            PARSE_ERR(@2, (String("Variable '") + name + "' is not an array variable.").c_str());
            $$ = new IntLiteral(0);
        } else if ($3->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Array variable '") + name + "' accessed with non integer expression.").c_str());
            $$ = new IntLiteral(0);
        } else {
            if ($3->isConstExpr() && $3->asInt()->evalInt() >= ((IntArrayVariableRef)var)->arraySize())
                PARSE_WRN(@3, (String("Index ") + ToString($3->asInt()->evalInt()) +
                               " exceeds size of array variable '" + name +
                               "' which was declared with size " +
                               ToString(((IntArrayVariableRef)var)->arraySize()) + ".").c_str());
            $$ = new IntArrayElement(var, $3);
        }
    }
    | '(' expr ')'  {
        $$ = $2;
    }
    | functioncall  {
        $$ = $1;
    }
    | '-' unary_expr  {
        $$ = new Neg($2);
    }
    | BITWISE_NOT unary_expr  {
        if ($2->exprType() != INT_EXPR) {
            PARSE_ERR(@2, (String("Right operand of bitwise operator '.not.' must be an integer expression, is ") + typeStr($2->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new BitwiseNot($2);
        }
    }
    | NOT unary_expr  {
        if ($2->exprType() != INT_EXPR) {
            PARSE_ERR(@2, (String("Right operand of operator 'not' must be an integer expression, is ") + typeStr($2->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Not($2);
        }
    }

expr:
    concat_expr

concat_expr:
    logical_or_expr
    | concat_expr '&' logical_or_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->isConstExpr() && rhs->isConstExpr()) {
            $$ = new StringLiteral(
                lhs->evalCastToStr() + rhs->evalCastToStr()
            );
        } else {
            $$ = new ConcatString(lhs, rhs);
        }
    }

logical_or_expr:
    logical_and_expr
    | logical_or_expr OR logical_and_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator 'or' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator 'or' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Or(lhs, rhs);
        }
    }

logical_and_expr:
    bitwise_or_expr  {
        $$ = $1;
    }
    | logical_and_expr AND bitwise_or_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator 'and' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator 'and' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new And(lhs, rhs);
        }
    }

bitwise_or_expr:
    bitwise_and_expr
    | bitwise_or_expr BITWISE_OR bitwise_and_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of bitwise operator '.or.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new BitwiseOr(lhs, rhs);
        }
    }

bitwise_and_expr:
    rel_expr  {
        $$ = $1;
    }
    | bitwise_and_expr BITWISE_AND rel_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of bitwise operator '.and.' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new BitwiseAnd(lhs, rhs);
        }
    }

rel_expr:
      add_expr
    | rel_expr '<' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '<' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '<' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Relation(lhs, Relation::LESS_THAN, rhs);
        }
    }
    | rel_expr '>' add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '>' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '>' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Relation(lhs, Relation::GREATER_THAN, rhs);
        }
    }
    | rel_expr LE add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '<=' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '<=' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Relation(lhs, Relation::LESS_OR_EQUAL, rhs);
        }
    }
    | rel_expr GE add_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '>=' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '>=' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Relation(lhs, Relation::GREATER_OR_EQUAL, rhs);
        }
    }
    | rel_expr '=' add_expr  {
        $$ = new Relation($1, Relation::EQUAL, $3);
    }
    | rel_expr '#' add_expr  {
        $$ = new Relation($1, Relation::NOT_EQUAL, $3);
    }

add_expr:
      mul_expr
    | add_expr '+' mul_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '+' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '+' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Add(lhs,rhs);
        }
    }
    | add_expr '-' mul_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '-' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '-' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Sub(lhs,rhs);
        }
    }

mul_expr:
    unary_expr
    | mul_expr '*' unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '*' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '*' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Mul(lhs,rhs);
        }
    }
    | mul_expr '/' unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of operator '/' must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of operator '/' must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Div(lhs,rhs);
        }
    }
    | mul_expr MOD unary_expr  {
        ExpressionRef lhs = $1;
        ExpressionRef rhs = $3;
        if (lhs->exprType() != INT_EXPR) {
            PARSE_ERR(@1, (String("Left operand of modulo operator must be an integer expression, is ") + typeStr(lhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else if (rhs->exprType() != INT_EXPR) {
            PARSE_ERR(@3, (String("Right operand of modulo operator must be an integer expression, is ") + typeStr(rhs->exprType()) + " though.").c_str());
            $$ = new IntLiteral(0);
        } else {
            $$ = new Mod(lhs,rhs);
        }
    }

%%

void InstrScript_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err) {
    //fprintf(stderr, "%d: %s\n", locp->first_line, err);
    context->addErr(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, err);
}

void InstrScript_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt) {
    //fprintf(stderr, "WRN %d: %s\n", locp->first_line, txt);
    context->addWrn(locp->first_line, locp->last_line, locp->first_column+1, locp->last_column+1, txt);
}

/// Custom implementation of yytnamerr() to ensure quotation is always stripped from token names before printing them to error messages.
int InstrScript_tnamerr(char* yyres, const char* yystr) {
  if (*yystr == '"') {
      int yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
/*
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
*/
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
/*
    do_not_strip_quotes: ;
*/
    }

  if (! yyres)
    return (int) yystrlen (yystr);

  return int( yystpcpy (yyres, yystr) - yyres );
}
