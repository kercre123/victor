#!/usr/bin/env python2

import sys
import anki_github
import argparse
import subprocess


def parse_args(argv=[]):
    parser = argparse.ArgumentParser(description='')

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


# eventually all our git commands should be placed in their own unit.
def git_checkout_pr(branch_number, fetch=True, depth=3, location="origin", force=True, prune=True):
    merge_branch = str(branch_number) + "/merge"
    if fetch:
        git_init = ['git', 'fetch', '--depth={0}'.format(depth), '--progress']
        if force: git_init.append('--force')
        if prune: git_init.append('--prune')
        git_init.append(location)
        git_init.append("+refs/pull/{0}:{0}".format(merge_branch))

        p = subprocess.Popen(git_init, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
        status = p.poll()

        if status != 0:
            sys.exit(stderr)

    git_cmd = ['git', 'checkout', '--recurse-submodules', '--progress']
    if force: git_cmd.append('--force')
    git_cmd.append(merge_branch)

    p = subprocess.Popen(git_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    status = p.poll()

    if status != 0:
        sys.exit(stderr)


def git_current_sha():
    # git log -1 -format=format:%H
    git_sha = ['git', 'log', '-1', '--format=format:%H']

    p = subprocess.Popen(git_sha, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    status = p.poll()

    if status != 0:
        sys.exit(stderr)

    return stdout


def main(argv):
    args = parse_args(argv[1:])
    if args.auth_token is not None:
        if args.pull_request_branch is not None:
            try:
                branch_number = int(
                    filter(str.isdigit, args.pull_request_branch))  # warning filter returns an iterator in p3
            except ValueError as e:
                print("WARNING: '{0}' has no PR number in it. Assuming building base branch.".format(e))
                return True
        else:
            branch_number = args.pull_request_number
        pull_request_status = anki_github.pull_request_merge_info(args.auth_token, branch_number, 6,
                                                                  args.verbose, args.repository_name)

        if pull_request_status["mergeable"]:
            git_checkout_pr(branch_number)
            local_head = git_current_sha()

            if pull_request_status["merge_commit_sha"] == local_head:
                print("---- CHECKOUT OF MERGE BRANCH WAS SUCCESSFUL ----")
            else:
                sys.exit("Github says branch should be on {0} but teamcity is on {1}".format(
                    pull_request_status["merge_commit_sha"], local_head))

            return True

        elif pull_request_status["mergeable"] is None:
            # https://developer.github.com/v3/pulls/
            # mergeable is true, false, or null.
            sys.exit("ERROR: You have not met Github's requirements.\nNote:All PR's require at least one reviewer.")
        else:
            sys.exit("ERROR: Github reports there is a merge conflict. Please check back with github.")
    else:
        sys.exit("ERROR: Authentication token is mandatory to connect to Github.")


# function git_checkout {
#  git fetch --depth=3 --prune --force --progress origin +refs/pull/$1:$1
#  git checkout -f $1
#  git submodule update --init --recursive
#  git submodule update --recursive
# }

# MERGE_BRANCH=`echo $BRANCH_NAME | sed 's/\([0-9]*\)\/\(head\)/\1\/merge/'`
#  git_checkout $MERGE_BRANCH
# compare


if __name__ == '__main__':
    print(sys.argv)
    main(sys.argv)
