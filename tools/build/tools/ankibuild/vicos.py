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

def get_anki_downloads_dir():
    return os.path.join(os.path.expanduser('~'), '.anki', 'vicos', 'vicos-downloads')

def get_anki_dist_dir():
    return os.path.join(os.path.expanduser('~'), '.anki', 'vicos', 'vicos-repository')

def get_version_tag(version):
    return 'vicos-sdk-{}'.format(version)

def install_vicos_sdk(version):
    platform_map = {
        'darwin': 'x86_64-apple-darwin',
        'linux': 'x86_64-linux-gnu'
    }

    platform_name = platform.system().lower()

    subver_tag = ''
    if platform_name == 'linux':
        import lsb_release
        subver = lsb_release.get_lsb_information().get('RELEASE')
        subver_tag = '-ubuntu-{}'.format(subver)

    platform_tag = '{}{}'.format(platform_map.get(platform_name), subver_tag)

    url_prefix = "https://sai-general.s3.amazonaws.com/build-assets/"
    platform_name = platform.system().lower()
    sdk_base_name = "vicos-sdk-{0}-{1}".format(version, platform_tag)
    anki_sdk_url = "{0}{1}.tar.bz2".format(url_prefix, sdk_base_name)
    anki_hash_url = "{0}{1}-SHA-256.txt".format(url_prefix, sdk_base_name)

    downloads_path = toolget.get_anki_tool_downloads_directory('vicos-sdk')
    dist_path = toolget.get_anki_tool_dist_directory('vicos-sdk')

    #vicos_basename = get_version_tag(version)
    vicos_basename = sdk_base_name
    #downloads_path = get_anki_downloads_dir()
    #dist_path = os.path.join(get_anki_dist_dir(), get_version_tag(version))
    toolget.download_and_install(anki_sdk_url,
                                 anki_hash_url,
                                 downloads_path,
                                 dist_path,
                                 vicos_basename,
                                 vicos_basename,
                                 version,
                                 "vicos-sdk")

def get_sdk_version_from_sdk_dir(sdk_dir):
    version_file = os.path.join(sdk_dir, 'VERSION')
    version = None
    with open(version_file, 'r') as f:
        version = f.read().strip()
    return version

def get_anki_sdk_dir(required_ver):
    # check for a version in .anki
    #vicos_sdk_tag = get_version_tag(required_ver)
    anki_vicos_base_dir = toolget.get_anki_tool_dist_directory('vicos-sdk')
    anki_vicos_dir = os.path.join(anki_vicos_base_dir, required_ver)
    return anki_vicos_dir

def find_sdk_root_dir(required_ver):
    env_value = os.environ.get('VICOS_SDK_HOME')
    if env_value:
        # check environment defined value
        version = get_sdk_version_from_sdk_dir(env_value)
        if version == required_ver:
            return env_value

    # check for a version in .anki
    anki_vicos_dir = get_anki_sdk_dir(required_ver)
    
    if not os.path.exists(anki_vicos_dir):
        return None
    else:
        return anki_vicos_dir

def find_or_install_vicos_sdk(required_ver):
    sdk_root_dir = find_sdk_root_dir(required_ver)

    if sdk_root_dir:
        return sdk_root_dir
    
    install_vicos_sdk(required_ver)
    sdk_root_dir = find_sdk_root_dir(required_ver)

    return sdk_root_dir

def parseArgs(scriptArgs):
    version = '1.0'
    parser = argparse.ArgumentParser(description='finds or installs vicos sdk', version=version)
    parser.add_argument('--install',
                        action='store',
                        dest='required_version',
                        nargs='?')
    (options, args) = parser.parse_known_args(scriptArgs)
    return options


def main(argv):
    options = parseArgs(argv)
    if options.required_version:
        path = find_or_install_vicos_sdk(options.required_version)
        if not path:
            return 1
        print("%s" % path)
        return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)

