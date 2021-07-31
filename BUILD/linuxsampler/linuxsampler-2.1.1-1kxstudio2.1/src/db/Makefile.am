if HAVE_SQLITE3
AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS) $(SNDFILE_CFLAGS) $(LIB_SF2)
AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)
METASOURCES = AUTO

sqlite3_lflags = $(SQLITE3_LIBS)
sqlite3_cflags = $(SQLITE3_CFLAGS)
instruments_db_headers = \
	InstrumentsDb.h InstrumentsDbUtilities.h
instruments_db_sources = \
	InstrumentsDb.cpp InstrumentsDb.h \
	InstrumentsDbUtilities.cpp InstrumentsDbUtilities.h

## no need to expose the instruments DB to the C++ API ATM
#liblinuxsamplerdbincludedir = $(includedir)/linuxsampler/db
#liblinuxsamplerdbinclude_HEADERS = \
#	$(instruments_db_headers)

noinst_LTLIBRARIES = liblinuxsamplerdb.la
liblinuxsamplerdb_la_SOURCES = \
	$(instruments_db_sources)

liblinuxsamplerdb_la_LIBADD = $(sqlite3_lflags)
liblinuxsamplerdb_la_CFLAGS = $(sqlite3_cflags)
liblinuxsamplerdb_la_CXXFLAGS = $(sqlite3_cflags)
endif
