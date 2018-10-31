import importlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

package_path = Path(os.path.dirname(os.path.realpath(__file__)))
versions_file = package_path / "versions.json"

__all__ = []

def clone(path, repo, tag, **_):
    shutil.rmtree(package_path / path, ignore_errors=True)
    process = subprocess.Popen(
        ["git", "clone", repo, path],
        cwd=package_path,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    ) #TODO: add tag
    code = process.wait()
    if code != 0:
        print(process.stdout.read())
        sys.exit(code)
    return

with open(versions_file, "r") as versions:
    versions_data = json.load(versions)

for version in versions_data:
    if not (package_path / version).exists():
        clone(path=version, **versions_data[version])
    importlib.import_module(f".{version}", package="interface_version")
    __all__.append(version)
