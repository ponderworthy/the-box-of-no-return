AM_CPPFLAGS = $(all_includes)
METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

SUBDIRS = editor

AM_YFLAGS = -d
BUILT_SOURCES = parser.h

liblinuxsamplerscriptvmincludedir = $(includedir)/linuxsampler/scriptvm
liblinuxsamplerscriptvminclude_HEADERS = \
	common.h \
	ScriptVM.h \
	ScriptVMFactory.h

noinst_LTLIBRARIES = liblinuxsamplerscriptvm.la
liblinuxsamplerscriptvm_la_SOURCES = \
	common.h common.cpp \
	scanner.cpp \
	parser.h parser.cpp \
	tree.h tree.cpp \
	CoreVMFunctions.h CoreVMFunctions.cpp \
	CoreVMDynVars.h CoreVMDynVars.cpp \
	ScriptVM.h ScriptVM.cpp \
	ScriptVMFactory.h ScriptVMFactory.cpp

yacc_sources = parser.y

EXTRA_DIST = $(yacc_sources) scanner.l

# automatically (re)generate scanner.cpp with flex / lex if the
# lex source file(s) have been changed
scanner.cpp: scanner.l tree.h parser_shared.h
	$(top_srcdir)/scripts/generate_instrument_script_parser.sh

# automatically (re)generate parser.cpp with bison / yacc if the
# yacc source file(s) have been changed
parser.cpp: $(yacc_sources) scanner.l tree.h parser_shared.h
	$(top_srcdir)/scripts/generate_instrument_script_parser.sh

.PHONY: parser

# "make parser" was explicitly requested
parser:
	$(top_srcdir)/scripts/generate_instrument_script_parser.sh
