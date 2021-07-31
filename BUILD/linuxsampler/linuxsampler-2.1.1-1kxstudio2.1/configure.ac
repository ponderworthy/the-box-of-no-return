#------------------------------------------------------------------------------------
# LinuxSampler's / liblinuxsampler's "official" release version:

m4_define(linuxsampler_release_major, 2)
m4_define(linuxsampler_release_minor, 1)
m4_define(linuxsampler_release_build, 1)


AC_INIT([linuxsampler],[linuxsampler_release_major.linuxsampler_release_minor.linuxsampler_release_build])
AC_CONFIG_SRCDIR([configure.ac])

#------------------------------------------------------------------------------------
# The following is the libtool / shared library version. This doesn't have to
# do anything with the release version. It MUST conform to the following rules:
#
#  1. Start with version information of `0:0:0' for each libtool library.
#  2. Update the version information only immediately before a public release of
#     your software. More frequent updates are unnecessary, and only guarantee
#     that the current interface number gets larger faster.
#  3. If the library source code has changed at all since the last update, then
#     increment revision (`c:r:a' becomes `c:r+1:a').
#  4. If any interfaces have been added, removed, or changed since the last update,
#     increment current, and set revision to 0.
#  5. If any interfaces have been added since the last public release, then increment
#     age.
#  6. If any interfaces have been removed since the last public release, then set age
#     to 0.

LIBLINUXSAMPLER_LT_CURRENT=5
LIBLINUXSAMPLER_LT_REVISION=0
LIBLINUXSAMPLER_LT_AGE=0
SHARED_VERSION_INFO="$LIBLINUXSAMPLER_LT_CURRENT:$LIBLINUXSAMPLER_LT_REVISION:$LIBLINUXSAMPLER_LT_AGE"

#------------------------------------------------------------------------------------
# the LSCP specification version this LinuSampler release complies with:

LSCP_RELEASE_MAJOR=1
LSCP_RELEASE_MINOR=7

AC_DEFINE_UNQUOTED(LSCP_RELEASE_MAJOR, ${LSCP_RELEASE_MAJOR}, [LSCP spec major version this release complies with.])
AC_DEFINE_UNQUOTED(LSCP_RELEASE_MINOR, ${LSCP_RELEASE_MINOR}, [LSCP spec minor version this release complies with.])

AM_INIT_AUTOMAKE([subdir-objects])
AC_PROG_CC
AC_PROG_CXX
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_LEX
AC_PROG_YACC

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)

module=yes eval LIB_EXT=$shrext_cmds
AC_SUBST(LIB_EXT)

AC_C_BIGENDIAN
AC_CANONICAL_HOST

PKG_PROG_PKG_CONFIG

###########################################################################
# General Checks

AM_CONDITIONAL(CROSS_COMPILING, test $cross_compiling = yes)

AC_MSG_CHECKING([whether x86 architecture])
def_arch_x86=0
case $host_cpu in
  "i386" | "i486" | "i586" | "i686" | "i786" | "x86_64")
    echo "yes"
    def_arch_x86=1;;
  *)
    echo "no";;
esac
AC_DEFINE_UNQUOTED(ARCH_X86,$def_arch_x86,[Define to 1 if you build for x86 architecture.])

# determine the right gcc switch for CPU specific optimizations
# (only if the user did not provide one)
CXX_CPU_SWITCH=
if ! echo "X $CXXFLAGS " | grep -q -- " \(-march=\|-mcpu=\|-mtune=\|-arch=\)" ; then
  if test "$def_arch_x86" = 1 -a "$host_cpu" != "x86_64"; then
    CXX_CPU_SWITCH="-march=$host_cpu"
  elif test "$target_cpu" = "ppc"; then
    CXX_CPU_SWITCH="-arch=$host_cpu"
  fi
fi
AC_SUBST([CXX_CPU_SWITCH])

mac=no
linux=no
case "$host" in
    *-*-darwin*)
        mac=yes
        ;;
    *-*-linux*)
        linux=yes
        ;;
esac
AM_CONDITIONAL(LINUX, test "$linux" = yes)
AM_CONDITIONAL(MAC, test "$mac" = yes)

# check if we're on MS Windows
AC_CHECK_HEADERS(
    mmsystem.h,
    have_windows=1,
    have_windows=0,
    [#include <windef.h>]
)
AM_CONDITIONAL(HAVE_WINDOWS, test $have_windows = "1")

AC_MSG_CHECKING([whether UNIX98 compatible])
AC_LANG_PUSH([C])
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <features.h>
void main(void) {
#if _XOPEN_SOURCE >= 500
exit(0); /* UNIX98 compatible */
#else
exit(-1); /* not UNIX98 compatible */
#endif
}
]])],
have_unix98="yes",
have_unix98="no",
have_unix98="no"
)
AC_LANG_POP([C])
AC_MSG_RESULT([$have_unix98])
if test "$have_unix98" = "no" -a "$have_windows" = "0" -a "$mac" = "no" ; then
    if test "x$HAVE_UNIX98" = "x"; then
        echo "LinuxSampler only runs on UNIX98 compatible systems, which is mandatory for"
        echo "pthread_mutexattr_settype() call in Mutex.cpp. You may want to run"
        echo "./configure with environment variable HAVE_UNIX98=1 in case you think you"
        echo "have a UNIX98 compatible system."
        exit -1;
    fi
fi

# check for <features.h>
AC_CHECK_HEADERS(features.h)

# test for POSIX thread library
m4_ifdef([m4_include(m4/pthread.m4)],,
             [sinclude([m4/pthread.m4])])
ACX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"


# check for compiler's pragma diagnostic capabilities
m4_ifdef([m4_include(m4/pragma_diagnostics.m4)],,
             [sinclude([m4/pragma_diagnostics.m4])])
ACX_CXX_PRAGMA_DIAGNOSTICS


# check for a bug in NPTL-enabled glibc
# (see Gentoo bug report #194076)
AC_ARG_ENABLE(nptl-bug-check,
  [  --disable-nptl-bug-check
                          Disable check for a bug in certain NPTL-enabled
                          glibc versions that caused crashs.],
  [config_check_nptl_bug="$enableval"],
  [config_check_nptl_bug="yes"]
)
if test "$config_check_nptl_bug" = "yes"; then
    m4_ifdef([m4_include(m4/nptl_bug.m4)],,
             [sinclude([m4/nptl_bug.m4])])
    ACX_NPTL_GLIBC_BUG([
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        echo "You seem to have a buggy PTHREAD library! LinuxSampler would"
        echo "probably crash due to this. Please report us the system you are"
        echo "using and/or file a bug report to the bug tracking system of"
        echo "your distribution."
        echo "If you have a NPTL-enabled glibc AND it was compiled for TLS as"
        echo "well, you can try to circumvent this problem for now by setting"
        echo "the environment variable LD_ASSUME_KERNEL=\"2.4.1\" , which"
        echo "should cause this test to pass."
        echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
        AC_MSG_ERROR([possibly NPTL glibc bug detected])
    ])
else
    echo "NPTL glibc bug check disabled"
fi


# In case Bison is available, determine the exact version, since we need to
# use different custom parser code for Bison 2.x vs. Bison 3.x generated
# parser yacc tables.
if echo "$YACC" | grep -q bison; then
    # NOTE: m4 removes [], that's why it needs to be escaped
    bison_version=[`$YACC --version | head -n 1 | sed -e 's/[^0-9.]*\([-0-9.]\+\)$/\1/'`]
    bison_version_major=`echo $bison_version | cut -d. -f1`
    bison_version_minor=`echo $bison_version | cut -d. -f2`
    AC_DEFINE_UNQUOTED(HAVE_BISON_MAJ,$bison_version_major,[Define to the major version of the GNU Bison program installed.])
    AC_DEFINE_UNQUOTED(HAVE_BISON_MIN,$bison_version_minor,[Define to the minor version of the GNU Bison program installed.])
