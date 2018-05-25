#!/usr/bin/env python2

from __future__ import print_function

import argparse
import os
import platform
import re
import string
import subprocess
import sys

# ankibuild
import toolget

PROTOBUF = 'protobuf'

def get_protoc_version_from_command(exe):
    version = None
    if exe and os.path.exists(exe):
        output = subprocess.check_output([exe, '--version'])
        if not output:
            return None
        m = re.match('^libprotoc (\d+\.\d+\.\d+)', output)
        if not m:
            return None
        version = m.group(1)

    return version

def find_anki_protoc_exe(version):
    d = toolget.get_anki_tool_dist_directory(PROTOBUF)
    d_ver = os.path.join(d, version)

    for root, dirs, files in os.walk(d_ver):
        if os.path.basename(root) == 'bin':
            if 'protoc' in files:
                return os.path.join(d_ver, root, 'protoc') 

    return None

def install_protobuf(version):
    platform_map = {
        'darwin': 'osx-x86_64',
        'linux': 'linux-x86_64'
    }

    platform_name = platform.system().lower()

    (major, minor, patch) = version.split('.')
    short_ver = "{}.{}".format(major, minor)
    # http://sai-general.s3.amazonaws.com/build-assets/protoc-3.5.1-osx-x86_64-SHA-256.txt
    url_prefix = "http://sai-general.s3.amazonaws.com/build-assets"

    pb_platform = platform_map[platform_name]
    pb_basename = "protoc-{}-{}".format(version, pb_platform)
    pb_archive_url = "{}/{}.tar.gz".format(url_prefix, pb_basename)
    pb_hash_url = "{}/{}-SHA-256.txt".format(url_prefix, pb_basename)

    pb_downloads_path = toolget.get_anki_tool_downloads_directory(PROTOBUF)
    pb_dist_path = toolget.get_anki_tool_dist_directory(PROTOBUF)

    toolget.download_and_install(pb_archive_url,
                                 pb_hash_url,
                                 pb_downloads_path,
                                 pb_dist_path,
                                 pb_basename,
                                 pb_basename,
                                 version,
                                 "protobuf")

def find_protoc(required_ver, exe=None):
#    if not exe:
#        try:
#            exe = subprocess.check_output(['which', 'protoc']).rstrip()
#        except subprocess.CalledProcessError as e:
#            pass

    if get_protoc_version_from_command(exe) == required_ver:
        return exe

    exe = find_anki_protoc_exe(required_ver)
    if get_protoc_version_from_command(exe) == required_ver:
        return exe

    return None

def find_or_install_protoc(required_ver, exe=None):
    exe = find_protoc(required_ver, exe)
    if exe:
        return exe

    install_protobuf(required_ver)
    return find_anki_protoc_exe(required_ver)

def setup_protobuf(required_ver):
    exe = find_or_install_protoc(required_ver)
    if not exe:
        raise RuntimeError("Could not find protoc (protobuf) version {0}"
                            .format(required_ver))
    return exe

def parseArgs(scriptArgs):
    version = '1.0'
    default_version = "3.5.1"
    parser = argparse.ArgumentParser(description='finds or installs protobuf', version=version)
    parser.add_argument('--install',
                        nargs='?',
                        const=default_version,
                        default=None)
    parser.add_argument('--find',
                        nargs='?',
                        const=default_version,
                        default=None)
    (options, args) = parser.parse_known_args(scriptArgs)
    return options


def main(argv):
    options = parseArgs(argv)
    if options.install:
        path = find_or_install_protoc(options.install)
        if not path:
            return 1
        print("%s" % path)
        return 0
    elif options.find:
        path = find_protoc(options.find)
        if not path:
            return 1
        print("%s" % path)
        return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)

