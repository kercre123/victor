#!/usr/bin/env python

import serial
from time import sleep
from sys import stdout, stdin
import argparse

ser = serial.Serial()
ser.baudrate = 1000000

def print_output():
    sleep(0.01)
    while ser.in_waiting > 1:
        stdout.write(ser.read(ser.in_waiting).decode('utf-8'))
        sleep(0.01)

def interactive_console():
    ser.write(b'\x1b\r\n')
    print_output()
    ser.write(b'help\n')
    print_output()
    print_output()

    while True:
        line = input("> ")+ "\r\n"
        ser.write(bytes(line,'utf-8'))
        print_output()
        if line == "exit\r\n":
            return

def newline_at(s):
    try:
        return s.index('\n')
    except ValueError:
        return -1
        
def log_console():
    log_index = 0
    buf = ""
    while True:
        sleep(0.01)
        while ser.in_waiting > 1:
            buf += ser.read(ser.in_waiting).decode('utf-8')
            idx = newline_at(buf)
            while idx > -1:
                line = buf[0:idx+1]

                if line[0:2] in [">>", "<<"]:
                    if line[0:10] == ">>logstart":
                        print("-------------- LOG %i START --------------" % log_index)
                    elif line[0:9] == ">>logstop":
                        print("-------------- LOG %i START --------------" % log_index)
                        log_index = log_index + 1
                    stdout.write(line)
                        
                else:
                    stdout.write(line)
                buf = buf[idx+1:]
                idx = newline_at(buf)
            sleep(0.01)

def firmware_update(fixture_safe):
    header = fixture_safe.read(4)
    ser.write(header)
    stdout.write(ser.read(4).decode('utf-8')) # Firmware sends 4 different indicators that we've hit checkpoints
    stdout.flush()
    file_contents = fixture_safe.read(2048+32-4)

    while file_contents != b'':
#        print("LEN %s\n" % len(file_contents))
        ser.write(file_contents)
        stdout.write(ser.read(1).decode('utf-8')) # wait until this packet is programmed.
        stdout.flush()
        file_contents = fixture_safe.read(2048+32)
    print("LEN %s\n" % len(file_contents))

def main():
        
    parser = argparse.ArgumentParser(description="CLI interface for Cozmo Fixture USB connector")

    parser.add_argument('command', metavar='CMD', type=str, help='log/console/update')
    parser.add_argument('--device', type=str, required=True, help='USB Device /dev/cu.usbserialXXXXXX on mac or COM99 on windows')
    parser.add_argument('--firmware', type=str, help='Fixture firmware to update')
                    
    args = parser.parse_args()
    print(repr(args))

    ser.port = args.device
    ser.open()

    if args.command == 'log':
        log_console()
    elif args.command == 'console':
        interactive_console()
    elif args.command == 'update':
        if args.firmware is None:
            raise RuntimeError("No firmware provided. Need --firmware")
        fixture_update_file = open(args.firmware,"rb")
        firmware_update(fixture_update_file)
    else:
        raise RuntimeError("Unknown command %s" % args.command)

if __name__ == "__main__":
    main()

