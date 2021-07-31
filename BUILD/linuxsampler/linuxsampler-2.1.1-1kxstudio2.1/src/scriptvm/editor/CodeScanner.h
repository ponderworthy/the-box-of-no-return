/*
 * Copyright (c) 2015-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include <iostream>
#include <sstream>
#include <vector>
#include "SourceToken.h"

#ifndef CRUDEBYTE_CODE_SCANNER_H
#define CRUDEBYTE_CODE_SCANNER_H

namespace LinuxSampler {

class CodeScanner {
public:
    void* scanner;
    std::istream* is;
    SourceToken token;
    int line;
    int column;

    CodeScanner(std::istream* is);
    virtual ~CodeScanner();
    std::vector<SourceToken> tokens() const { return m_tokens; }
    bool isMultiLine() const;

protected:
    std::vector<SourceToken> m_tokens;

    virtual int processScanner() = 0;
    SourceToken processOneToken();
    void processAll();
    void trim();
};

// base types
#define EofToken() SourceToken()
#define NewLineToken() SourceToken(SourceToken::NEW_LINE, "\n")
#define KeywordToken(s) SourceToken(SourceToken::KEYWORD, s)
#define VariableNameToken(s) SourceToken(SourceToken::VARIABLE_NAME, s)
#define IdentifierToken(s) SourceToken(SourceToken::IDENTIFIER, s)
#define NumberLiteralToken(s) SourceToken(SourceToken::NUMBER_LITERAL, s)
#define StringLiteralToken(s) SourceToken(SourceToken::STRING_LITERAL, s)
#define CommentToken(s) SourceToken(SourceToken::COMMENT, s)
#define PreprocessorToken(s) SourceToken(SourceToken::PREPROCESSOR, s)
#define OtherToken(s) SourceToken(SourceToken::OTHER, s)

// extended types
#define IntegerVariableToken(s) SourceToken(SourceToken::INTEGER_VARIABLE, s)
#define StringVariableToken(s) SourceToken(SourceToken::STRING_VARIABLE, s)
#define ArrayVariableToken(s) SourceToken(SourceToken::ARRAY_VARIABLE, s)
#define EventHandlerNameToken(s) SourceToken(SourceToken::EVENT_HANDLER_NAME, s)

} // namespace LinuxSampler

#endif // CRUDEBYTE_CODE_SCANNER_H
