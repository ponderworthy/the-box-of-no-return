#ifndef __LS_CONDITIONTEST_H__
#define __LS_CONDITIONTEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// needed for sleep() and usleep() calls
#include <unistd.h>

// the Condition class we want to test
#include "../common/Condition.h"

// we need an additional thread to test the Mutex class
#include "../common/Thread.h"

using namespace LinuxSampler;

class ConditionTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(ConditionTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testDoesntBlockOnDesiredCondtion);
    CPPUNIT_TEST(testBlocksIfNotDesiredCondition);
    CPPUNIT_TEST(testBlocksUntilDesiredCondition);
    CPPUNIT_TEST(testWaitIFBlocksConcurrentThread);
    CPPUNIT_TEST_SUITE_END();

    private:
        // simulates a thread waiting for a condition
        class ConditionChecker : public Thread {
            public:
                int resource;
                Condition condition;

                ConditionChecker(bool waitFor);
                int Main();
            protected:
                bool waitFor;
        };

        // simulates a thread which sets the condition
        class ConditionSetter : public Thread {
            public:
                int resource;
                Condition* condition;

                ConditionSetter(Condition* condition, bool toSet);
                int Main();
            private:
                bool toSet;
        };

        // same as 'ConditionChecker' except that this one uses WaitIf() instead of WaitAndUnlockIf()
        class ConditionCheckerLocking : public ConditionChecker {
            public:
                bool doUnlock;
                static Condition staticcondition;

                ConditionCheckerLocking(bool waitFor);
                int Main();
        };
    public:
        void setUp();
        void tearDown();

        void printTestSuiteName();

        void testDoesntBlockOnDesiredCondtion();
        void testBlocksIfNotDesiredCondition();
        void testBlocksUntilDesiredCondition();
        void testWaitIFBlocksConcurrentThread();
};

#endif // __LS_CONDITIONTEST_H__