fi



###########################################################################
# Checks for available audio and MIDI systems / drivers
# (we throw an error if there's not at least one system for audio output and MIDI input available)

have_midi_input_driver="false"
have_audio_output_driver="false"

# ALSA
AC_ARG_ENABLE(alsa-driver,
  [  --disable-alsa-driver
                          Disable support for the Advanced Linux Sound
                          Architecture (ALSA).],
  [config_alsa_driver="$enableval"],
  [config_alsa_driver="yes"]
)
have_alsa=0
if test "$config_alsa_driver" = "yes"; then
    AC_CHECK_HEADER(alsa/asoundlib.h,
        AC_CHECK_LIB(asound, main,
                                 have_alsa=1
                                 ,
                                 have_alsa=0
                    )
                    ,
                    have_alsa=0
    )
    if test "$have_alsa" = "1"; then
        have_midi_input_driver="true"
        have_audio_output_driver="true";
    fi

    echo -n "checking Alsa version... "
    AC_LANG_PUSH([C])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
        #include <alsa/asoundlib.h>
        void main(void) {
            /* ensure backward compatibility */
            #if !defined(SND_LIB_MAJOR) && defined(SOUNDLIB_VERSION_MAJOR)
            #define SND_LIB_MAJOR SOUNDLIB_VERSION_MAJOR
            #endif
            exit(SND_LIB_MAJOR);
        }
    ]])],
    alsa_major=0,
    alsa_major=$?,
    alsa_major=0
    )
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
        #include <alsa/asoundlib.h>
        void main(void) {
            /* ensure backward compatibility */
            #if !defined(SND_LIB_MINOR) && defined(SOUNDLIB_VERSION_MINOR)
            #define SND_LIB_MINOR SOUNDLIB_VERSION_MINOR
            #endif
            exit(SND_LIB_MINOR);
        }
    ]])],
    alsa_minor=0,
    alsa_minor=$?,
    alsa_minor=0
    )
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
        #include <alsa/asoundlib.h>
        void main(void) {
            /* ensure backward compatibility */
            #if !defined(SND_LIB_SUBMINOR) && defined(SOUNDLIB_VERSION_SUBMINOR)
            #define SND_LIB_SUBMINOR SOUNDLIB_VERSION_SUBMINOR
            #endif
            exit(SND_LIB_SUBMINOR);
        }
    ]])],
    alsa_subminor=0,
    alsa_subminor=$?,
    alsa_subminor=0
    )
    AC_LANG_POP([C])
    echo "$alsa_major.$alsa_minor.$alsa_subminor";
    AC_DEFINE_UNQUOTED(ALSA_MAJOR,$alsa_major,[Define to the major version number of your Alsa installation.])
    AC_DEFINE_UNQUOTED(ALSA_MINOR,$alsa_minor,[Define to the minor version number of your Alsa installation.])
    AC_DEFINE_UNQUOTED(ALSA_SUBMINOR,$alsa_subminor,[Define to the subminor version number of your Alsa installation.])
else
    echo "ALSA support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_ALSA, test $have_alsa = "1")
AC_DEFINE_UNQUOTED(HAVE_ALSA,$have_alsa,[Define to 1 if you have ALSA installed.])
config_have_alsa="no"
if test $have_alsa = "1"; then
    config_have_alsa="yes"
fi

# JACK
AC_ARG_ENABLE(jack-driver,
  [  --disable-jack-driver
                          Disable support for the Jack Audio Connection Kit
                          (JACK).],
  [config_jack_driver="$enableval"],
  [config_jack_driver="yes"]
)
have_jack=0
if test "$config_jack_driver" = "yes"; then
    PKG_CHECK_MODULES(JACK, jack, have_jack=1, have_jack=0)
    if test $have_jack = "1"; then
        AC_SUBST(JACK_LIBS)
        AC_SUBST(JACK_CFLAGS)
        linuxsampler_save_LIBS=$LIBS
        LIBS="$JACK_LIBS $LIBS"
        AC_CHECK_FUNCS(jack_client_name_size jack_client_open \
                       jack_on_info_shutdown)
        LIBS=$linuxsampler_save_LIBS
        have_audio_output_driver="true";
    fi
else
    echo "JACK support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_JACK, test $have_jack = "1")
AC_DEFINE_UNQUOTED(HAVE_JACK,$have_jack,[Define to 1 if you have JACK installed.])
config_have_jack="no"
if test $have_jack = "1"; then
    config_have_jack="yes"
fi

# JACK MIDI
have_jack_midi=0
if test $have_jack = "1"; then
    linuxsampler_save_CFLAGS=$CFLAGS
    linuxsampler_save_LIBS=$LIBS
    CFLAGS="$JACK_CFLAGS $CFLAGS"
    LIBS="$JACK_LIBS $LIBS"
    AC_CHECK_HEADER(jack/midiport.h, have_jack_midi=1, have_jack_midi=0)
    if test $have_jack_midi = "1"; then
        AC_CHECK_FUNCS(jack_midi_get_event_count)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <jack/midiport.h>]], [[
            jack_midi_clear_buffer(0, 0);
        ]])], [AC_DEFINE(JACK_MIDI_FUNCS_NEED_NFRAMES, 1,
                  [Define to 1 if you have the old jack midi functions that need an nframes argument.])])
        have_midi_input_driver="true"
    fi
    CFLAGS=$linuxsampler_save_CFLAGS
    LIBS=$linuxsampler_save_LIBS
fi
AM_CONDITIONAL(HAVE_JACK_MIDI, test $have_jack_midi = "1")
AC_DEFINE_UNQUOTED(HAVE_JACK_MIDI, $have_jack_midi,
                   [Define to 1 if you have JACK with MIDI support installed.])
config_have_jack_midi="no"
if test $have_jack_midi = "1"; then
    config_have_jack_midi="yes"
fi

# ARTS
AC_ARG_ENABLE(arts-driver,
  [  --disable-arts-driver
                          Disable support for the Analogue Realtime System
                          (aRts).],
  [config_arts_driver="$enableval"],
  [config_arts_driver="yes"]
)
have_arts=0
if test "$config_arts_driver" = "yes"; then
    m4_ifdef([m4_include(m4/arts.m4)],,
             [sinclude([m4/arts.m4])])
    AM_PATH_ARTS(0.9.5, have_arts=1, have_arts=0)
    if test "$have_arts" = "1"; then
        have_audio_output_driver="true"
    fi
else
    echo "ARTS support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_ARTS, test "$have_arts" = "1")
AC_DEFINE_UNQUOTED(HAVE_ARTS,$have_arts,[Define to 1 if you have aRts installed.])
config_have_arts="no"
if test $have_arts = "1"; then
    config_have_arts="yes"
fi

# ASIO AUDIO (Win32)
AC_ARG_ENABLE(asiosdk-dir,
  [  --enable-asiosdk-dir
                          Directory where the ASIO SDK is located, this automatically
                          enables the compilation of the ASIO audio output driver.],
  [config_asiosdk_dir="${enableval}"],
  [config_asiosdk_dir="."]
)
AC_SUBST(config_asiosdk_dir)

