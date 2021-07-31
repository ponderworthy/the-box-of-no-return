#!/bin/bash

#######################################
# BOOT-INITIAL
#
# This runs first at BNR startup.
# It does everything that is simplest
# if done in bash rather than needing
# careful process control.
########################################

# Change this according to your sound card name
# as found in /proc/asound/cards
export SOUNDCARD_NAME=Generic

echo "Set volume for audio device..."

# Some sound cards need us to raise the
# hardware volume each time.  To find out
# what number and subdevice name you need,
# use 'alsamixer'.  In general it's helpful
# to go with one notch less than maximum.

# This worked for one motherboard.
# amixer -D hw:PCH sset Master 86

# This is for the ESI U22 XT.
# amixer -D hw:USB sset PCM 108

# Another motherboard, setting both
# master and headphones out.
amixer -D hw:$SOUNDCARD_NAME sset Master 63
amixer -D hw:$SOUNDCARD_NAME sset Headphone 63
amixer -D hw:$SOUNDCARD_NAME sset Front 63
amixer -D hw:$SOUNDCARD_NAME sset Line 63


echo "Reset JACK log..."

rm ~/.log/jack/jackdbus.log
touch ~/.log/jack/jackdbus.log

# Reset any saved JACK configuration info
rm -f ~/.jackdrc
rm -f ~/.config/jack/conf.xml
rm -f ~/.config/rncbc.org/QjackCtl.conf

# Start JACK hard server
echo "Start JACK hard server..."

# The below was working for a while with the FIIO device.
# /usr/bin/jackd -dalsa -dhw:Audio -r96000 -p256 -n3 -P > jackd-HARD.log &

/usr/bin/jackd -dalsa -dhw:$SOUNDCARD_NAME -r96000 -p128 -n2 -P > jackd-HARD.log &

nohup patchage > /dev/null &

if [ $1 ]; then
	echo Exiting BOOT-INITIAL due to command-line parameter.
	exit
fi

echo "Starting BOOT-GENERAL..."
/usr/bin/python3 ~/BNR/BOOT-GENERAL.py

read -rsp $'Press any key to continue...\n' -n1 key

