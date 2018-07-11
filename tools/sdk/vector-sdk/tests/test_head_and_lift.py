#!/usr/bin/env python3

import argparse
import asyncio
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector
from vector.util import degrees

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing head and lift actions ------")

    # The robot shall lower and raise his head and lift
    with vector.Robot(args.ip, str(cert)) as robot:
        robot.set_head_angle(degrees(-50.0))
        time.sleep(2)

        robot.set_head_angle(degrees(50.0))
        time.sleep(2)

        robot.set_head_angle(degrees(0.0))
        time.sleep(2)

        robot.set_lift_height(100.0)
        time.sleep(2)

        robot.set_lift_height(0.0)
        time.sleep(2)

    print("------ finished testing head and lift actions ------")

if __name__ == "__main__":
    main()