AC_ARG_ENABLE(asio-driver,
  [  --disable-asio-driver
                          Disable support for the Windows ASIO driver.],
  [config_asio_driver="$enableval"],
  [config_asio_driver="yes"]
)
have_asio=0
ASIOSDK_BASEDIR=
if test "$config_asio_driver" = "yes"; then
    asiosdk_headerfile=$config_asiosdk_dir/ASIOSDK2/common/asio.h
    echo -n "checking for ASIO headerfile: $asiosdk_headerfile ...."
    if test -e $asiosdk_headerfile ; then
        echo yes
        have_asio=1
        ASIOSDK_BASEDIR="$config_asiosdk_dir"
    else
        echo no
        have_asio=0
    fi
    if test "$have_asio" = "1"; then
        have_audio_output_driver="true"
    fi
else
    echo "Windows ASIO support disabled by configure script parameter"
fi
AC_SUBST(ASIOSDK_BASEDIR)
AM_CONDITIONAL(HAVE_ASIO, test $have_asio = "1")
AC_DEFINE_UNQUOTED(HAVE_ASIO,$have_asio,[Define to 1 if you have ASIO installed.])
config_have_asio="no"
if test $have_asio = "1"; then
    config_have_asio="yes"
fi

# MidiShare (Linux, OS X, Windows)
AC_ARG_ENABLE(midishare-driver,
  [  --disable-midishare-driver
                          Disable support for the MidiShare system.],
  [config_midishare_driver="$enableval"],
  [config_midishare_driver="yes"]
)
have_midishare=0
if test "$config_midishare_driver" = "yes"; then
    AC_CHECK_HEADER(MidiShare.h,
        AC_CHECK_LIB(MidiShare, MidiCountEvs,
                                have_midishare=1,
                                have_midishare=0
                    )
                    ,
                    have_midishare=0
    )
    if test "$have_midishare" = "1"; then
        have_midi_input_driver="true"
    fi
else
    echo "MidiShare support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_MIDISHARE, test $have_midishare = "1")
AC_DEFINE_UNQUOTED(HAVE_MIDISHARE,$have_midishare,[Define to 1 if you have MidiShare installed.])
config_have_midishare="no"
if test $have_midishare = "1"; then
    config_have_midishare="yes"
fi

# CoreMIDI (OS X)
AC_ARG_ENABLE(coremidi-driver,
  [  --disable-coremidi-driver
                          Disable support for the Apple CoreMIDI system.],
  [config_coremidi_driver="$enableval"],
  [config_coremidi_driver="yes"]
)
have_coremidi=0
if test "$config_coremidi_driver" = "yes"; then
    AC_CHECK_HEADER(CoreMIDI/CoreMIDI.h,
        have_coremidi=1,
        have_coremidi=0
    )
    if test "$have_coremidi" = "1"; then
        have_midi_input_driver="true"
    fi
else
    echo "CoreMIDI support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_COREMIDI, test $have_coremidi = "1")
AC_DEFINE_UNQUOTED(HAVE_COREMIDI,$have_coremidi,[Define to 1 if you have CoreMIDI installed.])
config_have_coremidi="no"
if test $have_coremidi = "1"; then
    config_have_coremidi="yes"
fi

# CoreAudio (OS X)
AC_ARG_ENABLE(coreaudio-driver,
  [  --disable-coreaudio-driver
                          Disable support for the Apple CoreAudio system.],
  [config_coreaudio_driver="$enableval"],
  [config_coreaudio_driver="yes"]
)
have_coreaudio=0
if test "$config_coreaudio_driver" = "yes"; then
    AC_CHECK_HEADER(CoreAudio/CoreAudio.h,
        have_coreaudio=1,
        have_coreaudio=0
    )
    if test "$have_coreaudio" = "1"; then
        have_audio_output_driver="true"
    fi
else
    echo "CoreAudio support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_COREAUDIO, test $have_coreaudio = "1")
AC_DEFINE_UNQUOTED(HAVE_COREAUDIO,$have_coreaudio,[Define to 1 if you have CoreAudio installed.])
config_have_coreaudio="no"
if test $have_coreaudio = "1"; then
    config_have_coreaudio="yes"
fi

# MME MIDI (Win32)
AC_ARG_ENABLE(mmemidi-driver,
  [  --disable-mmemidi-driver
                          Disable support for the Windows MME MIDI system.],
  [config_mmemidi_driver="$enableval"],
  [config_mmemidi_driver="yes"]
)
have_mmemidi=0
if test "$config_mmemidi_driver" = "yes"; then
    AC_CHECK_HEADERS(mmsystem.h,
        have_mmemidi=1,
        have_mmemidi=0
    )
    if test "$have_mmemidi" = "1"; then
        have_midi_input_driver="true"
    fi
else
    echo "MME MIDI support disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_MME_MIDI, test $have_mmemidi = "1")
AC_DEFINE_UNQUOTED(HAVE_MME_MIDI,$have_mmemidi,[Define to 1 if you have MME MIDI installed.])
config_have_mme="no"
if test $have_mmemidi = "1"; then
    config_have_mme="yes"
fi

# DSSI
AC_CHECK_HEADERS(dssi.h,
	config_have_dssi="yes",
	config_have_dssi="no")
AM_CONDITIONAL(HAVE_DSSI, test $config_have_dssi = "yes")

# LV2
PKG_CHECK_MODULES(LV2, lv2 >= 1.0.0, config_have_lv2="yes", config_have_lv2="no")
if test "$config_have_lv2" != "yes"; then
    PKG_CHECK_MODULES(LV2, lv2core >= 1.0.0, config_have_lv2="yes", config_have_lv2="no")
fi
AC_SUBST(LV2_CFLAGS)
AC_SUBST(LV2_LIBS)
AM_CONDITIONAL(HAVE_LV2, test $config_have_lv2 = "yes")

# VST
AC_ARG_ENABLE(vstsdk-dir,
  [  --enable-vstsdk-dir
                          Directory where the VST SDK is located.
                          This automatically enables the compilation
                          of the VST plugin.],
  [VSTSDK_DIR="${enableval}"],
  [VSTSDK_DIR=]
)
AC_SUBST(VSTSDK_DIR)
AM_CONDITIONAL(HAVE_VST, test "x$VSTSDK_DIR" != "x")
config_have_vst="no"
if test "x$VSTSDK_DIR" != "x"; then
    config_have_vst="yes"
fi

# AU
AC_CHECK_HEADERS(AudioUnit/AudioUnit.h,
	config_have_au="yes",
	config_have_au="no")
AM_CONDITIONAL(HAVE_AU, test $config_have_au = "yes")
AM_CONDITIONAL(HAVE_AUFLAGS, test "$AUFLAGS" != "")

if test $config_have_au = "yes" ; then
    if test "x$DEVELOPER_EXTRAS_DIR" = "x" ; then
        if test -d /Developer/Extras ; then
            DEVELOPER_EXTRAS_DIR=/Developer/Extras
        else      
            DEVELOPER_EXTRAS_DIR=/Developer/Examples
        fi
    fi
    AC_SUBST(DEVELOPER_EXTRAS_DIR)
fi

if test "x$MAC_PLUGIN_INSTALL_DIR" = "x" ; then
    MAC_PLUGIN_INSTALL_DIR=/Library/Audio/Plug-Ins
fi
AC_SUBST(MAC_PLUGIN_INSTALL_DIR)

# have we found at least one MIDI input and one audio output driver ?
if test "$have_midi_input_driver" = "false"; then
    echo "No supported MIDI input system found!"
    echo "Sorry, LinuxSampler only supports the following MIDI drivers at the moment:"
    echo "ALSA, JACK, MIDIShare, CoreMIDI, MME."
    echo "If you think you have one of those available on your system, make sure you"
    echo "also have the respective development (header) files installed."
    exit -1;
