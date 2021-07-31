#!/usr/bin/python2

# Keeping this as a Python2 script, because have not found
# documentation for mididings syntax changes for Python3.

# Level 1:
# All control changes (e.g., the foot-pedal) are sent to all active JACK ports (currently, 1 2 4 5)
# Everything which is not a control change (e.g., notes) are split:
#	If it comes in on MIDI channel 1, it is designated for applet JACK port 'SRO'
#	If it comes in on MIDI channel 2, it is designated for applet JACK port 'Strings'
#	If it comes in on MIDI channel 3, it is designated for applet JACK ports 'SRO' plus 'Strings'
#	If it comes in on MIDI channel 4, it is designated for applet JACK port 'FlowBells'
#	If it comes in on MIDI channel 5, it is designated for applet JACK ports 'Strings' + 'FlowBells'

# Level 2:
# All signals, after leaving Level 1, are designated for MIDI channel 1, and sent out the applet JACK port(s)
# designated in Level 1.

# Double patches are described below.  Relative volumes
# are adjusted in MIDI as velocity.  Overload is always a concern.

# Channel 3 is SRO+Strings.
# Trying Port 1 (SRO) at 0.5, Port 2 (Strings) at 0.5.

# Channel 5 is Strings+FlowBells.
# Trying Port 2 (Strings) at 0.85, Port 4 (FlowBells) at 0.35.

from mididings import *

config(
    backend='jack',
    client_name='Distribute.py',
    in_ports=1,
    out_ports=['SRO', 'Strings', 'FlowBells', 'Many-Horns'],
)

# Documentation for the "run" code below.
#
# The general theory is:
#  - MIDI data comes in
#  - Non-note data (CTRL data) gets routed to all active
#    output ports of this applet
#  - Note data gets routed such that we get one BNR
#    audio channel possible per MIDI channel as set in the
#    physical keyboard controller.  For instance:
#  - If MIDI data is on channel 1, it is output port 1
#  - If MIDI data is on special channels, e.g., 3,
#    it is replicated and massaged for use with multiple
#    channels at once
#
# Specifics to the below:
#
## CtrlFilter(64) >> [Port('SRO'), Port('Strings'), Port('FlowBells')],
# The above line takes all CTRL data of type 64, which means 
# all sustain/damper pedal changes only, and routes them to 
# all four live patch channels at the same time.  We don't route
# to combinations, because that doubles up.
#
## Filter(NOTEON | NOTEOFF) >> ChannelSplit({
# The above line, before the >>, takes all note on and note off data.
# After the >> begins perhaps the 'meat' of Distribute.py,
# where all data coming in on MIDI Channel 1 is routed
# to output JACK MIDI port #1 of this process, and
# some are handled specially, e.g., #3, #5, #6.
#
# The above are comma-separated in square brackets, which
# splits the MIDI stream into two parts.
#
## ] >> Channel(1)
# The above line takes both sets of MIDI data, and 
# sets them to MIDI channel 1, which every element of the BNR
# needs for reception and action: the BNR uses just one
# MIDI channel, #1, internally, but it chooses which
# of its patches for MIDI commands, by way of the
# MIDI channel it receives on input.

run(

    [
        CtrlFilter(64) >> [Port('SRO'), Port('Strings'), Port('FlowBells')],
        Filter(NOTEON | NOTEOFF) >> ChannelSplit({
            1: Port('SRO'),
            2: Port('Strings'),
            3: [Port('SRO') >> Velocity(multiply=0.5), Port('Strings') >> Velocity(multiply=0.5)],
            4: Port('FlowBells'),
            5: [Port('Strings') >> Velocity(multiply=0.55), Port('FlowBells')],
            6: Port('Many-Horns')
#            7: Port(7),
#            8: Port(8),
#            9: Port(9),
#            10: Port(10),
#            11: Port(11),
#            12: Port(12),
#            13: Port(13),
#            14: Port(14),
#            15: Port(15),
#            16: Port(16),
        })
    ] >> Channel(1)

)
