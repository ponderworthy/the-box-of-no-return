# require automake (>= 1.5) due to 'dist-bzip2' option
AUTOMAKE_OPTIONS = 1.5 dist-bzip2

SUBDIRS = man src scripts osx Artwork Documentation

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = linuxsampler.pc

EXTRA_DIST = Doxyfile.in \
             linuxsampler.pc.in \
             linuxsampler.kdevelop \
             benchmarks/gigsynth.cpp \
             benchmarks/Makefile \
             benchmarks/triang.cpp

# let GCC auto detect whether it (sh/c)ould output its messages in color
export GCC_COLORS=auto

.PHONY: parser testcases

if HAVE_SQLITE3
have_sqlite = "yes"
else
have_sqlite = "no"
endif

if !CROSS_COMPILING
install-data-local:
	@if [ ! -e "$(DESTDIR)/var/lib/linuxsampler/instruments.db" -a \
	$(have_sqlite) = "yes" ] ; then \
	echo "****************************************************************"; \
	echo "*** LinuxSampler was compiled with instruments DB support and"; \
	echo "*** $(DESTDIR)/var/lib/linuxsampler/instruments.db"; \
	echo "*** doesn't exist yet, creating it now:"; \
	mkdir -p $(DESTDIR)/var/lib/linuxsampler && ./src/linuxsampler \
	--create-instruments-db $(DESTDIR)/var/lib/linuxsampler/instruments.db ; \
	fi
endif

# generate parser with yacc
parser:
	@cd $(srcdir)/src/network && make $@
	@cd $(srcdir)/src/scriptvm && make $@

# compile test cases for the LinuxSampler codebase
testcases:
	@cd $(srcdir)/src/testcases && make linuxsamplertest

tests: testcases

docs: Doxyfile
	@echo '*** Running doxygen ***'
	doxygen Doxyfile
