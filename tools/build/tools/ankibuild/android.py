#!/usr/bin/env python

from __future__ import print_function

import argparse
import ConfigParser
import hashlib
import io
import os
import pipes
import platform
import re
import shutil
import string
import subprocess
import sys
import urllib
import zipfile

# ankibuild
import util

def get_toplevel_directory():
    toplevel = None
    try:
        toplevel = util.Git.repo_root()
    except KeyError:
        script_path = os.path.dirname(os.path.realpath(__file__))
        toplevel = os.path.abspath(os.path.join(script_path, os.pardir, os.pardir))
    return toplevel

def parse_buckconfig_as_ini(path):
    ini_lines = []
    with open(path) as f:
        for ini_line in f:
            ini_lines.append(ini_line.strip())
    ini_data = '\n'.join(ini_lines)

    config = ConfigParser.RawConfigParser(allow_no_value=True)
    config.readfp(io.BytesIO(ini_data))

    return config

def get_ndk_version_from_buckconfig(path):
    if not path or not os.path.isfile(path):
        return None
    config = parse_buckconfig_as_ini(path)
    return config.get('ndk', 'ndk_version')

def read_properties_from_file(path):
    properties = {}
    if path and os.path.isfile(path):
        with open(path) as f:
            for line in f:
                name, var = line.partition("=")[::2]
                properties[name.strip()] = str(var.strip())
    return properties

def get_ndk_version_from_source_properties(path):
    properties = read_properties_from_file(path)
    return properties["Pkg.Revision"]

def get_ndk_version_from_release_text(path):
    with open(path) as f:
        return f.readline().split()[0].split('-')[0]

def get_ndk_version_from_ndk_dir(path):
    if not path:
        return None
    source_properties_path = os.path.join(path, 'source.properties')
    release_txt_path = os.path.join(path, 'RELEASE.TXT')
    if os.path.isfile(source_properties_path):
        return get_ndk_version_from_source_properties(source_properties_path)
    elif os.path.isfile(release_txt_path):
        return get_ndk_version_from_release_text(release_txt_path)
    return None

def get_android_sdk_ndk_dir_from_local_properties():
    path_to_local_properties = os.path.join(get_toplevel_directory(), 'local.properties')
    properties = read_properties_from_file(path_to_local_properties)
    return properties

def write_local_properties():
    path_to_local_properties = os.path.join(get_toplevel_directory(), 'local.properties')
    properties = get_android_sdk_ndk_dir_from_local_properties()
    properties['ndk.dir'] = os.environ['ANDROID_NDK_ROOT']
    properties['sdk.dir'] = os.environ['ANDROID_HOME']
    with open(path_to_local_properties, 'wb') as f:
        for key, value in properties.items():
            f.write("{0}={1}\n".format(key,value))

def get_anki_android_directory():
    anki_android_dir = os.path.join(os.path.expanduser("~"), ".anki", "android")
    util.File.mkdir_p(anki_android_dir)
    return anki_android_dir

def get_anki_android_ndk_repository_directory():
    ndk_repo_directory_path = os.path.join(get_anki_android_directory(), 'ndk-repository')
    util.File.mkdir_p(ndk_repo_directory_path)
    return ndk_repo_directory_path

def get_anki_android_ndk_downloads_directory():
    ndk_download_directory_path = os.path.join(get_anki_android_directory(), 'ndk-downloads')
    util.File.mkdir_p(ndk_download_directory_path)
    return ndk_download_directory_path

