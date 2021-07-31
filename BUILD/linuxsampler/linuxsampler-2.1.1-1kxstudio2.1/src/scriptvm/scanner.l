/*
 * Copyright (c) 2014-2017 Christian Schoenebeck and Andreas Persson
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

/* Token scanner for NKSP real-time instrument script language. */

%{

#include "parser_shared.h"
#include <math.h>
// reentrant scanner data context
#define YY_EXTRA_TYPE ParserContext*
// set line number each time a token is recognized
#define YY_USER_ACTION                  \
{                                       \
    yylloc->first_line = yylineno;      \
    yylloc->last_line  = yylineno;      \
    yylloc->first_column = yycolumn;    \
    yycolumn += yyleng;                 \
    yylloc->last_column = yycolumn - 1; \
    /*printf("lex: line '%s'\n", yytext);*/  \
}
// custom (f)lex input for reading from std::istream object
#define YY_INPUT(buf,result,max_size)   \
{                                       \
    char c = yyextra->is->get();        \
    if (yyextra->is->eof())             \
        result = YY_NULL;               \
    else {                              \
        buf[0] = c;                     \
        result = 1;                     \
    }                                   \
}

static void scanner_error(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* err) {
    context->addErr(locp->first_line, locp->last_line, locp->first_column, locp->last_column, err);
}

static void scanner_warning(YYLTYPE* locp, LinuxSampler::ParserContext* context, const char* txt) {
    context->addWrn(locp->first_line, locp->last_line, locp->first_column, locp->last_column, txt);
}

#define SCANNER_ERR(txt)  scanner_error(yylloc, yyextra, txt)
#define SCANNER_WRN(txt)  scanner_warning(yylloc, yyextra, txt)

// shut up warning that 'register' keyword is deprecated as of C++11
#if defined(__cplusplus) && __cplusplus >= 201103L
# define register
#endif

using namespace LinuxSampler;

static int countNewLineChars(const char* txt) {
    int n = 0;
    for (int i = 0; txt[i]; ++i)
        if (txt[i] == '\n') ++n;
    return n;
}

static int countCharsPastLastNewLine(const char* txt) {
    const int n = (int)strlen(txt);
    for (int i = n - 1; i >= 0; --i)
        if (txt[i] == '\n')
            return n - i - 1;
    return n;
}

#define processLocation() { \
    const int nl = countNewLineChars(yytext); \
    yylineno += nl; \
    if (nl) yycolumn = countCharsPastLastNewLine(yytext); \
}

// if compiled for debugging, throw an exception instead of exiting on fatal
// lexer errors (so the debugger may pause with the appropriate back trace)
#if DEBUG
# include <stdexcept>
# define YY_FATAL_ERROR(msg) throw std::runtime_error(msg)
#endif

%}

/* use Flex's built-in support for line numbers
   (disabled, because it seems to be unreliable, so we are using our own
   tracking code in the respective scanner rules below) */
/*%option yylineno*/
/* generate a reentrant safe scanner */
%option reentrant
/* avoid symbol collision with other (i.e. future) scanner symbols */
%option prefix="InstrScript_"
/* bison-bridge adds an argument yylval to yylex, and bison-locations adds an
   argument code yylloc for location tracking. */
%option bison-bridge
%option bison-locations
/* yywrap() would be called at EOF, we don't need it */
%option noyywrap
/* enable functions yy_push_state(), yy_pop_state(), yy_top_state() */
%option stack

/* inclusive scanner conditions */
%s PREPROC_BODY_USE
/* exclusive scanner conditions */
%x PREPROC_SET_COND PREPROC_RESET_COND PREPROC_IF PREPROC_IF_NOT PREPROC_BODY_EAT PREPROC_PRE_BODY_USE PREPROC_PRE_BODY_EAT

DIGIT    [0-9]
ID       [a-zA-Z0-9_]+

%%

\"[^"]*\" {
    yylval->sValue = strdup(yytext + 1);
    yylval->sValue[strlen(yylval->sValue) - 1] = '\0';
    return STRING;
}

{DIGIT}+ {
    yylval->iValue = atoi(yytext);
    return INTEGER;
}

 /* there is currently no support for floating point numbers in NKSP yet */
 /*{DIGIT}+"."{DIGIT}* {
    printf("A float: %s (%g)\n", yytext, atof(yytext));
 }*/


 /* Preprocessor statement:  SET_CONDITION(name) */

<*>"SET_CONDITION"[ \t]*"(" {
    //printf("SET_CONDITION\n");
    yy_push_state(PREPROC_SET_COND, yyscanner);
}
<PREPROC_SET_COND>{ID} {
    //printf("preproc set condition '%s'\n", yytext);
    bool success = yyextra->setPreprocessorCondition(yytext);
    if (!success) {
        SCANNER_WRN((String("Preprocessor: Condition '") +
                    yytext + "' is already set.").c_str());
    }
}
<PREPROC_SET_COND>[ \t]*")" {
    //printf("End of PREPROC_SET_COND\n");
    yy_pop_state(yyscanner);
}


 /* Preprocessor statement:  RESET_CONDITION(name) */

