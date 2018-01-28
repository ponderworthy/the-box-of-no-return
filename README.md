# the-box-of-no-return

The Box of No Return is a Linux synth platform suitable for live musicianship, designed to handle patches with enormous demands, and switch between them with zero delay and zero cutout.  Its current production design is discussed at https://lsn.ponderworthy.com.  This repository was created to receive the files for its next major iteration, which is using the [MultiJACK](https://github.com/jebofponderworthy/MultiJACK) architecture at its core.  The MultiJACK BNR using these files is up and running, production tests passed, and will be tested in real live music shortly.

## the name

The original name for the project was "Supermega Rumblic Organ", or SRO, after its first patch.  But a Strings patch was soon added.  It is called the Box of No Return because after playing it, its creator cannot go back to his former instrumentation, at least not for long.

## why this project and its benefits

To build more and better.  The rest is details :-)

From the beginning, the BNR was desired to handle patches of maximum, profound and terrible, tonal content.  Current patches include one with three simultaneous Yoshimis, another with five simultaneous large FluidSynth soundfonts, and the ability to mix the two and a third together.  Many prayers granted, and much help from the Linux Audio community, study, work, and trial and error, has gone into making this happen reliably and well.

The current known box running with this code, is an eight-core 4GHz machine with 8G RAM.  In an earlier iteration, it used the conventional single JACK server; but less than one-quarter of its CPU capacity was being used, while JACK reported that 75% of its capacity was being used.  In 2015 began an effort, now successful, to use multiple JACK processes together in one box, and this is the rebuild of the BNR with MultiJACK at its core.  Attempts have not yet been made to run this code on a quad-core box, though the previous single-JACK iteration of the BNR is running well on quad 3GHz with 8G RAM.

There are quite a few people using tools like NetJACK to increase JACK capacity with multiple boxes, or at least multiple motherboards.  But if you value portability, if you value space, and/or if you are a gigging musician, a fragile or heavy (one, the other, or both; not neither) multi-motherboard construct is not preferable.  Most users of Linux-hardware synths limit themselves as a result of what one JACK server can do, and other platforms limit analogously; here we have an effort to blow the door open a bit more!

## implementation notes

* These files are designed principally to build a MIDI tone synthesizer as a headless Linux box, which 
one connects via either MIDI interface or USB cord to an appropriate keyboard controller.  The box must be set up to good Linux production audio standards.  As of this writing, the Manjaro default kernel does very well, and it is used with a sysctl.conf.d file with the [full wired networking set desribed here](https://notes.ponderworthy.com/linux-networking-speed-and-responsiveness), the only change being swappiness set at 10.

* To run the whole BNR, you'll want at least a quad-core, 3GHz, 8G RAM probably.  It can be scaled up or down.  

* All files in this repository need to be placed in a folder named BNR, located just off a user profile root.

* If headless run is indicated, the machine needs to boot into a user profile, without password, and the environment needs to run shell script "boot" at boot.  Otherwise just run ~/BNR/boot to get it all under way.  

* Any dot-files (e.g., .calfpresets) need to be placed in the root of the user profile 
used for this purpose.

* In the current implementation, there is just one sound card, so there is just one JACK server connected to real audio hardware; this is the default, which can be studied and controlled initially using 'cadence'.  In production, it is configured and started at boot in BOOT-INITIAL.sh.  Wiring can be seen and changed in 'catia'.

* There are three JACK servers set up in these files which do not connect to hardware, they use the "dummy" driver.  These are started in BOOT-INITIAL.sh also.  

* The "soft" JACK servers each run zita-j2n IP transmitters, and the "hard" JACK server runs three zita-n2j IP receivers.  This is how the JACK servers transmit the audio data to each other.  The Zitas give the JACK servers independence, using the extraordinarily high-quality resampling which they contain.

* MIDI is handled wonderfully by ALSA MIDI, through a2jmidi_bridge processes.  a2jmidi_bridge is non-service binaries provided with the a2jmidid package.  One of these is run on each soft server.  Wiring is done through catia with ALSA MIDI enabled, and aj-snapshot saves the wiring ("./soft1 aj-snapshot" to save) and redoes it at boot.

* One of the more difficult challenges was solved in earlier iterations of the BNR: startup.  Each JACK client has to "settle" at startup before the next one begins loading.  The python library jpctrl.py was created to handle this, and is used in BOOT-GENERAL.py.



