#!/usr/bin/env python2
#
# File: iosLogcat.py
# 
# Author: jliptak
# Created: 05/25/2016
# 
# Description: Wrapper script around libidevice to get ios syslog information and logging
# 
# Copyright: Anki, Inc. 2016
# 
# 

import argparse
import subprocess
import re
import time
import os
import logging
from datetime import datetime, date, time

logSystems = dict()

# Based on the log level portion of an apple message
logLevelIndex = 1
# Priority: V, D, I, N, W, E, F, S
pythonLogLevels = {'V':logging.DEBUG,
                   'D':logging.DEBUG,
                   'I':logging.INFO,
                   'N':logging.INFO,
                   'W':logging.WARNING,
                   'E':logging.ERROR,
                   'F':logging.CRITICAL,
                   'S':logging.CRITICAL}

# Determine if we should log this based on the given system and log level
# This will be looked up in the logSystems table which maps systems to log levels that we want to log
def shouldLog(system, logLevel):
    # If there is nothing in the log systems we want to log everything
    if len(logSystems) == 0:
        return True

    # apple system names are given in a format like OverDrive[123]
    # or OverDrive(libsystem_network.dylib)[281]
    # We need to grab just the OverDrive string
    systemNameFull = system.split('[')[0]
    systemName = systemNameFull.split('(')[0]
    if (systemName.lower() in logSystems):
        # log levels are given in the format <Warning>:
        # We need to grab the first character
        if logLevel[logLevelIndex] in logSystems[systemName.lower()]:
            return True
    return False

# Read the log of the given device and output to the file is wanted also
# If no Udid is given the first device will be opened via default libidevicesyslog behavior
def readLog(deviceUdid):
    # construct the command which will be ran
    syslogString = list(["idevicesyslog"])
    if len(deviceUdid) != 0:
        syslogString.append("-u")
        syslogString.append(deviceUdid)

    # Open the process and loop waiting for lines while to be read for the command
    # Also grab standard out output so we can read what is happening there
    p = subprocess.Popen(syslogString, stdout=subprocess.PIPE)

    lastLogLevel = logging.NOTSET
    # We verify this is a log line and not a log continuation line by matching these known values:
    timeFormat = re.compile("..:..:..") # 09:17:50
    systemFormat = re.compile(".+\[.+\]") # OverDrive[123]
    levelFormat = re.compile("<.+>:") # <Warning>:

    # Since we don't know if a given line is the start of a log message or a continuation we must
    # log everything seen and then at the start of a log message output the last log message to screen
    while p.poll() == None:
        logLine = p.stdout.readline()
        # Sample log messages:
        # Jun  6 11:36:43 AppleDevice mdmd[6143] <Notice>: (Note ) MDM: mdmd stopping.
        # Jun  6 11:36:43  mdmd[6143] <Notice>: (Note ) MDM: mdmd stopping.
        # Jun 16 11:36:43  mdmd[6143] <Notice>: (Note ) MDM: mdmd stopping.
        # Jun 16 11:36:43 Apple Device mdmd[6143] <Notice>: (Note ) MDM: mdmd stopping.

        # A proper apple log message will have 6 pieces of information seperated by spaces and the log message
        # i.e. month, day(can be single digit leaving 2 spaces), time, device(can be blank but will leave the spaces), system, logLevel and then the message
        # See if we can parse into that, if not this means the line is a log message continuation line

        # Since we have both single and double digit dates where the single dates cause an extra space, we have to remove this extra space
        # i.e. Jan  6 -> Jan 6
        # This makes spliting work on a single space work. We cant just split on white space since if the device is not listed it leaves two spaces making the spliting correct
        # i.e. Jun 6 11:36:43  mdmd[6143] <Notice>: (Note ) MDM: mdmd stopping.
        # Will parse correcly
        if len(logLine) >= 5 and logLine[4] == ' ':
            logLine = logLine.replace(' ', '', 1)
        truncated = logLine.split(' ', 6)
        if len(truncated) == 7:
            month, day, time, device, system, logLevel, msg = truncated

            # See if this is the start of a log message, if not print if we printed the last log line
            # because that will mean this is a continuation of that line
            if not timeFormat.match(time) or not levelFormat.match(logLevel) or not systemFormat.match(system):
                if lastLogLevel != logging.NOTSET:
                    logging.log(lastLogLevel,logLine[:-1])
                continue

            # We now know this is a real log message
            # so print out the log message
            # otherwise set the logging level to NOTSET so the next lines are not printed
            # if this log line has extra lines connected
            if shouldLog(system, logLevel):
                logging.log(lastLogLevel,logLine[:-1])
                lastLogLevel = pythonLogLevels[logLevel[logLevelIndex]]
            else:
                lastLogLevel = logging.NOTSET
        elif lastLogLevel != logging.NOTSET:
            logging.log(lastLogLevel,logLine[:-1])

