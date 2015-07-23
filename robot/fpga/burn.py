#!/usr/bin/env python3
"""
Programs the FPGA firmware via FTDI
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, subprocess

sys.path.insert(0, os.path.join('..', '..', 'python'))

from latticeFT2232Prog import *

def checkForCDC():
    "Returns whether the FTDI_SIO driver is loaded"
    lsmod = subprocess.Popen(['lsmod'], stdout=subprocess.PIPE)
    ecode = lsmod.wait()
    rslt, err = lsmod.communicate()
    if ecode != 0:
        exit(err)
    else:
        if rslt.find(b"ftdi_sio") != -1:
            return True
        else:
            return False

def unloadCDC():
    "Unloades the FTDI_SIO driver or dies"
    ecode = subprocess.call(["rmmod", "ftdi_sio"])
    if ecode != 0:
        sys.exit(ecode)

def testSPI(port):
    # Write some stuff
    checkResult(port.PinLow(nCS))
    checkResult(port.Start())
    checkResult(port.Write(bytes([0xcf, 0x0F, 0x55, 0xFE])))
    time.sleep(0.030)
    checkResult(port.Stop())
    checkResult(port.PinHigh(nCS))
    time.sleep(0.016)
    # Read some stuff
    checkResult(port.PinLow(nCS))
    checkResult(port.Start())
    resp = port.Read(4)
    sys.stdout.write("Read {}\r\n".format(['0x{:02x}'.format(b) for b in resp]))
    checkResult(port.Stop())
    checkResult(port.PinHigh(nCS))
    

if __name__ == "__main__":
    if checkForCDC():
        ans = input("CDC driver is present, should it be unloaded? [Y,n]: ")
        if not ans or ans.lower().startswith("y"):
            unloadCDC()
    port = setupFTDI()
    programFPGA(port, os.path.join("fpgaisp_Implmnt", "sbt", "outputs", "bitmap", "top_bitmap.bin"))
    testSPI(port)
    port.Close()
