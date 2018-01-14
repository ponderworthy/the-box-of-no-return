#########################################################################
# JACK Process Control module
#
# by Jonathan E. Brickman
#
# version 0.8, 2017-01-14, Python 3
# (c) 2017 Jonathan E. Brickman
# released under the LGPL:
# https://www.gnu.org/copyleft/lesser.html
#
# This file is the scene of very active development right now.
# A 2015 working version, in Python 2, is visible here:
# https://lsn.ponderworthy.com/doku.php/concurrent_patch_management
###########################################################################

import subprocess
import psutil
import shlex
import time
import os
import jack     # This is JACK-Client, installed as python-jack-client for python3
                # see https://pypi.python.org/pypi/JACK-Client/

# This and the next depend on a Linux command by the name of "beep",
# In some distros it requires "sudo" to run for permissions.
# Great info here: https://wiki.archlinux.org/index.php/Beep

def exit_with_beep():
    for count in range(5):
        try:
            os.system('sudo beep')
        except:
            exit(1)
        sleep(0.1)
    exit(1)


def beep(freq, length):
    try:
        os.system('sudo beep ' + '-f ' + freq + '-l ' + length)
    except:
        return False
    return True

##############################################################################
# Try once to set up new jpctrl JACK client in the optionally named server
# and return the JACK client's object, or None if fails
def setup_jack_client(context_string, jack_client_name='jpctrl_client', jack_server_name='default'):

    # Connect to JACK if possible.
    try:
        jack_client = jack.Client(jack_client_name, false, false, jack_server_name)
        jack_client.activate()
    except:
        print(context_string + ' JACK client setup failed.')
        print('Aborting.')
        jack_client = None
        return None
      
    if jack_client.status.failure or jack_client.status.server_error or jack_client.status.client_zombie:
        jack_client = None
        return None

    return jack_client
  
###################################################################################  
# For 20 seconds, try to set up new jpctrl JACK client, trying once per second.
# Return JACK client object on success, None on failure.
def wait_for_jack(jack_client_name='jpctrl_client', jack_server_name='default'):

    timecount = 0
    while True:
        jack_client = setup_jack_client('wait_for_jack', jack_client_name, jack_server_name)
        if jack_client is None:
            timecount += 1
            if timecount > 20:
                print('JACK server error.  Aborting.')
                return None
            else:
                sleep(1)
        else:
            print('JACK server discovered, verified, and client created!')
            return jack_client

#######################################################################
# Wait for a particular port to become present in the JACK server.
# Returns true on success, false on 6-second timeout and/or failure.
def wait_for_jackport(name2chk, jack_client_name='jpctrl_client', jack_server_name='default'):

    if setup_jack_client('wait_for_jackport', jack_client_name, jack_server_name):
        print('wait_for_jackport() could not connect to JACK server.')
        print('Aborting.')
        return False

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
                sleep(1)
                timecount += 1
            else:
                return True
        except:
            sleep(1)
            timecount += 1

# Find JACK port by substring in the name.
# Return True on success, False on failure
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


# A jpctrl-standardized sleep method.
# Accepts fractions as well as positive integers.
def sleep(time_in_secs):
    time.sleep(time_in_secs)


# Starts a process in the background.
# Return True if successful, False if fails.
def spawn_background(cmd_and_args):
    if try_popen(cmd_and_args) != None:
        return True
    else:
        return False


# Start a process in the background, wait until its I/O
# stats can be retrieved, and one more second.
def spawn_and_settle(cmdstr):
    p_popen = try_popen(cmdstr)
    if p_popen == None:
        print('spawn_and_settle failed for: ', cmdstr)
        print('Could not start process.')
        return False
    p_psutil = psutil.Process(p_popen.pid)
    p_io = try_pio(p_psutil)
    if p_io == None:
        print('spawn_and_settle failed for: ', cmdstr)
        print('Could not get pio data.')
        return False
    else:
        print('spawn_and_settle: process appears ready:', cmdstr)
        sleep(1)
        return True

# Tries to get the p_io data used in spawn_and_settle.
# Waits 15 seconds maximum.  This turns out to be valuable
# towards stability, because as load on the system increases,
# processes take longer at startup to become ready
# to give p_io stats.

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
            sleep(1)
            continue
        else:
            print('successful!')
            return p_io

# Tries to start a background process.
def try_popen(cmdstr):
    timecount = 0
    p_popen = subprocess.Popen(shlex.split(cmdstr), -1)
    while True:
        sleep(1)
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


marker = 1

def marker():
    print('Marker: ' + marker)
    marker += 1
