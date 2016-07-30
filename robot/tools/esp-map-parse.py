#!/usr/bin/env python3
"""
Utility for parsing useful data out of Espressif map files
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, argparse, re, mmap

LAYOUT = {
    "dram0": (0x3FFE8000,  0x14000),
    "brom":  (0x40000000,  0x10000),
    "iram":  (0x40100000,  0x10000),
    "flash": (0x40200000, 0x100000)
}

SIZE_RE = re.compile(rb".*0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(.+)")

def main():
    parser = argparse.ArgumentParser(prog="Espressif map parser",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-s', '--size', action="store_true", help="Parse map file for code size information")
    parser.add_argument("mapfile", type=argparse.FileType('rb'), help="Map file to parse")
    args = parser.parse_args()
    
    with mmap.mmap(args.mapfile.fileno(), 0, access=mmap.ACCESS_READ) as mapfile:
        if args.size:
            entries = [(int(m[0], 16), int(m[1], 16), m[2]) for m in SIZE_RE.findall(mapfile)]
            regions = {r: sorted((e for e in entries if e[0] >= b[0] and e[0] < (b[0] + b[1])), key=lambda e: e[1], reverse=True) for r, b in LAYOUT.items()}
            for region, code in regions.items():
              print("================================================")
              print(region, ":")
              for address, size, thing in code:
                if size:
                  print(hex(size), thing.decode())


if __name__ == '__main__':
  main()