fi
if test "$have_audio_output_driver" = "false"; then
    echo "No supported audio output system found!"
    echo "Sorry, LinuxSampler only supports ALSA, JACK, ARTS and ASIO as audio output"
    echo "driver at the moment!"
    exit -1;
fi



###########################################################################
# Checks for various DLL libraries

# Check presence of libgig
libgig_version="4.2.0"
PKG_CHECK_MODULES(GIG, gig >= $libgig_version, HAVE_GIG=true, HAVE_GIG=false)
if test "$HAVE_GIG" = "false"; then
    echo "Required libgig version not found!"
    echo "You need to have libgig version ${libgig_version} installed!"
    exit -1;
else
    echo "yes, found libgig $libgig_version"
fi
AC_SUBST(GIG_CFLAGS)
AC_SUBST(GIG_LIBS)

# SF2 Engine (requires libgig)
AC_ARG_ENABLE(sf2-engine,
  [  --disable-sf2-engine
                          Disable compilation of the SF2 engine.
			  You need to have libgig installed.],
  [config_sf2_engine="$enableval"],
  [config_sf2_engine="yes"]
)
HAVE_SF2=0;
if test "$config_sf2_engine" = "yes"; then
    HAVE_SF2=1
else
    echo "SF2 engine disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_SF2, test $HAVE_SF2 = 1)
AC_DEFINE_UNQUOTED(HAVE_SF2,$HAVE_SF2,[Define to 1 if you want SF2 engine and have libsf2 installed.])
config_have_sf2="no"
if test $HAVE_SF2 = "1"; then
    config_have_sf2="yes"
fi

# Check presence of libsndfile
libsndfile_version="1.0"
PKG_CHECK_MODULES(SNDFILE, sndfile >= $libsndfile_version, HAVE_SNDFILE=true, HAVE_SNDFILE=false)
if test "$HAVE_SNDFILE" = "false"; then
    echo "Required libsndfile version not found!"
    echo "You need to have libsndfile version ${libsndfile_version} installed!"
    exit -1;
else
    echo "yes, found libsndfile $libsndfile_version"
fi
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

