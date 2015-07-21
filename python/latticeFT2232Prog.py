#!/usr/bin/env python3
"""
Script for writing Lattice FPGA bit files over an FT2232 doing SPI
"""
author = "Daniel Casner <daniel@anki.com>"

import sys, os, time

from mpsse import *

USAGE ="""
{} <program file>

Writes the program file to the 0th FT2232 channel available
""".format(sys.argv[0])

nCS    = GPIOL0
CDONE  = GPIOL2
nReset = GPIOL3

def checkResult(ret, exitOnFail=True):
    "Checks the result of an MPSSE call"
    if ret is None or ret == MPSSE_OK:
        return True
    else:
        sys.stderr.write("MPSSE error: {}\r\n".format(ret))
        if exitOnFail:
            sys.exit(1)
        else:
            return False

def setupFTDI():
    port = MPSSE(SPI0, TWO_MHZ, MSB)
    sys.stdout.write("Opened {}\r\n".format(port.GetDescription()))
    checkResult(port.PinHigh(nCS))
    checkResult(port.PinHigh(CDONE))
    checkResult(port.PinHigh(nReset))
    return port

def programFPGA(port, program_file_path_name):
    checkResult(port.PinLow(nCS))
    checkResult(port.PinLow(nReset))
    time.sleep(0.016)
    checkResult(port.PinHigh(nReset))
    time.sleep(0.1)
    checkResult(port.PinHigh(nCS))
    time.sleep(0.016)
    progFile = open(program_file_path_name, 'rb').read() # Read in the programming file
    checkResult(port.PinLow(nCS))
    checkResult(port.Start())
    checkResult(port.Write(progFile)) # Write programming file 
    checkResult(port.Write(bytes(range(256))))# Write the dummy bits
    time.sleep(0.030)
    checkResult(port.Stop())
    checkResult(port.PinHigh(nCS))
    checkResult(port.PinHigh(nReset))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        exit(USAGE)
    elif not os.path.isfile(sys.argv[1]):
        exit("{} Doesn't seem to be a readable file\r\n".format(sys.argv[1]))
    else:
        port = setupFTDI()
        programFPGA(port, sys.argv[1])
        port.Close()
