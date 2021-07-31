/*
 * LSCP Shell
 *
 * Copyright (c) 2014 - 2016 Christian Schoenebeck
 *
 * This program is part of LinuxSampler and released under the same terms.
 */

#include "LSCPClient.h"
#include "lscp.h"
#include <strings.h>

/**
 * This function is called by the LSCPClient receiving thread to identify
 * whether the received line is a) just a single line response or b) the first
 * line of a multi line response. LSCPClient will provide this information via
 * its multiLine() and messageComplete() methods, which are accessed by the
 * application's main thread to find out whether it should wait until all lines
 * of the multi line message were completely received. This strategy (to let
 * the main app thread wait until a multi line message is really entirely
 * received) is preferable since it would otherwise require a very complex code
 * with a bunch of state variables in the main apps loop, which would also make
 * it very error prone.
 *
 * @param line - input: the input line to check
 * @param skipLine - output: whether @a line shall be silently ignored
 * @returns true if @a line indicates a multi line response
 */
static bool _lscpLineIndicatesMultiLineResponse(const String& line, bool& skipLine) {
    static const String multiLineKey = LSCP_SHK_EXPECT_MULTI_LINE;
    static const String lscpRefDocKey = "SHD:1";
    if (line.substr(0, multiLineKey.length()) == multiLineKey) {
        skipLine = true;
        return true;
    }
    if (line.substr(0, lscpRefDocKey.length()) == lscpRefDocKey) {
        skipLine = false;
        return true;
    }
    skipLine = false;
    return false;
}

/** @brief Default constructor.
 *
 * Initializes a yet unconnected LSCP client. You need to call connect()
 * afterwards to actually let it connect to some LSCP server for being able to
 * use the client for something useful.
 */
LSCPClient::LSCPClient() :
    Thread(false, false, 1, -1),
    hSocket(-1), m_callback(NULL), m_errorCallback(NULL),
    m_multiLineExpected(false)
{
}

/** @brief Destructor.
 *
 * Disconnects this client automatically if it is currently still connected to
 * some LSCP server.
 */
LSCPClient::~LSCPClient() {
    disconnect();
}

/**
 * Let this client connect to a (already running) LSCP server listening on
 * host name @a host and TCP port number @a port. If a connection was
 * successfully established, this method will also start a thread which will
 * constantly wait for receiving new data sent by the LSCP server. This thread
 * will store all received data in an internal buffer which can then be pulled
 * out with popLine().
 *
 * @param host - host name of LSCP server
 * @param port - TCP port number of LSCP server
 * @returns true if connection was successful, false on errors.
 */
bool LSCPClient::connect(String host, int port) {
    m_lineBuffer.clear();
    m_lines.clear();
    // resolve given host name
    hostent* server = ::gethostbyname(host.c_str());
    if (!server) {
        std::cerr << "Error: Could not resolve host \"" << host << "\".\n";
        return false;
    }
    // create local TCP socket
    hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket < 0) {
        std::cerr << "Error: Could not create local socket.\n";
        return false;
    }
    // TCP connect to server
    sockaddr_in addr;
    bzero((char*)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&addr.sin_addr.s_addr, server->h_length);
    addr.sin_port = htons(port);
    if (::connect(hSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: Could not connect to host \"" << host << "\".\n";
        std::cerr << "Is linuxsampler running and listening on port " << port << " ?\n";
        disconnect();
        return false;
    }
    StartThread();
    return true; // success
}

/**
 * Disconnect this client from the currently connected LSCP server. If there
 * is no connection, then this method does nothing.
 */
void LSCPClient::disconnect() {
    if (hSocket >= 0) {
        StopThread();
        ::close(hSocket);
        hSocket = -1;
    }
}

/**
 * Returns true if this LSCP client is currently connected to a LSCP server.
 */
bool LSCPClient::isConnected() const {
    return hSocket >= 0;
}

/**
 * Send one byte given by @a c to the connected LSCP server asynchronously.
 *
 * @param c - byte (character) to be sent
 */
bool LSCPClient::send(char c) {
    String s;
    s += c;
    return send(s);
}

/**
 * Send a sequence of bytes given by @a s to the connected LSCP server
 * asynchronously.
 *
 * @param s - sequence of bytes (string) to be sent
 */
bool LSCPClient::send(String s) {
    //lscpLog("[send] '%s'\n", s.c_str());
    if (!isConnected()) return false;
    int n = (int) ::write(hSocket, &s[0], s.size());
    return n == s.size();
}

/**
 * Send a sequence of bytes given by @a s to the connected LSCP server
 * synchronously. This method will then block until a response line was
 * received from the LSCP server and will return that response line as result
 * to this method. Since this currently assumes a single line response, it
 * should just be used for sending LSCP commands which will exepct a single
 * line response from the LSCP server.
 *
 * @param s - sequence of bytes (string) to be sent
 * @returns response of LSCP server
 */
String LSCPClient::sendCommandSync(String s) {
    m_linesMutex.Lock();
    m_lines.clear();
    m_linesMutex.Unlock();

    m_sync.Set(true);
    if (!send(s + "\n")) return "";
    m_sync.WaitIf(true);

    m_linesMutex.Lock();
    String sResponse = m_lines.back().data;
    m_lines.clear();
    m_linesMutex.Unlock();

    return sResponse;
}

/**
 * Pulls out and returns the next line received from the LSCP server. If the
 * return value of this method evaluates to false, then there is currently no
 * new line received. The line returned will be removed from the internal
 * receive buffer. If that's not what you want, then use lookAheadLine()
 * instead.
 */
