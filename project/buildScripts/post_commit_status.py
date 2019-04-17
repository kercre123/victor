#!/usr/bin/env python

import os, sys
from anki_github import post_pending_commit_status
import argparse

def parse_args(argv=[]):

    parser = argparse.ArgumentParser(description='Create pending commit status on Github')

    parser.add_argument('-p', '--pull_request_number', type=int, action='store', default='0', required=True,
                        help='Pull Request to create status on')
    parser.add_argument('-u', '--build_url', type=str, action='store', default=None, required=True,
                        help='Status URL redirect')
    parser.add_argument('-b', '--build_context', type=str, action='store', default=None, required=True,
                        help='Status name (i.e. ci/jenkins/blahblah)')
    parser.add_argument('-d', '--description', type=str, action='store', default=None, required=True,
                        help='Status description')
    args = parser.parse_args(argv)
    return args


def main(argv):
    args = parse_args(argv[1:])
    auth = os.environ['GITHUB_PAT']
    res = post_pending_commit_status(auth, args.pull_request_number, args.build_url, args.build_context, args.description)
    print(res)

if __name__ == "__main__":
    main(sys.argv)