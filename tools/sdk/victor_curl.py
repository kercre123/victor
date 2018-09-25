#!/usr/bin/env python3

import argparse
import configparser
import os
from pathlib import Path
import sys

def main():
    parser = argparse.ArgumentParser(description="mimic curl requests but with auth handled by python")
    parser.add_argument("-s", "--serial", help="Vector's serial number. By default, this will try to load the ANKI_ROBOT_SERIAL environment variable. e.x. 00e20115", default=os.environ.get('ANKI_ROBOT_SERIAL', None))
    args = parser.parse_args()

    config_file = str(Path.home() / ".anki_vector" / "sdk_config.ini")
    config = configparser.ConfigParser()
    config.read(config_file)

    serial = args.serial
    if serial not in config:
        sys.exit("ERROR: Could not find serial number: '{}'. Have you configured your robot with tools/sdk/vector-sdk/configure.py ?".format(serial))

    if "port" not in config[serial]:
        config[serial]["port"] = "443"
    print("curl --resolve {name}:{port}:{ip} --cacert {cert} --header \"Authorization: Bearer {guid}\" https://{name}:{port}".format(**config[serial]))

if __name__ == "__main__":
    main()