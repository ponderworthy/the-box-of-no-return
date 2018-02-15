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

import sys
import os
import jack
import jpctrl  # our own Jack Process Control

from os.path import expanduser
bnr_dir = expanduser("~") + '/BNR/'

# Detect debug mode.  On with any command-line argument.
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

print('-----------------------------------------------------------------')
print('Start Calf reverb for hard server...')
print('-----------------------------------------------------------------')

print('\n')
if not jpctrl.spawn_and_settle('calfjackhost --client BNR-STD-Reverb reverb:BNR-STD'):
    jpctrl.exit_with_beep()
print('\n')

print('-----------------------------------------------------------------')
print('Start all soft servers...')
print('-----------------------------------------------------------------')

print('\nSOFT1 ...\n')

if not jpctrl.spawn_and_settle('/usr/bin/jackd -nSOFT1 -P 90 -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT1.log',
    'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jack('SOFT1'):
    jpctrl.exit_with_beep()

print('\nSOFT2 ...\n')
if not jpctrl.spawn_and_settle('/usr/bin/jackd -nSOFT2 -P 90 -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT2.log',
    'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jack('SOFT2'):
    jpctrl.exit_with_beep()

print('\nSOFT3 ...\n')
if not jpctrl.spawn_and_settle('/usr/bin/jackd -nSOFT3 -P 90 -ddummy -r96000 -p256 -C 0 -P 0 > jackd-SOFT3.log',
    'SOFT3'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jack('SOFT3'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start all a2jmidi_bridge processes for soft servers...')
print('-----------------------------------------------------------------')

# The idea here is that all of the JACK soft servers are given
# access to MIDI hardware via j2amidi_bridge, which is installed
# as part of a2jmidid.

print('\non SOFT1...\n')

if not jpctrl.spawn_and_settle('a2jmidi_bridge soft1_midi', 'SOFT1'):
     jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('soft1_midi:capture', 'SOFT1'):
    print('soft1_midi confirmed on SOFT1.')
else:
    print('wait_for_jackport on soft1_midi/capture failed.')
    jpctrl.exit_with_beep()

print('\non SOFT2...\n')

if not jpctrl.spawn_and_settle('a2jmidi_bridge soft2_midi', 'SOFT2'):
     jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('soft2_midi:capture', 'SOFT2'):
    print('soft2_midi confirmed on SOFT2.')
else:
    print('wait_for_jackport on soft2_midi/capture failed.')
    jpctrl.exit_with_beep()

print('\non SOFT3...\n')
if not jpctrl.spawn_and_settle('a2jmidi_bridge soft3_midi', 'SOFT3'):
     jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('soft3_midi:capture', 'SOFT3'):
    print('soft3_midi confirmed on SOFT3.')
else:
    print('wait_for_jackport on soft3_midi/capture failed.')
    jpctrl.exit_with_beep()

# Still needed to prevent crash of Distribute.
# Don't know why.  Would love solution.
jpctrl.stdsleep(3)

print('-----------------------------------------------------------------')
print('Start Distribute.py on all soft servers...')
print('-----------------------------------------------------------------')

# - If we run a2jmidi_bridge on each soft server we
# should be able to connect each soft server to MIDI
# hardware, through ALSA.
# - This does require a separate Distribute.py process for each
# JACK soft server, and none on the hard server.
# - They can all be exactly the same Distribute.py code.
# - Given that we are designing for most effective (not stingy)
# use of truly ample CPU and RAM, this is probably the best,
# because it keeps the stage count at a minimum for each
# MIDI signal sent and received.
# - Even though each Distribute.py process will be considerably
# larger than necessary, using the same code (the same
# executable script!) for each will keep the system as a whole
# as simple as possible for study and change.

# spawn_and_settle is not sufficient to determine readiness
# of Distribute.py (mididings).  We must use wait_for_jackport.

# The original wait_for_jackport used jack.Client objects
# created for each JACK server.  For some reason, this did not work.
# This test code still exists in BOOT-GENERAL.BAK.py and jpctrl.py.
# The code below creates a new jack.Client for each test.

print('\nStarting Distribute.py on SOFT1...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT1'):
    jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('Distribute.py:SRO', 'SOFT1'):
    print('Distribute.py confirmed on SOFT1.')
else:
    print('wait_for_jackport on Distribute.py/SOFT1 failed.')
    jpctrl.exit_with_beep()

print('\nStarting Distribute.py on SOFT2...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT2'):
    jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('Distribute.py:Strings', 'SOFT2'):
    print('Distribute.py confirmed on SOFT2.')
else:
    print('wait_for_jackport on Distribute.py/SOFT2 failed.')
    jpctrl.exit_with_beep()

print('\nStarting Distribute.py on SOFT3...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT3'):
    jpctrl.exit_with_beep()

if jpctrl.wait_for_jackport('Distribute.py:FlowBells', 'SOFT3'):
    print('Distribute.py confirmed on SOFT3.')
else:
    print('wait_for_jackport on Distribute.py/SOFT3 failed.')
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start Zita IP bridge processes...')
print('-----------------------------------------------------------------')

print('\nThree receivers on the hard server...\n')

if not jpctrl.spawn_and_settle('zita-n2j --filt 32 --buff 14 --jname zita-n2j-4soft1 127.0.0.1 55551'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-n2j-4soft1:out_1'):
    print('wait_for_jackport on zita-n2j-4soft1/hard failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-n2j-4soft1 confirmed on hard server.\n')

if not jpctrl.spawn_and_settle('zita-n2j --filt 32 --buff 14 --jname zita-n2j-4soft2 127.0.0.2 55552'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-n2j-4soft2:out_1'):
    print('wait_for_jackport on zita-n2j-4soft2/hard failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-n2j-4soft2 confirmed on hard server.\n')

if not jpctrl.spawn_and_settle('zita-n2j --filt 32 --buff 14 --jname zita-n2j-4soft3 127.0.0.3 55553'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-n2j-4soft3:out_1'):
    print('wait_for_jackport on zita-n2j-4soft3/hard failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-n2j-4soft3 confirmed on hard server.\n')

print('\nOne transmitter on each soft server...\n')

if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft1 --jserv SOFT1 127.0.0.1 55551'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-j2n-soft1:in_1', 'SOFT1'):
    print('wait_for_jackport on zita-j2n/SOFT1 failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-j2n confirmed on SOFT1.\n')

if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft2 --jserv SOFT2 127.0.0.2 55552'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-j2n-soft2:in_1', 'SOFT2'):
    print('wait_for_jackport on zita-j2n/SOFT2 failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-j2n confirmed on SOFT2.\n')

if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft3 --jserv SOFT3 127.0.0.3 55553'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('zita-j2n-soft3:in_1', 'SOFT3'):
    print('wait_for_jackport on zita-j2n/SOFT3 failed.')
    jpctrl.exit_with_beep()
else:
    print('zita-j2n confirmed on SOFT3.\n')


print('-----------------------------------------------------------------')
print('Start non-mixers...')
print('-----------------------------------------------------------------')

print('\nnon-mixer Mixer-Hard...')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-Hard ' + bnr_dir + 'non-mixer/Mixer-Hard'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-Hard/FinalOutput:out-1'):
    print('wait_for_jackport on Mixer-Hard failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-Hard ports confirmed.')

print('\nnon-mixer Mixer-SOFT1...')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-SOFT1 ' + bnr_dir + 'non-mixer/Mixer-SOFT1', 'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-SOFT1/FinalOutput:out-1', 'SOFT1'):
    print('wait_for_jackport on Mixer-SOFT1 failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-SOFT1 ports confirmed.')

print('\nnon-mixer Mixer-SOFT2...')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-SOFT2 ' + bnr_dir + 'non-mixer/Mixer-SOFT2', 'SOFT2'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-SOFT2/FinalOutput:out-1', 'SOFT2'):
    print('wait_for_jackport on Mixer-SOFT2 failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-SOFT2 ports confirmed.')

print('\n')

jpctrl.stdsleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch SRO, on server SOFT1...')
print('-----------------------------------------------------------------')

print('\nStart Yoshimi SRO 1...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO1 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart1.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('\nStart Yoshimi SRO 2...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO2 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart2.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('\nStart Yoshimi SRO 3...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO3 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart3.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('\nStart CalfSRO...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client CalfSRO eq12:SRO ! multibandcompressor:SRO',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('\n')

jpctrl.stdsleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch Strings, on server SOFT2...')
print('-----------------------------------------------------------------')

print('\nStart StringsSSO...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client StringsSSO fluidsynth:StringsSSO',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('\nStart StringsBassAdd...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client StringsBassAdd ' +
        'fluidsynth:BassoonsSustain fluidsynth:ContrabassoonSolo fluidsynth:GeneralBass',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('\nStart MaxStringsFilters...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client MaxStringsFilters eq12:MaxStrings ! multibandcompressor:Strings',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('\n')

print('-----------------------------------------------------------------')
print('Start component for patch FlowBells, on server SOFT3...')
print('-----------------------------------------------------------------')

print('\nStart Yoshimi for FlowBells...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshFlowBells -j -J -l ' + bnr_dir + 'YOSHIMI/FlowBells.xmz',
        'SOFT3'):
    jpctrl.exit_with_beep()

print('\n')

print('-----------------------------------------------------------------')
print('Start component for patch Many-Horns, on server SOFT3...')
print('-----------------------------------------------------------------')

print('\nStart Many-Horns...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client Many-Horns fluidsynth:Many-Horns',
        'SOFT3'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Create JACK connections using aj-snapshot, on all servers...')
print('-----------------------------------------------------------------')

print('\naj-snapshot for hard server...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJhard.xml'):
    jpctrl.exit_with_beep()

print('\naj-snapshot for server SOFT1...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT1.xml', 'SOFT1'):
    jpctrl.exit_with_beep()

print('\naj-snapshot for server SOFT2...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT2.xml', 'SOFT2'):
    jpctrl.exit_with_beep()

print('\naj-snapshot for server SOFT3...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT3.xml', 'SOFT3'):
    jpctrl.exit_with_beep()


