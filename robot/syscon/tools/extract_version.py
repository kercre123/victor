#!/usr/bin/env python3

import sys

file_name = sys.argv[1]

f = open(file_name,"rb")

f.read(0x118)
version = f.read(0xf).decode('utf-8')


if version.startswith("ws") or version.startswith("DevBuild"):
    print(version)
else:
    raise RuntimeError("Not a syscon.raw or syscon.dfu file!")
