if HAVE_AU

plugindir = $(libdir)/au
examples_dir = $(DEVELOPER_EXTRAS_DIR)
au_public_dir = $(examples_dir)/CoreAudio/AudioUnits/AUPublic
ca_public_dir = $(examples_dir)/CoreAudio/PublicUtility
LinuxSamplerAU_CPPFLAGS = -I$(au_public_dir)/AUBase -I$(au_public_dir)/OtherBases \
    -I$(au_public_dir)/Utility -I$(ca_public_dir) $(GIG_CFLAGS) $(SQLITE3_CFLAGS) $(SNDFILE_CFLAGS)

noinst_PROGRAMS = LinuxSamplerAU

# Uncomment for debug messages.
#debug_messages = -DAU_DEBUG_DISPATCHER=1

LinuxSamplerAU_SOURCES = PluginAU.cpp PluginAU.h PluginAUVersion.h ausdk.cpp

LinuxSamplerAU_LDADD = $(top_builddir)/src/liblinuxsampler.la

if HAVE_AUFLAGS
else
AUFLAGS = $(GIG_LIBS) $(JACK_LIBS) $(SQLITE3_LIBS) $(SNDFILE_LIBS)
endif

LinuxSamplerAU_CPPFLAGS += $(debug_messages)
LinuxSamplerAU_LDFLAGS = -bundle $(AUFLAGS) \
    -exported_symbols_list $(srcdir)/PluginAU.exp \
    -framework CoreServices -framework CoreMIDI -framework CoreAudio \
    -framework CoreFoundation -framework AudioToolbox -framework AudioUnit

COMPONENT_DIR = LinuxSamplerAU.component

all-local: $(COMPONENT_DIR)

LinuxSamplerAU.rsrc: $(srcdir)/PluginAU.r $(srcdir)/PluginAUVersion.h
	Rez $(srcdir)/PluginAU.r -d SystemSevenOrLater=1 -useDF -script Roman -d ppc_YES -d i386_YES -d x86_64_YES \
	    -I /System/Library/Frameworks/CoreServices.framework/Frameworks/CarbonCore.framework/Versions/Current/Headers \
	    -I $(au_public_dir)/AUBase/ -arch i386 -arch x86_64 -arch ppc -o LinuxSamplerAU.rsrc

$(COMPONENT_DIR): $(srcdir)/PkgInfo $(srcdir)/Info.plist LinuxSamplerAU LinuxSamplerAU.rsrc
	mkdir -p "$(COMPONENT_DIR)/Contents/Frameworks"
	mkdir -p "$(COMPONENT_DIR)/Contents/MacOS"
	mkdir -p "$(COMPONENT_DIR)/Contents/Resources"

	cp $(srcdir)/PkgInfo "$(COMPONENT_DIR)/Contents"
	cp $(srcdir)/Info.plist "$(COMPONENT_DIR)/Contents"
	$(LIBTOOL) --mode=install cp LinuxSamplerAU "$(PWD)/$(COMPONENT_DIR)/Contents/MacOS"
	cp LinuxSamplerAU.rsrc "$(COMPONENT_DIR)/Contents/Resources"
	@touch $@

#	    cp "`pkg-config --variable=libdir gig`/libgig.6.dylib" "$(COMPONENT_DIR)/Contents/Frameworks/"; \
#	    install_name_tool -id @executable_path/../Frameworks/libgig.6.dylib \
#	        "$(COMPONENT_DIR)/Contents/Frameworks/libgig.6.dylib"; \
#	    install_name_tool -change "`pkg-config --variable=libdir gig`/libgig.6.dylib" \
#	        @executable_path/../Frameworks/libgig.6.dylib \
#	        "$(COMPONENT_DIR)/Contents/MacOS/LinuxSamplerAU"
#
#if HAVE_SQLITE3
#	    cp "`pkg-config --variable=libdir sqlite3`/libsqlite3.dylib" "$(COMPONENT_DIR)/Contents/Frameworks/"; \
#	    install_name_tool -id @executable_path/../Frameworks/libsqlite3.dylib \; \
#	        "$(COMPONENT_DIR)/Contents/Frameworks/libsqlite3.dylib"; \
#	    install_name_tool -change "`pkg-config --variable=libdir sqlite3`/libsqlite3.dylib" \
#	        @executable_path/../Frameworks/libsqlite3.dylib \
#	        "$(COMPONENT_DIR)/Contents/MacOS/LinuxSamplerAU"
#endif
#
#if HAVE_JACK
#	    cp "`pkg-config --variable=libdir jack`/libjack.dylib" "$(COMPONENT_DIR)/Contents/Frameworks/"; \
#	    install_name_tool -id @executable_path/../Frameworks/libjack.dylib \
#	        "$(COMPONENT_DIR)/Contents/Frameworks/libjack.dylib"; \
#	    install_name_tool -change "`pkg-config --variable=libdir jack`/libjack.dylib" \
#	        @executable_path/../Frameworks/libjack.dylib \
#	        "$(COMPONENT_DIR)/Contents/MacOS/LinuxSamplerAU"
#endif

clean-local:
	rm -f LinuxSamplerAU.rsrc
	rm -rf $(COMPONENT_DIR)

install-exec-hook:
	if mkdir -p $(MAC_PLUGIN_INSTALL_DIR)/Components && \
	test -w $(MAC_PLUGIN_INSTALL_DIR)/Components ; then \
	cp -r $(COMPONENT_DIR) $(MAC_PLUGIN_INSTALL_DIR)/Components/ ; \
	if test -n "$(INSTALL_STRIP_FLAG)" ; then $(STRIP) -x $(MAC_PLUGIN_INSTALL_DIR)/Components/$(COMPONENT_DIR)/Contents/MacOS/LinuxSamplerAU ; fi ; fi

uninstall-hook:
	rm -rf $(MAC_PLUGIN_INSTALL_DIR)/Components/$(COMPONENT_DIR)
endif

EXTRA_DIST = PluginAU.exp PluginAU.r PkgInfo Info.plist
