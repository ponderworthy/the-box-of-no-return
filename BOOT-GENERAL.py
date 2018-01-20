#################################################
# BOOT-GENERAL
#
# This is called by BOOT-INITIAL.
# JACK clients cannot be simply started
# all at once, there is relationship
# and interaction which has to be respected
# between each of them and their JACK server.
# jpctrl, a Python library for Jack Process Control,
# handles this, and thus this file is a Python3
# script.
#
# This file is the scene of very active development right now.
# A 2015 working version, in Python 2, is visible here:
# https://lsn.ponderworthy.com/doku.php/concurrent_patch_management
####################################################################

#!/usr/bin/python3

import sys
import os
import jpctrl  # our own Jack Process Control library

# Detect debug mode.
# In debug mode, Yoshimi is run with GUI enabled, else with GUI disabled.
cmdargcount = len(sys.argv)
if cmdargcount == 2:
    # Use debug mode
    print('BOOT-GENERAL initiated.')
    print('Debug mode on!')
    debugmode = 1
else:
    # Debug mode off
    print('BOOT-GENERAL initiated.')
    print('Running normally, debug mode off.')
    debugmode = 0

# Set different PIO checks for debug mode.
if debugmode:
    yoshimi_debug_param = ''
else:
    yoshimi_debug_param = '-i'

# Verify / wait for JACK server 'default'.  This is the hard server.
print('Verify / wait for JACK hard server...')
jack_client_hard = jpctrl.wait_for_jack('jpctrl_client')
If jack_client_hard is None:
    jpctrl.exit_with_beep()

# Verify / wait for JACK soft server 'SOFT1'.
print('Verify / wait for JACK soft server SOFT1...')
jack_client_soft1 = jpctrl.wait_for_jack('jpctrl_client', 'SOFT1')
if jack_client_soft1 is None:
    jpctrl.exit_with_beep()

# Verify / wait for JACK soft server 'SOFT2'.
print('Verify / wait for JACK soft server SOFT2...')
jack_client_soft2 = jpctrl.wait_for_jack('jpctrl_client', 'SOFT2')
if jack_client_soft2 is None:
    jpctrl.exit_with_beep()

# Verify / wait for JACK soft server 'SOFT3'.
print('Verify / wait for JACK soft server SOFT3...')
jack_client_soft3 = jpctrl.wait_for_jack('jpctrl_client', 'SOFT3')
if jack_client_soft3 is None:
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start Distribute on hard server...')
print('-----------------------------------------------------------------')

if jpctrl.spawn_and_settle('/home/jeb/Distribute'):
    jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('Distribute:out_1') or jpctrl.wait_for_jackport('Distribute:out_16'):
    print('wait_for_jackport on Distribute failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute ports confirmed.')


print('-----------------------------------------------------------------')
print('Start non-mixer, Mixer-General...')
print('-----------------------------------------------------------------')

if jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-General /home/jeb/non-mixer/Mixer-General'):
    jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('Mixer-General/FinalOutput:out-1') or jpctrl.wait_for_jackport('Mixer-General/FinalOutput:out-2'):
    print('wait_for_jackport on Mixer-General failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-General ports confirmed.')

jpctrl.sleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch SRO...')
print('-----------------------------------------------------------------')

print('Start Yoshimi SRO 1...')
if jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO1 -j -J -l /home/jeb/YOSHIMI/SROpart1.xmz'):
    jpctrl.exit_with_beep()

print('Start Yoshimi SRO 2...')
if jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO2 -j -J -l /home/jeb/YOSHIMI/SROpart2.xmz'):
    jpctrl.exit_with_beep()

print('Start Yoshimi SRO 3...')
if jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO3 -j -J -l /home/jeb/YOSHIMI/SROpart3.xmz'):
    jpctrl.exit_with_beep()

print('Start CalfSRO...')
if jpctrl.spawn_and_settle(
        'calfjackhost --client CalfSRO eq12:SRO ! reverb:SRO ! multibandcompressor:SRO'):
    jpctrl.exit_with_beep()

jpctrl.sleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch Strings...')
print('-----------------------------------------------------------------')

print('Start StringsSSO...')
if jpctrl.spawn_and_settle(
        'calfjackhost --client StringsSSO fluidsynth:StringsSSO'):
    jpctrl.exit_with_beep()

print('Start StringsBassAdd...')
if jpctrl.spawn_and_settle(
    'calfjackhost --client StringsBassAdd ' +
        'fluidsynth:BassoonsSustain fluidsynth:ContrabassoonSolo fluidsynth:GeneralBass'):
    jpctrl.exit_with_beep()

print('Start MaxStringsFilters...')
if jpctrl.spawn_and_settle(
        'calfjackhost --client MaxStringsFilters eq12:MaxStrings ! reverb:MaxStrings ! multibandcompressor:Strings'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start component for patch FlowBells...')
print('-----------------------------------------------------------------')

print('Start Yoshimi for FlowBells...')
if jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshFlowBells -j -J -l /home/jeb/YOSHIMI/FlowBells.xmz'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('And lastly, create JACK connections using aj-snapshot...')
print('-----------------------------------------------------------------')

if jpctrl.spawn_background('aj-snapshot -r /home/jeb/AJBoot.xml'):
    jpctrl.exit_with_beep()
