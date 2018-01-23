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

print('-----------------------------------------------------------------')
print('Create JACK clients, and use them to verify JACK servers...')
print('-----------------------------------------------------------------')

jack_client_hard = jack.Client('jack_client_hard', servername='default')
jack_client_hard.activate()
jack_client_hard.outports.register('dummy_4hard')

print('JACK ports for hard server:')
print(jack_client_hard.name)
print(jack_client_hard.get_ports())
print('')

jack_client_soft1 = jack.Client('jack_client_soft1', servername='SOFT1')
jack_client_soft1.activate()
jack_client_soft1.outports.register('dummy_4soft1')

print('JACK ports for server SOFT1:')
print(jack_client_soft1.name)
print(jack_client_soft1.get_ports())
print('')

jack_client_soft2 = jack.Client('jack_client_soft2', servername='SOFT2')
jack_client_soft2.activate()
jack_client_soft2.outports.register('dummy_4soft2')

print('JACK ports for server SOFT2:')
print(jack_client_soft2.name)
print(jack_client_soft2.get_ports())
print('')

jack_client_soft3 = jack.Client('jack_client_soft3', servername='SOFT3')
jack_client_soft3.activate()
jack_client_soft3.outports.register('dummy_4soft3')

print('JACK ports for server SOFT3:')
print(jack_client_soft3.name)
print(jack_client_soft3.get_ports())
print('')

input("Press Enter to continue...")

exit(0)

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
print('Start Distribute on all soft servers...')
print('-----------------------------------------------------------------')

# - If we run a2jmidi_bridge on each soft server we
# should be able to connect each soft server to MIDI
# hardware, through ALSA.
# - This does require a separate Distribute process for each
# JACK soft server, and none on the hard server.
# - They can all be exactly the same Distribute code.
# - Given that we are designing for most effective (not stingy)
# use of truly ample CPU and RAM, this is probably the best,
# because it keeps the stage count at a minimum for each
# MIDI signal sent and received.
# - Even though each Distribute process will be considerably
# larger than necessary, using the same code (the same
# executable script!) for each will keep the system as a whole
# as simple as possible for study and change.

print('Starting Distribute on SOFT1...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute', 'SOFT1'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport(jack_client_soft1, 'Distribute:out_1')   \
        or not jpctrl.wait_for_jackport(jack_client_soft1, 'Distribute:out_16'):
    print('wait_for_jackport on Distribute failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute confirmed on SOFT1.')

print('Starting Distribute on SOFT2...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute', 'SOFT2'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport(jack_client_soft2, 'Distribute:out_1')   \
        or not jpctrl.wait_for_jackport(jack_client_soft2, 'Distribute:out_16'):
    print('wait_for_jackport on Distribute failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute confirmed on SOFT2.')

print('Starting Distribute on SOFT3...')

if not jpctrl.spawn_and_settle(bnr_dir + 'Distribute', 'SOFT3'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport(jack_client_soft3, 'Distribute:out_1')   \
        or not jpctrl.wait_for_jackport(jack_client_soft3, 'Distribute:out_16'):
    print('wait_for_jackport on Distribute failed.')
    jpctrl.exit_with_beep()
else:
    print('Distribute confirmed on SOFT3.')


print('-----------------------------------------------------------------')
print('Start Zita IP bridge processes...')
print('-----------------------------------------------------------------')

# Three receivers on the hard server, and one transmitter on each soft server.




print('-----------------------------------------------------------------')
print('Start non-mixer, Mixer-hard, on hard server...')
print('-----------------------------------------------------------------')

if not jpctrl.spawn_and_settle(
        'non-mixer --instance Mixer-Hard ' + bnr_dir + 'non-mixer/Mixer-Hard'):
    jpctrl.exit_with_beep()

if not jpctrl.wait_for_jackport('Mixer-Hard/FinalOutput:out-1', jack_client_hard)    \
        or not jpctrl.wait_for_jackport('Mixer-Hard/FinalOutput:out-2', jack_client_hard):
    print('wait_for_jackport on Mixer-Hard failed.')
    jpctrl.exit_with_beep()
else:
    print('Mixer-Hard ports confirmed.')

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

print('-----------------------------------------------------------------')
print('Clean up JACK clients created for BNR system boot...')
print('-----------------------------------------------------------------')

for jc in [jack_client_hard, jack_client_soft1, jack_client_soft2, jack_client_soft3]:
    jc.deactivate()
    jc.close()