<*>"RESET_CONDITION"[ \t]*"(" {
    //printf("RESET_CONDITION\n");
    yy_push_state(PREPROC_RESET_COND, yyscanner);
}
<PREPROC_RESET_COND>{ID} {
    //printf("preproc reset condition '%s'\n", yytext);
    bool success = yyextra->resetPreprocessorCondition(yytext);
    if (!success) {
        SCANNER_ERR((String("Preprocessor: could not reset condition '") +
                    yytext + "' (either not set or a built-in condition).").c_str());
    }
}
<PREPROC_RESET_COND>[ \t]*")" {
    //printf("End of RESET_CONDITION\n");
    yy_pop_state(yyscanner);
}


 /* Preprocessor conditional statements:

    USE_CODE_IF(name)
      ...
    END_USE_CODE

    and:

    USE_CODE_IF_NOT(name)
      ...
    END_USE_CODE    
 */

<*>"USE_CODE_IF"[ \t]*"(" {
    //printf("{%s}\n", yytext);
    yy_push_state(PREPROC_IF, yyscanner);
}
<PREPROC_BODY_EAT>"USE_CODE_IF"[ \t]*"("{ID}")" {
    //printf("[EAT{%s}\n", yytext);
    yy_push_state(PREPROC_BODY_EAT, yyscanner);
}
<*>"USE_CODE_IF_NOT"[ \t]*"(" {
    //printf("USE_CODE_IF_NOT\n");
    yy_push_state(PREPROC_IF_NOT, yyscanner);
}
<PREPROC_BODY_EAT>"USE_CODE_IF_NOT"[ \t]*"("{ID}")" {
    //printf("[EAT{%s}\n", yytext);
    yy_push_state(PREPROC_BODY_EAT, yyscanner);
}
<PREPROC_IF>{ID} {
    //printf("preproc use code if '%s'\n", yytext);
    yy_pop_state(yyscanner);
    if (yyextra->isPreprocessorConditionSet(yytext))
        yy_push_state(PREPROC_PRE_BODY_USE, yyscanner);
    else
        yy_push_state(PREPROC_PRE_BODY_EAT, yyscanner);
}
<PREPROC_IF_NOT>{ID} {
    //printf("preproc use code if not '%s'\n", yytext);
    yy_pop_state(yyscanner);
    if (!yyextra->isPreprocessorConditionSet(yytext))
        yy_push_state(PREPROC_PRE_BODY_USE, yyscanner);
    else
        yy_push_state(PREPROC_PRE_BODY_EAT, yyscanner);
}
<PREPROC_PRE_BODY_USE>[ \t]*")" {
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_BODY_USE, yyscanner);
}
<PREPROC_PRE_BODY_EAT>[ \t]*")" {
    //printf("PREPROCESSOR EAT : {%s}\n", yytext);
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_BODY_EAT, yyscanner);
}
<PREPROC_BODY_EAT,PREPROC_BODY_USE>"END_USE_CODE" {
    //printf("-->END_USE_CODE\n");
    yy_pop_state(yyscanner);
}
<PREPROC_BODY_EAT>[ \t\r\n]* { /* eat up code block filtered out by preprocessor */
    //printf("PREPROCESSOR EAT2 : {%s}\n", yytext);
    processLocation();
}
<PREPROC_BODY_EAT>.* { /* eat up code block filtered out by preprocessor */
    //printf("PREPROCESSOR EAT3 : {%s}\n", yytext);
    yyextra->addPreprocessorComment(yylloc->first_line, yylloc->last_line,
                                    yylloc->first_column+1, yylloc->last_column+1);
}


 /* Language keywords */

"on" return ON;
"end" return END;
"note" return NOTE;
"init" return INIT;
"declare" return DECLARE;
"while" return WHILE;
"if" return IF;
".or." return BITWISE_OR;
"or" return OR;
"release" return RELEASE;
".and." return BITWISE_AND;
"and" return AND;
".not." return BITWISE_NOT;
"not" return NOT;
"else" return ELSE;
"controller" return CONTROLLER;
"case" return CASE;
"select" return SELECT;
"to" return TO;
"<=" return LE;
">=" return GE;
"const" return CONST_; // note: "CONST" is already defined for C/C++ compilers on Windows by default
"polyphonic" return POLYPHONIC;
"mod" return MOD;
"function" return FUNCTION;
"call" return CALL;
"synchronized" return SYNCHRONIZED;

[&,()[\]<>=*+#/-] {
    return *yytext;
}

("$"|"@"|"%"){ID} {
    yylval->sValue = strdup(yytext);
    return VARIABLE;
}

{ID} {
    yylval->sValue = strdup(yytext);
    return IDENTIFIER;
}

":=" return ASSIGNMENT;

\n+ {
     yylineno += countNewLineChars(yytext);
     yycolumn = 0;
    //printf("lex: new line %d\n", yylineno, yytext);
    //return LF;
}

"{"[^}]*"}" { /* eat up comments */
    processLocation();
}

[ \t\r]+ /* eat up whitespace */

. {
    printf( "Unrecognized character: '%s' (line %d, column %d)\n", yytext, yylineno, yycolumn);
    return UNKNOWN_CHAR;
}

%%

namespace LinuxSampler {

void ParserContext::createScanner(std::istream* is) {
   if (scanner) destroyScanner();
   this->is = is;
   yylex_init(&scanner);
   yyset_extra(this, scanner);
}

void ParserContext::destroyScanner() {
   if (!scanner) return;
   yylex_destroy(scanner);
   scanner = NULL;
}

} // namespace LinuxSampler
