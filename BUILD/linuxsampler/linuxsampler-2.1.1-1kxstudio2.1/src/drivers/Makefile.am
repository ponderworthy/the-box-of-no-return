AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS) $(SQLITE3_CFLAGS)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

liblinuxsamplerdriversincludedir = $(includedir)/linuxsampler/drivers
liblinuxsamplerdriversinclude_HEADERS = \
	Device.h \
	DeviceParameter.h

noinst_LTLIBRARIES = liblinuxsamplerdrivers.la
liblinuxsamplerdrivers_la_SOURCES = \
	Device.cpp Device.h \
	DeviceParameter.cpp DeviceParameter.h \
	DeviceParameterFactory.cpp DeviceParameterFactory.h \
	Plugin.cpp Plugin.h
liblinuxsamplerdrivers_la_LDFLAGS =
#liblinuxsamplerdrivers_la_DEPENDENCIES =
liblinuxsamplerdrivers_la_LIBADD =
SUBDIRS = audio midi
