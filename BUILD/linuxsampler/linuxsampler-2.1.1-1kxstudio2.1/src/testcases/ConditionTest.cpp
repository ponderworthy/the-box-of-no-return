#include "ConditionTest.h"

#include <iostream>

CPPUNIT_TEST_SUITE_REGISTRATION(ConditionTest);

using namespace std;


// ConditionChecker

ConditionTest::ConditionChecker::ConditionChecker(bool waitFor) : Thread(false, false, 0, -4) {
    resource = 0;
    this->waitFor = waitFor;
}

int ConditionTest::ConditionChecker::Main() {
    condition.WaitAndUnlockIf(!waitFor);
    resource++;
}


// ConditionSetter

ConditionTest::ConditionSetter::ConditionSetter(Condition* condition, bool toSet) : Thread(false, false, 0, -4) {
    resource = 0;
    this->toSet = toSet;
    this->condition = condition;
}

int ConditionTest::ConditionSetter::Main() {
    condition->Set(toSet);
    resource++;
}


// ConditionCheckerLocking

Condition ConditionTest::ConditionCheckerLocking::staticcondition; // we sheare the same Condition object between all instances of ConditionCheckerLocking

ConditionTest::ConditionCheckerLocking::ConditionCheckerLocking(bool waitFor) : ConditionChecker(waitFor) {
    doUnlock = false;
}

int ConditionTest::ConditionCheckerLocking::Main() {
    staticcondition.WaitIf(!waitFor);
    resource++;
    while (!doUnlock) {
		usleep(1000); // sleep until ordered to unlock the condition again
#if CONFIG_PTHREAD_TESTCANCEL
		TestCancel();
#endif
	}
    staticcondition.Unlock();
}


// ConditionTest

void ConditionTest::printTestSuiteName() {
    cout << "\b \nRunning Condition Tests: " << flush;
}

void ConditionTest::setUp() {
}

void ConditionTest::tearDown() {
}


// Check if Condition class doesn't block if desired condition is already reached
void ConditionTest::testDoesntBlockOnDesiredCondtion() {
    ConditionChecker t(false);
    t.SignalStartThread();
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(t.resource == 1);
}

// Check if Condition class blocks if desired condition is not alredy reached
void ConditionTest::testBlocksIfNotDesiredCondition() {
    ConditionChecker t(true);
    t.SignalStartThread();
    usleep(400000); // wait 400ms
    CPPUNIT_ASSERT(t.resource == 0);
}

// Check if Condition class blocks until desired condition is reached
void ConditionTest::testBlocksUntilDesiredCondition() {
    ConditionChecker checker(true); // let that thread wait until condition equals 'true'
    ConditionSetter  setter(&checker.condition, true); // let this thread set the condition to 'true'
    checker.SignalStartThread();
    usleep(50000); // wait 50ms
    CPPUNIT_ASSERT(checker.resource == 0); // check if 'checker' thread has not passed the WaitAndUnlockIf() point yet
    setter.SignalStartThread();
    usleep(100000); // wait 100ms
    CPPUNIT_ASSERT(checker.resource == 1); // check if 'checker' thread passed the WaitAndUnlockIf() point
}

// Check if the WaitIf() call blocks concurrent threads until the first WaitIf() caller calls Unlock()
void ConditionTest::testWaitIFBlocksConcurrentThread() {
    // if the following three tests fail, then it doesn't make sense to continue with the actual one here
    testDoesntBlockOnDesiredCondtion();
    testBlocksIfNotDesiredCondition();
    testBlocksUntilDesiredCondition();

    // actual new test
    ConditionCheckerLocking checker1(false);
    ConditionCheckerLocking checker2(false);
    checker1.SignalStartThread();
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(checker1.resource == 1);
    checker2.SignalStartThread();
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(checker2.resource == 0); // check if condition locked by 'checker1'

    // now order 'checker1' thread to Unlock() the condition again
    checker1.doUnlock = true;
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(checker2.resource == 1); // check if condition was unlocked by 'checker1'
}
