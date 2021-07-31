/*
 * LSCP Shell
 *
 * Copyright (c) 2014 - 2016 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#include "TerminalCtrl.h"

#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include "../common/Mutex.h"

using namespace LinuxSampler;

///////////////////////////////////////////////////////////////////////////
// class 'TerminalSetting'

static Mutex g_mutexReferences;
static std::map<void*,int> g_terminalSettingReferences;

static termios* _newTermios() {
    termios* p = new termios;
    LockGuard lock(g_mutexReferences);
    g_terminalSettingReferences[p] = 1;
    return p;
}

static void _releaseTermiosRef(void* p) {
    if (!p) return;
    LockGuard lock(g_mutexReferences);
    std::map<void*,int>::iterator it = g_terminalSettingReferences.find(p);
    assert(it != g_terminalSettingReferences.end());
    assert(it->second > 0);
    it->second--;
    if (!it->second) {
        g_terminalSettingReferences.erase(it);
        delete (termios*) p;
    }
}

static termios* _bumpTermiosRef(void* p) {
    if (!p) return NULL;
    LockGuard lock(g_mutexReferences);
    std::map<void*,int>::iterator it = g_terminalSettingReferences.find(p);
    assert(it != g_terminalSettingReferences.end());
    assert(it->second > 0);
    it->second++;
    return (termios*) p;
}

TerminalSetting::TerminalSetting() : p(NULL) {
}

TerminalSetting::TerminalSetting(const TerminalSetting& ts) : p(_bumpTermiosRef(ts.p)) {
}

TerminalSetting::~TerminalSetting() {
    _releaseTermiosRef(p);
}

bool TerminalSetting::valid() const {
    return p != NULL;
}

///////////////////////////////////////////////////////////////////////////
// class 'TerminalCtrl'

static termios g_defaults;

static void _saveDefaults() {
    static bool done = false;
    if (done) return;
    if (tcgetattr(0, &g_defaults) < 0) return;
    done = true;
}

int TerminalCtrl::reset() {
    _saveDefaults();
    if (tcsetattr(0, TCSANOW, &g_defaults) < 0) return -1;
    return 0; // success
}

int TerminalCtrl::restore(const TerminalSetting& setting) {
    _saveDefaults();
    termios* s = (termios*) setting.p;
    if (tcsetattr(0, TCSADRAIN, s) < 0) return -1;
    return 0; // success
}

TerminalSetting TerminalCtrl::now() {
    _saveDefaults();
    TerminalSetting setting;
    if (!setting.p) setting.p = _newTermios();
    if (tcgetattr(0, (termios*) setting.p) < 0)
        return TerminalSetting(); // error, return invalid setting
    return setting; // success
}

int TerminalCtrl::echoInput(bool b) {
    _saveDefaults();
    termios settings = {};
    if (tcgetattr(0, &settings) < 0) return -1;
    if (b) {
        settings.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &settings) < 0) return -1;
    } else {
        settings.c_lflag &= ~ECHO;
        if (tcsetattr(0, TCSANOW, &settings) < 0) return -1;
    }
    return 0; // success
}

int TerminalCtrl::immediateInput(bool b) {
    _saveDefaults();
    termios settings = {};
    if (tcgetattr(0, &settings) < 0) return -1;
    if (b) {
        settings.c_lflag &= ~ICANON;
        settings.c_cc[VMIN] = 0;
        settings.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &settings) < 0) return -1;
    } else {
        settings.c_lflag |= ICANON;
        if (tcsetattr(0, TCSADRAIN, &settings) < 0) return -1;
    }
    return 0; // success
}

std::vector<char> TerminalCtrl::getChars(int max, int blockUntilMin, int burstDeciSeconds) {
    _saveDefaults();
    std::vector<char> v;

    TerminalSetting original = now();
    if (!original.valid()) return v;

    termios setting = *(termios*)original.p; // copy
    setting.c_lflag &= ~ICANON;
    setting.c_cc[VMIN] = blockUntilMin;
    setting.c_cc[VTIME] = burstDeciSeconds;
    if (tcsetattr(0, TCSANOW, &setting) < 0) return v;
    v.resize(max);
    const int n = (int)read(0, &v[0], max);
    if (n != max) v.resize(n < 0 ? 0 : n);

    restore(original);
    return v;
}

std::vector<char> TerminalCtrl::getCharsToDelimiter(char delimiter, bool includeDelimiter) {
    _saveDefaults();
    std::vector<char> result;
    while (true) {
        std::vector<char> v = getChars(1, 1);
        if (v.empty()) break;
        if (v[0] == delimiter && !includeDelimiter) break;
        result.push_back(v[0]);
        if (v[0] == delimiter) break;
    }
    return result;
}

std::string TerminalCtrl::getStringToDelimiter(char delimiter, bool includeDelimiter) {
    _saveDefaults();
    std::string s;
    std::vector<char> v = getCharsToDelimiter(delimiter, includeDelimiter);
    if (v.empty()) return s;
    v.push_back(0);
    int n = (int)strlen(&v[0]);
    s.resize(n);
    memcpy(&s[0], &v[0], n);
    return s;
}

int TerminalCtrl::columns() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return w.ws_col;
}

int TerminalCtrl::rows() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return w.ws_row;
}
