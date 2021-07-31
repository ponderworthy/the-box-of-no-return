AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

liblinuxsamplercommonincludedir = $(includedir)/linuxsampler/common
liblinuxsamplercommoninclude_HEADERS = \
	Exception.h \
	Thread.h \
	global.h \
	optional.h \
	Mutex.h \
	SynchronizedConfig.h \
	Condition.h \
	ConstCapacityArray.h \
	lsatomic.h

noinst_LTLIBRARIES = liblinuxsamplercommon.la
liblinuxsamplercommon_la_SOURCES = \
	global_private.cpp global_private.h \
	atomic.h \
	ArrayList.h \
	DynamicLibraries.cpp DynamicLibraries.h \
	IDGenerator.cpp IDGenerator.h \
	Condition.cpp Condition.h \
	ConditionServer.cpp ConditionServer.h \
	Features.cpp Features.h \
	Mutex.cpp \
	optional.cpp \
	Pool.h \
	ResourceManager.h \
	RingBuffer.h \
	RTMath.cpp RTMath.h \
	stacktrace.c stacktrace.h \
	Thread.cpp Thread.h \
	WorkerThread.cpp WorkerThread.h \
	Path.cpp Path.h \
	File.cpp File.h \
	ladspa.h \
	Ref.h Ref.cpp \
	ChangeFlagRelaxed.h

# create the plugins directory (i.e. /usr/lib/linuxsampler/plugins)
install-exec-hook:
	mkdir -p $(DESTDIR)$(config_plugin_dir)

# remove the plugins directory (if empty)
uninstall-hook:
	rmdir --ignore-fail-on-non-empty $(DESTDIR)$(config_plugin_dir)
