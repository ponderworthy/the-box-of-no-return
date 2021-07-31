#########################################################################
# JACK Process Control module
#
# by Jonathan E. Brickman
#
# version 0.8, 2017-01-14, Python 3
# (c) 2017 Jonathan E. Brickman
# released under the LGPL:
# https://www.gnu.org/copyleft/lesser.html
###########################################################################

import subprocess
import os
import psutil	# This is python3-psutil
import shlex
import time
import jack     # This is JACK-Client, installed as python3-jack-client for python3
                # see https://pypi.python.org/pypi/JACK-Client/

# This and the next depend on a Linux command by the name of "beep",
# In some distros it requires "sudo" to run for permissions.
# Great info here: https://wiki.archlinux.org/index.php/Beep
# Currently we have this set for permissions: chmod +s beep

def exit_with_beep():
    for count in range(5):
        try:
            os.system('beep')
        except:
            exit(1)
        stdsleep(0.1)
    exit(1)

def beep(freq, length):
    try:
        os.system('beep ' + '-f ' + freq + '-l ' + length)
    except:
        return False
    return True

##############################################################################
# Try once to set up new jpctrl JACK client in the optionally named server
# and return true on success, false on failure
def setup_jack_client(jack_client_name, jack_server_name='default'):

    # Connect to JACK if possible.
    try:
        jack_client_temp = jack.Client(jack_client_name, servername=jack_server_name)
        jack_client_temp.activate()
    except:
        print('setup_jack_client() failed.')
        print('Aborting.')
        jack_client_temp = None
        return False

    if jack_client_temp.status.failure or \
            jack_client_temp.status.server_error or \
            jack_client_temp.status.client_zombie:
        jack_client_temp = None
        return False

    jack_client_temp.deactivate()
    jack_client_temp.close()
    return True

###################################################################################
# For 20 seconds, try to set up new jpctrl JACK client, trying once per second.
# Return JACK client object on success, None on failure.
def wait_for_jack(jack_server_name='default'):

    timecount = 0
    while True:
        test_jack = setup_jack_client('jpctrl.wait_for_jack-temp', jack_server_name)
        if not test_jack:
            timecount += 1
            if timecount > 20:
                print('JACK server error.  Aborting.')
                return False
            else:
                stdsleep(1)
        else:
            print('JACK server discovered, verified, and client created!')
            return True

#######################################################################
# wait_for_jackport()
#
# Wait for a particular port to become present in a JACK server.
# Returns true on success, false on 6-second timeout and/or failure.
# Requires an active, running, confirmed JACK server, referred to
# by an object given by a function like wait_for_jack or setup_jack_client
# above.
#
# wait_for_jackport_old presumes a pre-created jack_client object.
# Not known why it doesn't work.  Its working version creates a new JACK
# client each time it's run.  Not efficient, but works well.

def wait_for_jackport(name2chk, jack_server_name='default'):

    try:
        jack_client_temp = jack.Client('jack_client_temp', servername=jack_server_name)
        jack_client_temp.activate()
        jack_client_temp.outports.register('jpctrl.wait_for_jackport-temp')
    except:
        return False

    timecount = 0
    while True:
        if timecount > 5:
            print('wait_for_jackport timed out waiting for port: ', name2chk)
            print('Aborting.')
            jack_client_temp.deactivate()
            jack_client_temp.close()
            return False

        print('wait_for_jackport: get_port_by_name attempt ',
            timecount, ' for ', name2chk)

        try:
            if jack_client_temp.get_port_by_name(name2chk) is None:
                stdsleep(1)
                timecount += 1
            else:
                jack_client_temp.deactivate()
                jack_client_temp.close()
                return True
        except:
            stdsleep(1)
            timecount += 1

