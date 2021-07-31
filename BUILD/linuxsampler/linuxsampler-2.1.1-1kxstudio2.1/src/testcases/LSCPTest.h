#ifndef __LS_LSCPTEST_H__
#define __LS_LSCPTEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <string>
#include <vector>

#include "../Sampler.h"
#include "../network/lscpserver.h"

using namespace std;
using namespace LinuxSampler;

class LSCPTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(LSCPTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testLaunchLSCPServer);
    CPPUNIT_TEST(testConnectToLSCPServer);
    CPPUNIT_TEST(test_ADD_CHANNEL);
    CPPUNIT_TEST(test_GET_CHANNELS);
    CPPUNIT_TEST(test_REMOVE_CHANNEL);
    CPPUNIT_TEST(test_GET_AUDIO_OUTPUT_CHANNEL_PARAMETER_INFO);
    CPPUNIT_TEST(test_SET_ECHO);
    CPPUNIT_TEST(testShutdownLSCPServer);
    CPPUNIT_TEST_SUITE_END();

    private:
        bool launchLSCPServer();
        bool shutdownLSCPServer();

        bool connectToLSCPServer();
        bool closeConnectionToLSCPServer();

        void sendCommandToLSCPServer(string cmd);
        string receiveSingleLineAnswerFromLSCPServer(uint timeout_seconds = 0) throw (Exception);
        vector<string> receiveMultiLineAnswerFromLSCPServer(uint timeout_seconds = 0) throw (Exception);
        string receiveAnswerFromLSCPServer(string delimiter, uint timeout_seconds = 0) throw (Exception);
        void clearInputBuffer();
    public:
        void setUp();
        void tearDown();

        void printTestSuiteName();

        void testLaunchLSCPServer();
        void testConnectToLSCPServer();
        void test_ADD_CHANNEL();
        void test_GET_CHANNELS();
        void test_REMOVE_CHANNEL();
        void test_GET_AUDIO_OUTPUT_CHANNEL_PARAMETER_INFO();
        void test_SET_ECHO();
        void testShutdownLSCPServer();
};

#endif // __LS_LSCPTEST_H__
