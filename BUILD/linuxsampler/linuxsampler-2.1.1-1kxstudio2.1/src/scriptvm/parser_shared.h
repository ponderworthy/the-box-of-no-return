/*
 * Copyright (c) 2014-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

// types shared between auto generated lexer and parser ...

#ifndef LS_INSTRSCRIPTSPARSER_SHARED_H
#define LS_INSTRSCRIPTSPARSER_SHARED_H

#include <stdio.h>
#include "tree.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include "parser.tab.h"
#else
# include "parser.h"
#endif

#include "../common/global_private.h"
    
struct _YYSTYPE {
    union {
        int iValue;
        char* sValue;
    };
    LinuxSampler::EventHandlersRef nEventHandlers;
    LinuxSampler::EventHandlerRef nEventHandler;
    LinuxSampler::StatementsRef nStatements;
    LinuxSampler::StatementRef nStatement;
    LinuxSampler::FunctionCallRef nFunctionCall;
    LinuxSampler::ArgsRef nArgs;
    LinuxSampler::ExpressionRef nExpression;
    LinuxSampler::CaseBranch nCaseBranch;
    LinuxSampler::CaseBranches nCaseBranches;
};
#define YYSTYPE _YYSTYPE
#define yystype YYSTYPE     ///< For backward compatibility.
#ifndef YYSTYPE_IS_DECLARED
# define YYSTYPE_IS_DECLARED ///< We tell the lexer / parser that we use our own data structure as defined above.
#endif

#endif // LS_INSTRSCRIPTSPARSER_SHARED_H
