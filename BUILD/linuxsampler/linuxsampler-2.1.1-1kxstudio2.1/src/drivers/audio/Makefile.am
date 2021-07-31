METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

if HAVE_ALSA
alsa_src  = AudioOutputDeviceAlsa.cpp AudioOutputDeviceAlsa.h
alsa_ladd = -lasound
else
alsa_src  =
alsa_ladd =
endif

if HAVE_JACK
jack_src      = AudioOutputDeviceJack.cpp AudioOutputDeviceJack.h
jack_lflags   = $(JACK_LIBS)
jack_includes = $(JACK_CFLAGS)
else
jack_src      =
jack_lflags   =
jack_includes =
endif

if HAVE_ARTS
arts_src      = AudioOutputDeviceArts.cpp AudioOutputDeviceArts.h
arts_ladd     = @ARTS_LIBS@
arts_includes = @ARTS_CFLAGS@
else
arts_src      =
arts_ladd     =
arts_includes =
endif

if HAVE_ASIO
asio_includes = -I@ASIOSDK_BASEDIR@/ASIOSDK2/common -I@ASIOSDK_BASEDIR@/ASIOSDK2/host -I@ASIOSDK_BASEDIR@/ASIOSDK2/host/pc
asio_src  = AudioOutputDeviceAsio.cpp AudioOutputDeviceAsio.h \
	iasiothiscallresolver.cpp iasiothiscallresolver.h \
	asiosdk-asio.cpp asiosdk-asiodrivers.cpp asiosdk-asiolist.cpp
asio_ladd =
else
asio_src  =
asio_includes  =
asio_ladd =
endif


if HAVE_COREAUDIO
coreaudio_src  = AudioOutputDeviceCoreAudio.cpp AudioOutputDeviceCoreAudio.h \
    CAAudioDeviceModel.cpp CAAudioDeviceModel.h
coreaudio_ldflags = -framework CoreAudio -framework AudioToolbox
else
coreaudio_src  =
coreaudio_ldflags =
endif

liblinuxsampleraudiodriverincludedir = $(includedir)/linuxsampler/drivers/audio
liblinuxsampleraudiodriverinclude_HEADERS = \
	AudioChannel.h \
	AudioOutputDevice.h

AM_CPPFLAGS = $(all_includes) $(arts_includes) $(asio_includes) $(jack_includes)

noinst_LTLIBRARIES = liblinuxsampleraudiodriver.la
liblinuxsampleraudiodriver_la_SOURCES = \
	AudioChannel.cpp AudioChannel.h \
	AudioOutputDevice.cpp AudioOutputDevice.h \
	AudioOutputDeviceFactory.cpp AudioOutputDeviceFactory.h \
	$(alsa_src) $(jack_src) $(arts_src) $(asio_src) $(coreaudio_src) \
	AudioOutputDevicePlugin.cpp AudioOutputDevicePlugin.h

liblinuxsampleraudiodriver_la_LIBADD = $(alsa_ladd) $(arts_ladd)
liblinuxsampleraudiodriver_la_LDFLAGS = $(jack_lflags) $(coreaudio_ldflags)
