#ifndef __LS_POOLTEST_H__
#define __LS_POOLTEST_H__

#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

// set options for Pool.h

#ifdef CONFIG_DEVMODE
# undef CONFIG_DEVMODE
#endif
#define CONFIG_DEVMODE 1

#ifdef CONFIG_RT_EXCEPTIONS
# undef CONFIG_RT_EXCEPTIONS
#endif
#define CONFIG_RT_EXCEPTIONS 1

#include "../common/Pool.h"

class PoolTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE(PoolTest);
    CPPUNIT_TEST(printTestSuiteName);
    CPPUNIT_TEST(testAllocPool);
    CPPUNIT_TEST(testAllocAppendElements);
    CPPUNIT_TEST(testIterateForward);
    CPPUNIT_TEST(testIterateBackward);
    CPPUNIT_TEST(testFreeElements);
    CPPUNIT_TEST(testAllocPrependElements);
    CPPUNIT_TEST(testIterateForward);
    CPPUNIT_TEST(testIterateBackward);
    CPPUNIT_TEST(testClear);
    CPPUNIT_TEST(testAllocList);
    CPPUNIT_TEST(testAllocElementsOnList);
    CPPUNIT_TEST(testIterateForwardOnList);
    CPPUNIT_TEST(testIterateBackwardOnList);
    CPPUNIT_TEST(testClearList);
    CPPUNIT_TEST(testMoveToEnd);
    CPPUNIT_TEST(testMoveToBegin);
    CPPUNIT_TEST(testFreeList);
    CPPUNIT_TEST(testFreePool);
    CPPUNIT_TEST(testAccessElements);
    CPPUNIT_TEST(testInvalidIterators);
    CPPUNIT_TEST_SUITE_END();

    public:
        void printTestSuiteName();
        void testAllocPool();
        void testAllocAppendElements();
        void testIterateForward();
        void testIterateBackward();
        void testFreeElements();
        void testAllocPrependElements();
        void testClear();
        void testAllocList();
        void testAllocElementsOnList();
        void testIterateForwardOnList();
        void testIterateBackwardOnList();
        void testClearList();
        void testMoveToEnd();
        void testMoveToBegin();
        void testFreeList();
        void testFreePool();
        void testAccessElements();
        void testInvalidIterators();
};

#endif // __LS_POOLTEST_H__
