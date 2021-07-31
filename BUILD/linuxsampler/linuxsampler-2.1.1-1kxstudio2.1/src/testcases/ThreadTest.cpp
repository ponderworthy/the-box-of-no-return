#include "ThreadTest.h"

#include <iostream>

CPPUNIT_TEST_SUITE_REGISTRATION(ThreadTest);

using namespace std;

static ThreadTest::DummyThread dummythread;


// DummyThread

ThreadTest::DummyThread::DummyThread() : LinuxSampler::Thread(false, false, 0, -4) {
    wasRunning = false;
}

int ThreadTest::DummyThread::Main() {
    wasRunning = true;
    while (true) {
		someVariable = -1;
#if CONFIG_PTHREAD_TESTCANCEL
		TestCancel();
#endif
	}
}


// HelperThread

ThreadTest::HelperThread::HelperThread(DummyThread* pDummyThread) : LinuxSampler::Thread(false, false, 0, -4) {
    returnedFromDummyStop = false;
    this->pDummyThread = pDummyThread;
}

int ThreadTest::HelperThread::Main() {
    pDummyThread->StopThread();
    pDummyThread->someVariable = 0; // we set this to another value than -1 so we can check if the DummyThread was still running
    returnedFromDummyStop = true;
}

bool ThreadTest::HelperThread::dummyThreadWasNotRunningAnymoreAfter_StopThread_call() {
    return (pDummyThread->someVariable == 0);
}


// WaitingThread

ThreadTest::WaitingThread::WaitingThread() : LinuxSampler::Thread(false, false, 0, -4) {
}

int ThreadTest::WaitingThread::Main() {
    while (true) condition.WaitIf(false);
}


// ThreadTest

void ThreadTest::printTestSuiteName() {
    cout << "\b \nRunning Thread Tests: " << flush;
}

// Check if Thread class actually deploys a thread
void ThreadTest::testThreadRunning() {
    dummythread.StartThread();
    usleep(25000); // wait 25ms
    CPPUNIT_ASSERT(dummythread.wasRunning);
    CPPUNIT_ASSERT(dummythread.IsRunning());
}

// Check if SignalStopThread() method actually stops the thread
void ThreadTest::testSignalStopThread() {
    CPPUNIT_ASSERT(dummythread.wasRunning);
    CPPUNIT_ASSERT(dummythread.IsRunning());
    dummythread.SignalStopThread();
    usleep(80000); // wait 40ms
    CPPUNIT_ASSERT(!dummythread.IsRunning());
}

// Check if the thread can be relaunched
void ThreadTest::testRelaunchThread() {
    dummythread.StartThread();
    usleep(25000); // wait 25ms
    CPPUNIT_ASSERT(dummythread.wasRunning);
    CPPUNIT_ASSERT(dummythread.IsRunning());
    dummythread.StopThread();
    usleep(25000); // wait 25ms
    dummythread.wasRunning = false;
    dummythread.StartThread();
    usleep(25000); // wait 25ms
    CPPUNIT_ASSERT(dummythread.wasRunning);
    CPPUNIT_ASSERT(dummythread.IsRunning());
}

// Check if the StopThread() method actually stops the thread and doesn't freeze the calling thread which wants to stop it and also check if the thread was still running after the StopThread() method was called.
void ThreadTest::testStopThread() {
    HelperThread* phelper = new HelperThread(&dummythread);
    phelper->StartThread(); // let the helper thread kill the dummy thread
    usleep(25000); // wait 25ms
    CPPUNIT_ASSERT(!dummythread.IsRunning());
    CPPUNIT_ASSERT(phelper->returnedFromDummyStop);
    bool wasnotrunning = phelper->dummyThreadWasNotRunningAnymoreAfter_StopThread_call();
    if (wasnotrunning && phelper) delete phelper;
    CPPUNIT_ASSERT(wasnotrunning);
}

// Check if the thread can be stopped even when it's waiting for a condition
void ThreadTest::testThreadKillableWhenWaiting() {
    WaitingThread* pwaitingthread = new WaitingThread;
    pwaitingthread->SignalStartThread();
    usleep(150000); // wait 150ms
    CPPUNIT_ASSERT(pwaitingthread->IsRunning());
    pwaitingthread->SignalStopThread();
    for (uint trials = 400; trials; trials--) {
        bool success = !pwaitingthread->IsRunning();
        if (success) {
            usleep(15000); // wait 15ms
            delete pwaitingthread;
            CPPUNIT_ASSERT(true); // success
            return;
        }
        else usleep(150000); // wait 150ms and try again
    }
    CPPUNIT_ASSERT(false); // failure
}
