#!/usr/bin/env python

import argparse

import os
import re
import subprocess
import sys

def parse_args(args):
    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s\n' % message)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(description="List commits in master not in release/candidate")

    parser.add_argument('-x', '--exclude-file', action='store',
                        help="Path to file listing commits to exclude from list. One commit per line starting with sha1")
    parser.add_argument('--exclude-url', action='store',
                        help="URL to list of commits to exclude from list. One commit per line starting with sha1")

    parser.add_argument('master_branch',
                        action='store',
                        default='master',
                        help="git branch containing source commits")


    parser.add_argument('release_branch',
                        action='store',
                        default='release/candidate',
                        help="git branch where commits will be cherry-picked")

    options = parser.parse_args(args)
    return options

def git_sync_branch(branch):
    cmd = [
        'git', 'checkout', branch
    ]
    subprocess.call(cmd)
    cmd = [
        'git', 'pull'
    ]
    subprocess.call(cmd)
    cmd = [
        'git', 'submodule', 'update', '--init', '--recursive'
    ]
    subprocess.call(cmd)


def git_sync_branches(master_branch, release_branch):
    cmd = [
        'git', 'branch',
        '--no-color'
    ]
    list_of_branches = subprocess.check_output(cmd)
    current_branch = ''
    for line in list_of_branches.split('\n'):
        if (line.startswith('*')):
            current_branch = line.split()[1]
    print ('current_branch ' + current_branch + ' --')
    git_sync_branch(master_branch)
    git_sync_branch(release_branch)
    if release_branch != current_branch:
        cmd = [
            'git', 'checkout', current_branch
        ]
        subprocess.call(cmd)


def git_compare_branches(master_branch, release_branch):
    ref_spec = "%s...%s" % (options.master_branch, options.release_branch)
    cmd = [
        'git', 'log',
        '--cherry-pick',
        '--oneline',
        '--no-merges',
        '--left-only',
        ref_spec
    ]

    output = subprocess.check_output(cmd)
    return output.strip().split("\n")

def filter_commits(commits, exclude_list=[]):

    excluded_commits = [ c[0:7] for c in exclude_list ]

    remaining_commits = []
    for commit_info in commits:
        sha1 = commit_info[0:7]
        msg = commit_info[8:]
        if sha1 not in excluded_commits:
            remaining_commits.append(commit_info)

    return remaining_commits

def fetch_excludes(excludes_url):
    import urllib2
    response = urllib2.urlopen(excludes_url)
    html = response.read()
    return html

def parse_excludes(excludes_content):
    exclude_list = []

    if not excludes_content:
        return exclude_list

    for raw_line in excludes_content.split("\n"):
        line = raw_line.strip() 
        if re.search('^\#', line) or len(line) == 0:
            continue
        exclude_list.append(line)

    return exclude_list


def main(options):

    git_sync_branches(options.master_branch, options.release_branch)
    commits = git_compare_branches(options.master_branch, options.release_branch)
    #print(commits)

    exclude_list = []

    if options.exclude_file and os.path.exists(options.exclude_file):
        with open(options.exclude_file, 'r') as f:
            contents = f.read()
            xlist = parse_excludes(contents)
            exclude_list.extend(xlist)

    if options.exclude_url:
        contents = fetch_excludes(options.exclude_url)
        if (contents):
            xlist = parse_excludes(contents)
            exclude_list.extend(xlist)

    remaining = filter_commits(commits, exclude_list)
    #remaining.reverse()
    print('the list:\n\n')
    for c in remaining:
        print(c)

    return 0

if __name__ == '__main__':
    options = parse_args(sys.argv[1:])
    #print(options)

    r = main(options)
    sys.exit(r)
