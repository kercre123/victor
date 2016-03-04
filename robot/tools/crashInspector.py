#!/usr/bin/env python3
"""
Parser and inspector for robot crash report files.
This file is closely tied to espressif/app/driver/crash.c and it's output format
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, struct, time
if sys.version_info.major < 3:
    sys.exit("Python below version 3 not supported")
sys.path.insert(0, "tools")

# From linker script
INSTRUCTION_START_ADDR = 0x40100000 
INSTRUCTION_END_ADDR   = 0x4026e010

def looksLikeInstruction(i):
    return i >= INSTRUCTION_START_ADDR and i <= INSTRUCTION_END_ADDR

EXCCAUSE_CODES = {
    0:   "IllegalInstruction",
    1:   "Syscall",
    2:   "InstructionFetchError",
    3:   "LoadStoreError",
    4:   "Level1Interrupt",
    6:   "IntegerDivideByZero",
    8:   "Privileged",
    9:   "LoadStoreAlignment",
    12:  "InstrPIFDataErrorCause",
    13:  "LoadStorePIFDataErrorCause",
    14:  "InstrPIFAddrErrorCause",
    15:  "InstrPIFAddrErrorCause",
    16:  "InstTLBMissCause",
    17:  "InstTLBMultiHitCause",
    18:  "InstFetchPrivilegeCause",
    20:  "InstFetchProhibitedCause",
    24:  "LoadStoreTLBMissCause",
    25:  "LoadStoreTLBMultiHitCause",
    26:  "LoadStorePrivilegeCause",
    28:  "LoadProhibitedCause",
    29:  "StoreProhibitedCause",
    32:  "Coprocessor0Disabled",
    33:  "Coprocessor1Disabled",
    34:  "Coprocessor2Disabled",
    35:  "Coprocessor3Disabled",
    36:  "Coprocessor4Disabled",
    37:  "Coprocessor5Disabled",
    38:  "Coprocessor6Disabled",
    39:  "Coprocessor7Disabled",
}

class CrashDump(struct.Struct):
    "Class wrapper for crash dump data"
    
    CRASH_DUMP_STORAGE_HEADER = 0xCDFAF320
    RAW_FORMAT = "128I"
    
    def __init__(self, data, verbose=False):
        struct.Struct.__init__(self, self.RAW_FORMAT)
        if len(data) != self.size:
            raise ValueError("parseFile given wrong amount of data, need {:d} bytes, not {:d} provided".format(self.size, len(data)))
        raw = list(self.unpack(data))
        header = raw.pop(0)
        if header != self.CRASH_DUMP_STORAGE_HEADER:
            raise ValueError("Header 0x{:x} doesn't match".format(header))
        self.version_commit = raw.pop(0)
        self.build_date     = raw.pop(0)
        if verbose:
            sys.stdout.write("Version commit: {:x}, {:s}{linesep}".format(self.version_commit, time.ctime(self.build_date), linesep=os.linesep))
        self.epc1 = raw.pop(0)
        self.ps   = raw.pop(0)
        self.sar  = raw.pop(0)
        self.xx1  = raw.pop(0)
        self.a    = [raw.pop(0) for i in range(16)]
        self.exccause = raw.pop(0)
        if verbose:
            sys.stdout.write("epc1 = {:x}\tps = {:x}\texccause = {:s}{linesep}sar = {:x}\txx1 = {:x}{linesep}a = {:s}{linesep}".format(self.epc1, self.ps, EXCCAUSE_CODES.get(self.exccause, hex(self.exccause)), self.sar, self.xx1, ", ".join(["{:08x}".format(a) for a in self.a]), linesep=os.linesep))
        self.rt_canaries = raw.pop(0)
        if verbose:
            sys.stdout.write("Reliable transport canaries: 0x{:x}{linesep}".format(self.rt_canaries, linesep=os.linesep))
        if raw[0] == 0:
            if verbose:
                sys.stdout.write("No stack included{linesep}".format(linesep=os.linesep))
        else:
            self.excvaddr = raw.pop(0)
            self.depc = raw.pop(0)
            self.stack = raw
            
    def dumpStack(self):
        sys.stdout.write("Stack:{linesep}".format(linesep=os.linesep))
        for i in range(len(self.stack)//8):
            sys.stdout.write(" ".join(["{:08x}".format(e) for e in self.stack[i*8:(i+1)*8]]))
            sys.stdout.write(os.linesep)

    def proposeInstructions(self):
        sys.stdout.write("{linesep}{linesep}Possible instructions:{linesep}".format(linesep=os.linesep))
        sys.stdout.write("epc: {:x}{linesep}".format(self.epc1, linesep=os.linesep))
        if looksLikeInstruction(self.xx1):
            sys.stdout.write("xx1: {:x}{linesep}".format(self.xx1, linesep=os.linesep))
        sys.stdout.write("Registers:{linesep}{}{linesep}".format(os.linesep.join(["{:x}".format(a) for a in self.a if looksLikeInstruction(a)]), linesep=os.linesep))
        sys.stdout.write("Stack:{linesep}{}{linesep}".format(os.linesep.join(["{:x}".format(s) for s in self.stack if looksLikeInstruction(s)]), linesep=os.linesep))

if __name__ == '__main__':
    c = CrashDump(open(sys.argv[1], "rb").read(), True)
    c.dumpStack()
    c.proposeInstructions()