# Check for Vorbis and FLAC support in libsndfile
linuxsampler_save_CFLAGS=$CFLAGS
CFLAGS="$SNDFILE_CFLAGS $CFLAGS"
AC_CHECK_DECLS([SF_FORMAT_VORBIS, SF_FORMAT_FLAC], [], [], [[#include <sndfile.h>]])

# Check for loop functionality in libsndfile
AC_CHECK_MEMBERS([SF_INSTRUMENT.loops],,
		 [AC_MSG_WARN([Your version of libsndfile does not support
    reading of loop information. LinuxSampler will not be able to
    extract loop information from sample files.])],
		 [#include <sndfile.h>])
CFLAGS=$linuxsampler_save_CFLAGS

# Instruments DB feature (requires SQLite 3.3)
AC_ARG_ENABLE(instruments-db,
  [  --disable-instruments-db
                          Disable compilation of the sampler's instruments
                          database feature. You need to have SQLite 3.3
                          or younger installed for this feature.],
  [config_instruments_db="$enableval"],
  [config_instruments_db="yes"]
)
HAVE_SQLITE3=0;
if test "$config_instruments_db" = "yes"; then
    # Check presence of sqlite3
    sqlite_version="3.3"
    PKG_CHECK_MODULES(SQLITE3, sqlite3 >= $sqlite_version, HAVE_SQLITE3=true, HAVE_SQLITE3=false)
    AC_SUBST(SQLITE3_LIBS)
    AC_SUBST(SQLITE3_CFLAGS)
    if test $HAVE_SQLITE3 = false; then
        HAVE_SQLITE3=0
        config_instruments_db="no"
        echo "*** Required sqlite version not found!"
        echo "*** You need to have sqlite version ${sqlite_version} or higher"
        echo "*** for instruments database support to be enabled."
        echo "*** Support for instruments DB will be disabled!"
    else
        HAVE_SQLITE3=1
    fi
else
    echo "Instruments DB feature disabled by configure script parameter"
fi
AM_CONDITIONAL(HAVE_SQLITE3, test $HAVE_SQLITE3 = 1)
AC_DEFINE_UNQUOTED(HAVE_SQLITE3,$HAVE_SQLITE3,[Define to 1 if you want the instruments DB feature and have SQLITE3 installed.])



###########################################################################
# Handle Configuration Options

# TODO: should we use AC_ARG_VAR(variable, description) instead?

AC_ARG_ENABLE(asm,
  [  --disable-asm
                          Enable hand-crafted assembly optimizations
                          (default=on). LinuxSampler provides CPU specific
                          assembly optimizations. This is currently limited
                          to just enter a fast (denormal) FPU mode on x86
                          platforms. There are currently no synthesis core
                          assembly optimizations anymore since LS 0.4.0],
  [config_asm="$enableval"],
  [config_asm="yes"]
)
if test "$config_asm" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_ASM, 1, [Define to 1 if you want to enable asm optimizations.])
fi

AC_ARG_ENABLE(dev-mode,
  [  --enable-dev-mode
                          Enable development mode (default=off). In that mode
                          we do some extra sanity checks here and there.
                          This helps to spot possible problems, but reduces
                          efficiency a bit],
  [config_dev_mode="$enableval"],
  [config_dev_mode="no"]
)
if test "$config_dev_mode" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_DEVMODE, 1, [Define to 1 if you want to enable development mode.])
fi

AC_ARG_ENABLE(debug-level,
  [  --enable-debug-level
                          Specify verbosity of console messages (default=1).
                          The higher the value, the higher will be verbosity.
                          A value of 0 means no console output at all.
                          There's not really an upper limit but the usual
                          level of all messages is currently somewhere less
                          than 10.],
  [config_debug_level="${enableval}"],
  [config_debug_level="1"]
)
AC_DEFINE_UNQUOTED(CONFIG_DEBUG_LEVEL, $config_debug_level, [Define console verbosity.])

AC_ARG_ENABLE(rt-exceptions,
  [  --enable-rt-exceptions
                          Enable exceptions in the realtime thread
                          (default=no). If this is enabled, exceptions will
                          be thrown on critical errors in the realtime
                          context as well. Otherwise if disabled
                          segmentation faults will be forced by the
                          application on critical errors.],
  [config_rt_exceptions="$enableval"],
  [config_rt_exceptions="no"]
)
if test "$config_rt_exceptions" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_RT_EXCEPTIONS, 1, [Define to 1 to allow exceptions in the realtime context.])
fi

config_pthread_testcancel="$mac"
AC_ARG_ENABLE(pthread-testcancel,
  [  --enable-pthread-testcancel
                          Enable pthread_testcancel() calls and avoid
                          asynchronous cancel of pthreads (default=yes
                          for Mac targets, no otherwise).],
  [config_pthread_testcancel="$enableval"],
  [])
if test "$config_pthread_testcancel" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_PTHREAD_TESTCANCEL, 1, [Define to 1 to enable pthread_testcancel() calls.])
fi

AC_ARG_ENABLE(preload-samples,
  [  --enable-preload-samples
                          Due to seeking and latency issues with hard drives
                          we have to cache a small part of samples' head in
                          RAM (default=32768). The higher this value the
                          more memory will be occupied for each sample, but
                          the safer this will be in regards of possible
                          droputs. A 'good' value depends on the running
                          system and usage dependant factors.],
  [config_preload_samples="${enableval}"],
  [config_preload_samples="32768"]
)
AC_DEFINE_UNQUOTED(CONFIG_PRELOAD_SAMPLES, $config_preload_samples, [Define amount of sample points to be cached in RAM.])

AC_ARG_ENABLE(max-pitch,
  [  --enable-max-pitch
                          Specify the maximum allowed pitch value in octaves
                          (default=4). To lower memory usage you might want
                          set a smaller value.],
  [config_max_pitch="${enableval}"],
  [config_max_pitch="4"]
)
AC_DEFINE_UNQUOTED(CONFIG_MAX_PITCH, $config_max_pitch, [Define max. allowed pitch.])

AC_ARG_ENABLE(max-events,
  [  --enable-max-events
                          Specify the maximum allowed amount of events to be
                          processed per fragment (default=1024).],
  [config_max_events="${enableval}"],
  [config_max_events="1024"]
)
AC_DEFINE_UNQUOTED(CONFIG_MAX_EVENTS_PER_FRAGMENT, $config_max_events, [Define max. allowed events per fragment.])

AC_ARG_ENABLE(eg-bottom,
  [  --enable-eg-bottom
                          Bottom limit of envelope generators
                          (default=0.001). Certain kinds of curve types like
                          exponential curves converge against 0 but never
                          reach 0. So we have to define a certain low value
                          after which we should consider all smaller values
                          to be 'almost zero'. The smaller this value, the
                          longer will voices survive in EG's release stage
                          and thus waste voices. If this value is too high
                          will cause click sounds though.],
  [config_eg_bottom="${enableval}"],
  [config_eg_bottom="0.001"]
)
AC_DEFINE_UNQUOTED(CONFIG_EG_BOTTOM, $config_eg_bottom, [Define bottom limit of envelopes.])

AC_ARG_ENABLE(eg-min-release-time,
  [  --enable-eg-min-release-time
                          Specify the lowest allowed release time in seconds
                          (default=0.0025). This value will also be used to
                          ramp down voices on voice stealing. This value
                          should always be less than the period time of the
                          used audio driver, as in case of voice stealing
                          the killed voice needs to be completely ramped
                          down in the same fragment.],
  [config_eg_min_release_time="${enableval}"],
  [config_eg_min_release_time="0.0025"]
)
AC_DEFINE_UNQUOTED(CONFIG_EG_MIN_RELEASE_TIME, $config_eg_min_release_time, [Define min. release time.])

AC_ARG_ENABLE(refill-streams,
  [  --enable-refill-streams
                          Number of streams that should be refilled in each
                          disk thread cycle (default=4).],
  [config_refill_streams="${enableval}"],
  [config_refill_streams="4"]
)
AC_DEFINE_UNQUOTED(CONFIG_REFILL_STREAMS_PER_RUN, $config_refill_streams, [Define amount of streams to be refilled per cycle.])

AC_ARG_ENABLE(stream-min-refill,
  [  --enable-stream-min-refill
                          Minimum refill size for disk streams (default=1024).
                          The disk thread will go to sleep for a while if no
                          stream had to be refilled more than this value in
                          a disk thread cycle.],
  [config_stream_min_refill="${enableval}"],
  [config_stream_min_refill="1024"]
)
AC_DEFINE_UNQUOTED(CONFIG_STREAM_MIN_REFILL_SIZE, $config_stream_min_refill, [Define min. stream refill size.])

AC_ARG_ENABLE(stream-max-refill,
  [  --enable-stream-max-refill
                          Maximum refill size for disk streams
                          (default=65536). The disk thread will refill
                          each stream only by a size of this value per
                          disk thread cycle.],
  [config_stream_max_refill="${enableval}"],
  [config_stream_max_refill="65536"]
)
AC_DEFINE_UNQUOTED(CONFIG_STREAM_MAX_REFILL_SIZE, $config_stream_max_refill, [Define max. stream refill size.])

AC_ARG_ENABLE(stream-size,
  [  --enable-stream-size
                          Size of each stream's ring buffer in sample points
                          (default=262144).],
  [config_stream_size="${enableval}"],
  [config_stream_size="262144"]
)
AC_DEFINE_UNQUOTED(CONFIG_STREAM_BUFFER_SIZE, $config_stream_size, [Define each stream's ring buffer size.])

AC_ARG_ENABLE(max-streams,
  [  --enable-max-streams
                          Initial maximum amount of disk streams
                          (default=90). This value can be changed at
                          runtime. It should always be higher than the
                          maximum amount of voices.],
  [config_max_streams="${enableval}"],
  [config_max_streams="90"]
)
AC_DEFINE_UNQUOTED(CONFIG_DEFAULT_MAX_STREAMS, $config_max_streams, [Define initial max. streams.])

AC_ARG_ENABLE(max-voices,
  [  --enable-max-voices
                          Initial maximum amount of voices (default=64).
                          This value can be changed at runtime. It should
                          always be lower than the maximum amount of disk
                          streams.],
  [config_max_voices="${enableval}"],
  [config_max_voices="64"]
)
AC_DEFINE_UNQUOTED(CONFIG_DEFAULT_MAX_VOICES, $config_max_voices, [Define initial max. voices.])

AC_ARG_ENABLE(subfragment-size,
  [  --enable-subfragment-size
                          Every audio fragment will be splitted into
                          subfragments. Where each subfragment renders
                          audio with constant synthesis parameters. This is
                          done for efficiency reasons. This parameter
                          defines the default size of a subfragment in
                          sample points. A large value means less CPU time
                          whereas a low value means better audio quality
                          (default=32).],
  [config_subfragment_size="${enableval}"],
  [config_subfragment_size="32"]
)
AC_DEFINE_UNQUOTED(CONFIG_DEFAULT_SUBFRAGMENT_SIZE, $config_subfragment_size, [Define default subfragment size (in sample points).])

AC_ARG_ENABLE(global-attenuation-default,
  [  --enable-global-attenuation-default
                          To prevent clipping all samples will be lowered
                          in amplitude by this given default factor (can
                          be overridden at runtime).
                          (default=0.35)],
  [config_global_attenuation_default="${enableval}"],
  [config_global_attenuation_default="0.35"]
)
AC_DEFINE_UNQUOTED(CONFIG_GLOBAL_ATTENUATION_DEFAULT, $config_global_attenuation_default, [Define default global volume attenuation (as floating point factor).])

AC_ARG_ENABLE(voice-steal-algo,
  [  --enable-voice-steal-algo
                          Voice stealing algorithm to be used. Currently
                          available options:
                            none:
                              Disable voice stealing completely.
                            oldestvoiceonkey (default):
                              Try to kill a voice on the same key first,
                              if no success, proceed with the oldest key.
                            oldestkey:
                              Try to kill a voice from the oldest active
                              key.],
  [ if test ! "(" "${enableval}" = "none" \
              -o "${enableval}" = "oldestvoiceonkey" \
              -o "${enableval}" = "oldestkey" ")" ; then
      AC_MSG_ERROR([Unknown voice stealing algorithm for parameter --enable-voice-steal-algo])
    else
      config_voice_steal_algo="${enableval}"
    fi
  ],
  [config_voice_steal_algo="oldestvoiceonkey"]
)
AC_DEFINE_UNQUOTED(CONFIG_VOICE_STEAL_ALGO, voice_steal_algo_${config_voice_steal_algo}, [Define voice stealing algorithm to be used.])

AC_ARG_ENABLE(sysex-buffer-size,
  [  --enable-sysex-buffer-size
                          System Exclusive Message buffer size in kB
                          (default=2048).],
  [config_sysex_buffer_size="${enableval}"],
  [config_sysex_buffer_size="2048"]
)
AC_DEFINE_UNQUOTED(CONFIG_SYSEX_BUFFER_SIZE, $config_sysex_buffer_size, [Define SysEx buffer size.])

AC_ARG_ENABLE(force-filter,
  [  --enable-force-filter
                          If enabled will force filter to be used even if
                          no usage was define in instrument patch files.
                          (default=no).],
  [config_force_filter="$enableval"],
  [config_force_filter="no"]
)
if test "$config_force_filter" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_FORCE_FILTER, 1, [Define to 1 to force filter usage.])
fi

AC_ARG_ENABLE(filter-cutoff-min,
  [  --enable-filter-cutoff-min
                          Minimum filter cutoff frequency in Hz
                          (default=100.0).],
  [config_filter_cutoff_min="${enableval}"],
  [config_filter_cutoff_min="100.0"]
)
AC_DEFINE_UNQUOTED(CONFIG_FILTER_CUTOFF_MIN, ${config_filter_cutoff_min}f, [Define min. filter cutoff frequency.])

AC_ARG_ENABLE(filter-cutoff-max,
  [  --enable-filter-cutoff-max
                          Maximum filter cutoff frequency in Hz
                          (default=10000.0).],
  [config_filter_cutoff_max="${enableval}"],
  [config_filter_cutoff_max="10000.0"]
)
AC_DEFINE_UNQUOTED(CONFIG_FILTER_CUTOFF_MAX, ${config_filter_cutoff_max}f, [Define max. filter cutoff frequency.])

AC_ARG_ENABLE(override-cutoff-ctrl,
  [  --enable-override-cutoff-ctrl
                          Override filter cutoff MIDI controller (default=no).
                          Note: you have to define the MIDI controller number
                          here, it's not a boolean parameter type! If this
                          option is used, controller number given by
                          instrument patch will be ignored and instead this
                          supplied value will be used.],
  [config_override_cutoff_ctrl="${enableval}"],
  [config_override_cutoff_ctrl="no"]
)
if test ! "$config_override_cutoff_ctrl" = "no"; then
  AC_DEFINE_UNQUOTED(CONFIG_OVERRIDE_CUTOFF_CTRL, $config_override_cutoff_ctrl, [Define to a MIDI controller number to override cutoff control.])
fi

AC_ARG_ENABLE(override-resonance-ctrl,
  [  --enable-override-resonance-ctrl
                          Override filter resonance MIDI controller
                          (default=no). Note: you have to define the MIDI
                          controller number here, it's not a boolean
                          parameter type! If this option is used, controller
                          number given by instrument patch will be ignored
                          and instead this supplied value will be used.],
  [config_override_resonance_ctrl="${enableval}"],
  [config_override_resonance_ctrl="no"]
)
if test ! "$config_override_resonance_ctrl" = "no"; then
  AC_DEFINE_UNQUOTED(CONFIG_OVERRIDE_RESONANCE_CTRL, $config_override_resonance_ctrl, [Define to a MIDI controller number to override resonance control.])
fi

AC_ARG_ENABLE(override-filter-type,
  [  --enable-override-filter-type
                          Override filter type (default=no). Options:
                          hp: for highpass
                          bp: for bandpass
                          br: for bandreject
                          lp: for lowpass
                          lpt: for lowpass turbo],
  [ if   test "${enableval}" = "hp" ; then
       config_override_filter_type="::gig::vcf_type_highpass"
    elif test "${enableval}" = "bp" ; then
       config_override_filter_type="::gig::vcf_type_bandpass"
    elif test "${enableval}" = "br" ; then
       config_override_filter_type="::gig::vcf_type_bandreject"
    elif test "${enableval}" = "lp" ; then
       config_override_filter_type="::gig::vcf_type_lowpass"
    elif test "${enableval}" = "lpt" ; then
       config_override_filter_type="::gig::vcf_type_lowpassturbo"
    elif test ! "${enableval}" = "no"; then
       AC_MSG_ERROR([Unknown filter type for parameter --enable-override-filter-type])
    fi
  ],
  [config_override_filter_type="no"]
)
if test ! "$config_override_filter_type" = "no"; then
  AC_DEFINE_UNQUOTED(CONFIG_OVERRIDE_FILTER_TYPE, $config_override_filter_type, [Define to a filter type to always force that filter type.])
fi

AC_ARG_ENABLE(gs-checksum,
  [  --enable-gs-checksum
                          Enable Roland General Synth SysEx checksum check
                          (default=no). If this is enabled, all GS SysEx
                          messages which do not provide a correct checksum
                          will be ignored. This is disabled by default as
                          not all devices honor GS checksums.],
  [config_assert_gs_sysex_checksum="$enableval"],
  [config_assert_gs_sysex_checksum="no"]
)
if test "config_assert_gs_sysex_checksum" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_ASSERT_GS_SYSEX_CHECKSUM, 1, [Define to 1 if you want to enable GS SysEx check.])
fi

AC_ARG_ENABLE(portamento-time-min,
  [  --enable-portamento-time-min
                          Minimum Portamento time in seconds
                          (default=0.1).],
  [config_portamento_time_min="${enableval}"],
  [config_portamento_time_min="0.1"]
)
AC_DEFINE_UNQUOTED(CONFIG_PORTAMENTO_TIME_MIN, $config_portamento_time_min, [Define min. portamento time.])

AC_ARG_ENABLE(portamento-time-max,
  [  --enable-portamento-time-max
                          Maximum Portamento time in seconds
                          (default=32).],
  [config_portamento_time_max="${enableval}"],
  [config_portamento_time_max="32"]
)
AC_DEFINE_UNQUOTED(CONFIG_PORTAMENTO_TIME_MAX, $config_portamento_time_max, [Define max. portamento time.])

AC_ARG_ENABLE(portamento-time-default,
  [  --enable-portamento-time-default
                          Default Portamento time in seconds
                          (default=1).],
  [config_portamento_time_default="${enableval}"],
  [config_portamento_time_default="1"]
)
AC_DEFINE_UNQUOTED(CONFIG_PORTAMENTO_TIME_DEFAULT, $config_portamento_time_default, [Define default portamento time.])

AC_ARG_ENABLE(signed-triang-algo,
  [  --enable-signed-triang-algo
                          Signed triangular wave algorithm to be used (e.g. for LFOs).
                          Currently available options:
                            intmath:
                              Uses integer math without any branch will then be
                              converted to floating point value for each sample point.
                              This int->float conversion might hurt on some systems.
                            intmathabs:
                              Similar to intmath but uses abs() function.
                              Depending on compiler and platrofm this could
                              perform better than integer math as it avoids
                              an extra integer multiply instruction.
                            diharmonic:
                              The triangular wave will be approximated by adding two
                              sinusoidials. This solution might especially hurt on
                              systems with weak floating point unit.
                            benchmark (default):
                              This is not an algorithm. Use this option if the
                              appropriate algorithm should be automatically
                              chosen by the configure script by performing a
                              benchmark between the algorithms mentioned above.
                              This will NOT work for cross compilation!],
  [ if test ! "(" "${enableval}" = "intmath" \
              -o "${enableval}" = "intmathabs" \
              -o "${enableval}" = "diharmonic" ")" ; then
      AC_MSG_ERROR([Unknown triangular wave algorithm for parameter --enable-signed-triang-algo])
    else
      config_signed_triang_algo="${enableval}"
    fi
  ],
  [config_signed_triang_algo="benchmark"]
)

AC_ARG_ENABLE(unsigned-triang-algo,
  [  --enable-unsigned-triang-algo
                          Unsigned triangular wave algorithm to be used (e.g. for LFOs).
                          Currently available options:
                            intmath:
                              Uses integer math without any branch will then be
                              converted to floating point value for each sample point.
                              This int->float conversion might hurt on some systems.
                            intmathabs:
                              Similar to intmath but uses abs() function.
                              Depending on compiler and platrofm this could
                              perform better than integer math as it avoids
                              an extra integer multiply instruction.
                            diharmonic:
                              The triangular wave will be approximated by adding two
                              sinusoidials. This solution might especially hurt on
                              systems with weak floating point unit.
                            benchmark (default):
                              This is not an algorithm. Use this option if the
                              appropriate algorithm should be automatically
                              chosen by the configure script by performing a
                              benchmark between the algorithms mentioned above.
                              This will NOT work for cross compilation!],
  [ if test ! "(" "${enableval}" = "intmath" \
              -o "${enableval}" = "intmathabs" \
              -o "${enableval}" = "diharmonic" ")" ; then
      AC_MSG_ERROR([Unknown triangular wave algorithm for parameter --enable-unsigned-triang-algo])
    else
      config_unsigned_triang_algo="${enableval}"
    fi
  ],
  [config_unsigned_triang_algo="benchmark"]
)

AC_ARG_ENABLE(process-muted-channels,
  [  --enable-process-muted-channels
                          Enable processing of muted channels (default=no).
                          In that mode all MIDI events in the muted channels
                          will be processed. This will provide information
                          about the active voices in the muted channels and
                          will not discard notes, triggered in mute mode,
                          when the channel is unmuted. But also will reduce
                          the efficiency.],
  [config_process_muted_channels="$enableval"],
  [config_process_muted_channels="no"]
)
if test "$config_process_muted_channels" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_PROCESS_MUTED_CHANNELS, 1, [Define to 1 if you want to enable processing of muted channels.])
fi

