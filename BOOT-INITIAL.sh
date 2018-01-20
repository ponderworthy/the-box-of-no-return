#######################################
# BOOT-INITIAL
#
# This runs first at BNR startup.
# It does everything that is simplest
# if done in bash rather than needing
# careful process control.
########################################

#!/bin/bash

echo "Reset JACK log..." > /home/jeb/BOOT-INITIAL.log

rm ~/.log/jack/jackdbus.log
touch ~/.log/jack/jackdbus.log

# Reset any saved JACK configuration info
rm -f ~/.jackdrc
rm -f ~/.config/jack/conf.xml
rm -f ~/.config/rncbc.org/QjackCtl.conf

# Configure and start JACK hard server
echo "Configure and start JACK hard server ..." >> /home/jeb/BOOT-INITIAL.log
jack_control ds alsa
jack_control dps device hw:O12,0
jack_control dps rate 96000
jack_control dps period 64
jack_control dps nperiods 3
jack_control dps midi-driver none
jack_control eps realtime True
jack_control eps realtime-priority 90
jack_control eps clock-source 0
jack_control eps sync false
jack_control start

# Configure and start JACK soft servers
/usr/bin/jackd -nSOFT1 -ddummy -r96000 -p64 > jackd-SOFT1.log > ~/SOFT1.log &
/usr/bin/jackd -nSOFT2 -ddummy -r96000 -p64 > jackd-SOFT2.log > ~/SOFT2.log &
/usr/bin/jackd -nSOFT3 -ddummy -r96000 -p64 > jackd-SOFT3.log > ~/SOFT3.log &

# Can be useful to start JACK setup and connection GUIs in place of this
# block of comments during development.
#
# Not using qjackctl anymore, it can cause problems with what we are doing here.
#
# 'cadence' works well for hard JACK setup, and 'catia' for connections.
#
# 'patchage' may be helpful for soft server connections, to help keep visible
# the distinction between hard and soft.  It is unclear whether you will
# ever need a GUI configurator for soft servers.

if [ $1 ]; then
	echo Exiting BOOT-INITIAL due to command-line parameter.
	exit
fi

echo "Starting BOOT-GENERAL..." >> ~/BOOT-INITIAL.log
# Start BOOT-GENERAL
~/BNR/BOOT-GENERAL > ~/BOOT-GENERAL.log
