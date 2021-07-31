/*
 * Copyright (c) 2014-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "common.h"
#include <iostream>
#include "editor/SourceToken.h"

namespace LinuxSampler {

    VMIntExpr* VMExpr::asInt() const {
        return const_cast<VMIntExpr*>( dynamic_cast<const VMIntExpr*>(this) );
    }

    VMStringExpr* VMExpr::asString() const {
        return const_cast<VMStringExpr*>( dynamic_cast<const VMStringExpr*>(this) );
    }

    VMIntArrayExpr* VMExpr::asIntArray() const {
        return const_cast<VMIntArrayExpr*>( dynamic_cast<const VMIntArrayExpr*>(this) );
    }

    bool VMExpr::isModifyable() const {
        const VMVariable* var = dynamic_cast<const VMVariable*>(this);
        return (!var) ? false : var->isAssignable();
    }

    void VMFunction::wrnMsg(const String& txt) {
        std::cout << "[ScriptVM] " << txt << std::endl;
    }

    void VMFunction::errMsg(const String& txt) {
        std::cerr << "[ScriptVM] " << txt << std::endl;
    }
    
    ///////////////////////////////////////////////////////////////////////
    // class 'VMSourceToken'

    VMSourceToken::VMSourceToken() : m_token(NULL) {
    }

    VMSourceToken::VMSourceToken(SourceToken* ct) : m_token(ct) {
    }

    VMSourceToken::VMSourceToken(const VMSourceToken& other) {
        if (other.m_token) {
            m_token = new SourceToken;
            *m_token = *other.m_token;
        } else m_token = NULL;
    }

    VMSourceToken::~VMSourceToken() {
        if (m_token) {
            delete m_token;
            m_token = NULL;
        }
    }

    VMSourceToken& VMSourceToken::operator=(const VMSourceToken& other) {
        if (m_token) delete m_token;
        if (other.m_token) {
            m_token = new SourceToken;
            *m_token = *other.m_token;
        } else m_token = NULL;
        return *this;
    }

    String VMSourceToken::text() const {
        return (m_token) ? m_token->text() : "";
    }

    int VMSourceToken::firstLine() const {
        return (m_token) ? m_token->firstLine() : 0;
    }

    int VMSourceToken::firstColumn() const {
        return (m_token) ? m_token->firstColumn() : 0;
    }

    bool VMSourceToken::isEOF() const {
        return (m_token) ? m_token->isEOF() : true;
    }

    bool VMSourceToken::isNewLine() const {
        return (m_token) ? m_token->isNewLine() : false;
    }

    bool VMSourceToken::isKeyword() const {
        return (m_token) ? m_token->isKeyword() : false;
    }

    bool VMSourceToken::isVariableName() const {
        return (m_token) ? m_token->isVariableName() : false;
    }

    bool VMSourceToken::isIdentifier() const {
        return (m_token) ? m_token->isIdentifier() : false;
    }

    bool VMSourceToken::isNumberLiteral() const {
        return (m_token) ? m_token->isNumberLiteral() : false;
    }

    bool VMSourceToken::isStringLiteral() const {
        return (m_token) ? m_token->isStringLiteral() : false;
    }

    bool VMSourceToken::isComment() const {
        return (m_token) ? m_token->isComment() : false;
    }

    bool VMSourceToken::isPreprocessor() const {
        return (m_token) ? m_token->isPreprocessor() : false;
    }

    bool VMSourceToken::isOther() const {
        return (m_token) ? m_token->isOther() : true;
    }

    bool VMSourceToken::isIntegerVariable() const {
        return (m_token) ? m_token->isIntegerVariable() : false;
    }

    bool VMSourceToken::isStringVariable() const {
        return (m_token) ? m_token->isStringVariable() : false;
    }

    bool VMSourceToken::isArrayVariable() const {
        return (m_token) ? m_token->isArrayVariable() : false;
    }

    bool VMSourceToken::isEventHandlerName() const {
        return (m_token) ? m_token->isEventHandlerName() : false;
    }

} // namespace LinuxSampler
