#include "LSCPTest.h"

#include "../common/global_private.h"
#include "../common/optional.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

CPPUNIT_TEST_SUITE_REGISTRATION(LSCPTest);

// Note:
// we have to declare all those variables which we want to use for all
// tests within this test suite static because there are side effects which
// occur on transition to the next test which would change the values of our
// variables
static Sampler*    pSampler    = NULL;
static LSCPServer* pLSCPServer = NULL;
static int         hSocket     = -1;

/// Returns first token from \a sentence and removes that token (and evtl. delimiter) from \a sentence.
static optional<string> __ExtractFirstToken(string* pSentence, const string Delimiter) {
    if (*pSentence == "") return optional<string>::nothing;

    string::size_type pos = pSentence->find(Delimiter);

    // if sentence has only one token
    if (pos == string::npos) {
        string token = *pSentence;
        *pSentence = "";
        return token;
    }

    // sentence has more than one token, so extract the first token
    string token = pSentence->substr(0, pos);
    *pSentence   = pSentence->replace(0, pos + 1, "");
    return token;
}

// split the multi line response string into the individual lines and remove the last (delimiter) line and the line feed characters in all lines
static vector<string> __ConvertMultiLineMessage(string msg) {
    vector<string> res;

    // erase the (dot) delimiter line
    static const string dotlinedelimiter = ".\r\n";
    string::size_type pos = msg.rfind(dotlinedelimiter);
    msg = msg.replace(pos, dotlinedelimiter.length(), "");

    // now split the lines
    static const string linedelimiter = "\r\n";
    while (true) {
        pos = msg.find(linedelimiter, 0);

        if (pos == string::npos) break; // if we're done

        // get the line without the line feed and put it at the end of the vector
        string line = msg.substr(0, pos);
        res.push_back(line);

        // remove the line from the input string
        pos += linedelimiter.length();
        msg = msg.substr(pos, msg.length() - pos);
    }

    return res;
}


// LSCPTest

// returns false if the server could not be launched
bool LSCPTest::launchLSCPServer() {
    const long timeout_seconds = 10; // we give the server max. 10 seconds to startup, otherwise we declare the startup as failed
    try {
        pSampler    = new Sampler;
        pLSCPServer = new LSCPServer(pSampler, htonl(LSCP_ADDR), htons(LSCP_PORT));
        pLSCPServer->StartThread();
        int res = pLSCPServer->WaitUntilInitialized(timeout_seconds);
        if (res < 0) throw;

        return true; // success
    }
    catch (...) {
        pSampler    = NULL;
        pLSCPServer = NULL;
        return false; // failure
    }
}

// returns false if the server could not be destroyed without problems
bool LSCPTest::shutdownLSCPServer() {
    try {
        pLSCPServer->StopThread();
        if (pLSCPServer) {
            delete pLSCPServer;
            pLSCPServer = NULL;
        }
        if (pSampler) {
            delete pSampler;
            pSampler = NULL;
        }
        return true; // success
    }
    catch (...) {
        return false; // failure
    }
}

// returns false if client connection to the LSCP server could not be established
bool LSCPTest::connectToLSCPServer() {
    const int iPort = LSCP_PORT; // LSCP server listening port (from lscpserver.h)
    hSocket = -1;

    hostent* pHost = gethostbyname("localhost");
    if (pHost == NULL) return false;

    hSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (hSocket < 0) return false;

    sockaddr_in addr;
    memset((char*) &addr, 0, sizeof(sockaddr_in));
    addr.sin_family = pHost->h_addrtype;
    memmove((char*) &(addr.sin_addr), pHost->h_addr, pHost->h_length);
    addr.sin_port = htons((short) iPort);

    if (connect(hSocket, (sockaddr*) &addr, sizeof(sockaddr_in)) < 0) {
        close(hSocket);
        return false;
    }

    // set non-blocking mode
    int socket_flags = fcntl(hSocket, F_GETFL, 0);
    fcntl(hSocket, F_SETFL, socket_flags | O_NONBLOCK);

    return true;
}

