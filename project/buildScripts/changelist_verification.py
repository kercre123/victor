#!/usr/bin/env python2 -u

import subprocess
import dependencies
import sys
import re
import anki_github
import os
import argparse


def parse_args(argv=[]):

    parser = argparse.ArgumentParser(description='Verify that pr changes list is not going to need a firmware build.')

    parser.add_argument('-v', '--verbose', action='store_true',
                        help='print verbose output')
    parser.add_argument('-a', '--auth_token', action='store', default=None,
                        help='Mandatory Github authentication token. Create one if you want to run this locally.')
    parser.add_argument('-p', '--pull_request_number', type=int, action='store', default='0',
                        help='Enter Pull request number you want to compare')
    parser.add_argument('-b', '--pull_request_branch', action='store', default=None,
                        help='Enter Pull request branch you want to compare. Expected format=1234/head')
    parser.add_argument('-r', '--repository_name', action='store', default='anki/cozmo-one',
                        help='Enter the repository to verify PRs from. Default=anki/cozmo-one')
    args = parser.parse_args(argv)
    return args


def verify_regex(file_list, reg="\Arobot\/"):
    if file_list is not None:
        prog = re.compile(reg)
        for f in file_list:
            result = prog.match(f)
            if result is not None:
                return True
    return False

def main(argv):
    args = parse_args(argv[1:])
    if args.auth_token is not None:
        if args.pull_request_branch is not None:
            branch_number = int(filter(str.isdigit, args.pull_request_branch))
        else:
            branch_number = args.pull_request_number
        file_list = anki_github.get_file_change_list(args.auth_token, branch_number,
                                                     args.verbose, args.repository_name)
        if verify_regex(file_list) is False:
            sys.exit(os.linesep + "Pull Request was triggered erroneously from Teamcity plugin. "
                                  "Termination of build is intentional.")
    else:
        sys.exit("ERROR: Authentication token is mandatory to connect to Github.")

if __name__ == '__main__':
    main(sys.argv)