def find_ndk_root_for_ndk_version(version_arg):
    ndk_version_arg_to_version = {
        'r13b': '13.1.3345770',
        '13.1.3345770': '13.1.3345770',
        'r10e': 'r10e',
    }
    version = ndk_version_arg_to_version[version_arg]
    ndk_root_env_vars = [
        'ANDROID_NDK_ROOT',
        'ANDROID_NDK_HOME',
        'ANDROID_NDK',
        'NDK_ROOT'
    ]
    for env_var in ndk_root_env_vars:
        env_var_value = os.environ.get(env_var)
        if env_var_value:
            ndk_ver = get_ndk_version_from_ndk_dir(env_var_value)
            if ndk_ver == version:
                return env_var_value
    local_properties = get_android_sdk_ndk_dir_from_local_properties()
    ndk_dir = local_properties.get('ndk.dir')
    if ndk_dir:
        ndk_ver = get_ndk_version_from_ndk_dir(ndk_dir)
        if ndk_ver == version:
            return ndk_dir

    # Check to see if it is installed via homebrew
    brew_path = os.path.join(os.sep, 'usr', 'local', 'opt', 'android-ndk')
    ndk_ver = get_ndk_version_from_ndk_dir(brew_path)
    if ndk_ver == version:
        return brew_path

    # Check to see if it is installed via Android Studio
    studio_ndk_path = os.path.join(os.path.expanduser("~"), "Library", "Android", "sdk", "ndk-bundle")
    ndk_ver = get_ndk_version_from_ndk_dir(studio_ndk_path)
    if ndk_ver == version:
        return studio_ndk_path

    ndk_repo_path = os.environ.get('ANDROID_NDK_REPOSITORY')
    if not ndk_repo_path:
        ndk_repo_path = get_anki_android_ndk_repository_directory()
        os.environ['ANDROID_NDK_REPOSITORY'] = ndk_repo_path
    if ndk_repo_path and os.path.isdir(ndk_repo_path):
        for d in os.listdir(ndk_repo_path):
            full_path_for_d = os.path.join(ndk_repo_path, d)
            if os.path.isdir(full_path_for_d):
                ndk_ver = get_ndk_version_from_ndk_dir(full_path_for_d)
                if ndk_ver == version:
                    return full_path_for_d
    return None

def sha1sum(filename):
    sha1 = hashlib.sha1()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            sha1.update(chunk)
    return sha1.hexdigest()

def dlProgress(bytesWritten, totalSize):
    percent = int(bytesWritten*100/totalSize)
    sys.stdout.write("\r" + "  progress = %d%%" % percent)
    sys.stdout.flush()

def install_ndk(revision):
    ndk_revision_to_version = {
        '13.1.3345770': 'r13b',
        'r10e': 'r10e',
        'r13b': 'r13b'
    }

    ndk_info_darwin = {
        'r13b': {'size': 665967997, 'sha1': '71fe653a7bf5db08c3af154735b6ccbc12f0add5'},
        'r10e': {'size': 1083342126, 'sha1': '6be8598e4ed3d9dd42998c8cb666f0ee502b1294'},
    }

    ndk_info_linux = {
        'r13b': {'size':687311866, 'sha1': '0600157c4ddf50ec15b8a037cfc474143f718fd0'},
        'r10e': {'size':1110915721, 'sha1': 'f692681b007071103277f6edc6f91cb5c5494a32'},
    }

    ndk_platform_info = {
        'darwin': ndk_info_darwin,
        'linux': ndk_info_linux,
    }
    platform_name = platform.system().lower()

    ndk_info = ndk_platform_info.get(platform_name)

    ndk_repo_path = os.environ.get('ANDROID_NDK_REPOSITORY')
    if not ndk_repo_path:
        ndk_repo_path = get_anki_android_ndk_repository_directory()
    util.File.mkdir_p(ndk_repo_path)

    ndk_downloads_path = get_anki_android_ndk_downloads_directory()

    version = ndk_revision_to_version[revision]
    url_prefix = "https://dl.google.com/android/repository/"
    ndk_base_name = "android-ndk-{0}".format(version)
    android_ndk_url = "{0}{1}-{2}-x86_64.zip".format(url_prefix, ndk_base_name, platform_name)

    ndk_tmp_extract_path = os.path.join(ndk_downloads_path, ndk_base_name)
    ndk_final_extract_path = os.path.join(ndk_repo_path, ndk_base_name)
    if os.path.isdir(ndk_tmp_extract_path):
        shutil.rmtree(ndk_tmp_extract_path)
    if os.path.isdir(ndk_final_extract_path):
        shutil.rmtree(ndk_final_extract_path)
    ndk_final_path = ndk_tmp_extract_path + ".zip"
    ndk_download_path = ndk_final_path + ".tmp"
    if os.path.isfile(ndk_final_path):
        os.remove(ndk_final_path)
    if os.path.isfile(ndk_download_path):
        os.remove(ndk_download_path)
    ndk_download_size = ndk_info.get(version).get('size')
    ndk_download_hash = ndk_info.get(version).get('sha1')
    handle = urllib.urlopen(android_ndk_url)
    code = handle.getcode()
    if code >= 200 and code < 300:
        ndk_download_file = open(ndk_download_path, 'w')
        block_size = 1024 * 1024
        sys.stdout.write("\nDownloading Android NDK {0}:\n  url = {1}\n  dst = {2}\n"
                         .format(version, android_ndk_url, ndk_final_path))
        for chunk in iter(lambda: handle.read(block_size), b''):
            ndk_download_file.write(chunk)
            dlProgress(ndk_download_file.tell(), ndk_download_size)

        ndk_download_file.close()
        sys.stdout.write("\n")
        sys.stdout.write("Verifying that SHA1 hash matches {0}\n".format(ndk_download_hash))
        if sha1sum(ndk_download_path) == ndk_download_hash:
            os.rename(ndk_download_path, ndk_final_path)
            zip_ref = zipfile.ZipFile(ndk_final_path, 'r')
            sys.stdout.write("Extracting NDK from {0}.  This could take several minutes.\n"
                             .format(ndk_final_path))
            zip_ref.extractall(ndk_downloads_path)
            zip_info_list = zip_ref.infolist()
            # Fix permissions so we can execute the NDK tools
            for zip_info in zip_info_list:
                extracted_path = os.path.join(ndk_downloads_path, zip_info.filename)
                perms = zip_info.external_attr >> 16L
                os.chmod(extracted_path, perms)
            zip_ref.close()
            os.rename(ndk_tmp_extract_path, ndk_final_extract_path)
        else:
            sys.stderr.write("Failed to validate {0} downloaded from {1}\n"
                             .format(ndk_download_path, android_ndk_url))
            os.remove(ndk_download_path)
            return -1
    else:
        sys.stderr.write("Failed to download Android NDK {0} from {1} : {2}\n"
                         .format(version, android_ndk_url, code))
        return -1

