AM_CPPFLAGS = $(all_includes)
AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH) -DCONFIG_PLUGIN_DIR=\"$(config_plugin_dir)\"
METASOURCES = AUTO

liblinuxsamplerpluginsincludedir = $(includedir)/linuxsampler/plugins
liblinuxsamplerpluginsinclude_HEADERS = \
	InstrumentEditor.h \
	InstrumentEditorFactory.h

noinst_LTLIBRARIES = liblinuxsamplerplugins.la
liblinuxsamplerplugins_la_SOURCES = \
	InstrumentEditor.cpp InstrumentEditor.h \
	InstrumentEditorFactory.cpp InstrumentEditorFactory.h
