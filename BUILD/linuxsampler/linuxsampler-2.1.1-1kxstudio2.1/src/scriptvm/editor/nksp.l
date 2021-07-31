/*
 * Copyright (c) 2015-2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

/* Token scanner used for generating syntax highlighting for NKSP instrument
   script language code (this is not used by the sampler itself, but rather
   provided for external script editor applications). */

%{

#include "NkspScanner.h"
// reentrant scanner data context
#define YY_EXTRA_TYPE NkspScanner*
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
// handle position (line, column) for each recognized token
#define YY_USER_ACTION               \
    yyextra->line   = yylineno - 1;  \
    yyextra->column = yycolumn;      \
    yycolumn += yyleng;

using namespace LinuxSampler;

static int countNewLineChars(const char* txt) {
    int n = 0;
    for (int i = 0; txt[i]; ++i)
        if (txt[i] == '\n') ++n;
    return n;
}

// shut up warning that 'register' keyword is deprecated as of C++11
#if defined(__cplusplus) && __cplusplus >= 201103L
# define register
#endif

// Since this parser is solely used by script code editors, thus not used in a
// real-time context, always throw an exception instead of exiting on fatal
// lexer errors (so the debugger may pause with the appropriate back trace)
#include <stdexcept>
#define YY_FATAL_ERROR(msg) throw std::runtime_error(msg)

%}

/* generate a reentrant safe scanner */
%option reentrant
/* avoid symbol collision with ones of other scanners */
%option prefix="Nksp_"
/* yywrap() would be called at EOF, we don't need it */
%option noyywrap
/* enable functions yy_push_state(), yy_pop_state(), yy_top_state() */
%option stack

/* inclusive scanner conditions */
%s PREPROC_BODY_USE
/* exclusive scanner conditions */
%x PREPROC_SET_COND PREPROC_RESET_COND PREPROC_IF PREPROC_IF_NOT PREPROC_BODY_EAT PREPROC_PRE_BODY_USE PREPROC_PRE_BODY_EAT PREPROC_EVENT_NAME PREPROC_END_NAME

DIGIT    [0-9]
ID       [a-zA-Z0-9_]+
END_ID   on|while|if|select|function|synchronized

%%

\"[^"]*\" {
    yyextra->token = StringLiteralToken(yytext);
    return yyextra->token.baseType;
}

{DIGIT}+ {
    yyextra->token = NumberLiteralToken(yytext);
    return yyextra->token.baseType;
}

 /* there is currently no support for floating point numbers in NKSP yet */
 /*{DIGIT}+"."{DIGIT}* {
    yyextra->token = NumberLiteralToken(yytext);
    return yyextra->token.baseType;
 }*/


 /* Preprocessor statement:  SET_CONDITION(name) */

