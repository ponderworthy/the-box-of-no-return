AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS) $(SNDFILE_CFLAGS) $(LIB_SF2)
AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH) -DCONFIG_PLUGIN_DIR=\"$(config_plugin_dir)\"
METASOURCES = AUTO
SUBDIRS = gig sf2 sfz common

liblinuxsamplerengineincludedir = $(includedir)/linuxsampler/engines
liblinuxsamplerengineinclude_HEADERS = \
	Engine.h \
	EngineChannel.h \
	FxSend.h \
	InstrumentManager.h

noinst_LTLIBRARIES = liblinuxsamplerengines.la
liblinuxsamplerengines_la_SOURCES = \
	Engine.cpp Engine.h \
	AbstractEngine.cpp AbstractEngine.h \
	EngineBase.h \
	EngineChannel.cpp EngineChannel.h \
	AbstractEngineChannel.cpp AbstractEngineChannel.h \
	EngineChannelBase.h \
	EngineFactory.cpp EngineFactory.h \
	EngineChannelFactory.cpp EngineChannelFactory.h \
	FxSend.cpp FxSend.h \
	InstrumentManager.cpp InstrumentManager.h \
	InstrumentManagerBase.h \
	InstrumentManagerThread.cpp InstrumentManagerThread.h
