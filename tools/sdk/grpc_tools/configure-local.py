#!/usr/bin/env python3

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
    # Write cert to a file
    home = Path.home()
    anki_dir = home / ".anki-vector"
    os.makedirs(str(anki_dir), exist_ok=True)
    config_file = str(anki_dir / "sdk_config.ini")
    print("Writing config file to '{}'...".format(colored(config_file, "cyan")))
    config = configparser.ConfigParser()

    cert = Path.cwd() / "trust_mac.cert"
    if not cert.exists():
        sys.exit("{}: Must create mac cert first!".format(colored("ERROR", "red")))
    config.read(config_file)
    config["Local"] = {}
    config["Local"]["cert"] = str(cert)
    config["Local"]["ip"] = "localhost"
    config["Local"]["name"] = "Vector-Local"
    config["Local"]["guid"] = "local"
    config["Local"]["port"] = "8443"
    with os.fdopen(os.open(config_file, os.O_WRONLY | os.O_CREAT, 0o600), 'w') as f:
        config.write(f)
    print(colored("SUCCESS!", "green"))

if __name__ == "__main__":
    main()
