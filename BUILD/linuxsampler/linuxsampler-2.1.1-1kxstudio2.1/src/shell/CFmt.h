/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef CFMT_H
#define CFMT_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>

/**
 * Writes "ANSI escape codes" to stdout to control color, font weight and
 * brightness of text written to the command line terminal.
 *
 * This is a lite-weight class to prevent a dependency to i.e. libncurses.
 *
 * This class is not designed as singleton, because the idea was to reset the
 * terminals output format as soon as the respective CFmt object is destructed.
 * This way a developer is not forces to reset the output format each time he
 * used something special.
 */
class CFmt {
public:
    CFmt() : m_bright(false) {}
    virtual ~CFmt() { reset(); }

    CFmt& black()   { setColorFg("0"); return *this; }
    CFmt& red()     { setColorFg("1"); return *this; }
    CFmt& green()   { setColorFg("2"); return *this; }
    CFmt& yellow()  { setColorFg("3"); return *this; }
    CFmt& blue()    { setColorFg("4"); return *this; }
    CFmt& magenta() { setColorFg("5"); return *this; }
    CFmt& cyan()    { setColorFg("6"); return *this; }
    CFmt& white()   { setColorFg("7"); return *this; }

    CFmt& bold()      { setCode("1"); return *this; }
    //CFmt& faint()      { setCode("2"); return *this; } // barely supported by any terminal
    //CFmt& italic()    { setCode("3"); return *this; } // barely supported by any terminal
    CFmt& underline() { setCode("4"); return *this; }
    //CFmt& frame()     { setCode("51"); return *this; } // barely supported by any terminal
    //CFmt& encircle()  { setCode("52"); return *this; } // barely supported by any terminal
    //CFmt& overline()  { setCode("53"); return *this; } // barely supported by any terminal
    
    CFmt& bright(bool b = true) { m_bright = b; return *this; }

    CFmt& reset() {
        setCode("0");
        m_bright = false;
        return *this;
    }

protected:
    CFmt& setColorFg(std::string colorCode) {
        setCode((m_bright ? "9" : "3") + colorCode);
        return *this;
    }

    static void setCode(std::string sCode) {
        std::cout << modStr(sCode) << std::flush;
    }

    static std::string modStr(std::string code) {
        return "\033[" + code + "m";
    }

private:
    bool m_bright;
};

#endif // CFMT_H
