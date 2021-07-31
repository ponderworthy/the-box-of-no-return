/*
 * LSCP Shell
 *
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#ifndef LSCPCLIENT_H
#define LSCPCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>

#if defined(WIN32)
#include <windows.h>
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "../common/global.h"
#include "../common/Thread.h"
#include "../common/Mutex.h"
#include "../common/Condition.h"
#include "../common/optional.h"
#include "../network/lscp.h"

using namespace LinuxSampler;

/**
 * Implements network communication with LinuxSampler's LSCP server. A separate
 * thread is spawned which will constantly read on new incoming data coming from
 * the LSCP server, it can call a callback function whenever new response data
 * arrived.
 */
class LSCPClient : protected Thread {
public:
    typedef void(*Callback_t)(LSCPClient*);

    LSCPClient();
    virtual ~LSCPClient();
    bool connect(String host, int port);
    void disconnect();
    bool isConnected() const;
    bool send(char c);
    bool send(String s);
    String sendCommandSync(String s);
    bool lineAvailable() const;
    bool messageComplete();
    bool multiLine();
    optional<String> lookAheadLine(int index);
    optional<String> popLine();
    void setCallback(Callback_t fn);
    void setErrorCallback(Callback_t fn);
protected:
    int Main() OVERRIDE;
    optional<String> receiveLine();
private:
    struct Line {
        String data;
        bool isMultiLine;
        bool isMultiLineComplete;
    };
    int hSocket;
    String m_lineBuffer;
    std::list<Line> m_lines;
    Mutex m_linesMutex;
    Callback_t m_callback;
    Callback_t m_errorCallback;
    Condition m_sync;
    bool m_multiLineExpected;
};

#endif // LSCPCLIENT_H
