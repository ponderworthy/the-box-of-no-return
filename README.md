# the-box-of-no-return

The Box of No Return is a Linux synth platform suitable for live musicianship, designed to handle patches with enormous demands, and switch between 
them with zero delay and zero cutout.  Its current production design is discussed at https://lsn.ponderworthy.com.  This repository was created to 
receive the files for its next major iteration, which is using the [MultiJACK](https://github.com/jebofponderworthy/MultiJACK) architecture at its core.

## the name

The original name for the project was "Supermega Rumblic Organ", or SRO, after its first patch.  But more patches came about, not organ-ic.  It is 
called the Box of No Return because after playing it, its creator cannot go back to his former instrumentation, at least not for long.

## Implementation Notes

* These files are designed for building a standalone, headless box, which 
one connects via either MIDI interface or USB cord to an appropriate keyboard
controller.  As such, the machine needs to boot into a user profile,
without password, and BOOT-INITIAL.sh needs to be automatically run at boot.  

* Any dot-files (e.g., .calfpresets) need to be placed in the root of the user profile 
used for this purpose.

* Everything else is designed to work from any other folder, e.g., ~/BNR.



