#!/usr/bin/env python2

""" Utility to generate version numbers.
"""

from __future__ import print_function

import argparse
import os
import pprint
import re
import shutil
import subprocess
import sys
import time

def parse_args(argv=[], print_usage=False):

    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s\n' % message)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(description='Generate version number of the form <major>.<minor>.<patch>.<build_number>.<YYMMDD>.<HHMM>.<build_type>.<git_hash>')
    parser.add_argument('-v', '--verbose', action='store_true', 
                        help='print verbose output')

    parser.add_argument('semantic_version', action='store', default='0.0.1',
                        help='Input semantic version <major>.<minor>.<patch>')
    parser.add_argument('--build-version', action='store', default='0',
                        help='Input build version number')
    parser.add_argument('--build-type', action='store', default='d',
                        choices=['dev', 'beta', 'production'],
                        help='Build type')


    if print_usage:
        parser.print_help()
        sys.exit(2)

    args = parser.parse_args(argv)
    return args

def git_version():
    """Return the short git hash for the HEAD of the current repo."""
    v = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'])
    return v.strip()

def run(args):
    if args.verbose:
        pprint.pprint(vars(args))

    exit_code = 0

    m = re.match('([a-zA-Z0-9]+)\.(\d+)\.?(\d*)', args.semantic_version)

    major = m.group(1)
    minor = m.group(2)
    patch = m.group(3)

    if not patch:
        patch = '0'
    
    git_hash = git_version()

    # format: YYMMDD.HHMM
    date = time.strftime("%y%m%d.%H%M")

    ver = [ major, minor, patch, args.build_version, date, args.build_type[0], git_hash ] 

    if args.verbose:
        pprint.pprint(ver)

    print('.'.join(ver))

    return exit_code

if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    result = run(args)
    exit(result)
