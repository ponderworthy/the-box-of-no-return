/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef TERMINALPRINTER_H
#define TERMINALPRINTER_H

#include <string>

/**
 * Simple helper class that remembers the amount of lines advanced on the
 * terminal (since this object was constructed) by printing out text to the
 * terminal with this class. This way the content been printed on the terminal
 * can completely be erased afterwards once necessary.
 */
class TerminalPrinter {
public:
    TerminalPrinter();
    virtual ~TerminalPrinter();
    int linesAdvanced() const;
    TerminalPrinter& operator<< (const std::string s);
protected:
    void printChar(char c);
private:
    int m_lines;
    int m_col;
    int m_screenWidth; // in characters
};

#endif // TERMINALPRINTER_H
