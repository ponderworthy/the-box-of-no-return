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

bnr_dir = os.getcwd() + '/'

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
print('Start all a2jmidi_bridge processes for soft servers...')
print('-----------------------------------------------------------------')

# The idea here is that all of the JACK soft servers are given
# access to MIDI hardware via j2amidi_bridge, which is installed
# as part of a2jmidid.

# on SOFT1
if not jpctrl.spawn_and_settle('a2jmidi_bridge', 'SOFT1'):
    jpctrl.exit_with_beep()

# on SOFT2
if not jpctrl.spawn_and_settle('a2jmidi_bridge', 'SOFT2'):
    jpctrl.exit_with_beep()

# on SOFT3
if not jpctrl.spawn_and_settle('a2jmidi_bridge', 'SOFT3'):
    jpctrl.exit_with_beep()

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

print('Starting Distribute.py on SOFT1...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Distribute.py:SRO', 'SOFT1'):
    print('wait_for_jackport on Distribute.py/SOFT1 failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute.py confirmed on SOFT1.')

print('Starting Distribute.py on SOFT2...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT2'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Distribute.py:Strings', 'SOFT2'):
    print('wait_for_jackport on Distribute.py/SOFT2 failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute.py confirmed on SOFT2.')

print('Starting Distribute.py on SOFT3...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute.py', 'SOFT3'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Distribute.py:FlowBells', 'SOFT3'):
    print('wait_for_jackport on Distribute.py/SOFT3 failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute.py confirmed on SOFT3.')

print('-----------------------------------------------------------------')
print('Start Zita IP bridge processes...')
print('-----------------------------------------------------------------')

print('Three receivers on the hard server...')

if not jpctrl.spawn_and_settle('zita-n2j --jname zita-n2j-4soft1 127.0.0.1 55551'):
    jpctrl.exit_with_beep()
if not jpctrl.spawn_and_settle('zita-n2j --jname zita-n2j-4soft2 127.0.0.2 55552'):
    jpctrl.exit_with_beep()
if not jpctrl.spawn_and_settle('zita-n2j --jname zita-n2j-4soft3 127.0.0.3 55553'):
    jpctrl.exit_with_beep()

print('One transmitter on each soft server...')

if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft1 --jserv SOFT1 127.0.0.1 55551'):
    jpctrl.exit_with_beep()
if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft2 --jserv SOFT2 127.0.0.2 55552'):
    jpctrl.exit_with_beep()
if not jpctrl.spawn_and_settle('zita-j2n --jname zita-j2n-soft3 --jserv SOFT3 127.0.0.3 55553'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start non-mixers...')
print('-----------------------------------------------------------------')

print('')
print('non-mixer Mixer-Hard...')
print('')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-Hard ' + bnr_dir + 'non-mixer/Mixer-Hard'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-Hard/FinalOutput:out-1'):
    print('wait_for_jackport on Mixer-Hard failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-Hard ports confirmed.')

print('')
print('non-mixer Mixer-SOFT1...')
print('')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-SOFT1 ' + bnr_dir + 'non-mixer/Mixer-SOFT1', 'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-SOFT1/FinalOutput:out-1', 'SOFT1'):
    print('wait_for_jackport on Mixer-SOFT1 failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-SOFT1 ports confirmed.')

print('')
print('non-mixer Mixer-SOFT2...')
print('')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-SOFT2 ' + bnr_dir + 'non-mixer/Mixer-SOFT2', 'SOFT2'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-SOFT1/FinalOutput:out-1', 'SOFT2'):
    print('wait_for_jackport on Mixer-SOFT2 failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-SOFT2 ports confirmed.')

jpctrl.stdsleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch SRO, on server SOFT1...')
print('-----------------------------------------------------------------')

print('Start Yoshimi SRO 1...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO1 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart1.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('Start Yoshimi SRO 2...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO2 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart2.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('Start Yoshimi SRO 3...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshSRO3 -j -J -l ' + bnr_dir + 'YOSHIMI/SROpart3.xmz',
        'SOFT1'):
    jpctrl.exit_with_beep()

print('Start CalfSRO...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client CalfSRO eq12:SRO ! reverb:SRO ! multibandcompressor:SRO',
        'SOFT1'):
    jpctrl.exit_with_beep()

jpctrl.stdsleep(3)

print('-----------------------------------------------------------------')
print('Start components for patch Strings, on server SOFT2...')
print('-----------------------------------------------------------------')

print('Start StringsSSO...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client StringsSSO fluidsynth:StringsSSO',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('Start StringsBassAdd...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client StringsBassAdd ' +
        'fluidsynth:BassoonsSustain fluidsynth:ContrabassoonSolo fluidsynth:GeneralBass',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('Start MaxStringsFilters...')
if not jpctrl.spawn_and_settle(
        'calfjackhost --client MaxStringsFilters eq12:MaxStrings ! reverb:MaxStrings ! multibandcompressor:Strings',
        'SOFT2'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Start component for patch FlowBells, on server SOFT3...')
print('-----------------------------------------------------------------')

print('Start Yoshimi for FlowBells...')
if not jpctrl.spawn_and_settle(
        'yoshimi ' + yoshimi_debug_param + ' -c -I -N YoshFlowBells -j -J -l ' + bnr_dir + 'YOSHIMI/FlowBells.xmz',
        'SOFT3'):
    jpctrl.exit_with_beep()

print('-----------------------------------------------------------------')
print('Create JACK connections using aj-snapshot, on all servers...')
print('-----------------------------------------------------------------')

print('aj-snapshot for hard server...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJhard.xml'):
    jpctrl.exit_with_beep()

print('aj-snapshot for server SOFT1...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT1.xml'):
    jpctrl.exit_with_beep()

print('aj-snapshot for server SOFT2...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT2.xml'):
    jpctrl.exit_with_beep()

print('aj-snapshot for server SOFT3...')
if not jpctrl.spawn_background('aj-snapshot -r ' + bnr_dir + 'AJSOFT3.xml'):
    jpctrl.exit_with_beep()


