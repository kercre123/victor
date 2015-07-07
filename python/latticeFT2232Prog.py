#!/usr/bin/env python3
"""
Script for writing Lattice FPGA bit files over an FT2232 doing SPI
"""
author = "Daniel Casner <daniel@anki.com>"

import sys, os

from mpsse import *

USAGE ="""
{} <program file>

Writes the program file to the 0th FT2232 channel available
""".format(sys.argv[0])

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

def programFPGA(program_file_path_name):
    port = MPSSE(SPI0, THIRTY_MHZ, MSB)
    checkResult(port.Start())
    checkResult(port.SetCSIdle(0)) # Set CS low for Reset
    input("Reset the FPGA") # Wait for the user
    checkResult(port.Stop())
    checkResult(port.Start())
    checkResult(port.SetCSIdle(1)) # Put CS back into the right mode
    progFile = open(program_file_path_name, 'rb').read() # Read in the programming file
    checkResult(port.Write(progFile + (b"\x00"*50))) # Write programming file plus dummy bits
    checkResult(port.Stop())
    checkResult(port.Close())

if __name__ == "__main__":
    if len(sys.argv) != 2:
        exit(USAGE)
    elif not os.path.isfile(sys.argv[1]):
        exit("{} Doesn't seem to be a readable file\r\n".format(sys.argv[1]))
    else:
        programFPGA(sys.argv[1])
