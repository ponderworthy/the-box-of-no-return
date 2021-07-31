#ifndef __LS_THREADTEST_H__
#define __LS_THREADTEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// needed for sleep() and usleep() calls
#include <unistd.h>

// the Thread class we want to test
#include "../common/Thread.h"

// we need that to check if our thread is killable even if waiting for a condition
#include "../common/Condition.h"

class ThreadTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(ThreadTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testThreadRunning);
    CPPUNIT_TEST(testSignalStopThread);
    CPPUNIT_TEST(testRelaunchThread);
    CPPUNIT_TEST(testStopThread);
    CPPUNIT_TEST(testThreadKillableWhenWaiting);
    CPPUNIT_TEST_SUITE_END();

    public:
        // this is our test implementation of the abstract base class Thread which we will use to test class Thread
        class DummyThread : public LinuxSampler::Thread {
            public:
                bool wasRunning; // this variable is false on startup and will be switched to true by the thread, so we can check if the thread actually runs
                volatile int someVariable; // we constantly set this variable to -1 in the DummyThread Main() loop, so we can check in our main test thread if the DummyThread is still running by changing that value to something else than -1

                DummyThread();
                int Main();
        };

    private:
        // this is only a helper thread to avoid that the entire test case run hangs when we try to stop the dummy thread
        class HelperThread : public LinuxSampler::Thread {
            private:
                DummyThread* pDummyThread;
            public:
                bool returnedFromDummyStop;

                HelperThread(DummyThread* pDummyThread);
                int Main();
                bool dummyThreadWasNotRunningAnymoreAfter_StopThread_call();
        };

        // this simulates a thread that is waiting for a condition (thus sleeping til then)
        class WaitingThread : public LinuxSampler::Thread {
            public:
                WaitingThread();
                int Main();
            private:
                LinuxSampler::Condition condition;
        };
    public:
        void setUp() {
        }

        void tearDown() {
        }

        void printTestSuiteName();

        void testThreadRunning();
        void testSignalStopThread();
        void testRelaunchThread();
        void testStopThread();
        void testThreadKillableWhenWaiting();
};

#endif // __LS_THREADTEST_H__
