#!/usr/bin/env python3
"""
Tool for inspecting crash records located in complete espressif flash dump
"""

import argparse
import struct
import sys
import os

def print_raw(data):
    for offset in range(0, len(data), 4):
        if (offset%32 == 0):
            print("\n\t",end="")
        print("{:08x} ".format(struct.unpack("<I",data[offset:offset+4])[0]),end='')
    print("")

def fetch_registers(data, regs):
    return {regs[i]: struct.unpack("<I",data[i*4:i*4+4])[0] for i in range(len(regs))}

def print_registers(data,regs):
    offset = 0
    regset = fetch_registers(data, regs)
    for r in regs:
        print("\t{}\t: {:08x}".format(r,regset[r]))
    return regset


'''Register names from  include/anki/cozmo/robot/crashLogs.h'''

def print_NRF(data):
    print_registers(data,
                    ["r8", "r9", "r10", "r11", "r4", "r5", "r6", "r7",
                     "r0", "r1", "r2", "r3", "r12", "lr", "pc", "psr"])

def print_K2(data):
    print_registers(data, 
                    ["r8", "r9", "r10", "r11", "r4", "r5", "r6", "r7",
                     "r0", "r1", "r2", "r3", "r12", "lr", "pc", "psr",
                     "bfar", "cfsr", "hfsr", "dfsr", "afsr", "shcsr"])

def print_ESP(data):
    regs = ["epc1", "ps", "sar", "xx1", "a0", "a2", "a3", "a4",
            "a5", "a6", "a7", "a8", "a9", "a10", "a11", "a12",
            "a13", "a14", "a15", "exccause", "sp", "excvaddr", "depc", "stack_depth"]
    EspCauses = {
        0: "IllegalInstruction",  1: "Syscall",
        2: "InstructionFetchError", 3: "LoadStoreError",
        4: "Level1Interrupt", 5: "Alloca",
        6: "IntegerDivideByZero", 8: "Privileged",
        9: "LoadStoreAlignment", 12:  "InstrPIFDataError",
        13: "LoadStorePIFDataError", 14: "InstrPIFAddrError",
        15: "LoadStorePIFAddrError", 16: "InstTLBMiss",
        17: "InstTLBMultiHit", 18: "InstFetchPrivilege",
        20: "InstFetchProhibited", 24: "LoadStoreTLBMiss",
        25: "LoadStoreTLBMultiHit", 26: "LoadStorePrivilege",
        28: "LoadProhibited", 29: "StoreProhibited"
        }
    regset = print_registers(data, regs)
    stack_data_start = len(regs)*4
    stack_size = regset["stack_depth"]
    if stack_size < (1024-16)-stack_data_start:
        stack_addrs = ["\t{:08x}".format(regset["sp"]+i*4) for i in range(stack_size)]
        print_registers(data[stack_data_start:], stack_addrs)
    print("cause = ",EspCauses[regset["exccause"]])

    
def print_regs(reporter, data):
    if reporter == 0:
        print_ESP(data)
    elif reporter == 1:
        print_NRF(data)
    elif reporter == 2:
        print_K2(data)
    print()


parser = argparse.ArgumentParser(description='Extract crashdumps from Espressif NVStorage')
parser.add_argument('filename', help='the binary datafile')
parser.add_argument('-das', action='store_true')
args = parser.parse_args()

START = 0x02000
SegSize = 0x1000
CRASH_RECORD_SIZE = 1024
Names = {0:"Esp ", 1:"Body", 2:"RTIP"}

if args.das:
    START = 0

with open(args.filename, 'rb') as infile:
    infile.seek(START)
    content = infile.read(SegSize)
    offset = 0;
    dumpdir = None
    entry = 0;
        
    while offset < len(content):
        if not args.das:
            wasWritten,wasReported,reporter,errcode = struct.unpack("<IIII", content[offset:offset+16])
            offset +=16
        else:
            wasWritten,wasReported = (0,0)
#            reporter = #extract from filename
            reporter,errcode = (0,0)
        
        print(args.das,wasWritten,wasReported,reporter,errcode)
        if wasWritten == 0:
            print("entry {}. {} crashed w/error {}, reported={}\n".format(
                entry, Names.get(reporter, "????"), errcode, hex(wasReported)))
            if (errcode == 0):
                print_regs(reporter, content[offset:offset+CRASH_RECORD_SIZE])
        elif wasWritten == 0xFFFFFFFF:
            print("entry {}. EMPTY\n".format(entry))
        else:
            print("entry {}. GARBAGE\n".format(entry))
            print_raw(content[offset:offset+CRASH_RECORD_SIZE])
        offset += CRASH_RECORD_SIZE
        entry +=1


