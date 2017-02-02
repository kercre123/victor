#!/usr/bin/env python3
"""
Tool for inspecting binary dump of expressif flash storage
  can inspect segment '1', '2', or 'f'actory data.
  optionally write the tag data to <filename>.blobs/
"""

import argparse
import struct
import sys
import os

TagList = {
    0xFFFFffff : "NVEntry_Invalid",
    0xFFFE0000 : "NVEntry_WipeAll",
    0x2 : "NVEntry_GameSkillLevels",
    0x3 : "NVEntry_OnboardingData",
    0xFFFF : "NVEntry_Junk",
    0x00010000 : "NVEntry_GameUnlocks",
    0x00FA0000 : "NVEntry_FaceAlbumData",
    0x00FE0000 : "NVEntry_FaceEnrollData",
    0x7FFF0000 : "NVEntry_MultiBlobJunk",
    0x80000000 : "NVEntry_BirthCertificate",
    0x80000001 : "NVEntry_CameraCalib",
    0x80000002 : "NVEntry_ToolCodeInfo",
    0x80000003 : "NVEntry_CalibPose",
    0x80000004 : "NVEntry_CalibMetaInfo",
    0x80000005 : "NVEntry_ObservedCubePose",
    0x80000006 : "NVEntry_IMUInfo",
    0x80000007 : "NVEntry_CliffValOnDrop",
    0x80000008 : "NVEntry_CliffValOnGround",
    0x80000010 : "NVEntry_PlaypenTestResults",
    0x80000011 : "NVEntry_FactoryLock",
    0x80010000 : "NVEntry_CalibImage1",
    0x80020000 : "NVEntry_CalibImage2",
    0x80030000 : "NVEntry_CalibImage3",
    0x80040000 : "NVEntry_CalibImage4",
    0x80050000 : "NVEntry_CalibImage5",
    0x80060000 : "NVEntry_CalibImage6",
    0x80100000 : "NVEntry_ToolCodeImageLeft",
    0x80110000 : "NVEntry_ToolCodeImageRight",
    0xC0000000 : "NVEntry_PrePlaypenResults",
    0xC0000001 : "NVEntry_PrePlaypenCentroids",
    0xC0000000 : "NVEntry_IMUAverages"
}
Segments = {'f': 0xde000, '1': 0x1c000c, '2':0x1de000, '*':0x0000}
SegSize = 0x1e000

UNREADABLE = "????????"
UNKNOWN = "UNKNOWN"

def partialTagMatch(tag):
    for key in TagList:
        if (tag & 0xFFFF0000) == (key & 0xFFFF0000):
            return TagList[key] + "-{:x}".format(tag & 0xFFFF)
    return UNKNOWN
    

def dumptag(path,tag,addr, data):
    filename = os.path.join(path, "Flash-{:08x}-{}.raw".format(addr,tag))
    with open(filename, "wb") as of:
        of.write(data)

def seek(addr, data):
    state = "00"
    while addr < SegSize:
        value = struct.unpack("<I", data[addr:addr+4])[0]
        if state == "00":
            if value == 0:
                state = "ff"
                addr += 8
        elif state == "ff":
            if value == 0xffffffff:
                return addr-8
            else:
                state="00"
        addr += 4
    return SegSize

def make_dump_directory(filename):
    dir,fname = os.path.split(os.path.abspath(filename))
    dumpdir = os.path.join(dir, "{}.blobs".format(fname))
    print(dumpdir, dir, fname)
    os.makedirs(dumpdir)
    return dumpdir


parser = argparse.ArgumentParser(description='Extract tags from Espressif NVStorage')
parser.add_argument('--dump', help='write the tags to disk', action='store_true')
parser.add_argument('filename', help='the binary datafile')
parser.add_argument('segment', help='storage segment', nargs='?', choices=['f','1','2','*'], default='f')
args = parser.parse_args()

START = Segments[args.segment]

with open(args.filename, 'rb') as infile:
    infile.seek(START)
    content = infile.read(SegSize)
    offset = 0;
    dumpdir = None
        
    while offset < len(content):
        size,tag,ff = struct.unpack("<III", content[offset:offset+12])
        tagname = TagList.get(tag, UNKNOWN)
        if tagname is UNKNOWN:
            tagname = partialTagMatch(tag)
        if offset + size < len(content):
            tail = hex(struct.unpack("<I", content[offset+size-4:offset+size])[0])
            next_offset = offset + size;
        else:
            tail = UNREADABLE
            if tagname != "NVEntry_Invalid":
                next_offset = seek(offset, content)
            else:
                next_offset = len(content)
            
        print("{:08X}: Found tag {:X} ({}). size = {:5d}, succesor = {:X},  tail = {}".format(
            offset+START, tag, tagname, size, ff, tail))
        if tail is UNREADABLE:
            print("Skipping forward {} bytes".format(next_offset - offset))
        if args.dump:
            if dumpdir is None:
                dumpdir = make_dump_directory(args.filename)

            dumpdata = content[offset+12:next_offset-4]
            if tail is UNREADABLE:  #include the bad tag data
                dumpdata = content[offset:next_offset]
                
            dumptag(dumpdir, tagname, offset+START, dumpdata)
        offset = next_offset