bool LSCPTest::closeConnectionToLSCPServer() {
    //cout << "closeConnectionToLSCPServer()\n" << flush;
    if (hSocket >= 0) {
        close(hSocket);
        hSocket = -1;
    }
    return true;
}

// send a command to the LSCP server
void LSCPTest::sendCommandToLSCPServer(string cmd) {
    if (hSocket < 0) {
        cout << "sendCommandToLSCPServer() error: client socket not ready\n" << flush;
        return;
    }
    cmd += "\r\n";
    send(hSocket, cmd.c_str(), cmd.length(), 0);
}

// wait until LSCP server answers with a single line answer (throws LinuxSampler::Exception if optional timeout exceeded)
string LSCPTest::receiveSingleLineAnswerFromLSCPServer(uint timeout_seconds) throw (Exception) {
    string msg = receiveAnswerFromLSCPServer("\n", timeout_seconds);
    // remove carriage return characters
    string::size_type p = msg.find('\r');
    for (; p != string::npos; p = msg.find(p, '\r')) msg.erase(p, 1);
    // remove the line feed at the end
    static const string linedelimiter = "\n";
    string::size_type pos = msg.rfind(linedelimiter);
    return msg.substr(0, pos);
}

/// wait until LSCP server answers with a multi line answer (throws LinuxSampler::Exception if optional timeout exceeded)
vector<string> LSCPTest::receiveMultiLineAnswerFromLSCPServer(uint timeout_seconds) throw (Exception) {
    string msg = receiveAnswerFromLSCPServer("\n.\r\n", timeout_seconds);
    return __ConvertMultiLineMessage(msg);
}

void LSCPTest::clearInputBuffer() {
    char c;
    while (recv(hSocket, &c, 1, 0) > 0);
}

/// wait until LSCP server answers with the given \a delimiter token at the end (throws LinuxSampler::Exception if optional timeout exceeded or socket error occured)
string LSCPTest::receiveAnswerFromLSCPServer(string delimiter, uint timeout_seconds) throw (Exception) {
    string message;
    char c;
    fd_set sockSet;
    timeval timeout;

    while (true) {
        if (timeout_seconds) {
            FD_ZERO(&sockSet);
            FD_SET(hSocket, &sockSet);
            timeout.tv_sec  = timeout_seconds;
            timeout.tv_usec = 0;
            int res = select(hSocket + 1, &sockSet, NULL, NULL, &timeout);
            if (!res) { // if timeout exceeded
                if (!message.size()) throw Exception("LSCPTest::receiveAnswerFromLSCPServer(): timeout (" + ToString(timeout_seconds) + "s) exceeded, no answer received");
                else                 throw Exception("LSCPTest::receiveAnswerFromLSCPServer(): timeout (" + ToString(timeout_seconds) + "s) exceeded waiting for expected answer (end), received answer: \'" + message + "\'");
            }
            else if (res < 0) {
                throw Exception("LSCPTest::receiveAnswerFromLSCPServer(): select error");
            }
        }

        // there's something to read, so read one character
        int res = recv(hSocket, &c, 1, 0);
        if (!res) throw Exception("LSCPTest::receiveAnswerFromLSCPServer(): connection to LSCP server closed");
        else if (res < 0) {
            switch(errno) {
                case EBADF:
                    throw Exception("The argument s is an invalid descriptor");
                case ECONNREFUSED:
                    throw Exception("A remote host refused to allow the network connection (typically because it is not running the requested service).");
                case ENOTCONN:
                    throw Exception("The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).");
                case ENOTSOCK:
                    throw Exception("The argument s does not refer to a socket.");
                case EAGAIN:
                    continue;
                    //throw Exception("The socket is marked non-blocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.");
                case EINTR:
                    throw Exception("The receive was interrupted by delivery of a signal before any data were available.");
                case EFAULT:
                    throw Exception("The receive buffer pointer(s) point outside the process's address space.");
                case EINVAL:
                    throw Exception("Invalid argument passed.");
                case ENOMEM:
                    throw Exception("Could not allocate memory for recvmsg.");
                default:
                    throw Exception("Unknown recv() error.");
            }
        }

        message += c;
        string::size_type pos = message.rfind(delimiter); // ouch, but this is only a test case, right? ;)
        if (pos != string::npos) return message;
    }
}



