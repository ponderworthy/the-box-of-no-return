AM_CPPFLAGS = $(all_includes) $(SNDFILE_CFLAGS) $(GIG_CFLAGS)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math

noinst_LTLIBRARIES = liblinuxsamplersfzengine.la
liblinuxsamplersfzengine_la_SOURCES = \
	sfz.cpp sfz.h \
	InstrumentResourceManager.cpp InstrumentResourceManager.h \
	DiskThread.cpp DiskThread.h \
	EngineChannel.cpp EngineChannel.h \
	Engine.cpp Engine.h \
        InstrumentScriptVM.cpp InstrumentScriptVM.h \
	Voice.cpp Voice.h \
	Stream.cpp Stream.h \
	EGADSR.cpp EGADSR.h \
	EG.cpp EG.h \
	SfzSignalUnitRack.cpp SfzSignalUnitRack.h \
	LookupTable.cpp LookupTable.h
