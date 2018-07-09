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

    print("------ begin testing fetching robot stats ------")

    # Fetch robot stats
    with vector.Robot(args.ip, str(cert)) as robot:
        robot.get_robot_stats()
        time.sleep(2.0) # Let enough time pass to fetch stats
        

    print("------ finished testing fetching robot stats ------")


if __name__ == "__main__":
    main()
