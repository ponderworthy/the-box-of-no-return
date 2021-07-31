#ifndef __LS_MUTEXTEST_H__
#define __LS_MUTEXTEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// needed for sleep() and usleep() calls
#include <unistd.h>

// the Mutex class we want to test
#include "../common/Mutex.h"

// we need an additional thread to test the Mutex class
#include "../common/Thread.h"

using namespace LinuxSampler;

class MutexTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(MutexTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testLockAndUnlockBySingleThread);
    CPPUNIT_TEST(testLock);
    CPPUNIT_TEST(testUnlock);
    CPPUNIT_TEST(testDoubleLock);
    CPPUNIT_TEST(testDoubleLockStillBlocksConcurrentThread);
    CPPUNIT_TEST_SUITE_END();

    private:
        // dummy thread to simulate usage of a shared resource
        class ConcurrentThread : public Thread {
            public:
                int resource;
                Mutex mutex;

                ConcurrentThread();
                int Main();
        };

        // dummy thread we use to check if the Mutex falsely blcoks if we do a double lock and to avoid that our unit test main thread gets blocked
        class DummyThread : public Thread {
            public:
                int resource;

                DummyThread();
                int Main();
            private:
                Mutex mutex;
        };

        bool doubleLockSucceeded; // true if testDoubleLock() was successful
    public:
        void setUp();
        void tearDown();

        void printTestSuiteName();

        void testLockAndUnlockBySingleThread();
        void testLock();
        void testUnlock();
        void testDoubleLock();
        void testDoubleLockStillBlocksConcurrentThread();
};

#endif // __LS_MUTEXTEST_H__
