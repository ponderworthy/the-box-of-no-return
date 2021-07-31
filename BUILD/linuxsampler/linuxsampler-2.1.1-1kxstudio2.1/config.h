/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to the major version number of your Alsa installation. */
#define ALSA_MAJOR 1

/* Define to the minor version number of your Alsa installation. */
#define ALSA_MINOR 1

/* Define to the subminor version number of your Alsa installation. */
#define ALSA_SUBMINOR 8

/* Define to 1 if you build for x86 architecture. */
#define ARCH_X86 1

/* Define to 1 if you want to enable asm optimizations. */
#define CONFIG_ASM 1

/* Define to 1 if you want to enable GS SysEx check. */
/* #undef CONFIG_ASSERT_GS_SYSEX_CHECKSUM */

/* Define console verbosity. */
#define CONFIG_DEBUG_LEVEL 1

/* Only when instruments DB feature is enabled: default location of the DB
   file. */
#define CONFIG_DEFAULT_INSTRUMENTS_DB_LOCATION "/var/lib/linuxsampler/instruments.db"

/* Define initial max. streams. */
#define CONFIG_DEFAULT_MAX_STREAMS 90

/* Define initial max. voices. */
#define CONFIG_DEFAULT_MAX_VOICES 64

/* Define default subfragment size (in sample points). */
#define CONFIG_DEFAULT_SUBFRAGMENT_SIZE 32

/* Define to 1 if you want to enable development mode. */
/* #undef CONFIG_DEVMODE */

/* Define bottom limit of envelopes. */
#define CONFIG_EG_BOTTOM 0.001

/* Define min. release time. */
#define CONFIG_EG_MIN_RELEASE_TIME 0.0025

/* Define max. filter cutoff frequency. */
#define CONFIG_FILTER_CUTOFF_MAX 10000.0f

/* Define min. filter cutoff frequency. */
#define CONFIG_FILTER_CUTOFF_MIN 100.0f

/* Define to 1 to force filter usage. */
/* #undef CONFIG_FORCE_FILTER */

/* Define default global volume attenuation (as floating point factor). */
#define CONFIG_GLOBAL_ATTENUATION_DEFAULT 0.35

/* Define to 1 if you want to enable interpolation of volume modulation. */
#define CONFIG_INTERPOLATE_VOLUME 1

/* Define to 1 if you want global volume sysex message only be applied to the
   respective MIDI port. */
/* #undef CONFIG_MASTER_VOLUME_SYSEX_BY_PORT */

/* Define max. allowed events per fragment. */
#define CONFIG_MAX_EVENTS_PER_FRAGMENT 1024

/* Define max. allowed pitch. */
#define CONFIG_MAX_PITCH 4

/* Define to a MIDI controller number to override cutoff control. */
/* #undef CONFIG_OVERRIDE_CUTOFF_CTRL */

/* Define to a filter type to always force that filter type. */
/* #undef CONFIG_OVERRIDE_FILTER_TYPE */

/* Define to a MIDI controller number to override resonance control. */
/* #undef CONFIG_OVERRIDE_RESONANCE_CTRL */

/* Define default portamento time. */
#define CONFIG_PORTAMENTO_TIME_DEFAULT 1

/* Define max. portamento time. */
#define CONFIG_PORTAMENTO_TIME_MAX 32

/* Define min. portamento time. */
#define CONFIG_PORTAMENTO_TIME_MIN 0.1

/* Define amount of sample points to be cached in RAM. */
#define CONFIG_PRELOAD_SAMPLES 32768

/* Define to 1 if you want to enable processing of All-Notes-Off MIDI
   messages. */
#define CONFIG_PROCESS_ALL_NOTES_OFF 1

/* Define to 1 if you want to enable processing of muted channels. */
/* #undef CONFIG_PROCESS_MUTED_CHANNELS */

/* Define to 1 to enable pthread_testcancel() calls. */
/* #undef CONFIG_PTHREAD_TESTCANCEL */

/* Define amount of streams to be refilled per cycle. */
#define CONFIG_REFILL_STREAMS_PER_RUN 4

/* Define to 1 to allow exceptions in the realtime context. */
/* #undef CONFIG_RT_EXCEPTIONS */

