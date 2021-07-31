# set the include path found by configure
AM_CPPFLAGS= $(all_includes)

AM_CXXFLAGS = -Wreturn-type $(CXX_CPU_SWITCH)

bin_PROGRAMS = lscp
lscp_SOURCES = lscp.cpp TerminalCtrl.cpp LSCPClient.cpp KeyboardReader.cpp TerminalPrinter.cpp
lscp_LDADD = $(top_builddir)/src/liblinuxsampler.la
