#!/usr/bin/env python2

from __future__ import print_function

import argparse
import os
import platform
import re
import string
import subprocess
import sys

import toolget
import util

def get_dep_directory(project, name):
    d = os.path.join(os.path.expanduser("~"), ".anki", "deps", project, name)
    util.File.mkdir_p(d)
    return d

def get_dep_downloads_directory(project, name):
    d = os.path.join(get_dep_directory(project, name), 'downloads')
    util.File.mkdir_p(d)
    return d

def get_dep_dist_directory(project, name):
    d = os.path.join(get_dep_directory(project, name), 'dist')
    util.File.mkdir_p(d)
    return d

def get_dep_dist_directory_for_version(project, name, version):
    d = os.path.join(get_dep_dist_directory(project, name), version)
    return d

def install_dep(project, name, version, url_prefix, sha256=None):
    base_name = "{0}-{1}".format(name, version)
    if sha256:
        archive_url = "{0}sha256/{1}".format(url_prefix, sha256)
        download_hash = sha256
    else:
        archive_url = "{0}{1}.tar.bz2".format(url_prefix, base_name)
        download_hash = "{0}{1}-SHA-256.txt".format(url_prefix, base_name)
    downloads_path = get_dep_downloads_directory(project, name)
    dist_path = get_dep_dist_directory(project, name)
    extracted_dir_name = name;
    title = "deps/{0}/{1}".format(project, name)

    toolget.download_and_install(archive_url,
                                 download_hash,
                                 downloads_path,
                                 dist_path,
                                 base_name,
                                 extracted_dir_name,
                                 version,
                                 title)

    final_dist_path = os.path.join(dist_path, version)
    if os.path.isdir(final_dist_path) and sha256:
        sha256_file = os.path.join(final_dist_path, "SHA256")
        with open(sha256_file, 'w') as f:
            f.write(sha256 + "\n")

def get_version_from_dir(dir):
    if not dir:
        return None
    version_file = os.path.join(dir, "VERSION")
    version = None
    if os.path.isfile(version_file):
        with open(version_file, 'r') as f:
            version = f.readline().strip()
    return version

def get_sha256_from_dir(dir):
    if not dir:
        return None
    sha256_file = os.path.join(dir, "SHA256")
    sha256 = None
    if os.path.isfile(sha256_file):
        with open(sha256_file, 'r') as f:
            sha256 = f.readline().strip()
    return sha256


def is_valid_dep_dir_for_version_and_sha256(dep_dir, required_version, required_sha256):
    version = get_version_from_dir(dep_dir)
    sha256 = get_sha256_from_dir(dep_dir)
    return version == required_version and sha256 == required_sha256

def find_dep_root_dir(project, name, required_ver, required_sha256=None, envname=None):
    dep_dir = os.environ.get(envname)
    if is_valid_dep_dir_for_version_and_sha256(dep_dir, required_ver, required_sha256):
        return dep_dir

    # check for a version in .anki/deps
    dep_dir = get_dep_dist_directory_for_version(project, name, required_ver)
    if is_valid_dep_dir_for_version_and_sha256(dep_dir, required_ver, required_sha256):
        return dep_dir

    return None

def find_or_install_dep(project, name, required_ver, url_prefix, required_sha256=None, install=True, envname=None):
    dep_root_dir = find_dep_root_dir(project, name, required_ver, required_sha256, envname)

    if dep_root_dir:
        return dep_root_dir

    if install:
        install_dep(project, name, required_ver, url_prefix, required_sha256)
    dep_root_dir = find_dep_root_dir(project, name, required_ver, required_sha256)

    return dep_root_dir

def parseArgs(scriptArgs):
    version = '1.0'
    parser = argparse.ArgumentParser(description='finds or installs dependency for a project', version=version)
    parser.add_argument('--project',
                        action='store',
                        dest='project',
                        nargs='?',
                        required=True)
    parser.add_argument('--name',
                        action='store',
                        dest='name',
                        nargs='?',
                        required=True)
    parser.add_argument('--url-prefix',
                        action='store',
                        dest='url_prefix',
                        nargs='?')
    parser.add_argument('--sha256',
                        action='store',
                        dest='sha256',
                        nargs='?')
    parser.add_argument('--envname',
                        action='store',
                        dest='envname',
                        default=None,
                        nargs='?')
    parser.add_argument('--install',
                        action='store',
                        dest='install_version',
                        nargs='?')
    parser.add_argument('--find',
                        action='store',
                        dest='find_version',
                        nargs='?')
    (options, args) = parser.parse_known_args(scriptArgs)
    return options


def main(argv):
    options = parseArgs(argv)
    version = options.install_version or options.find_version
    if version:
        path = find_or_install_dep(options.project,
                                   options.name,
                                   version,
                                   options.url_prefix,
                                   options.sha256,
                                   bool(options.install_version),
                                   options.envname)
        if not path:
            return 1
        print("%s" % path)
        return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)

