AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

if HAVE_COREMIDI
coremidi_ldflags = -framework CoreFoundation -framework CoreMIDI
else
coremidi_ldflags =
endif

EXTRA_PROGRAMS = linuxsamplertest
linuxsamplertest_SOURCES = \
	linuxsamplertest.cpp \
	PoolTest.cpp PoolTest.h \
	ThreadTest.cpp ThreadTest.h \
	MutexTest.cpp MutexTest.h \
	ConditionTest.cpp ConditionTest.h \
	LSCPTest.cpp LSCPTest.h
linuxsamplertest_LDFLAGS = $(coremidi_ldflags)
linuxsamplertest_LDADD = $(top_builddir)/src/liblinuxsampler.la -lcppunit
