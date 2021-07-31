if HAVE_SF2
AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math

noinst_LTLIBRARIES = liblinuxsamplersf2engine.la
liblinuxsamplersf2engine_la_SOURCES = \
	InstrumentResourceManager.cpp InstrumentResourceManager.h \
	DiskThread.cpp DiskThread.h \
	EngineChannel.cpp EngineChannel.h \
	Engine.cpp Engine.h \
	Voice.cpp Voice.h \
	SF2SignalUnitRack.cpp SF2SignalUnitRack.h \
	Stream.cpp Stream.h
liblinuxsamplersf2engine_la_LIBADD = $(GIG_LIBS)
liblinuxsamplersf2engine_la_LDFLAGS = $(GIG_CFLAGS)
endif
