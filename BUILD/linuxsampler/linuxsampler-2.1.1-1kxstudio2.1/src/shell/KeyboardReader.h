/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef KEYBOARDREADER_H
#define KEYBOARDREADER_H

#include <list>
#include <string>

#include "../common/global.h"
#include "../common/Thread.h"
#include "../common/Condition.h"
#include "TerminalCtrl.h"

#define KBD_BACKSPACE 127
#define KBD_ESCAPE    27

using namespace LinuxSampler;

/**
 * Implements a FIFO object that is constantly reading from stdin on a separate
 * thread. The reader thread can call a callback function as soon as new data
 * arrived from stdin. The reader thread will react on every single key stroke,
 * that is it will already fire before line end (RETURN / ENTER) was received.
 */
class KeyboardReader : protected Thread {
public:
    typedef void(*Callback_t)(KeyboardReader*);

    KeyboardReader();
    virtual ~KeyboardReader();
    void setCallback(Callback_t fn);
    void callback(bool b);
    bool charAvailable() const;
    char popChar();
    void startReading();
	void stopReading();
    std::string popStringToDelimiterSync(char delimiter, bool includeDelimiter = false);
    
    static KeyboardReader* singleton();
protected:
    int Main() OVERRIDE;
private:
    TerminalSetting m_originalTerminalSetting;
    Callback_t m_callback;
    std::list<char> m_fifo;
    Mutex m_fifoMutex;
    bool m_doCallback;
    Condition m_sync;
    char m_syncDelimiter;
};

#endif // KEYBOARDREADER_H
