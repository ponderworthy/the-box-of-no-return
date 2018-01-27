#!/bin/bash

export EDITOR=/usr/bin/nano
export VISUAL=/usr/bin/nano

sudo cpupower frequency-set -g performance

sudo echo -n 3072 | sudo tee /sys/class/rtc/rtc0/max_user_freq
sudo chmod 660 /dev/hpet /dev/rtc0
sudo chgrp audio /dev/hpet /dev/rtc0

nohup lxterminal -t "BNR Live Report" -e bash -i ~/BNR/BOOT-INITIAL.sh &

