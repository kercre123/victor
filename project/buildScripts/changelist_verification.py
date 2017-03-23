#!/usr/bin/env python -u

import subprocess
import dependencies
import sys
import re


def verify_regex(file_list, reg="\Arobot\/"):
    for f in file_list:
        prog = re.compile(reg)
        result = prog.match(f)
        if result is not None:
            return True
    return False

# Will be rewritten using the github api.
# Why? If the initial opening of the PR has multiple commits there is a changes for a false negative.
# However this is far better than the false positives that are currently happening.
def extract_file_list():
    if dependencies.is_tool("git") is False:
        sys.exit("[ERROR] Could not find git")
    git_cmd = ["git", "diff", "--name-only", "HEAD", "HEAD^"]
    p = subprocess.Popen(git_cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    status = p.poll()
    if status != 0:
        return None
    return stdout.splitlines()


def main():
    file_list = extract_file_list()
    if verify_regex(file_list) == False:
	sys.exit("PR was trigger erroneously from Teamcity.")

if __name__ == '__main__':
    main()