/* Define signed triangular wave algorithm to be used. */
#define CONFIG_SIGNED_TRIANG_ALGO 5

/* Define each stream's ring buffer size. */
#define CONFIG_STREAM_BUFFER_SIZE 262144

/* Define max. stream refill size. */
#define CONFIG_STREAM_MAX_REFILL_SIZE 65536

/* Define min. stream refill size. */
#define CONFIG_STREAM_MIN_REFILL_SIZE 1024

/* Define SysEx buffer size. */
#define CONFIG_SYSEX_BUFFER_SIZE 2048

/* Define unsigned triangular wave algorithm to be used. */
#define CONFIG_UNSIGNED_TRIANG_ALGO 5

/* Define voice stealing algorithm to be used. */
#define CONFIG_VOICE_STEAL_ALGO voice_steal_algo_oldestvoiceonkey

/* Define to 1 if you have ALSA installed. */
#define HAVE_ALSA 1

/* Define to 1 if you have aRts installed. */
#define HAVE_ARTS 0

/* Define to 1 if you have ASIO installed. */
#define HAVE_ASIO 0

/* Define to 1 if you have the <AudioUnit/AudioUnit.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOUNIT_H */

/* Define to the major version of the GNU Bison program installed. */
#define HAVE_BISON_MAJ 3

/* Define to the minor version of the GNU Bison program installed. */
#define HAVE_BISON_MIN 3

/* Define to 1 if you have CoreAudio installed. */
#define HAVE_COREAUDIO 0

/* Define to 1 if you have CoreMIDI installed. */
#define HAVE_COREMIDI 0

/* Define to 1 if your compiler supports embedded pragma diagnostics. */
#define HAVE_CXX_EMBEDDED_PRAGMA_DIAGNOSTICS 0

/* Define to 1 if you have the declaration of `SF_FORMAT_FLAC', and to 0 if
   you don't. */
#define HAVE_DECL_SF_FORMAT_FLAC 1

/* Define to 1 if you have the declaration of `SF_FORMAT_VORBIS', and to 0 if
   you don't. */
#define HAVE_DECL_SF_FORMAT_VORBIS 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <dssi.h> header file. */
#define HAVE_DSSI_H 1

/* Define to 1 if you have the <features.h> header file. */
#define HAVE_FEATURES_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have JACK installed. */
#define HAVE_JACK 1

/* Define to 1 if you have the `jack_client_name_size' function. */
#define HAVE_JACK_CLIENT_NAME_SIZE 1

/* Define to 1 if you have the `jack_client_open' function. */
#define HAVE_JACK_CLIENT_OPEN 1

/* Define to 1 if you have JACK with MIDI support installed. */
#define HAVE_JACK_MIDI 1

/* Define to 1 if you have the `jack_midi_get_event_count' function. */
#define HAVE_JACK_MIDI_GET_EVENT_COUNT 1

/* Define to 1 if you have the `jack_on_info_shutdown' function. */
#define HAVE_JACK_ON_INFO_SHUTDOWN 1

/* Define to 1 if you have the `artsc' library (-lartsc). */
/* #undef HAVE_LIBARTSC */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have MidiShare installed. */
#define HAVE_MIDISHARE 0

/* Define to 1 if you have MME MIDI installed. */
#define HAVE_MME_MIDI 0

/* Define to 1 if you have the <mmsystem.h> header file. */
/* #undef HAVE_MMSYSTEM_H */

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Define to 1 if you want SF2 engine and have libsf2 installed. */
#define HAVE_SF2 1

/* Define to 1 if `loops' is a member of `SF_INSTRUMENT'. */
#define HAVE_SF_INSTRUMENT_LOOPS 1

/* Define to 1 if you want the instruments DB feature and have SQLITE3
   installed. */
#define HAVE_SQLITE3 0

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the old jack midi functions that need an nframes
   argument. */
/* #undef JACK_MIDI_FUNCS_NEED_NFRAMES */

/* LSCP spec major version this release complies with. */
#define LSCP_RELEASE_MAJOR 1

/* LSCP spec minor version this release complies with. */
#define LSCP_RELEASE_MINOR 7

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "linuxsampler"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "linuxsampler"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "linuxsampler 2.1.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "linuxsampler"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.1.1"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.1.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
/* #undef YYTEXT_POINTER */