void LSCPTest::printTestSuiteName() {
    cout << "\b \nRunning LSCP Tests: " << flush;
}

void LSCPTest::setUp() {
}

void LSCPTest::tearDown() {
    clearInputBuffer(); // to avoid that the next test reads an answer from a previous test
}



// Check if we can launch the LSCP Server (implies that there's no other instance running at the moment).
void LSCPTest::testLaunchLSCPServer() {
    //cout << "testLaunchLSCPServer()\n" << flush;
    CPPUNIT_ASSERT(launchLSCPServer());
}

// Check if we can connect a client connection to the LSCP server and close that connection without problems.
void LSCPTest::testConnectToLSCPServer() {
    //cout << "testConnectToLSCPServer()\n" << flush;
    sleep(1); // wait 1s
    CPPUNIT_ASSERT(connectToLSCPServer());
    sleep(2); // wait 2s
    CPPUNIT_ASSERT(closeConnectionToLSCPServer());
}

// Check "ADD CHANNEL" LSCP command.
void LSCPTest::test_ADD_CHANNEL() {
    sleep(1); // wait 1s
    CPPUNIT_ASSERT(connectToLSCPServer());

    sendCommandToLSCPServer("ADD CHANNEL");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "OK[0]");

    sendCommandToLSCPServer("ADD CHANNEL");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "OK[1]");

    sendCommandToLSCPServer("ADD CHANNEL");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "OK[2]");
}

// Check "GET CHANNELS" LSCP command.
void LSCPTest::test_GET_CHANNELS() {
    sendCommandToLSCPServer("GET CHANNELS");
    string answer = receiveSingleLineAnswerFromLSCPServer();
    int initial_channels = atoi(answer.c_str());

    // add sampler channels and check if the count increases
    for (uint trial = 1; trial <= 3; trial++) {
        sendCommandToLSCPServer("ADD CHANNEL");
        answer = receiveSingleLineAnswerFromLSCPServer();
        sendCommandToLSCPServer("GET CHANNELS");
        answer = receiveSingleLineAnswerFromLSCPServer();
        int channels = atoi(answer.c_str());
        CPPUNIT_ASSERT(channels == initial_channels + trial);
    }
}

// Check "REMOVE CHANNEL" LSCP command.
void LSCPTest::test_REMOVE_CHANNEL() {
    // how many channels do we have at the moment?
    sendCommandToLSCPServer("GET CHANNELS");
    string answer = receiveSingleLineAnswerFromLSCPServer();
    int initial_channels = atoi(answer.c_str());

    // if there are no sampler channels yet, create some
    if (!initial_channels) {
        const uint create_channels = 4;
        for (uint i = 0; i < create_channels; i++) {
            sendCommandToLSCPServer("ADD CHANNEL");
            answer = receiveSingleLineAnswerFromLSCPServer();
        }
        initial_channels = create_channels;
    }

    // now remove the channels until there is no one left and check if we really need 'initial_channels' times to achieve that
    for (uint channels = initial_channels; channels; channels--) {
        sendCommandToLSCPServer("LIST CHANNELS");
        answer = receiveSingleLineAnswerFromLSCPServer();
        if (answer == "") CPPUNIT_ASSERT(false); // no sampler channel left already? -> failure

        // take the last channel number in the list which we will take to remove that sampler channel
        string::size_type pos = answer.rfind(",");
        string channel_to_remove = (pos != string::npos) ? answer.substr(pos + 1, answer.length() - (pos + 1)) /* "m,n,...,t */
                                                         : answer;                                   /* "k"        */

        //cout << " channel_to_remove: \"" << channel_to_remove << "\"\n" << flush;

        // remove that channel
        sendCommandToLSCPServer("REMOVE CHANNEL " + channel_to_remove);
        answer = receiveSingleLineAnswerFromLSCPServer();
        CPPUNIT_ASSERT(answer == "OK");
    }
    CPPUNIT_ASSERT(true); // success
}

