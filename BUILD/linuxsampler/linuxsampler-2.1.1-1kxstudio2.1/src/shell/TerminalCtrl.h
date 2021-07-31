/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef TERMINALCTRL_H
#define TERMINALCTRL_H

#include <vector>
#include <string>

class TerminalCtrl;

/**
 * Represents a state of the command line terminal for being restored later on
 * at arbitrary time.
 */
class TerminalSetting {
public:
    TerminalSetting();
    TerminalSetting(const TerminalSetting& ts);
    virtual ~TerminalSetting();
    bool valid() const;
protected:
    void* p;
    friend class TerminalCtrl;
};

/**
 * OS dependent implementation for controlling keyboard input behavior and
 * retrieving terminal screen size (rows and columns).
 *
 * Current implementation assumes a POSIX compatible system.
 *
 * This is a lite-weight class to prevent a dependency to i.e. libncurses.
 */
class TerminalCtrl {
public:
    static std::vector<char> getChars(int max, int blockUntilMin, int burstDeciSeconds = 0);
    static std::vector<char> getCharsToDelimiter(char delimiter, bool includeDelimiter = false);
    static std::string getStringToDelimiter(char delimiter, bool includeDelimiter = false);
    static int echoInput(bool b);
    static int immediateInput(bool b);
    static int reset();
    static int restore(const TerminalSetting& setting);
    static TerminalSetting now();
    static int columns();
    static int rows();
}; 

#endif // TERMINALCTRL_H
