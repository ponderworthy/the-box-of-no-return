// Unit Test Runner for LinuxSampler (text only version)
// -----------------------------------------------------
//
// This console application is used to run all written tests against current
// LinuxSampler codebase. Progress of the test runs and and the final result
// will be printed out as simple text on console.
//
// The test runner is not compiled by default (means by just running 'make'
// or 'make all'), you have to compile it explicitly by running
// 'make testcases' in the toplever directory or 'make linuxsamplertest' in
// this source directory. Note: you need to have cppunit installed on your
// system to be able to compile the unit tests.
//
// This file usually doesn't have to be changed, especially not for adding
// new tests !

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

int main( int argc, char **argv) {
    CppUnit::TextUi::TestRunner runner;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    runner.addTest( registry.makeTest() );
    bool wasSuccessful = runner.run( "", false );
    return wasSuccessful;
}
