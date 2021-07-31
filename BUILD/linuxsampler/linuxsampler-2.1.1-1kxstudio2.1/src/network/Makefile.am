AM_CPPFLAGS = $(all_includes) $(GIG_CFLAGS) $(SQLITE3_CFLAGS)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

if HAVE_WINDOWS
winsocket_ldflags = -lws2_32 -lole32
else
winsocket_ldflags = 
endif

noinst_LTLIBRARIES = liblinuxsamplernetwork.la
liblinuxsamplernetwork_la_SOURCES = \
	lscp.h \
	lscpparser.cpp lscpparser.h \
	lscpserver.cpp lscpserver.h \
	lscpsymbols.h \
	lscpresultset.cpp lscpresultset.h \
	lscp_shell_reference.cpp lscp_shell_reference.h \
	lscpevent.cpp lscpevent.h
liblinuxsamplernetwork_la_LIBADD = $(winsocket_ldflags)

yacc_sources = lscp.y

EXTRA_DIST = $(yacc_sources)

# automatically (re)generate lscpsymbols.h with bison / yacc if the
# yacc source file(s) have been changed
lscpsymbols.h: $(yacc_sources)
	$(top_srcdir)/scripts/generate_lscp_parser.sh

# automatically (re)generate lscpparser.cpp with bison / yacc if the
# yacc source file(s) have been changed
lscpparser.cpp: $(yacc_sources)
	$(top_srcdir)/scripts/generate_lscp_parser.sh

# automatically (re)generate lscp_shell_reference.cpp if the
# yacc source file or lscp.xml source have been changed
lscp_shell_reference.cpp: $(yacc_sources) ../../Documentation/lscp.xml
	$(top_srcdir)/scripts/generate_lscp_parser.sh

.PHONY: parser

# "make parser" was explicitly requested
parser:
	$(top_builddir)/scripts/generate_lscp_parser.sh
