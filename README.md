# the-box-of-no-return

The Box of No Return is a Linux synth platform suitable for live musicianship, designed to handle patches with enormous demands, and switch between them with zero delay and zero cutout.  Its current production design is discussed at https://lsn.ponderworthy.com.  This repository was created to receive the files for its next major iteration, which is using the [MultiJACK](https://github.com/jebofponderworthy/MultiJACK) architecture at its core.

## the name

The original name for the project was "Supermega Rumblic Organ", or SRO, after its first patch.  But a Strings patch was soon added.  It is called the Box of No Return because after playing it, its creator cannot go back to his former instrumentation, at least not for long.

## why this project

To build more and better.  The rest is details :-)

From the beginning, the BNR was designed to handle patches of maximum, profound and terrible, tonal content.  Current patches include one with three simultaneous Yoshimis, another with five simultaneous large FluidSynth soundfonts, and the ability to mix the two and a third together.  A very large amount of study, work, and trial and error has gone into making this happen reliably and well.

A quad-core 3 GHz CPU is used up very easily like this, and one of this writer's two BNRs is such a box.  The other is an eight-core 4GHz machine, and less than one-quarter of its CPU capacity was being used, while JACK reported that 75% of its capacity was being used; and in 2015 began an effort, now successful, to use multiple JACK processes together in one box, so more can be produced.

There are quite a few people using tools like NetJACK to do this with multiple boxes, or at least multiple motherboards.  But if you value portability, if you value space, and/or if you are a gigging musician, a fragile or heavy (one, the other, or both; not neither) multi-motherboard construct is not preferable.   

## implementation notes

* These files are designed principally to build a MIDI tone synthesizer as a headless Linux box, which 
one connects via either MIDI interface or USB cord to an appropriate keyboard controller.  The box must be set up to good Linux production audio standards.  

* To run the whole BNR, you'll want at least a quad-core, 3GHz, 8G RAM probably.  It can be scaled up or down.  

* All files in this repository need to be placed in a folder named BNR, located just off a user profile root.

* If headless run is indicated, the machine needs to boot into a user profile, without password, and the environment needs to run shell script "boot" at boot.  Otherwise just run ~/BNR/boot to get it all under way.  

* Any dot-files (e.g., .calfpresets) need to be placed in the root of the user profile 
used for this purpose.

* Everything else is designed to work from any other folder, e.g., ~/BNR.



