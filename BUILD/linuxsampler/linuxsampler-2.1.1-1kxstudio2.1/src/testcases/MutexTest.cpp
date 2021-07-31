#include "MutexTest.h"

#include <iostream>

CPPUNIT_TEST_SUITE_REGISTRATION(MutexTest);

using namespace std;


// ConcurrentThread

MutexTest::ConcurrentThread::ConcurrentThread() : Thread(false, false, 0, -4) {
    resource = 0;
}

int MutexTest::ConcurrentThread::Main() {
    mutex.Lock();
    resource++;
    mutex.Unlock();
}


// DummyThread

MutexTest::DummyThread::DummyThread() : Thread(false, false, 0, -4) {
    resource = 0;
}

int MutexTest::DummyThread::Main() {
    mutex.Lock();
    mutex.Lock();
    resource++;
    mutex.Unlock();
}


// MutexTest

void MutexTest::printTestSuiteName() {
    cout << "\b \nRunning Mutex Tests: " << flush;
}

void MutexTest::setUp() {
}

void MutexTest::tearDown() {
}

// Check with only one thread (thus no concurrency) if locking and unlocking the Mutex works without getting the thread to be locked falsely.
void MutexTest::testLockAndUnlockBySingleThread() {
    ConcurrentThread t;
    t.StartThread();
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(t.resource == 1);
}

// Now check with two concurrent threads if one thread can block the other one by using the Mutex.
void MutexTest::testLock() {
    ConcurrentThread t;
    t.mutex.Lock();
    t.SignalStartThread();
    usleep(400000); // wait 400ms
    CPPUNIT_ASSERT(t.resource == 0);
    t.mutex.Unlock();
}

// Now check if the blocked thread get unblocked when thread that owns the Mutex unlocks the Mutex again.
void MutexTest::testUnlock() {
    ConcurrentThread t;
    t.mutex.Lock();
    t.SignalStartThread();
    usleep(400000); // wait 400ms
    t.mutex.Unlock();
    usleep(400000); // wait 400ms
    CPPUNIT_ASSERT(t.resource == 1);
}

// Check if the same thread can lock the Mutex twice in a row without unlocking it and without getting blocked itself.
void MutexTest::testDoubleLock() {
    DummyThread t;
    t.SignalStartThread();
    usleep(200000); // wait 200ms
    CPPUNIT_ASSERT(t.resource == 1);
    doubleLockSucceeded = (t.resource == 1);
}

// Check if the previous tests namely 'testLock()' and 'testUnlock()' still work with double locking previously
void MutexTest::testDoubleLockStillBlocksConcurrentThread() {
  bool success = false;
  // check if testDoubleLock() succeeds, otherwise this test doesnt make sense anyway
  doubleLockSucceeded = false;
  testDoubleLock();
  if (doubleLockSucceeded) {
      ConcurrentThread t;
      t.mutex.Lock();
      t.mutex.Lock();
      t.SignalStartThread();
      usleep(400000); // wait 400ms
      success = (t.resource == 0);
      t.mutex.Unlock();

      if (success) {
          usleep(200000); // wait 200ms
          success = (t.resource == 1);
      }
  }
  CPPUNIT_ASSERT(success);
}
