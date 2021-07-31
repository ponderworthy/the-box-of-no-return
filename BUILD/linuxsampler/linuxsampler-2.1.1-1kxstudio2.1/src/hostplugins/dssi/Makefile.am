if HAVE_DSSI
AM_CPPFLAGS = $(GIG_CFLAGS) $(SQLITE3_CFLAGS)
plugindir = $(libdir)/dssi
plugin_LTLIBRARIES = linuxsampler.la
linuxsampler_la_SOURCES = PluginDssi.cpp PluginDssi.h
linuxsampler_la_LDFLAGS = -module -avoid-version
linuxsampler_la_LIBADD = $(top_builddir)/src/liblinuxsampler.la
endif
