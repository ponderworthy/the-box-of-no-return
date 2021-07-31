METASOURCES = AUTO

AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

if HAVE_ALSA
alsa_src  = MidiInputDeviceAlsa.cpp MidiInputDeviceAlsa.h
alsa_ladd = -lasound
else
alsa_src  =
alsa_ladd =
endif

if HAVE_MIDISHARE
midishare_src  = MidiInputDeviceMidiShare.cpp MidiInputDeviceMidiShare.h
midishare_ladd = -lMidiShare
else
midishare_src  =
midishare_ladd =
endif

if HAVE_COREMIDI
coremidi_src  = MidiInputDeviceCoreMidi.cpp MidiInputDeviceCoreMidi.h
coremidi_ldflags = -framework CoreMIDI
else
coremidi_src  =
coremidi_ldflags =
endif

if HAVE_MME_MIDI
mmemidi_src  = MidiInputDeviceMme.cpp MidiInputDeviceMme.h
mmemidi_ladd = -lwinmm
else
mmemidi_src  =
mmemidi_ladd =
endif

if HAVE_JACK_MIDI
jackmidi_src      = MidiInputDeviceJack.cpp MidiInputDeviceJack.h
jackmidi_includes = $(JACK_CFLAGS)
else
jackmidi_src      =
jackmidi_includes =
endif

liblinuxsamplermididriverincludedir = $(includedir)/linuxsampler/drivers/midi
liblinuxsamplermididriverinclude_HEADERS = \
	midi.h \
	MidiInputPort.h \
	MidiInputDevice.h \
	MidiInstrumentMapper.h \
	VirtualMidiDevice.h

AM_CPPFLAGS = $(all_includes) $(jackmidi_includes) $(GIG_CFLAGS)

noinst_LTLIBRARIES = liblinuxsamplermididriver.la
liblinuxsamplermididriver_la_SOURCES = \
	midi.h \
	MidiInputDevice.cpp MidiInputDevice.h \
	MidiInputDeviceFactory.cpp MidiInputDeviceFactory.h \
	MidiInputPort.cpp MidiInputPort.h \
	MidiInstrumentMapper.cpp MidiInstrumentMapper.h \
	VirtualMidiDevice.cpp VirtualMidiDevice.h \
	$(alsa_src) \
	$(midishare_src)\
	$(coremidi_src)\
	$(mmemidi_src)\
	$(jackmidi_src)\
	MidiInputDevicePlugin.cpp MidiInputDevicePlugin.h

liblinuxsamplermididriver_la_LIBADD = $(alsa_ladd) $(midishare_ladd) $(mmemidi_ladd)
liblinuxsamplermididriver_la_LDFLAGS = $(coremidi_ldflags)
