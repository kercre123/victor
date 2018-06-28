#!/usr/bin/env python3

'''
Test playing and listing animations
'''

import argparse
import asyncio
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()
    cert = Path(args.cert_file)
    cert.resolve()

    with vector.Robot(args.ip, str(cert)) as robot:
        print("------ begin querying animations ------")

        anim_names = robot.get_anim_names()

        print("found {0} animations: {1}".format(len(anim_names), anim_names))

        print("------ finish querying animations ------")

if __name__ == "__main__":
    main()
