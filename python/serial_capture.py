#!/usr/bin/python

"""serial_capture.py

Captures the data from the input serial connection and writes each line out to file, prefixed with the timestamp for each line.

Author: Lee Crippen
10-07-2015
"""

import os, argparse, serial, datetime, sys

""" Main function sets up the args, parses them to make sure they're valid, then uses them """
def main():
    parser = argparse.ArgumentParser(description='Listen to input serial device and output with timestamps. Specifying the output directory will auto create files named with timestamp for each capture.',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('inDevice', help='Device from which to listen to serial data.', action='store')
    parser.add_argument('-r', help='Baud rate for the serial device.', type=int, nargs='?', default=57600, dest='rate')
    parser.add_argument('time', help='Amount of time in seconds to capture data.', action='store', type=float)
    parser.add_argument('outDirectory', help='Output directory in which to store data or none for stdout.', action='store', nargs='?', default=None, type=is_directory_writable)
    args = parser.parse_args()
    capture(args.inDevice, args.rate, args.time, args.outDirectory)

""" Simple check on the directory to make sure it's valid """
def is_directory_writable(value):
    if value == None:
        return value

    if not os.path.isdir(value):
        raise argparse.ArgumentTypeError("{0} is not a valid directory!".format(value))
    elif not os.access(value, os.W_OK):
        raise argparse.ArgumentTypeError("{0} is not a writable directory!".format(value))
    return value

""" Capture function continuously pulls from serial device and writes to output for time duration """
def capture(inDevice, rate, time, outDir):

    f = sys.stdout
    if outDir != None:
        f = open(outDir + os.sep + 'serialOutput' + '_' + datetime.datetime.now().isoformat('_') + '.dat', 'wb')

    s = serial.Serial(inDevice, rate)
    s.flushInput()
    s.flushOutput()

    """ Throw out the first line in case it's partial """
    get_line(s)

    endTime = datetime.datetime.now() + datetime.timedelta(seconds=time)

    while True:
        currTime = datetime.datetime.now()
        f.write(currTime.isoformat('\t') + '\t' + get_line(s))
        if currTime > endTime:
            break

    f.close()
    s.close()

""" Grab the next few bytes of data up until the newline """
def get_line(serialIn):
    line = str()
    nextByte = ''
    while nextByte != '\n':
        nextByte = serialIn.read()
        line += nextByte
    return line

if __name__ == "__main__":
    main()