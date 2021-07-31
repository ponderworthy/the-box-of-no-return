#!/bin/bash

export EDITOR=/usr/bin/nano
export VISUAL=/usr/bin/nano

vmtouch -d -L ~/BNR/VPO

nohup lxterminal -t "BNR Live Report" -e bash -i ~/BNR/BOOT-INITIAL.sh &

