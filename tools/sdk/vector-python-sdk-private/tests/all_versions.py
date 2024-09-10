#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from pkg_resources import parse_version
import requests

try:
    from termcolor import colored  # pylint: disable=import-error
except:  # pylint: disable=bare-except
    def colored(text, color=None, on_color=None, attrs=None):  # pylint: disable=unused-argument
        return text

# As new versions are released to pypi, they need to be added to this list to be automatically tested.
VERSIONS = [
    "0.5.1",
    "latest",
]

# To explicitly skip old versions, list them here.
SKIP_VERSIONS = [
    "0.0.2",
]


def get_versions(name):
    url = "https://pypi.python.org/pypi/{}/json".format(name)
    print(f"Downloading {colored(name, 'cyan')} version information from {colored(url, 'cyan')}")
    return sorted(requests.get(url).json()["releases"], key=parse_version)


def verify_versions():
    verified = True
    try:
        released_versions = get_versions("anki-vector")
        for version in released_versions:
            if version not in VERSIONS and version not in SKIP_VERSIONS:
                verified = False
                print(f"{colored('WARNING', 'yellow')}: anki-vector version {version} not referenced in this script.\n"
                      "          It should be added to VERSIONS or SKIP_VERSIONS.")
        for version in VERSIONS:
            if version not in released_versions and version != "latest":
                verified = False
                print(f"{colored('WARNING', 'yellow')}: anki-vector test version {version} not found on pypi.")
    except:  # pylint: disable=bare-except
        verified = False
        print(f"{colored('WARNING', 'yellow')}: Failed to check versions from pypi. Unable to verify that all versions are valid.")
    return verified


class TestRunner:
    def __init__(self, version, verbose=False):
        self._version = version
        self._verbose = verbose

    def run_and_wait(self, command, env=None, show_output=False, cwd=None):
        if env is None:
            env = os.environ.copy()
        if cwd is None:
            # use the root of the repo as the working directory
            cwd = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        print(f"{colored('[' + self._version + ']', 'green')} > {colored(command, 'cyan')}")
        try:
            if show_output:
                process = subprocess.Popen(shlex.split(command),
                                           env=env,
                                           cwd=cwd)
            else:
                process = subprocess.Popen(shlex.split(command),
                                           env=env,
                                           cwd=cwd,
                                           stdout=subprocess.DEVNULL,
                                           stderr=subprocess.DEVNULL)
            return process.wait()
        finally:
            process.terminate()
            process.kill()
        return 1

    def setup(self):
        command = f"{sys.executable} -m virtualenv -p {sys.executable} ./tests/env --no-site-packages"
        code = self.run_and_wait(command, show_output=self._verbose)
        if code != 0:
            return code
        command = f"./tests/env/bin/python -m pip install -r requirements.txt"
        code = self.run_and_wait(command, show_output=self._verbose)
        if code != 0:
            return code

        if self._version == "latest":
            command = f"./tests/env/bin/python -m pip install ./sdk[3dviewer,docs,test]"
        else:
            command = f"./tests/env/bin/python -m pip install anki_vector[3dviewer,docs,test]=={self._version}"
        code = self.run_and_wait(command, show_output=self._verbose)
        if code != 0:
            return code
        return 0

    def run_test(self, serial=None):
        command = f"./tests/env/bin/python -m pytest{' -vv' if self._verbose else ''}"
        modified_env = os.environ.copy()
        if serial:
            modified_env["ANKI_ROBOT_SERIAL"] = serial
        return self.run_and_wait(command, env=modified_env, show_output=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-s", "--serial", help="Select the given serial number")
    parser.add_argument("-v", "--verbose", action="store_true", help="Show verbose output")
    args = parser.parse_args()

    root_dir = Path(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

    verify_versions()
    print(f"Testing versions: {colored(VERSIONS, 'cyan')}")

    failures = 0
    for version in VERSIONS:
        shutil.rmtree(f"{root_dir.resolve()}/tests/env", ignore_errors=True)
        runner = TestRunner(version, verbose=args.verbose)
        # Clear the virtual env for each run
        code = runner.setup()
        if code != 0:
            sys.exit(f"Setup failed {code}")
        code = runner.run_test(serial=args.serial)
        if code != 0:
            failures += 1
    if failures > 0:
        sys.exit(f"{failures} versions failed")


if __name__ == "__main__":
    main()