<*>"SET_CONDITION"[ \t]*"(" {
    //printf("SET_CONDITION\n");
    yy_push_state(PREPROC_SET_COND, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_SET_COND>{ID} {
    //printf("preproc set condition '%s'\n", yytext);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_SET_COND>[ \t]*")" {
    //printf("End of PREPROC_SET_COND\n");
    yy_pop_state(yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}


 /* Preprocessor statement:  RESET_CONDITION(name) */

<*>"RESET_CONDITION"[ \t]*"(" {
    //printf("RESET_CONDITION\n");
    yy_push_state(PREPROC_RESET_COND, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_RESET_COND>{ID} {
    //printf("preproc reset condition '%s'\n", yytext);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_RESET_COND>[ \t]*")" {
    //printf("End of RESET_CONDITION\n");
    yy_pop_state(yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
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
    //printf("USE_CODE_IF\n");
    yy_push_state(PREPROC_IF, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<*>"USE_CODE_IF_NOT"[ \t]*"(" {
    //printf("USE_CODE_IF_NOT\n");
    yy_push_state(PREPROC_IF_NOT, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_IF>{ID} {
    //printf("preproc use code if '%s'\n", yytext);
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_PRE_BODY_USE, yyscanner);
    //yy_push_state(PREPROC_PRE_BODY_EAT, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_IF_NOT>{ID} {
    //printf("preproc use code if not '%s'\n", yytext);
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_PRE_BODY_USE, yyscanner);
    //yy_push_state(PREPROC_PRE_BODY_EAT, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_PRE_BODY_USE>[ \t]*")" {
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_BODY_USE, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_PRE_BODY_EAT>[ \t]*")" {
    //printf("PREPROCESSOR EAT : \n");
    yy_pop_state(yyscanner);
    yy_push_state(PREPROC_BODY_EAT, yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<*>.*"END_USE_CODE" {
    //printf("-->END_USE_CODE\n");
    yy_pop_state(yyscanner);
    yyextra->token = PreprocessorToken(yytext);
    return yyextra->token.baseType;
}
<PREPROC_BODY_EAT>[ \t\r\n]* /* eat up code block filtered out by preprocessor */
<PREPROC_BODY_EAT>.* /* eat up code block filtered out by preprocessor */


 /* Event Handler Names (only if they occur alone in a document!) */
 
^\s*(init|note|release|controller) {
    yyextra->token = EventHandlerNameToken(yytext);
    return yyextra->token.baseType;
}


 /* Language keywords */

on {
    yy_push_state(PREPROC_EVENT_NAME, yyscanner);
    yyextra->token = KeywordToken(yytext);
    return yyextra->token.baseType;
}

<PREPROC_EVENT_NAME>[ \t]*{ID} {
    yy_pop_state(yyscanner);
    yyextra->token = EventHandlerNameToken(yytext);
    return yyextra->token.baseType;
}

end {
    yy_push_state(PREPROC_END_NAME, yyscanner);
    yyextra->token = KeywordToken(yytext);
    return yyextra->token.baseType;
}

<PREPROC_END_NAME>[ \t]*{END_ID}? {
    yy_pop_state(yyscanner);
    yyextra->token = KeywordToken(yytext);
    return yyextra->token.baseType;
}

".or."|".and."|".not." {
    yyextra->token = KeywordToken(yytext);
    return yyextra->token.baseType;
}

declare|while|if|or|and|not|else|case|select|to|mod|const|polyphonic|function|call|synchronized {
    yyextra->token = KeywordToken(yytext);
    return yyextra->token.baseType;
}


 /* Variables */
 
"$"{ID} {
    yyextra->token = IntegerVariableToken(yytext);
    return yyextra->token.baseType;
}

"@"{ID} {
    yyextra->token = StringVariableToken(yytext);
    return yyextra->token.baseType;
}

"%"{ID} {
    yyextra->token = ArrayVariableToken(yytext);
    return yyextra->token.baseType;
}

{ID} {
    yyextra->token = IdentifierToken(yytext);
    return yyextra->token.baseType;
}


 /* other */

<*>\n {
    yyextra->token = NewLineToken();
    ++yylineno;
    yycolumn = 0;
    return yyextra->token.baseType;
}

"{"[^}]*"}" {
    yyextra->token = CommentToken(yytext);
    yylineno += countNewLineChars(yytext);
    return yyextra->token.baseType;
}

<*>\t {
    yyextra->token = OtherToken("    ");
    return yyextra->token.baseType;
}

\r+ /* eat up \r */

<<EOF>> {
    yyextra->token = EofToken();
    yyterminate();
}

<*>. {
    yyextra->token = OtherToken(yytext);
    return yyextra->token.baseType;
}


%%

namespace LinuxSampler {

int NkspScanner::processScanner() {
    return Nksp_lex(scanner);
}

void NkspScanner::createScanner(std::istream* is) {
    if (scanner) destroyScanner();
    this->is = is;
    Nksp_lex_init(&scanner);
    Nksp_set_extra(this, scanner);
}

void NkspScanner::destroyScanner() {
    if (!scanner) return;
    Nksp_lex_destroy(scanner);
    scanner = NULL;
}

} // namespace LinuxSampler
