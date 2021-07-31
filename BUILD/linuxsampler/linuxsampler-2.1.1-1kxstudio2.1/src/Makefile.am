# set the include path found by configure
AM_CPPFLAGS= $(all_includes) $(GIG_CFLAGS) $(SQLITE3_CFLAGS)

if HAVE_WINDOWS
system_libs =
SHELL_SUBDIR =
else
system_libs = -lpthread -ldl
SHELL_SUBDIR = shell
endif

if HAVE_COREMIDI
coremidi_ldflags = -framework CoreAudio \
	-framework CoreMIDI \
	-framework CoreServices \
	-framework AudioUnit
else
coremidi_ldflags =
endif

if HAVE_SQLITE3
sqlite3_lib = $(top_builddir)/src/db/liblinuxsamplerdb.la
else
sqlite3_lib =
endif

if HAVE_SF2
sf2_engine_lib = $(top_builddir)/src/engines/sf2/liblinuxsamplersf2engine.la
else
sf2_engine_lib =
endif

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

SUBDIRS = scriptvm db network engines common testcases drivers \
          plugins effects . hostplugins $(SHELL_SUBDIR)

liblinuxsamplerincludedir = $(includedir)/linuxsampler
liblinuxsamplerinclude_HEADERS = Sampler.h EventListeners.h

pkglib_LTLIBRARIES = liblinuxsampler.la
liblinuxsampler_la_SOURCES = Sampler.cpp
liblinuxsampler_la_LIBADD = \
	$(sqlite3_lib) \
	$(top_builddir)/src/scriptvm/liblinuxsamplerscriptvm.la \
	$(top_builddir)/src/scriptvm/editor/liblinuxsamplerscriptvmeditor.la \
	$(top_builddir)/src/network/liblinuxsamplernetwork.la \
	$(top_builddir)/src/engines/gig/liblinuxsamplergigengine.la \
	$(sf2_engine_lib) \
	$(top_builddir)/src/engines/sfz/liblinuxsamplersfzengine.la \
	$(top_builddir)/src/engines/common/liblinuxsamplercommonengine.la \
	$(top_builddir)/src/engines/liblinuxsamplerengines.la \
	$(top_builddir)/src/drivers/liblinuxsamplerdrivers.la \
	$(top_builddir)/src/drivers/audio/liblinuxsampleraudiodriver.la \
	$(top_builddir)/src/drivers/midi/liblinuxsamplermididriver.la \
	$(top_builddir)/src/plugins/liblinuxsamplerplugins.la \
	$(top_builddir)/src/effects/liblinuxsamplereffects.la \
	$(top_builddir)/src/common/liblinuxsamplercommon.la \
	$(system_libs)

liblinuxsampler_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@ -no-undefined

bin_PROGRAMS = linuxsampler ls_instr_script

linuxsampler_SOURCES = linuxsampler.cpp
linuxsampler_LDADD = liblinuxsampler.la
linuxsampler_LDFLAGS = $(coremidi_ldflags)

ls_instr_script_SOURCES = ls_instr_script.cpp
ls_instr_script_LDADD = liblinuxsampler.la
