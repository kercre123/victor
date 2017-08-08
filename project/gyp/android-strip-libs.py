#!/usr/bin/env python2

import argparse
import os
import shutil
import subprocess
import sys

def parse_args(argv):
    parser = argparse.ArgumentParser(description="Copy & strip shared libraries")
    parser.add_argument('--ndk-toolchain',
                        action='store',
                        help="Path to Android NDK toolchain")
    parser.add_argument('--source-libs-dir',
                        action='store',
                        help="Directory to get unstripped libs from")
    parser.add_argument('--target-libs-dir',
                        action='store',
                        help="Directory to place stripped libs")

    options = parser.parse_args(argv[1:])
    return options

def strip(src_lib, stripped_lib, options):
    """Strip unneeded symbols (including debug symbols) from source lib and output to stripped_lib.
       Return True on success, False is an error occurred.
    """
    strip_exe = os.path.join(options.ndk_toolchain, 'bin', 'arm-linux-androideabi-strip')
    cmd = [strip_exe, '-v', '-p', '--strip-unneeded', '-o', stripped_lib, src_lib]
    result = subprocess.call(cmd)
    return (result == 0)

def is_file_up_to_date(target, dep):
    if not os.path.exists(dep):
        return False
    if not os.path.exists(target):
        return False

    target_time = os.path.getmtime(target)
    dep_time = os.path.getmtime(dep)

    return target_time >= dep_time

def getfilesrecursive(src):
    files = []
    if not os.path.exists(src):
        return files
    for item in os.listdir(src):
        full_item = os.path.join(src, item)
        if (os.path.isdir(full_item)):
            files.extend(map(lambda file: os.path.join(item, file), getfilesrecursive(full_item)))
        elif (item.endswith('.so')):
            files.append(item)
    return files

def main(argv):
    options = parse_args(argv)

    success = True
    target_dir = options.target_libs_dir
    source_dir = options.source_libs_dir
    lib_files = getfilesrecursive(source_dir)
    for src_lib in lib_files:
        target_lib = os.path.join(target_dir, src_lib)
        full_src_lib = os.path.join(source_dir, src_lib)
        if not is_file_up_to_date(target_lib, full_src_lib):
            dirname = os.path.dirname(target_lib)
            if not os.path.exists(dirname):
              os.makedirs(dirname)

            print("[android-strip-libs] strip %s -> %s" % (full_src_lib, target_lib))
            success = strip(full_src_lib, target_lib, options)
            if not success:
                break

    return success

if __name__ == '__main__':
    result = main(sys.argv)
    if not result:
        sys.exit(1)
    else:
        sys.exit(0)
