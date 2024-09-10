#!/usr/bin/env python3

import argparse
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import functools


class PipShowDetails:
    def __init__(self, name, lines):
        self._name = name
        self._license = "UNKNOWN"
        self._page = "UNKNOWN"
        self._version = "UNKNOWN"
        self._requires = ""
        self._required_by = ""
        self._functions = {
            "License": self._set_license,
            "Home-page": self._set_page,
            "Version": self._set_version,
            "Requires": self._set_requires,
            "Required-by": self._set_required_by,
        }
        for line in lines:
            l = line.decode("utf-8").strip().split(": ")
            fn = self._functions.get(l[0])
            if fn:
                fn(l[1].replace(",", ""))

    def _set_license(self, value):
        self._license = value

    def _set_page(self, value):
        self._page = value

    def _set_version(self, value):
        self._version = value

    def _set_requires(self, requires):
        self._requires = " ".join(requires.split(", "))

    def _set_required_by(self, required_by):
        self._required_by = " ".join(required_by.split(", "))

    def csv(self):
        return f"{self._name},{self._version},{self._license},{self._page},{self._requires},{self._required_by}\r\n"


def make_clean_env(working_dir):
    print("> Creating env... ", end="")
    sys.stdout.flush()
    shutil.rmtree(f"{working_dir.resolve()}/license_env", ignore_errors=True)
    command = shlex.split(
        f'virtualenv --no-site-packages -p "{sys.executable}" license_env'
    )
    process = subprocess.Popen(
        command, cwd=working_dir, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT
    )
    process.wait()
    if process.returncode:
        sys.exit(" ERROR")
    print("DONE")


def install_deps(working_dir):
    print("> Installing deps... ", end="")
    sys.stdout.flush()
    pip = (
        f"{working_dir.resolve()}/license_env/bin/pip install --no-warn-script-location"
    )
    commands = [
        "{} -r sdk/requirements.txt",
        "{} sdk/[3dviewer,docs,experimental,test]",
        "{} -r requirements.txt",
    ]
    for command in commands:
        c = shlex.split(command.format(pip))
        process = subprocess.Popen(
            c, cwd=working_dir, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT
        )
        process.wait()
        if process.returncode:
            sys.exit("ERROR")
    print("DONE")


def list_deps(working_dir):
    print("> Listing deps... ", end="")
    sys.stdout.flush()
    with open(f"{working_dir.resolve()}/pip-list.out", "w") as f:
        process = subprocess.Popen(
            f"source {working_dir.resolve()}/license_env/bin/activate ; {working_dir.resolve()}/license_env/bin/pip list",
            shell=True,
            cwd=working_dir,
            stdout=f,
        )
        process.wait()
    if process.returncode:
        sys.exit("ERROR")
    print("DONE")


def from_show(package, working_dir):
    print(f"> Running pip show on {package}... ", end="")
    sys.stdout.flush()
    command = shlex.split(f"{working_dir.resolve()}/license_env/bin/pip show {package}")
    process = subprocess.Popen(command, cwd=working_dir, stdout=subprocess.PIPE)
    process.wait()
    if process.returncode:
        sys.exit("ERROR")
    print("DONE")
    return PipShowDetails(package, process.stdout.readlines())


def parse_license(line, working_dir=None):
    package = line.split()[0]
    return from_show(package, working_dir)


def make_csv(working_dir, out):
    print("> Writing CSV... ")
    with open(f"{working_dir.resolve()}/pip-list.out", "r") as f:
        l = f.readlines()[2:]
    m = map(functools.partial(parse_license, working_dir=working_dir), l)

    with open(out, "w") as f:
        f.write("package,version,license,url,dependencies,dependent of\r\n")
        f.write(",,,,,\r\n")
        for thing in m:
            f.write(thing.csv())
    print("> Writing CSV... DONE")


def compare_last(working_dir):
    deps = []
    with open(f"{working_dir.resolve()}/known_licenses.csv", "r") as f:
        for i, line in enumerate(f.readlines()):
            if i < 2:
                continue
            deps = [*deps, line.split(",")[0]]
    with open(f"{working_dir.resolve()}/pip-list.out", "r") as f:
        for i, line in enumerate(f.readlines()):
            if i < 2:
                continue
            dep = line.split()[0]
            if dep in deps:
                continue
            print(f"Detected a new dependency: |{dep}|")


def generate_list(out=None):
    if not out:
        sys.exit("Must provide an output filename")
    working_dir = Path(__file__).resolve().parent
    make_clean_env(working_dir)
    install_deps(working_dir)
    list_deps(working_dir)
    make_csv(working_dir, out)
    compare_last(working_dir)
    print(
        f"licenses written to '{out}'. "
        "The urls are not the paths to licenses, "
        "and will need to be manually updated if necessary."
    )


def main():
    working_dir = Path(__file__).resolve().parent

    parser = argparse.ArgumentParser(
        description=(
            "Check the licenses used by the vector python sdk repo."
            "To make changes to the proto files, you need to modify "
            "them in the victor repo and push to the victor-proto subtree."
        )
    )
    parser.add_argument(
        "-o",
        "--out",
        help="Include tool dependencies in the licenses list",
        default=str(working_dir / "licenses.csv"),
    )
    args = parser.parse_args()

    generate_list(**vars(args))


if __name__ == "__main__":
    main()
