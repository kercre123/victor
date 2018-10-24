#!/usr/bin/env python3

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time

from termcolor import colored
from halo import Halo

def process_command(title, commands, working_dir=None):
    spinner = Halo(text="Running {}".format(title), spinner="dots")
    spinner.start()
    root_path = Path(os.path.dirname(os.path.realpath(__file__)))
    log_path = root_path / "logs"
    os.makedirs(str(log_path), exist_ok=True)
    file = "{}.log".format(title.replace(" ", "_"))
    output = str(log_path / file)
    if working_dir is not None:
        working_dir = root_path / working_dir
    else:
        working_dir = root_path
    with open(output, "wb") as out:
        for command in commands:
            spinner.start(text="Running {}: {}".format(title, colored(command.strip(), 'green')))
            process = subprocess.Popen(["/bin/bash", "-c", command], cwd=str(working_dir), stdout=out, stderr=out)
            process.wait()
            if process.returncode != 0:
                spinner.fail("Failed {}: {} -> {}".format(title, colored(command, "red"), output))
                break
        else:
            spinner.succeed("Finished {}".format(title))

def main():
    scripts_dir = Path(os.path.dirname(os.path.realpath(__file__)), "..", "scripts")
    process_command(
        "setup",
        [
            "cd {} ; ./update_proto.sh".format(scripts_dir.resolve()),
            "cd {} ; ./sdk_lint.sh".format(scripts_dir.resolve()),
        ]
    )

    with open("experimental.json", "r") as file:
        steps = json.loads(file.read())
        for step in steps:
            process_command(step, steps[step])


if __name__ == "__main__":
    main()
