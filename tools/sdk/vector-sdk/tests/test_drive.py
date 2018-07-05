#!/usr/bin/env python3

import argparse
import asyncio
import math
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector
from vector.util import degrees, distance_mm, speed_mmps

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing driving along a straight path and turning in place ------")

    # The robot shall drive straight, stop and then turn around
    with vector.Robot(args.ip, str(cert)) as robot:
        robot.drive_straight(distance_mm(200), speed_mmps(50))
        time.sleep(2.0) # Let enough time pass to drive straight
        
        robot.turn_in_place(degrees(180))
        time.sleep(2.0) # Let enough time pass before SDK mode is de-activated

    print("------ finished testing driving along a straight path and turning in place ------")


if __name__ == "__main__":
    main()