// Check "GET AUDIO_OUTPUT_CHANNEL_PARAMETER INFO" LSCP command.
void LSCPTest::test_GET_AUDIO_OUTPUT_CHANNEL_PARAMETER_INFO() {
    // first check if there's already an audio output device created
    sendCommandToLSCPServer("GET AUDIO_OUTPUT_DEVICES");
    string answer = receiveSingleLineAnswerFromLSCPServer();
    int devices   = atoi(answer.c_str());
    CPPUNIT_ASSERT(devices >= 0);
    if (!devices) { // if there's no audio output device yet, try to create one
        sendCommandToLSCPServer("LIST AVAILABLE_AUDIO_OUTPUT_DRIVERS");
        string drivers = receiveSingleLineAnswerFromLSCPServer();
        CPPUNIT_ASSERT(drivers.size());

        // iterate through all available drivers until device creation was successful
        do {
            optional<string> driver = __ExtractFirstToken(&drivers, ",");
            CPPUNIT_ASSERT(driver);

            sendCommandToLSCPServer("CREATE AUDIO_OUTPUT_DEVICE " + *driver);
            answer = receiveSingleLineAnswerFromLSCPServer(120); // wait 2 minutes for an answer
        } while (answer != "OK[0]");
    }

    // now we can check the "GET AUDIO_OUTPUT_CHANNEL_PARAMETER INFO" command
    const uint timeout_seconds = 2;
    sendCommandToLSCPServer("GET AUDIO_OUTPUT_CHANNEL_PARAMETER INFO 0 0 NAME");
    vector<string> vAnswer = receiveMultiLineAnswerFromLSCPServer(timeout_seconds);
    CPPUNIT_ASSERT(vAnswer.size() >= 4); // should at least contain tags TYPE, DESCRIPTION, FIX and MULTIPLICITY

    sendCommandToLSCPServer("GET AUDIO_OUTPUT_CHANNEL_PARAMETER INFO 0 0 IS_MIX_CHANNEL");
    vAnswer = receiveMultiLineAnswerFromLSCPServer(timeout_seconds);
    CPPUNIT_ASSERT(vAnswer.size() >= 4); // should at least contain tags TYPE, DESCRIPTION, FIX and MULTIPLICITY
}

// Check "SET ECHO" LSCP command.
void LSCPTest::test_SET_ECHO() {
    // enable echo mode
    sendCommandToLSCPServer("SET ECHO 1");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "OK");

    // check if commands will actually be echoed now
    sendCommandToLSCPServer("GET CHANNELS"); // send an arbitrary command
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer(2) == "GET CHANNELS");
    receiveSingleLineAnswerFromLSCPServer(2); // throws exception if no answer received after 2s (usually we expect the answer from our command here)

    // disable echo mode
    sendCommandToLSCPServer("SET ECHO 0");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "SET ECHO 0"); // this will be echoed though
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() == "OK");

    // check if commands will not be echoed now
    sendCommandToLSCPServer("GET CHANNELS");
    CPPUNIT_ASSERT(receiveSingleLineAnswerFromLSCPServer() != "GET CHANNELS");
}

// Check if we can shutdown the LSCP Server without problems.
void LSCPTest::testShutdownLSCPServer() {
    //cout << "testShutdownLSCPServer()\n" << flush;
    sleep(2); // wait 2s
    CPPUNIT_ASSERT(closeConnectionToLSCPServer());
    sleep(3); // wait 3s
    CPPUNIT_ASSERT(shutdownLSCPServer());
}
