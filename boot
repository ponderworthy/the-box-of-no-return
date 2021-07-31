#!/bin/bash

export EDITOR=/usr/bin/nano
export VISUAL=/usr/bin/nano

nohup lxterminal -t "BNR Live Report" -e bash -i ~/BNR/BOOT-INITIAL.sh &

