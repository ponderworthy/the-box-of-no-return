/*
 * LSCP Shell
 *
 * Copyright (c) 2014 - 2017 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#include "KeyboardReader.h"

static KeyboardReader* g_singleton = NULL;
static int g_instanceCount = 0;

KeyboardReader::KeyboardReader() : Thread(false, false, 1, -1),
    m_originalTerminalSetting(TerminalCtrl::now()),
    m_callback(NULL), m_doCallback(true)
{
    if (!g_instanceCount) g_singleton = this;
    g_instanceCount++;
    TerminalCtrl::echoInput(false);
}

KeyboardReader::~KeyboardReader() {
    StopThread();
    TerminalCtrl::restore(m_originalTerminalSetting);
    // ensures that no temporary copy objects delete the global reference
    g_instanceCount--;
    if (!g_instanceCount) g_singleton = NULL;
}

void KeyboardReader::setCallback(Callback_t fn) {
    m_callback = fn;
}

void KeyboardReader::callback(bool b) {
    m_doCallback = b;
}

int KeyboardReader::Main() {
    while (true) {
        std::vector<char> v = TerminalCtrl::getChars(1, 1);
        if (!v.empty()) {
            m_fifoMutex.Lock();
            m_fifo.push_back(v[0]);
            if (m_sync.GetUnsafe()) {
                bool delimiterReceived = false;
                for (std::list<char>::iterator it = m_fifo.begin(); it != m_fifo.end(); ++it) {
                    if ((*it) == m_syncDelimiter) {
                        delimiterReceived = true;
                        break;
                    }
                }
                m_fifoMutex.Unlock();
                if (delimiterReceived) m_sync.Set(false);
            } else {
                m_fifoMutex.Unlock();
                if (m_callback && m_doCallback) (*m_callback)(this);
            }
        }
        TestCancel();
    }
    return 0; // just to avoid a warning with some old compilers
}

bool KeyboardReader::charAvailable() const {
    return !m_fifo.empty(); // is thread safe
}

char KeyboardReader::popChar() {
    LockGuard lock(m_fifoMutex);
    if (m_fifo.empty()) return 0;
    char c = m_fifo.front();
    m_fifo.pop_front();
    return c;
}

std::string KeyboardReader::popStringToDelimiterSync(char delimiter, bool includeDelimiter) {
    m_syncDelimiter = delimiter;
    
    bool alreadyReceived = false;
    m_fifoMutex.Lock();
    for (std::list<char>::iterator it = m_fifo.begin(); it != m_fifo.end(); ++it) {
        if ((*it)== delimiter) {
            alreadyReceived = true;
            break;
        }
    }
    if (!alreadyReceived) m_sync.Set(true);
    m_fifoMutex.Unlock();
    if (!alreadyReceived) m_sync.WaitAndUnlockIf(true);

    // if we are here, then there is now a string with the requested delimiter
    // in the FIFO
    std::string s;
    while (charAvailable()) {
        char c = popChar();
        if (includeDelimiter || c != delimiter) s += c;
        if (c == delimiter) return s;
    }
    return s;
}

void KeyboardReader::startReading() {
    StartThread();
}

void KeyboardReader::stopReading() {
    StopThread();
}

KeyboardReader* KeyboardReader::singleton() {
    return g_singleton;
}
