#!/usr/bin/env python

import os, sys
from anki_github import post_ci_bot_comment
import argparse

def parse_args(argv=[]):

    parser = argparse.ArgumentParser(description='Post Jenkins PR Build Status to Github')

    parser.add_argument('-p', '--pull_request_number', type=int, action='store', default='0', required=True,
                        help='Pull Request to comment on')
    parser.add_argument('-c', '--comment_with_mrkdwn', type=str, action='store', default=None, required=True,
                        help='Github Markdown Flavored Comment body')
    args = parser.parse_args(argv)
    return args


def main(argv):
    args = parse_args(argv[1:])
    auth = os.environ['GITHUB_PAT']
    res = post_ci_bot_comment(auth, args.pull_request_number, args.comment_with_mrkdwn)
    print(res)

if __name__ == "__main__":
    main(sys.argv)