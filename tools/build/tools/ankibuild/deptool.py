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

def install_dep(project, name, version, s3_params, sha256):
    base_name = "{0}-{1}".format(name, version)
    s3_params['key'] = s3_params['key-prefix'] + sha256
    downloads_path = get_dep_downloads_directory(project, name)
    dist_path = get_dep_dist_directory(project, name)
    extracted_dir_name = name;
    title = "deps/{0}/{1}".format(project, name)

    toolget.download_and_install_from_s3(s3_params,
                                         sha256,
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

def find_or_install_dep(project, name, required_ver, s3_params, required_sha256=None, install=True, envname=None):
    dep_root_dir = find_dep_root_dir(project, name, required_ver, required_sha256, envname)

    if dep_root_dir:
        return dep_root_dir

    if install:
        install_dep(project, name, required_ver, s3_params, required_sha256)
    dep_root_dir = find_dep_root_dir(project, name, required_ver, required_sha256)

    return dep_root_dir
