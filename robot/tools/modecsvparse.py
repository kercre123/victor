#!/usr/bin/env python3
"""
Tool agagregating firmware crash dumps

(C)2016 Anki, Inc
Author:  Adam Shelly 

"""

import argparse
import base64
import csv
import sys
import os


sys.path.insert(0, os.path.join("tools"))
try:
    import crashdump
except ImportError:
    sys.exit("Couldn't import crashdump.py")

    


import crashdump 

# from CrashSource in cozmo-one/robot/clad/src/clad/types/robotErrors.clad
TypeMap = {
    "WiFiCrash" : 0,
    "RTIPCrash":  1,
    "BodyCrash":  2,
    "PropCrash":  3,
    "I2SpiCrash": 4,
    "BootError": 5
}

datapath = "/"
    
def extract_type(typestr):
    ## Expects string in form of "Crash Dump Type XXX", returns type.
    words = typestr.split()
    return " ".join(words[words.index("Type")+1:])


def parse_dump(typestr, b64str):
    content = base64.b64decode(b64str)
    type_id = TypeMap[typestr]
    registers = crashdump.fetch_registers(content,
                                          crashdump.RegisterMap[type_id])
    if type_id == 0:
        pc = registers.get("epc1", 0)
        cause = crashdump.EspCauses[registers.get("exccause")]
    elif type_id == 5:
        pc = registers.get("address", 0)
        cause = registers.get("error", "Unknown")
    else:
        pc = registers.get("pc", 0)
        cause = "crash"
    description = "{}_{:08X}_{}".format(typestr,pc,cause)
    return (description, registers)

def store_dump(dirname, registers, apprun):
    dumpfilename = os.path.join(dirname, str(apprun))
    with open(dumpfilename, "w+") as dumpfile:
        for r in registers:
            dumpfile.write("\t{}\t: {:08x}\n".format(r,registers[r]))

           
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser(description='Extract crashdumps from Mode CSV Dump')
    parser.add_argument('filename', help='the csv file')
    args = parser.parse_args()

    datapath = os.path.dirname(args.filename)

    with open(args.filename, 'r') as csvfile:
        table = csv.DictReader(csvfile)
        for row in table:
            if row['s_val'] is None:
                continue
            # print(row)
            try:
                summary = row['data'][0:16]
            except TypeError:
                summary = ""
            print("Parsing ",row['s_val'], summary)
            typestr = extract_type(row['s_val'])
            what, why = parse_dump(typestr, row['data'])
            dirname = os.path.join(datapath, what)
            os.makedirs(dirname, exist_ok = True);
            store_dump(dirname, why, row['apprun'])

        