optional<String> LSCPClient::popLine() {
    String s;
    LockGuard guard(m_linesMutex);
    if (m_lines.empty()) return optional<String>::nothing;
    s = m_lines.front().data;
    m_lines.pop_front();
    return s;
}

/**
 * Use this method instead of popLine() in case you want to access received
 * line(s) without removing them from the internal buffer.
 *
 * @param index - offset of line to be accessed (0 being the oldest line
 *                received)
 */
optional<String> LSCPClient::lookAheadLine(int index) {
    LockGuard guard(m_linesMutex);
    if (index < 0 || index >= m_lines.size())
        return optional<String>::nothing;
    std::list<Line>::iterator it = m_lines.begin();
    for (int i = 0; i < index; ++i) ++it;
    String s = (*it).data;
    return s;
}

/**
 * Returns true if the received line(s), waiting to be pulled out with
 * popLine(), is/are completely received. So if the next line waiting to be
 * pulled out with popLine() is a single line response, then this function
 * returns true. If the next line waiting to be pulled out with popLine() is
 * considered a part of multi line response though, this function only returns
 * true if all lines of that expected multi line response were entirely
 * received already.
 *
 * @see multiLine()
 */
bool LSCPClient::messageComplete() {
    LockGuard guard(m_linesMutex);
    if (m_lines.empty()) {
        //lscpLog("\t[messageComplete false - buffer empty]\n");
        return false;
    }
    if (!m_lines.front().isMultiLine) {
        //lscpLog("\t[messageComplete true - single line]\n");
        return true;
    }

//     // just for debugging purposes ...
//     int k = 0;
//     for (std::list<Line>::const_iterator it = m_lines.begin();
//          it != m_lines.end(); ++it, ++k)
//     {
//         lscpLog("\t[messageComplete k=%d : mul=%d cmpl=%d data='%s']\n", k, (*it).isMultiLine, (*it).isMultiLineComplete, (*it).data.c_str());
//     }

    // so next line is part of a multi line response, check if it's complete ...
    for (std::list<Line>::const_iterator it = m_lines.begin();
         it != m_lines.end(); ++it)
    {
        if ((*it).isMultiLineComplete) return true;
    }
    return false;
}

/**
 * Returns true if the next line to be pulled out with popLine() is considered
 * a single line response, it returns false if the next line is a part of a
 * multi line response instead. In the latter case you should call multiLine()
 * to check whether all lines of that multi line response were already received
 * before pulling them out with popLine().
 *
 * @see messageComplete()
 */
bool LSCPClient::multiLine() {
    LockGuard guard(m_linesMutex);
    if (m_lines.empty()) return false;
    bool bIsMultiLine = m_lines.front().isMultiLine;
    return bIsMultiLine;
}

/**
 * Returns true if new line(s) were received from LSCP server, waiting to be
 * pulled out with popLine().
 */
bool LSCPClient::lineAvailable() const {
    return !m_lines.empty(); // is thread safe
}

/**
 * You may call this method to register a callback function which shall be
 * notified if new data was received from the connected LSCP server.
 */
void LSCPClient::setCallback(Callback_t fn) {
    m_callback = fn;
}

/**
 * You may call this method to register a callback function which shall be
 * notified if some kind of network error occurred.
 */
void LSCPClient::setErrorCallback(Callback_t fn) {
    m_errorCallback = fn;
}

/**
 * This method is only called by the internal client thread: a call to this
 * method blocks until a complete new line was received from the LSCP server.
 */
optional<String> LSCPClient::receiveLine() {
    if (!isConnected()) return optional<String>::nothing;
    for (char c; true; ) {
        int n = (int) ::read(hSocket, &c, 1);
        if (n < 1) return optional<String>::nothing;
        if (c == '\r') continue;
        if (c == '\n') {
            String s = m_lineBuffer;
            m_lineBuffer.clear();
            return s;
        }
        //printf("->%c\n", c);
        m_lineBuffer += c;
    }
    return optional<String>::nothing;
}

/**
 * Client's internal receiving thread's loop. It does nothing else than waiting
 * for new data sent by LSCP server and puts the received lines into an
 * internal FIFO buffer.
 */
int LSCPClient::Main() {
    while (true) {
        optional<String> pLine = receiveLine();
        if (pLine) { // if there was a line received ...
            //lscpLog("[client receiveLine] '%s'\n", pLine->c_str());
            String s = *pLine;

            // check whether this is a multi line data ...
            bool bSkipLine = false;
            bool bMultiLineComplete = false;
            if (!m_multiLineExpected && _lscpLineIndicatesMultiLineResponse(s, bSkipLine)) {
                m_multiLineExpected = true;
            } else if (m_multiLineExpected && s.substr(0, 1) == ".") {
                bMultiLineComplete = true;
            }

            // store received line in internal FIFO buffer
            if (!bSkipLine) {
                Line l = { s, m_multiLineExpected, bMultiLineComplete };
                m_linesMutex.Lock();
                m_lines.push_back(l);
                m_linesMutex.Unlock();
            }

            // if this was the last line of a multi line response, reset
            // for next run
            if (m_multiLineExpected && bMultiLineComplete) {
                m_multiLineExpected = false;
            }

            if (m_sync.GetUnsafe()) m_sync.Set(false);
            else if (m_callback) (*m_callback)(this);
        } else if (m_errorCallback) (*m_errorCallback)(this);
        TestCancel();
    }
    return 0; // just to avoid a warning with some old compilers
}
