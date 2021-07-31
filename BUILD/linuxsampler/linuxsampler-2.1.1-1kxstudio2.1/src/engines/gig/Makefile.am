AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math

noinst_LTLIBRARIES = liblinuxsamplergigengine.la
liblinuxsamplergigengine_la_SOURCES = \
	EngineGlobals.h \
	DiskThread.cpp DiskThread.h \
	EGADSR.cpp EGADSR.h \
	EGDecay.cpp EGDecay.h \
	Engine.cpp Engine.h \
	Filter.h Filter.cpp \
	InstrumentScriptVM.h InstrumentScriptVM.cpp \
	InstrumentScriptVMFunctions.h InstrumentScriptVMFunctions.cpp \
	InstrumentResourceManager.cpp InstrumentResourceManager.h \
	Stream.cpp Stream.h \
	Voice.cpp Voice.h \
	Synthesizer.cpp Synthesizer.h \
	Profiler.cpp Profiler.h \
	EngineChannel.cpp EngineChannel.h \
	SmoothVolume.cpp SmoothVolume.h \
	SynthesisParam.h
liblinuxsamplergigengine_la_LIBADD = $(GIG_LIBS)
liblinuxsamplergigengine_la_LDFLAGS = $(GIG_CFLAGS)
