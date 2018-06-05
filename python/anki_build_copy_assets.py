#!/usr/bin/env python3

## python3 python/anki_build_copy_assets.py --target platform_config_etc --srclist_dir /Users/richard/projects/victor/generated/cmake --output_dir /Users/richard/projects/victor/_build/vicos/Debug --anki_platform_name vicos --cmake_current_source_dir /Users/richard/projects/victor/platform/config

import argparse
import os
import shutil
import sys

def anki_build_copy_assets(TARGET, SRCLIST_DIR, OUTPUT_DIR, ANKI_PLATFORM_NAME, CMAKE_CURRENT_SOURCE_DIR):

    _SRCS = []
    _DSTS = []

    file = os.path.join(SRCLIST_DIR, TARGET+".srcs.lst")
    if os.path.exists(file):
        with open(file, "r") as f:
            _SRCS = f.readlines() 

    file = os.path.join(SRCLIST_DIR, TARGET+".dsts.lst")
    if os.path.exists(file):
        with open(file, "r") as f:
            _DSTS = f.readlines() 

    _PLATFORM_SRCS = []
    _PLATFORM_DSTS = []

    file = os.path.join(SRCLIST_DIR, TARGET+"_"+ANKI_PLATFORM_NAME+".srcs.lst")
    if os.path.exists(file):
        with open(file, "r") as f:
            _PLATFORM_SRCS = f.readlines() 

    file = os.path.join(SRCLIST_DIR, TARGET+"_"+ANKI_PLATFORM_NAME+".dsts.lst")
    if os.path.exists(file):
        with open(file, "r") as f:
            _PLATFORM_DSTS = f.readlines() 

    SRCS = _SRCS + _PLATFORM_SRCS
    DSTS = _DSTS + _PLATFORM_DSTS

    for IDX in range(0, len(SRCS)):
        SRC = SRCS[IDX].strip()
        DST = DSTS[IDX].strip()

        # uncomment for DEBUGGING
        # print("cp %s %s" % (SRC, DST))

        SRC_PATH = os.path.join(CMAKE_CURRENT_SOURCE_DIR, SRC)
        DST_PATH = os.path.join(OUTPUT_DIR, DST)

        should_copy = False

        if os.path.exists(DST_PATH):
            src_modified = os.path.getmtime(SRC_PATH)
            dst_modified = os.path.getmtime(DST_PATH)
            delta = src_modified - dst_modified
            # small epsilon because two identical times, the product of shutil.copy2, are not identical
            if delta > 0.0001:
                should_copy = True
        else:
            should_copy = True

        if should_copy:
            # print("copying %s to %s" % (SRC, DST))
            try:
                # delete the file first in case permissions are read-only which prevents the copy
                # (see animation assets)
                if os.path.exists(DST_PATH):
                    os.remove(DST_PATH)

                DST_DIR = os.path.split(DST_PATH)[0]
                if not os.path.isdir(DST_DIR):
                    os.makedirs(DST_DIR)
                    
                shutil.copy2(SRC_PATH, DST_PATH)
            except OSError as why:
                 print("failed to copy %s to %s because %s" % (SRC_PATH, DST_PATH, str(why)))
                 raise

    return 0

def main():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--target', required = True)
    parser.add_argument('--srclist_dir', required = True)
    parser.add_argument('--output_dir', required = True)
    parser.add_argument('--anki_platform_name', required = True)
    parser.add_argument('--cmake_current_source_dir', required = True)

    options = parser.parse_args()
    ret = anki_build_copy_assets(options.target, options.srclist_dir, options.output_dir, options.anki_platform_name, options.cmake_current_source_dir)
    return ret

if __name__ == '__main__':
    ret = main()
    sys.exit(ret)
