#!/usr/bin/env python

from __future__ import print_function

import argparse
import hashlib
import io
import os
import platform
import re
import shutil
import string
import subprocess
import sys
import urllib
import tarfile

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

def get_cmake_version_from_command(cmake_exe):
    version = None
    if cmake_exe and os.path.exists(cmake_exe):
        output = subprocess.check_output([cmake_exe, '--version'])
        if not output:
            return None
        m = re.match('^cmake version (\d+\.\d+\.\d+)', output)
        if not m:
            return None
        version = m.group(1)

    return version

def get_anki_cmake_directory():
    d = os.path.join(os.path.expanduser("~"), ".anki", "cmake")
    util.File.mkdir_p(d)
    return d

def get_anki_cmake_downloads_directory():
    d = os.path.join(get_anki_cmake_directory(), 'downloads')
    util.File.mkdir_p(d)
    return d

def get_anki_cmake_dist_directory():
    d = os.path.join(get_anki_cmake_directory(), 'dist')
    util.File.mkdir_p(d)
    return d

def find_anki_cmake_exe(version):
    d = get_anki_cmake_dist_directory()
    d_ver = os.path.join(d, version)

    for root, dirs, files in os.walk(d_ver):
        if os.path.basename(root) == 'bin':
            if 'cmake' in files:
                return os.path.join(d_ver, root, 'cmake') 

    return None

def sha256sum(filename):
    sha1 = hashlib.sha256()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            sha1.update(chunk)
    return sha1.hexdigest()

def dlProgress(bytesWritten, totalSize):
    percent = int(bytesWritten*100/totalSize)
    sys.stdout.write("\r" + "  progress = %d%%" % percent)
    sys.stdout.flush()

def safe_rmdir(path):
    if os.path.isdir(path):
        shutil.rmtree(path)

def safe_rmfile(path):
    if os.path.isfile(path):
        os.remove(path)

def download_and_install(archive_url,
                         hash_url,
                         downloads_path,
                         dist_path,
                         base_name,
                         version,
                         title):
    tmp_extract_path = os.path.join(downloads_path, base_name)
    final_extract_path = os.path.join(dist_path, version)
    safe_rmdir(tmp_extract_path)
    safe_rmdir(final_extract_path)
    final_path = tmp_extract_path + ".tar.gz"
    download_path = final_path + ".tmp"
    safe_rmfile(final_path)
    safe_rmfile(download_path)

    handle = urllib.urlopen(hash_url)
    code = handle.getcode()

    archive_file = os.path.basename(archive_url)
    download_hash = None

    if code >= 200 and code < 300:
        digests_path = os.path.join(downloads_path, os.path.basename(hash_url))
        download_file = open(digests_path, 'w')
        block_size = 1024 * 1024
        sys.stdout.write("\nDownloading {0} {1}:\n  url = {2}\n  dst = {3}\n"
                         .format(title, version, hash_url, digests_path))
        for chunk in iter(lambda: handle.read(block_size), b''):
            download_file.write(chunk)

        download_file.close()
        with open(digests_path, 'r') as f:
            for line in f:
                if line.strip().endswith(archive_file):
                    download_hash = line[0:64]

    handle = urllib.urlopen(archive_url)
    code = handle.getcode()
    if code >= 200 and code < 300:
        download_file = open(download_path, 'w')
        block_size = 1024 * 1024
        sys.stdout.write("\nDownloading {0} {1}:\n  url = {2}\n  dst = {3}\n"
                         .format(title, version, archive_url, final_path))
        for chunk in iter(lambda: handle.read(block_size), b''):
            download_file.write(chunk)

        download_file.close()
        sys.stdout.write("\n")
        sys.stdout.write("Verifying that SHA1 hash matches {0}\n".format(download_hash))
        sha256 = sha256sum(download_path)
        if sha256 == download_hash:
            os.rename(download_path, final_path)
            tar_ref = tarfile.open(final_path, 'r')
            sys.stdout.write("Extracting {0} from {1}.  This could take several minutes.\n"
                             .format(title, final_path))
            tar_ref.extractall(downloads_path)
            os.rename(tmp_extract_path, final_extract_path)
        else:
            sys.stderr.write("Failed to validate {0} downloaded from {1}\n"
                             .format(download_path, archive_url))
    else:
        sys.stderr.write("Failed to download {0} {1} from {2} : {3}\n"
                         .format(title, version, archive_url, code))
    safe_rmfile(final_path)
    safe_rmfile(download_path)

def install_cmake(version):
    platform_map = {
        'darwin': 'Darwin-x86_64',
        'linux': 'Linux-x86_64'
    }

    platform_name = platform.system().lower()

    (major, minor, patch) = version.split('.')
    cmake_short_ver = "{}.{}".format(major, minor)
    cmake_url_prefix = "https://cmake.org/files/v{}".format(cmake_short_ver)

    cmake_platform = platform_map[platform_name]
    cmake_basename = "cmake-{}-{}".format(version, cmake_platform)
    cmake_archive_url = "{}/{}.tar.gz".format(cmake_url_prefix, cmake_basename)
    cmake_hash_url = "{}/cmake-{}-SHA-256.txt".format(cmake_url_prefix, version)

    cmake_downloads_path = get_anki_cmake_downloads_directory()
    cmake_dist_path = get_anki_cmake_dist_directory()

    download_and_install(cmake_archive_url,
                         cmake_hash_url,
                         cmake_downloads_path,
                         cmake_dist_path,
                         cmake_basename,
                         version,
                         "CMake")

def find_or_install_cmake(required_ver, cmake_exe=None):
    if not cmake_exe:
        try:
            cmake_exe = subprocess.check_output(['which', 'cmake'])
        except subprocess.CalledProcessError as e:
            pass
  
    needs_install = True
    if cmake_exe:
        version = get_cmake_version_from_command(cmake_exe)
        if version == required_ver:
            needs_install = False

    if needs_install:
        cmake_exe = find_anki_cmake_exe(required_ver)
        version = get_cmake_version_from_command(cmake_exe)
        if version == required_ver:
            needs_install = False
        
    if needs_install:
        install_cmake(required_ver)
        return find_anki_cmake_exe(required_ver)
    else:
        return cmake_exe

def setup_cmake(required_ver):
    cmake_exe = find_or_install_cmake(required_ver)
    if not cmake_exe:
        raise RuntimeError("Could not find cmake version {0}"
                            .format(required_ver))
    return cmake_exe

def parseArgs(scriptArgs):
    version = '1.0'
    parser = argparse.ArgumentParser(description='finds or installs android ndk/sdk', version=version)
    parser.add_argument('--install-cmake',
                        action='store',
                        dest='required_version',
                        nargs='?',
                        default="3.8.1")
    (options, args) = parser.parse_known_args(scriptArgs)
    return options


def main(argv):
    options = parseArgs(argv)
    if options.required_version:
        path = find_or_install_cmake(options.required_version)
        if not path:
            return 1
        print("%s" % path)
        return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)