def get_required_ndk_version():
    buck_config_path = os.path.join(get_toplevel_directory(), '.buckconfig')
    required_ndk_ver = get_ndk_version_from_buckconfig(buck_config_path)
    return required_ndk_ver

def find_ndk_root_dir(required_ndk_ver):
    ndk_root_dir = find_ndk_root_for_ndk_version(required_ndk_ver)
    return ndk_root_dir

def find_android_home():
    env_vars = [
        'ANDROID_HOME',
        'ANDROID_ROOT'
    ]
    for env_var in env_vars:
        path_from_env_var = os.environ.get(env_var)
        if path_from_env_var:
            path_to_android_tool = os.path.join(path_from_env_var, 'tools', 'android')
            if os.path.isfile(path_to_android_tool):
                return path_from_env_var
    local_properties = get_android_sdk_ndk_dir_from_local_properties()
    sdk_dir = local_properties.get('sdk.dir')
    if sdk_dir:
        path_to_android_tool = os.path.join(sdk_dir, 'tools', 'android')
        if os.path.isfile(path_to_android_tool):
            return sdk_dir

    brew_path = os.path.join(os.sep, 'usr', 'local', 'opt', 'android-sdk')
    path_to_android_tool = os.path.join(brew_path, 'tools', 'android')
    if os.path.isfile(path_to_android_tool):
        return brew_path

    studio_path = os.path.join(os.path.expanduser("~"), 'Library', 'Android', 'sdk')
    path_to_android_tool = os.path.join(studio_path, 'tools', 'android')
    if os.path.isfile(path_to_android_tool):
        return studio_path

    raise RuntimeError("Could not find android home directory")
    return None

def find_or_install_ndk(required_ndk_ver):
    ndk_root_dir = find_ndk_root_dir(required_ndk_ver)
    if not ndk_root_dir:
        install_ndk(required_ndk_ver)
        ndk_root_dir = find_ndk_root_dir(required_ndk_ver)
        if not ndk_root_dir:
            raise RuntimeError("Could not find Android NDK Root directory for version {0}"
                               .format(required_ndk_ver))
    return ndk_root_dir

def setup_android_ndk_and_sdk():
    required_ndk_ver = get_required_ndk_version()
    ndk_root_dir = find_or_install_ndk(required_ndk_ver)

    os.environ['ANDROID_NDK_ROOT'] = ndk_root_dir
    print("ANDROID_NDK_ROOT: %s" % ndk_root_dir)
    android_home_dir = find_android_home()
    os.environ['ANDROID_HOME'] = android_home_dir
    print("ANDROID_HOME: %s" % android_home_dir)
    write_local_properties()


def parseArgs(scriptArgs):
    version = '1.0'
    parser = argparse.ArgumentParser(description='finds or installs android ndk', version=version)
    parser.add_argument('--install-ndk', action='store', dest='required_ndk_version',
                        default=get_required_ndk_version())
    (options, args) = parser.parse_known_args(scriptArgs)
    return options


def main(argv):
    options = parseArgs(argv)
    ndk_path = find_or_install_ndk(options.required_ndk_version)
    if not ndk_path:
        return 1
    print("%s" % ndk_path)
    return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)

