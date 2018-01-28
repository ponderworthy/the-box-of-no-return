#######################################
# BOOT-INITIAL
#
# This runs first at BNR startup.
# It does everything that is simplest
# if done in bash rather than needing
# careful process control.
########################################

#!/bin/bash

echo "Reset JACK log..."

rm ~/.log/jack/jackdbus.log
touch ~/.log/jack/jackdbus.log

# Reset any saved JACK configuration info
rm -f ~/.jackdrc
rm -f ~/.config/jack/conf.xml
rm -f ~/.config/rncbc.org/QjackCtl.conf

# Configure and start JACK hard server
echo "Configure and start JACK hard server ..."
jack_control ds alsa
jack_control dps device hw:O12,0
jack_control dps rate 96000
jack_control dps period 128
jack_control dps nperiods 3
jack_control dps midi-driver none
jack_control eps realtime True
jack_control eps realtime-priority 90
jack_control eps clock-source 0
jack_control eps sync false
jack_control start

# Make sure a2jmidid is stopped
a2j_control stop

# Configure and start JACK soft servers

# /usr/bin/jackd -nSOFT1 -X alsarawmidi -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT1.log &
# /usr/bin/jackd -nSOFT2 -X alsarawmidi -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT2.log &
# /usr/bin/jackd -nSOFT3 -X alsarawmidi -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT3.log &

# Soft server notes, in command parameter order:
# -nSOFT1         JACK server name
# -X alsarawmidi  Run the ALSA raw midi slave driver
# -ddummy         Run the Dummy driver as master rather than connecting hardware audio
# -r96000         96000 kHz sampling
# -p256           Period of 256, twice that of the hard server, seems to help
# -C 0 -P 0       Zero JACK server ports, either input or output.  Input is MIDI,
#		  output is zita-j2n.

# Not using qjackctl anymore, it can cause problems with what we are doing here.
#
# 'cadence' works well for hard JACK setup, and 'catia' for connections.
# To see JACK hard server connections, just 'catia'
# To see JACK soft server SOFT1 connections, './soft1 catia'

if [ $1 ]; then
	echo Exiting BOOT-INITIAL due to command-line parameter.
	exit
fi

echo "Starting BOOT-GENERAL..."
/usr/bin/python ~/BNR/BOOT-GENERAL.py

read -rsp $'Press any key to continue...\n' -n1 key

