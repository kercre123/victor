#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
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
    return os.path.join(get_dep_dist_directory(project, name), version)

def install_dep(project, name, version, url_prefix):
    base_name = "{0}-{1}".format(name, version)
    archive_url = "{0}{1}.tar.bz2".format(url_prefix, base_name)
    hash_url = "{0}{1}-SHA-256.txt".format(url_prefix, base_name)
    downloads_path = get_dep_downloads_directory(project, name)
    dist_path = get_dep_dist_directory(project, name)
    title = "deps/{0}/{1}".format(project, name)

    toolget.download_and_install(archive_url,
                                 hash_url,
                                 downloads_path,
                                 dist_path,
                                 base_name,
                                 name,
                                 version,
                                 title)

def get_version_from_dir(dir):
    version_file = os.path.join(dir, "VERSION")
    if os.path.isfile(version_file):
        with open(version_file, 'r') as f:
            return f.readline().strip()
    return None

def find_dep_root_dir(project, name, required_ver, envname=None):
    if envname:
        env_value = os.environ.get(envname)
        if env_value:
            # check environment defined value
            version = get_version_from_dir(env_value)
            if version == required_ver:
                return env_value

    # check for a version in .anki/deps
    anki_dep_dir = get_dep_dist_directory_for_version(project, name, required_ver)
    return anki_dep_dir if os.path.isdir(anki_dep_dir) else None

def find_or_install_dep(project, name, required_ver, url_prefix, install=True, envname=None):
    dep_root_dir = find_dep_root_dir(project, name, required_ver, envname)

    if dep_root_dir:
        return dep_root_dir

    if install:
        install_dep(project, name, required_ver, url_prefix)

    return find_dep_root_dir(project, name, required_ver)

def parseArgs(scriptArgs):
    parser = argparse.ArgumentParser(description='finds or installs dependency for a project')
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
    return parser.parse_known_args(scriptArgs)[0]


def main(argv):
    options = parseArgs(argv)
    version = options.install_version or options.find_version
    if version:
        path = find_or_install_dep(options.project,
                                   options.name,
                                   version,
                                   options.url_prefix,
                                   bool(options.install_version),
                                   options.envname)
        if not path:
            return 1
        print("%s" % path)
        return 0

if __name__ == '__main__':
    ret = main(sys.argv)
    sys.exit(ret)
