/*
 * Copyright (c) 2015-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "CodeScanner.h"
#include "../../common/global_private.h"

namespace LinuxSampler {

CodeScanner::CodeScanner(std::istream* _is)
    : scanner(NULL), is(_is), line(0), column(0)
{
}

CodeScanner::~CodeScanner() {
}

SourceToken CodeScanner::processOneToken() {
    processScanner();
    token.line   = line;
    token.column = column;
    return token;
}

void CodeScanner::processAll() {
    for (SourceToken token = processOneToken(); token; token = processOneToken()) {
        if (!m_tokens.empty() && token.equalsType(*(m_tokens.end()-1)) && !token.isNewLine())
            (m_tokens.end()-1)->txt += token.text();
        else
            m_tokens.push_back(token);
    }
    //trim();
}

bool CodeScanner::isMultiLine() const {
    for (int i = 0; i < m_tokens.size(); ++i)
        if (m_tokens[i].isNewLine())
            return true;
    return false;    
}

void CodeScanner::trim() {
    // remove initial blank line(s)
    {
        std::vector<SourceToken>::iterator lineFeed = m_tokens.end();
        for (std::vector<SourceToken>::iterator it = m_tokens.begin();
            it != m_tokens.end(); ++it)
        {
            if (it->isNewLine()) {
                lineFeed = it;
            } else if (! ::trim(it->text()).empty()) {
                if (lineFeed != m_tokens.end())
                    m_tokens.erase(m_tokens.begin(), lineFeed+1);
                break;
            }
        }
    }
    // remove blank line(s) at end
    {
        std::vector<SourceToken>::reverse_iterator lineFeed = m_tokens.rend();
        for (std::vector<SourceToken>::reverse_iterator it = m_tokens.rbegin();
            it != m_tokens.rend(); ++it)
        {
            if (it->isNewLine()) {
                lineFeed = it;
            } else if (! ::trim(it->text()).empty()) {
                if (lineFeed != m_tokens.rend())
                    m_tokens.erase(--(lineFeed.base()));
                break;
            }
        }
    }
}

} // namespace LinuxSampler
