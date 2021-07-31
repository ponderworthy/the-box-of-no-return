AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

noinst_LTLIBRARIES = liblinuxsamplerscriptvmeditor.la
liblinuxsamplerscriptvmeditor_la_SOURCES = \
	CodeScanner.h CodeScanner.cpp \
	SourceToken.h \
	nksp.cpp \
	NkspScanner.h NkspScanner.cpp

EXTRA_DIST = nksp.l

# automatically (re)generate nksp.cpp with flex / lex if the
# lex source file(s) have been changed
nksp.cpp: nksp.l NkspScanner.h CodeScanner.h SourceToken.h
	$(top_srcdir)/scripts/generate_nksp_editor_parser.sh

.PHONY: parser

# "make parser" was explicitly requested
parser:
	$(top_srcdir)/scripts/generate_nksp_editor_parser.sh
