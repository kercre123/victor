#!/usr/bin/env python3

"""This script pushes the private SDK subtree to the public SDK repo."""


import argparse
from pathlib import Path
import shlex
import subprocess


def push_subtree(root_dir, prefix, repo, branch):
    git_command = f"git subtree push --prefix {prefix} {repo} {branch}"
    print(f"> About to deploy sdk to public repo. Git command:\n$ {git_command}")
    response = input("\n    Are you sure? [y/n]:")
    if response != "y":
        print("Aborting")
        return
    print("> Updating subtree...", end="")
    protoc_command = shlex.split(git_command)
    protoc_process = subprocess.Popen(protoc_command, cwd=root_dir)
    protoc_process.wait()
    print(" DONE")


def main():
    root_dir = Path(__file__).resolve().parent.parent

    parser = argparse.ArgumentParser(
        description=(
            "Update the proto files from the a given commit of the victor-proto repo."
            "To make changes to the proto files, you need to modify them in the victor repo and push to the victor-proto subtree."
        ))
    parser.add_argument("-b", "--branch", help="Name of the branch to push on the public repo.", required=True)
    parser.add_argument("-p", "--prefix", help="Custom subtree prefix.", default="sdk")
    parser.add_argument("-r", "--repo", help="Do not remove the app-only comments out of proto files after copying.", default="git@github.com:anki/vector-python-sdk.git")
    args = parser.parse_args()

    push_subtree(root_dir, args.prefix, args.repo, args.branch)


if __name__ == "__main__":
    main()