# expandSystem
# Should be given a system and log level in the format of "OverDrive:I" or "OverDrive"
# This will fill in a dictionary of systems to log levels which will be used to decide which
# lines of information to show
def expandSystem(systemLevel):
    # Priority: V, D, I, W, E, F, S
    # Information will also include notice, which is the apple version of notice
    # Adding support for both

    # This string must be in priority order
    # If N is selected all NWEFS will be added to the list
    # We are using the characters here since that is how we will match them later
    # And it only works because each level starts with something unique
    # Do note VDIN are not allowed in apple due to the /etc/asl.log file not allowing
    # Anythign below notice unless it's kernel
    logLevels = "VDINWEFS"
    if systemLevel.find(':') != -1:
        system, level = systemLevel.split(':')
    else: # log everything
        system = systemLevel
        level = 'V'

    if logLevels.find(level.upper()) != -1:
        # Grab the log level string and add that element and above to the set
        return (system.lower(), set(logLevels[logLevels.find(level.upper()):]))
    else:
        return (system.lower(), set(logLevels))

# This will list all attached idevices to this system
# idevice_id is from libimobiledevice
def getIDevices():
    return subprocess.check_output(["idevice_id", "-l"]).split()

def getDeviceName(deviceUdid):
    return subprocess.check_output(["ideviceinfo", "-u", deviceUdid, "-k", "DeviceName"])

def listDevices():
    devices = getIDevices()
    for device in devices:
        logging.log(logging.INFO, device + " : " + getDeviceName(device))

# openAllDevices
# Get a list of all idevices connected to this machine
# for each one open a new terminal and
# run this script with the given log parsing
def openAllDevices(loggingString):
    devices = getIDevices()

    # Using the timestamp that android uses for logcat but what android uses
    timestamp = datetime.today().strftime("%y%m%d-%H.%M")
    for device in devices:
        # Open a new command line and run this program against a single device
        newCommandline = "osascript -e".split()
        newCommandline.append("tell application \"Terminal\" to do script \""+os.path.realpath(__file__)+" -u "+device+" -f iosLogcat.py."+timestamp+"."+device+".txt "+loggingString+"\"")
        print newCommandline

        p = subprocess.Popen(newCommandline)

def main():
    # Quick check to see if the tools are there
    processInstalled = subprocess.call(["which", "idevicesyslog"])
    if processInstalled != 0:
        print "Please install libimobiledevice with:"
        print "brew install libimobiledevice"
        return -1


    parser = argparse.ArgumentParser()
    parser.add_argument('systems', metavar='N', nargs='*', help="Look for the given system at the logging level and above")
    parser.add_argument('-a', '--all-devices', dest='all_devices', default=False, action='store_const', const=True, help='Open a terminal for all devices connected to this machine')
    parser.add_argument('-f', '--filename', nargs='?', dest='filename', default='', help='Save to the given filename')
    parser.add_argument('-u', '--udid', nargs='?', dest='udid', default='', help='Connect to the given UDID')
    parser.add_argument('-l', '--list-devices', dest='listDevices', default=False, action='store_const', const=True, help='Print the list of all devices attached to this computer')

    # NYI - Flags were taken from logcat
    # parser.add_argument('-d', '--dump', dest='dumpLog', default=False, action='store_const', const=True, help='Dump the current syslog and exit')
    # parser.add_argument('-n', '--rotated-logs-size', dest='logCount', default=4, nargs=1, help='Number of logs that will be in rotation')
    # parser.add_argument('-r', '--log-file-size', dest='logSize', default=16000, nargs=1, help='Size of the log files')

    p = parser.parse_args()
    logString = ""

    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    # open file for appending, if we were given a file name
    # if there is no file name, we will not open or write to the file
    if len(p.filename) != 0:
        fileHandler = logging.FileHandler(p.filename)
        logger.addHandler(fileHandler)
    stdoutHandler = logging.StreamHandler()
    logger.addHandler(stdoutHandler)


    if len(p.systems) != 0:
        global logSystems
        logSystems = dict([expandSystem(system) for system in p.systems])
        # Get the given logging system string so if doing all devices
        # we can hand it off
        logString = reduce((lambda x, y: x + " " + y), p.systems)

    # If opening all the devices only do that and finish
    # Since each on will open up a terminal
    if p.all_devices:
        openAllDevices(logString)
    # If the device list was requested print that and nothing else
    elif p.listDevices:
        listDevices()
    else:
    # Read the wanted log
    # if udid is empty it will open the first one in the device list
    # if file name is empty a file will not be created by default
        readLog(p.udid)

if __name__ == "__main__":
    main()
