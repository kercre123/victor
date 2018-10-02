#!/usr/bin/env python3

import argparse
import configparser
import os
from pathlib import Path
import sys

try:
    # Non-critical import to add color output
    from termcolor import colored
except:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--cert", help="The config file location")
    args = parser.parse_args()

    home = Path.home()
    anki_dir = home / ".anki_vector"
    os.makedirs(str(anki_dir), exist_ok=True)
    config_file = str(anki_dir / "sdk_config.ini")
    print("Writing config file to '{}'...".format(colored(config_file, "cyan")))
    config = configparser.ConfigParser(strict=False)

    if not os.path.exists(args.cert):
        sys.exit("{}: Must create mac cert first!".format(colored("ERROR", "red")))
    try:
        config.read(config_file)
    except configparser.ParsingError:
        if os.path.exists(config_file):
            os.rename(config_file, config_file + "-error")
    config["Local"] = {}
    config["Local"]["cert"] = str(args.cert)
    config["Local"]["ip"] = "localhost"
    config["Local"]["name"] = "Vector-Local"
    config["Local"]["guid"] = "local"
    config["Local"]["port"] = "8443"
    temp_file = config_file + "-temp"
    if os.path.exists(config_file):
        os.rename(config_file, temp_file)
    try:
        with os.fdopen(os.open(config_file, os.O_WRONLY | os.O_CREAT, 0o600), 'w') as f:
            config.write(f)
    except Exception as e:
        if os.path.exists(temp_file):
            os.rename(temp_file, config_file)
        raise e
    else:
        if os.path.exists(temp_file):
            os.remove(temp_file)
    print(colored("SUCCESS!", "green"))

if __name__ == "__main__":
    main()