AC_ARG_ENABLE(process-all-notes-off,
  [  --disable-process-all-notes-off
                          Disable interpretation of All-Notes-Off MIDI
                          messages (default=on). By default LS will release
                          all voices whenever it receives an All-Notes-Off
                          MIDI message. You can disable this behavior, so
                          that LS simply ignores such messages.],
  [config_process_all_notes_off="$enableval"],
  [config_process_all_notes_off="yes"]
)
if test "$config_process_all_notes_off" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_PROCESS_ALL_NOTES_OFF, 1, [Define to 1 if you want to enable processing of All-Notes-Off MIDI messages.])
fi

AC_ARG_ENABLE(interpolate-volume,
  [  --disable-interpolate-volume
                          Disable interpolation of volume modulation
                          (default=on). With this enabled, the volume changes
                          generated by for example the envelope generator
                          will be smoother, minimizing the risk for audio
                          clicks. Disable it to reduce CPU usage.],
  [config_interpolate_volume="$enableval"],
  [config_interpolate_volume="yes"]
)
if test "$config_interpolate_volume" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_INTERPOLATE_VOLUME, 1, [Define to 1 if you want to enable interpolation of volume modulation.])
fi

AC_ARG_ENABLE(master-volume-sysex-by-port,
  [  --enable-master-volume-sysex-by-port
                          Whether global volume sysex message should be
                          applied globally to the whole sampler or only to
                          the sampler channels connected to the same MIDI
                          input port on which the sysex message arrived on.
                          By default global volume sysex messages apply
                          globally to the whole sampler, since many MIDI
                          devices behave that way.],
  [config_master_volume_sysex_by_port="$enableval"],
  [config_master_volume_sysex_by_port="no"]
)
if test "$config_master_volume_sysex_by_port" = "yes"; then
  AC_DEFINE_UNQUOTED(CONFIG_MASTER_VOLUME_SYSEX_BY_PORT, 1, [Define to 1 if you want global volume sysex message only be applied to the respective MIDI port.])
