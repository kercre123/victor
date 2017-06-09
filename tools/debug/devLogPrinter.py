#!/usr/bin/env python3
"""Command line tool for parsing recorded packet logs."""

import sys
import os
import struct
import argparse

def error(msg):
    "Writes a message out to stderr with newline (os.linesep)"
    sys.stderr.write(msg)
    sys.stderr.write(os.linesep)

# take byte chunk and format it into printed hex bytes
def format_data_string(chunk):
    "Format bytes as compact hex string"
    return ' '.join(map("{0:02x}".format, chunk))

def main():
    "Main function for command line tool"
    parser = argparse.ArgumentParser("Dev Log Printer")
    parser.add_argument('log_file', type=argparse.FileType('rb'),
                        help="Packet log file to parse and print")
    parser.add_argument('types', nargs='*', type=lambda x: int(x, 0),
                        help="packet IDs to print, default all")
    parser.add_argument('-i', '--interval', action='store_true',
                        help="Print the interval between packets rather than packet content")
    parser.add_argument('-p', '--plot', action='store_true',
                        help="Plot the result of certain functions instead of printing")
    parser.add_argument('-c', '--count', action='store_true',
                        help="Count the number of messages of each type rather than priting them")
    args = parser.parse_args()

    if args.plot:
        try:
            from matplotlib import pyplot
        except ImportError:
            sys.exit("matplotlib.pyplot not available")

    messages = parse_log_file(args.log_file, args.types)

    if args.interval:
        last_time = 0
        deltas = []
        for timestamp, size, msg_type, data in messages:
            delta = timestamp - last_time
            if args.plot:
                deltas.append(delta)
            elif not args.types:
                print("{0:>8d} type {1:02X}".format(delta, msg_type))
            else:
                print(delta)
            last_time = timestamp
        if args.plot:
            pyplot.plot(deltas)
            pyplot.show()
    elif args.count:
        msg_types = [m[2] for m in messages]
        for kind in args.types:
            print('0x{0:02x}\t{1:d}'.format(kind, msg_types.count(kind)))
    else:
        for timestamp, size, msg_type, data in messages:
            # split data into groups of 16 for pretty formatting
            data_chunks = (data[i:i+16] for i in range(0, len(data), 16))

            # print message time, type
            print('{0:08d} sz {1:<5d} type {2:02X}'.format(timestamp, size, msg_type))
            for chunk in data_chunks:
                #      12345678 sz 12345 type 00 data
                print('                               ', format_data_string(chunk))

def parse_log_file(log_file, types):
    "A generator for dev packet log file packets"
    while True:
        size = log_file.read(4)
        if not size:
            # eof
            break

        timestamp = log_file.read(4)
        if len(size) != 4 or len(timestamp) != 4:
            error("bad size of size/time")
            break

        # message format is: 4 byte size, 4 byte timestamp, message data
        # message size is little endian
        size = struct.unpack('<1I', size)[0]
        if size <= 8:
            error("invalid size {} for message".format(size))
            break

        timestamp = struct.unpack('<1I', timestamp)[0]

        # read remaining message data
        size = size - 8
        data = log_file.read(size)
        if len(data) != size:
            error("read {} but wanted {}".format(len(data), size))
            break

        # first byte is the type (tag) of CLAD message, rest is the message data
        msg_type = data[0]
        data = data[1:]

        # check if this message type was filtered out and we should skip printing it
        if types and msg_type not in types:
            continue

        yield (timestamp, size, msg_type, data)


if __name__ == '__main__':
    main()