### Nonworking ###
def wait_for_jackport_old(jack_client, name2chk):

    timecount = 0
    while True:
        if timecount > 5:
            print('wait_for_jackport timed out waiting for port: ', name2chk)
            print('Aborting.')
            return False

        print('wait_for_jackport: get_port_by_name attempt ',
            timecount, ' for ', name2chk)

        try:
            if jack_client.get_port_by_name(name2chk) is None:
                stdsleep(1)
                timecount += 1
            else:
                return True
        except:
            stdsleep(1)
            timecount += 1


#######################################################################
# Find JACK port by substring in the name.
# Return True on success, False on failure
# Requires an active, running, confirmed JACK server, referred to
# by an object given by a function like wait_for_jack or setup_jack_client
# above.
#
# Not currently in use.  Probably would need to create a new
# jack_client every time, due to the issue exposed
# by wait_for_jackport_old above.

def find_jackport_by_substring(jack_client, str2find):

    if jack_client is None:
        print('find_jackport_by_substring() failed: JACK client is invalid/None.')
        return False

    try:
        jack_client.get_ports('*' + str2find + '*')
    except:
        print(
            'find_jackport_by_substring() connected to JACK, but found no matching ports.')
        print('Aborting.')
        return False

    return True


# A jpctrl-namespace standard sleep method.
# Accepts fractions as well as positive integers.
def stdsleep(time_in_secs):
    time.sleep(time_in_secs)


# All process spawning code in jpctrl,
# has JACK server name choice capability.
# This is done by environment variable a la:
#
# my_env = os.environ.copy()
# my_env["JACK_DEFAULT_SERVER"] = jack_server_name
# subprocess.Popen(my_command, env=my_env)

# Starts a process in the background.
# Return True if successful, False if fails.
def spawn_background(cmd_and_args, jack_server_name='default'):
    if try_popen(cmd_and_args, jack_server_name) is not None:
        return True
    else:
        return False

# Start a process in the background, wait until its I/O
# stats can be retrieved, and one more second.
def spawn_and_settle(cmdstr, jack_server_name='default'):
    p_popen = try_popen(cmdstr, jack_server_name)
    if p_popen is None:
        print('spawn_and_settle failed for: ', cmdstr)
        print('Could not start process.')
        return False
    p_psutil = psutil.Process(p_popen.pid)
    p_io = try_pio(p_psutil) # try_pio is ours, see below
    if p_io is None:
        print('spawn_and_settle failed for: ', cmdstr)
        print('Could not get pio data.')
        return False
    else:
        print('spawn_and_settle: process appears ready:', cmdstr)
        stdsleep(1)
        return True

# Tries to get the p_io data used in spawn_and_settle.
# Waits 15 seconds maximum.  This turns out to be valuable
# towards determining stability, because as load on the
# system increases, processes take longer at startup to
# become ready to give p_io stats.

def try_pio(p_psutil):
    timecount = 0
    while True:
        try:
            print('Attempting to get pio stats...')
            p_io = p_psutil.io_counters()
        except:
            timecount += 1
            if timecount > 15:
                print('Could not get pio stats after 15 seconds; aborting.')
                return None
            stdsleep(1)
            continue
        else:
            print('successful!')
            return p_io

# Tries to start a background process.
def try_popen(cmdstr, jack_server_name='default'):

    # Set jack_server_name into the environment for the popen
    my_env = os.environ.copy()
    my_env["JACK_DEFAULT_SERVER"] = jack_server_name

    timecount = 0
    # Original command without jack_server_name handling
    # p_popen = subprocess.Popen(shlex.split(cmdstr), -1)
    # -1 was probably something needed for older python3
    p_popen = subprocess.Popen(shlex.split(cmdstr), bufsize=-1, env=my_env)
    while True:
        stdsleep(1)
        try:
            p_psutil = psutil.Process(p_popen.pid)
        except:
            timecount += 1
            if timecount > 5:
                print('Could not start ', cmdstr,
                      ' after 5 seconds; aborting.')
                return None
            continue
        else:
            return p_popen

# Useful for debugging and tracing.

marker = 1

def marker():
    print('Marker: ' + marker)
    marker += 1