fi

AC_ARG_ENABLE(plugin-dir,
  [  --enable-plugin-dir
                          Directory where the sampler shall look for potential plugins,
                          that is 3rd party shared libraries that should be loaded by
                          the sampler on startup. By default the sampler will search
                          for plugins in the subdirectory "plugins" below its own
                          library directory.
                          (i.e. /usr/local/lib/linuxsampler/plugins)],
  [config_plugin_dir="${enableval}"],
  [config_plugin_dir="${libdir}/linuxsampler/plugins"]
)
AC_SUBST(config_plugin_dir)

AC_ARG_ENABLE(default-instruments-db-location,
  [  --enable-default-instruments-db-location
                          Only when instruments DB feature is enabled: file name
                          which shall be taken as default location of the
                          instruments DB file. This location can still be
                          overridden at runtime with a command line switch.
                          (default: /var/lib/linuxsampler/instruments.db)],
  [config_default_instruments_db_file="${enableval}"],
  [config_default_instruments_db_file="/var/lib/linuxsampler/instruments.db"]
)
AC_DEFINE_UNQUOTED(
    CONFIG_DEFAULT_INSTRUMENTS_DB_LOCATION,
    "$config_default_instruments_db_file",
    [Only when instruments DB feature is enabled: default location of the DB file.]
)
AC_SUBST(config_default_instruments_db_file)


###########################################################################
# Automatic Benchmarks (to detect the best algorithms for the system)

if test "$config_signed_triang_algo" = "benchmark"; then
    echo -n "benchmarking for the best (signed) triangular oscillator algorithm... "
    AC_LANG_PUSH([C++])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
            #define SIGNED 1
            #define SILENT 1
            #include "${srcdir}/benchmarks/triang.cpp"
        ]])],
        triang_signed=0,
        triang_signed=$?,
        triang_signed=0
    )
    AC_LANG_POP([C++])
    if test "$triang_signed" = "2"; then
        config_signed_triang_algo="intmath"
        echo "integer math"
    elif test "$triang_signed" = "3"; then
        config_signed_triang_algo="diharmonic"
        echo "di harmonics"
    elif test "$triang_signed" = "5"; then
        config_signed_triang_algo="intmathabs"
        echo "integer math using abs()"
    else
        echo "Benchmark of signed triangular wave algorithms failed!"
        echo "Maybe you are doing cross compilation? In that case you have to select"
        echo "an algorithm manually with './configure --enable-signed-triang-algo=...'"
        echo "Call './configure --help' for further information or read configure.in."
        exit -1;
    fi
else
    case "$config_signed_triang_algo" in
        intmath)
            triang_signed=2 ;;
        diharmonic)
            triang_signed=3 ;;
        intmathabs)
            triang_signed=5 ;;
    esac
fi
AC_DEFINE_UNQUOTED(CONFIG_SIGNED_TRIANG_ALGO, ${triang_signed}, [Define signed triangular wave algorithm to be used.])

if test "$config_unsigned_triang_algo" = "benchmark"; then
    echo -n "benchmarking for the best (unsigned) triangular oscillator algorithm... "
    AC_LANG_PUSH([C++])
    AC_RUN_IFELSE([AC_LANG_SOURCE([[
            #define SIGNED 0
            #define SILENT 1
            #include "${srcdir}/benchmarks/triang.cpp"
        ]])],
        triang_unsigned=0,
        triang_unsigned=$?,
        triang_unsigned=0
    )
    AC_LANG_POP([C++])
    if test "$triang_unsigned" = "2"; then
        config_unsigned_triang_algo="intmath"
        echo "integer math"
    elif test "$triang_unsigned" = "3"; then
        config_unsigned_triang_algo="diharmonic"
        echo "di harmonics"
    elif test "$triang_unsigned" = "5"; then
        config_unsigned_triang_algo="intmathabs"
        echo "integer math using abs()"
    else
        echo "Benchmark of unsigned triangular wave algorithms failed!"
        echo "Maybe you are doing cross compilation? In that case you have to select"
        echo "an algorithm manually with './configure --enable-unsigned-triang-algo=...'"
        echo "Call './configure --help' for further information or read configure.in."
        exit -1;
    fi
