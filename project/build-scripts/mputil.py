#!/usr/bin/env python2

""" Utility to install and manage Apple provisioning profiles (*.mobileprovision) files.
"""

from __future__ import print_function

import argparse
import os
import shutil
import sys

def profile_install_path():
    return os.path.join(os.environ['HOME'], 'Library', 'MobileDevice', 'Provisioning Profiles')

def parse_args(argv=[], print_usage=False):

    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s\n' % message)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(description='Install/Remove/Inspect Apple Mobile Provisioning Profiles')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='print verbose output')
    parser.add_argument('-n', '--dry-run', action='store_true',
                        help='Do not actually do anything, just show what would be done.')
    parser.add_argument('--profile-path', action='store', default=profile_install_path(),
                        help='path to system installed profiles')

    subparsers = parser.add_subparsers(dest='command', help='sub-command help')

    list_cmd = subparsers.add_parser('list', help='list installed profiles')

    dump_cmd = subparsers.add_parser('dump', help='dump text of PropertyList')
    dump_cmd.add_argument('profiles', nargs='+')

    install_cmd = subparsers.add_parser('install', help='install profiles on system')
    install_cmd.add_argument('profiles', nargs='+')

    remove_cmd = subparsers.add_parser('remove', help='remove installed profiles from system')
    remove_cmd.add_argument('--all', action='store_true', help='remove all installed profiles')
    remove_cmd.add_argument('--bundle-id', action='store_true', help='treat arguments as bundle identifiers')
    remove_cmd.add_argument('profiles', nargs='*')

    if print_usage:
        parser.print_help()
        sys.exit(2)

    args = parser.parse_args(argv)
    return args

def _get_profiles(args):
    install_path = args.profile_path
    files = [f for f in os.listdir(install_path) if os.path.isfile(os.path.join(install_path,f))]
    return files

def list_profiles(args):
    print("\n".join(_get_profiles(args)))
    return

def dump_profiles(args):
    print(args)
    print("TODO: IMPLEMENT ME IF YOU NEED THIS.")

def install_profiles(args):
    for profile in args.profiles:
        if not os.path.exists(args.profile_path):
            if args.dry_run:
                print("mkdir %s" % args.profile_path)
            else:
                os.makedirs(args.profile_path)

        if args.dry_run:
            print("cp %s %s" % (profile, args.profile_path))
        else:
            shutil.copy(profile, args.profile_path)

def remove_profiles(args):
    profiles = args.profiles
    if args.all:
        profiles = _get_profiles(args)
    elif len(profiles) == 0:
        print("error: no arguments specified\n")
        parse_args(print_usage=True)

    install_path = args.profile_path
    for profile in profiles:
        basename = os.path.basename(profile)
        profile_path = os.path.join(install_path, basename)
        if args.dry_run:
            print("rm %s" % profile_path)
        elif os.path.isfile(profile_path):
            os.remove(profile_path)

def run(args):
    method_map = {
        'list': list_profiles,
        'dump': dump_profiles,
        'install': install_profiles,
        'remove': remove_profiles }

    return method_map[args.command](args)

if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    run(args)
