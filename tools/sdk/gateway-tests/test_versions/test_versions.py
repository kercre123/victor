#!/usr/bin/env python3

import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from halo import Halo

def clone_tag(path, repo, tag, **kwargs):
    path = str(Path(os.path.dirname(os.path.realpath(__file__))) / path)
    spinner = Halo(text="Cloning {} at tag {}".format(repo, tag), spinner="dots")
    spinner.start()
    shutil.rmtree(path, ignore_errors=True)
    process = subprocess.Popen(["git", "clone", repo, path], stdout=subprocess.PIPE, stderr=subprocess.STDOUT) #TODO: add tag
    code = process.wait()
    if code != 0:
        spinner.fail()
        print(process.stdout.read())
        sys.exit(code)
    spinner.succeed()

def run_test(path, command, args, **kwargs):
    fullpath = str(Path(os.path.dirname(os.path.realpath(__file__))) / path)
    print("[ Start Test: {}]".format(path))
    subprocess.call(shlex.split(command.format(**vars(args))), cwd=fullpath)
    print("[ Finish Test: {}]".format(path))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", dest="virtualenv", help="virtualenv directory root")
    args = parser.parse_args()
    path = Path(os.path.dirname(os.path.realpath(__file__)))
    with open(str(path / "versions.json")) as f:
        data = json.load(f)
        for test in data:
            clone_tag(**data[test])
        for test in data:
            run_test(**data[test], args=args)

if __name__ == "__main__":
    main()
