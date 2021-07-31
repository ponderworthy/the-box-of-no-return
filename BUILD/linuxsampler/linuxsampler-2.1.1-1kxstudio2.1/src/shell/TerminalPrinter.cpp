/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#include "TerminalPrinter.h"
#include <iostream>
#include "CCursor.h"

TerminalPrinter::TerminalPrinter() : m_lines(0) {
    m_col = CCursor::now().column();
    m_screenWidth = TerminalCtrl::columns();
}

TerminalPrinter::~TerminalPrinter() {
}

TerminalPrinter& TerminalPrinter::operator<< (std::string s) {
    for (int i = 0; i < s.size(); ++i)
        printChar(s[i]);
    std::cout << std::flush;
    return *this;
}

void TerminalPrinter::printChar(char c) {
    std::cout << c << std::flush;
    switch (c) {
        case '\r':
            m_col = 0;
            break;
        case '\n':
            m_col = 0;
            m_lines++;
            break;
        default:
            m_col++;
            if (m_col >= m_screenWidth) {
                m_col = 0;
                m_lines++;
            }
    }
}

int TerminalPrinter::linesAdvanced() const {
    return m_lines;
}
