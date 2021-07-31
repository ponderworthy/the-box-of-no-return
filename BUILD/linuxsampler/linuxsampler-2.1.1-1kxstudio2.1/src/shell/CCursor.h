/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef CCURSOR_H
#define CCURSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>

#include "TerminalCtrl.h"
#include "KeyboardReader.h"

/**
 * Writes (and reads) "ANSI escape codes" to/from stdout/stdin to control the
 * position and appearance of the text input cursor on comand line terminals.
 *
 * This is a lite-weight class to prevent a dependency to i.e. libncurses.
 */
class CCursor {
public:
    CCursor() : m_row(-1), m_col(-1) {}
    virtual ~CCursor() {}

    /**
     * Returns a CCursor storing the current position of the cursor on screen.
     * That position can be altered with CCursors methods or restored at any
     * time by calling restore().
     */
    static CCursor now() {
        CCursor cursor;
        TerminalSetting setting = TerminalCtrl::now();
        TerminalCtrl::echoInput(false);

        // Workaround: obviously only one thread can read from stdin, so if
        // there is already 'KeyboardReader' listening to stdin, then use its
        // read buffers, otherwise read "directly" from stdin with
        // 'TerminalCtrl' class.
        KeyboardReader* reader = KeyboardReader::singleton();
        if (reader) reader->callback(false); // disable its callback for a moment (so it won't deliver the input to somewhere)
        setCode("6n");
        std::string s = (reader) ? reader->popStringToDelimiterSync('R') : TerminalCtrl::getStringToDelimiter('R');
        if (reader) reader->callback(true); // re-enable its callback

        TerminalCtrl::restore(setting);
        char c;
        int row, col;
        int res = sscanf(s.c_str(), "%c[%d;%d", &c, &row, &col);
        if (res == 3) {
            cursor.m_row = row - 1;
            cursor.m_col = col - 1;
        }
        return cursor;
    }

    /**
     * Move the cursor on the terminal screen to the location stored in this
     * CCcursor object.
     */
    const CCursor& restore() const {
        if (!valid()) return *this;
        std::stringstream ss;
        ss << m_row+1 << ";" << m_col+1 << "f";
        setCode(ss.str());
        return *this;
    }

    /// Returns false if this object stores an invalid screen position.
    bool valid() const {
        return m_row >= 0 && m_col >= 0;
    }

    /**
     * Move the cursor on the terminal screen directly to the requested position
     * given by @a row and @a col.
     *
     * @returns CCursor object with new position
     */
    CCursor& moveTo(int row, int col) {
        if (row < 0 || col < 0) return *this;
        m_row = row;
        m_col = col;
        return const_cast<CCursor&>(restore());
    }

    CCursor& toColumn(int n) {
        if (n < 0) return *this;
        m_col = n;
        return const_cast<CCursor&>(restore());
    }

    CCursor& toRow(int n) {
        if (n < 0) return *this;
        m_row = n;
        return const_cast<CCursor&>(restore());
    }

    CCursor& up(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "A";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    CCursor& down(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "B";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    CCursor& right(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "C";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    CCursor& left(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "D";
        setCode(ss.str());
        *this = now();
        return *this;
    }

    int column() const {
        return m_col;
    }
    
    int row() const {
        return m_row;
    }

    CCursor& hide() {
        setCode("?25l");
        return *this;
    }

    CCursor& show() {
        setCode("?25h");
        return *this;
    }
    
    // NOTE: not ANSI.SYS!
    CCursor& lineDown(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "E";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    // NOTE: not ANSI.SYS!
    CCursor& lineUp(int n = 1) {
        if (n < 1) return *this;
        std::stringstream ss;
        ss << n << "F";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    // NOTE: not ANSI.SYS!
    CCursor& scrollUp(int nLines = 1) {
        if (nLines < 1) return *this;
        std::stringstream ss;
        ss << nLines << "S";
        setCode(ss.str());
        *this = now();
        return *this;
    }

    // NOTE: not ANSI.SYS!
    CCursor& scrollDown(int nLines = 1) {
        if (nLines < 1) return *this;
        std::stringstream ss;
        ss << nLines << "T";
        setCode(ss.str());
        *this = now();
        return *this;
    }
    
    CCursor& clearScreen() {
        restore();
        setCode("2J");
        restore(); // allegedly cursor moves to beginning of screen on some systems, so restore
        return *this;
    }
    
    CCursor& clearVerticalToTop() {
        restore();
        setCode("1J");
        return *this;
    }
    
    CCursor& clearVerticalToBottom() {
        restore();
        setCode("0J");
        return *this;
    }
    
    CCursor& clearLineToRight() {
        restore();
        setCode("0K");
        return *this;
    }
    
    CCursor& clearLineToLeft() {
        restore();
        setCode("1K");
        return *this;
    }
    
    CCursor& clearLine() {
        restore();
        setCode("2K");
        return *this;
    }

    //static void save()    { setCode("s"); } // barely supported by any terminal
    //static void restore() { setCode("u"); } // barely supported by any terminal
protected:
    static void setCode(std::string sCode) {
        std::cout << modStr(sCode) << std::flush;
    }
    static std::string modStr(std::string code) {
        return "\033[" + code;
    }
    
private:
    int m_row;
    int m_col;
};

#endif // CCURSOR_H
