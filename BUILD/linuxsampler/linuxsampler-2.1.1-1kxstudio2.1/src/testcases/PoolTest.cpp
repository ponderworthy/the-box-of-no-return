#include "PoolTest.h"

#include <iostream>

#define ELEMENTS 100

CPPUNIT_TEST_SUITE_REGISTRATION(PoolTest);

using namespace std;

// Note:
// we have to declare all those variables which we want to use for all
// tests within this test suite static because there are side effects which
// occur on transition to the next test which would change the values of our
// variables
static Pool<int>*   pPool = NULL;
static RTList<int>* pList = NULL;

// some data structure we use as template type
struct foo_t {
    int bar;
};


// PoolTest

void PoolTest::printTestSuiteName() {
    cout << "\b \nRunning Pool Tests: " << flush;
}

void PoolTest::testAllocPool() {
    //cout << "testAllocPool()" << flush;
    pPool = new Pool<int>(ELEMENTS);
    CPPUNIT_ASSERT(pPool != NULL);
    CPPUNIT_ASSERT(pPool->isEmpty());
    CPPUNIT_ASSERT(pPool->first() == pPool->end());
    CPPUNIT_ASSERT(pPool->last()  == pPool->begin());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testAllocAppendElements() {
    //cout << "testAllocAppendElements()" << flush;
    // allocate elements and assign a value to it
    for (int i = 0; i < ELEMENTS; i++) {
        Pool<int>::Iterator iter = pPool->allocAppend();
        CPPUNIT_ASSERT(iter != pPool->begin());
        CPPUNIT_ASSERT(iter != pPool->end());
        CPPUNIT_ASSERT(iter == pPool->last());
        *iter = i;
    }
    CPPUNIT_ASSERT(!pPool->isEmpty());
    CPPUNIT_ASSERT(pPool->first() != pPool->end());
    CPPUNIT_ASSERT(pPool->last()  != pPool->begin());
    CPPUNIT_ASSERT(pPool->poolIsEmpty());
}

void PoolTest::testIterateForward() {
    //cout << "testIterateForward()" << flush;
    // iterate forward through the allocated elements and check their values
    Pool<int>::Iterator iter = pPool->first();
    for (int i = 0; i < ELEMENTS; i++) {
        CPPUNIT_ASSERT(*iter == i);
        CPPUNIT_ASSERT(iter != pPool->begin());
        CPPUNIT_ASSERT(iter != pPool->end());
        iter++;
    }
    CPPUNIT_ASSERT(iter == pPool->end());
}

void PoolTest::testIterateBackward() {
    //cout << "testIterateBackward()" << flush;
    // iterate backward through the allocated elements and check their values
    Pool<int>::Iterator iter = pPool->last();
    for (int i = ELEMENTS - 1; i >= 0; i--) {
        CPPUNIT_ASSERT(*iter == i);
        CPPUNIT_ASSERT(iter != pPool->begin());
        CPPUNIT_ASSERT(iter != pPool->end());
        --iter;
    }
    CPPUNIT_ASSERT(iter == pPool->begin());
}

void PoolTest::testFreeElements() {
    //cout << "testFreeElements()" << flush;
    // free all elements
    Pool<int>::Iterator iter = pPool->first();
    for (int i = 0; i < ELEMENTS; i++) {
        pPool->free(iter);
        iter++;
    }
    CPPUNIT_ASSERT(pPool->isEmpty());
    CPPUNIT_ASSERT(pPool->first() == pPool->end());
    CPPUNIT_ASSERT(pPool->last()  == pPool->begin());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testAllocPrependElements() {
    //cout << "testAllocPrependElements()" << flush;
    for (int i = ELEMENTS - 1; i >= 0; i--) {
        Pool<int>::Iterator iter = pPool->allocPrepend();
        CPPUNIT_ASSERT(iter != pPool->begin());
        CPPUNIT_ASSERT(iter != pPool->end());
        CPPUNIT_ASSERT(iter == pPool->first());
        *iter = i;
    }
    CPPUNIT_ASSERT(!pPool->isEmpty());
    CPPUNIT_ASSERT(pPool->first() != pPool->end());
    CPPUNIT_ASSERT(pPool->last()  != pPool->begin());
    CPPUNIT_ASSERT(pPool->poolIsEmpty());
}

void PoolTest::testClear() {
    //cout << "testClear()" << flush;
    pPool->clear();
    CPPUNIT_ASSERT(pPool->isEmpty());
    CPPUNIT_ASSERT(pPool->first() == pPool->end());
    CPPUNIT_ASSERT(pPool->last()  == pPool->begin());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testAllocList() {
    //cout << "testAllocList()" << flush;
    pList = new RTList<int>(pPool);
    CPPUNIT_ASSERT(pList != NULL);
    CPPUNIT_ASSERT(pList->isEmpty());
    CPPUNIT_ASSERT(pList->first() == pList->end());
    CPPUNIT_ASSERT(pList->last()  == pList->begin());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testAllocElementsOnList() {
    //cout << "testAllocElementsOnList()" << flush;
    // allocate elements and assign a value to it
    for (int i = 0; i < ELEMENTS; i++) {
        Pool<int>::Iterator iter = pList->allocAppend();
        CPPUNIT_ASSERT(iter != pList->begin());
        CPPUNIT_ASSERT(iter != pList->end());
        CPPUNIT_ASSERT(iter == pList->last());
        *iter = i;
    }
    CPPUNIT_ASSERT(!pList->isEmpty());
    CPPUNIT_ASSERT(pList->first() != pList->end());
    CPPUNIT_ASSERT(pList->last()  != pList->begin());
    CPPUNIT_ASSERT(pPool->poolIsEmpty());
}

void PoolTest::testIterateForwardOnList() {
    //cout << "testIterateForwardOnList()" << flush;
    // iterate forward through the allocated elements and check their values
    RTList<int>::Iterator iter = pList->first();
    for (int i = 0; i < ELEMENTS; i++) {
        CPPUNIT_ASSERT(*iter == i);
        CPPUNIT_ASSERT(iter != pList->begin());
        CPPUNIT_ASSERT(iter != pList->end());
        iter++;
    }
    CPPUNIT_ASSERT(iter == pList->end());
}

void PoolTest::testIterateBackwardOnList() {
    //cout << "testIterateBackwardOnList()" << flush;
    // iterate backward through the allocated elements and check their values
    RTList<int>::Iterator iter = pList->last();
    for (int i = ELEMENTS - 1; i >= 0; i--) {
        CPPUNIT_ASSERT(*iter == i);
        CPPUNIT_ASSERT(iter != pList->begin());
        CPPUNIT_ASSERT(iter != pList->end());
        --iter;
    }
    CPPUNIT_ASSERT(iter == pList->begin());
}

void PoolTest::testClearList() {
    //cout << "testClearList()" << flush;
    pList->clear();
    CPPUNIT_ASSERT(pList->isEmpty());
    CPPUNIT_ASSERT(pList->first() == pList->end());
    CPPUNIT_ASSERT(pList->last()  == pList->begin());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testMoveToEnd() {
    testAllocElementsOnList();
    CPPUNIT_ASSERT(!pList->isEmpty());
    CPPUNIT_ASSERT(pPool->poolIsEmpty());

    //cout << "testMoveToEnd()" << flush;
    RTList<int> _2ndList(pPool);

    {
        RTList<int>::Iterator iter = pList->first();
        for (int i = 0; i < ELEMENTS; i++) {
            *iter = i;
            RTList<int>::Iterator iterOn2ndList = iter.moveToEndOf(&_2ndList);
            CPPUNIT_ASSERT(*iterOn2ndList == i);
            CPPUNIT_ASSERT(iterOn2ndList == _2ndList.last());
            iter++;
        }
        CPPUNIT_ASSERT(iter == pList->end());
        CPPUNIT_ASSERT(pList->isEmpty());
        CPPUNIT_ASSERT(!_2ndList.isEmpty());
    }

    {
        RTList<int>::Iterator iter = _2ndList.first();
        for (int i = 0; i < ELEMENTS; i++) {
            CPPUNIT_ASSERT(*iter == i);
            CPPUNIT_ASSERT(iter != _2ndList.begin());
            CPPUNIT_ASSERT(iter != _2ndList.end());
            iter++;
        }
        CPPUNIT_ASSERT(iter == _2ndList.end());
    }

    _2ndList.clear();
    CPPUNIT_ASSERT(_2ndList.isEmpty());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testMoveToBegin() {
    testAllocElementsOnList();
    CPPUNIT_ASSERT(!pList->isEmpty());
    CPPUNIT_ASSERT(pPool->poolIsEmpty());

    //cout << "testMoveToBegin()" << flush;
    RTList<int> _2ndList(pPool);

    {
        RTList<int>::Iterator iter = pList->first();
        for (int i = 0; i < ELEMENTS; i++) {
            *iter = i;
            RTList<int>::Iterator iterOn2ndList = iter.moveToBeginOf(&_2ndList);
            CPPUNIT_ASSERT(*iterOn2ndList == i);
            CPPUNIT_ASSERT(iterOn2ndList == _2ndList.first());
            iter++;
        }
        CPPUNIT_ASSERT(iter == pList->end());
        CPPUNIT_ASSERT(pList->isEmpty());
        CPPUNIT_ASSERT(!_2ndList.isEmpty());
    }

    {
        RTList<int>::Iterator iter = _2ndList.first();
        for (int i = ELEMENTS - 1; i >= 0; i--) {
            CPPUNIT_ASSERT(*iter == i);
            CPPUNIT_ASSERT(iter != _2ndList.begin());
            CPPUNIT_ASSERT(iter != _2ndList.end());
            iter++;
        }
        CPPUNIT_ASSERT(iter == _2ndList.end());
    }

    _2ndList.clear();
    CPPUNIT_ASSERT(_2ndList.isEmpty());
    CPPUNIT_ASSERT(!pPool->poolIsEmpty());
}

void PoolTest::testFreeList() {
    //cout << "testFreeList()" << flush;
    if (pList) delete pList;
}

void PoolTest::testFreePool() {
    //cout << "testFreePool()" << flush;
    if (pPool) delete pPool;
}

void PoolTest::testAccessElements() {
    Pool<foo_t> pool(100);
    CPPUNIT_ASSERT(!pool.poolIsEmpty());
    CPPUNIT_ASSERT(pool.isEmpty());

    for (int i = 0; i < 100; i++) pool.allocAppend();
    CPPUNIT_ASSERT(pool.poolIsEmpty());
    CPPUNIT_ASSERT(!pool.isEmpty());

    Pool<foo_t>::Iterator it = pool.first();
    for (int i = 0; i < 100; i++) {
        it->bar = i;
        ++it;
    }

    it = pool.last();
    for (int i = 99; i >= 0; i--) {
        CPPUNIT_ASSERT(it->bar == i);
        --it;
    }

    pool.clear();
    CPPUNIT_ASSERT(!pool.poolIsEmpty());
    CPPUNIT_ASSERT(pool.isEmpty());
}

void PoolTest::testInvalidIterators() {
    RTList<foo_t>::Iterator itNothing;
    bool passed = (itNothing) ? false : true;
    CPPUNIT_ASSERT(passed);
    CPPUNIT_ASSERT(itNothing != true);
    CPPUNIT_ASSERT(!itNothing);

    Pool<foo_t> pool(10);
    passed = (pool.begin()) ? false : true;
    CPPUNIT_ASSERT(passed);
    CPPUNIT_ASSERT(pool.begin() != true);
    CPPUNIT_ASSERT(!pool.begin());
    passed = (pool.end()) ? false : true;
    CPPUNIT_ASSERT(passed);
    CPPUNIT_ASSERT(pool.end() != true);
    CPPUNIT_ASSERT(!pool.end());
}