else
    case "$config_unsigned_triang_algo" in
        intmath)
            triang_unsigned=2 ;;
        diharmonic)
            triang_unsigned=3 ;;
        intmathabs)
            triang_unsigned=5 ;;
    esac
fi
AC_DEFINE_UNQUOTED(CONFIG_UNSIGNED_TRIANG_ALGO, ${triang_unsigned}, [Define unsigned triangular wave algorithm to be used.])


###########################################################################
# Create Build Files

AC_CONFIG_HEADERS([config.h])

AC_LANG([C++])

# autoconf 2.59/libtool 1.5.12 bug? work-around. Without a check like
# this, the dlfcn.h check in am_prog_libtool may fail.
AC_CHECK_HEADER(stdlib.h)

AC_CONFIG_FILES([\
    Makefile \
    man/Makefile \
    man/linuxsampler.1 \
    man/lscp.1 \
    src/Makefile \
    src/db/Makefile \
    src/network/Makefile \
    src/engines/Makefile \
    src/engines/gig/Makefile \
    src/engines/sf2/Makefile \
    src/engines/sfz/Makefile \
    src/engines/common/Makefile \
    src/effects/Makefile \
    src/common/Makefile \
    src/testcases/Makefile \
    src/drivers/Makefile \
    src/drivers/audio/Makefile \
    src/drivers/midi/Makefile \
    src/plugins/Makefile \
    src/hostplugins/Makefile \
    src/hostplugins/dssi/Makefile \
    src/hostplugins/lv2/Makefile \
    src/hostplugins/lv2/manifest.ttl \
    src/hostplugins/vst/Makefile \
    src/hostplugins/au/Makefile \
    src/scriptvm/Makefile \
    src/scriptvm/editor/Makefile \
    src/shell/Makefile \
    linuxsampler.spec \
    Artwork/Makefile \
    scripts/Makefile \
    osx/Makefile \
    osx/linuxsampler.xcodeproj/Makefile \
    Documentation/Makefile \
    Documentation/Engines/Makefile \
    Documentation/Engines/gig/Makefile \
    linuxsampler.pc \
    Doxyfile \
])
AC_OUTPUT

# resolve all nested variables in '${config_plugin_dir}'
# (merely for providing a human readable summary below)
while test $config_plugin_dir != `eval echo ${config_plugin_dir}` ; do
	config_plugin_dir=`eval echo ${config_plugin_dir}`
done


###########################################################################
# Output All Configuration Options

echo ""
echo "#####################################################################"
echo "# LinuxSampler Configuration                                        #"
echo "#-------------------------------------------------------------------#"
echo "# Release Version: ${VERSION}"
echo "# LSCP Version: ${LSCP_RELEASE_MAJOR}.${LSCP_RELEASE_MINOR}"
echo "#-------------------------------------------------------------------"
echo "# Assembly Optimizations: ${config_asm}"
echo "# Development Mode: ${config_dev_mode}"
echo "# Debug Level: ${config_debug_level}"
echo "# Use Exceptions in RT Context: ${config_rt_exceptions}"
echo "# Preload Samples: ${config_preload_samples}"
echo "# Maximum Pitch: ${config_max_pitch} (octaves)"
echo "# Maximum Events: ${config_max_events}"
echo "# Envelope Bottom Level: ${config_eg_bottom} (linear)"
echo "# Envelope Minimum Release Time: ${config_eg_min_release_time} s"
echo "# Streams to be refilled per Disk Thread Cycle: ${config_refill_streams}"
echo "# Minimum Stream Refill Size: ${config_stream_min_refill}"
echo "# Maximum Stream Refill Size: ${config_stream_max_refill}"
echo "# Stream Size: ${config_stream_size}"
echo "# Default Maximum Disk Streams: ${config_max_streams}"
echo "# Default Maximum Voices: ${config_max_voices}"
echo "# Default Subfragment Size: ${config_subfragment_size}"
echo "# Default Global Volume Attenuation: ${config_global_attenuation_default}"
echo "# Voice Stealing Algorithm: ${config_voice_steal_algo}"
echo "# Signed Triangular Oscillator Algorithm: ${config_signed_triang_algo}"
echo "# Unsigned Triangular Oscillator Algorithm: ${config_unsigned_triang_algo}"
echo "# SysEx Buffer Size: ${config_sysex_buffer_size} Byte"
echo "# Min. Portamento Time: ${config_portamento_time_min} s"
echo "# Max. Portamento Time: ${config_portamento_time_max} s"
echo "# Default Portamento Time: ${config_portamento_time_default} s"
echo "# Force Filter Usage: ${config_force_filter}"
echo "# Filter Cutoff Minimum: ${config_filter_cutoff_min} Hz"
echo "# Filter Cutoff Maximum: ${config_filter_cutoff_max} Hz"
echo "# Override Filter Cutoff Controller: ${config_override_cutoff_ctrl}"
echo "# Override Filter Resonance Controller: ${config_override_resonance_ctrl}"
echo "# Override Filter Type: ${config_override_filter_type}"
echo "# Assert GS SysEx Checksum: ${config_assert_gs_sysex_checksum}"
echo "# Process Muted Channels: ${config_process_muted_channels}"
echo "# Process All-Notes-Off MIDI message: ${config_process_all_notes_off}"
echo "# Apply global volume SysEx by MIDI port: ${config_master_volume_sysex_by_port}"
echo "# Interpolate Volume: ${config_interpolate_volume}"
echo "# Instruments database support: ${config_instruments_db}"
if test "$config_instruments_db" = "yes"; then
echo "# Instruments DB default location: ${config_default_instruments_db_file}"
fi
echo "# Plugin Path: ${config_plugin_dir}"
echo "#-------------------------------------------------------------------"
echo "# MIDI Input Drivers:"
echo "# ALSA: ${config_have_alsa}, JACK: ${config_have_jack_midi}, CoreMIDI: ${config_have_coremidi}, MME: ${config_have_mme}, MidiShare: ${config_have_midishare}"
echo "#-------------------------------------------------------------------"
echo "# Audio Output Drivers:"
echo "# ALSA: ${config_have_alsa}, JACK: ${config_have_jack}, ARTS: ${config_have_arts}, CoreAudio: ${config_have_coreaudio}, ASIO: ${config_have_asio}"
echo "#-------------------------------------------------------------------"
echo "# Sampler Engines:"
echo "# GIG: yes, SF2: ${config_have_sf2}, SFZ: yes"
echo "#-------------------------------------------------------------------"
echo "# Effect plugin systems for internal effects:"
echo "# LADSPA: yes"
echo "#-------------------------------------------------------------------"
echo "# Building sampler as plugin for following host standards:"
echo "# DSSI: ${config_have_dssi}, LV2: ${config_have_lv2}, VST: ${config_have_vst}, AU: ${config_have_au}"
echo "#-------------------------------------------------------------------#"
echo "# Read './configure --help' or file 'configure.in' for details.     #"
echo "#####################################################################"
echo ""
echo "Good. Now type 'make' to compile, followed by 'make install' as root."
echo ""
