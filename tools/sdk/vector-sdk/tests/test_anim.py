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

CUSTOM_ANIM_FOLDER = Path("test_assets", "custom_animations")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()
    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing animations ------")
    
    with vector.Robot(args.ip, str(cert)) as robot:

        print("receiving all loaded animations")
        anim_names = robot.get_anim_names()
        if len(anim_names) == 0:
            print("Error: no animations loaded")
        else:
            for n in range(0, len(anim_names)):
                s = anim_names[n]
                s = s[s.find("anim_"):]
                anim_names[n] = s
                print("%d: %s" % (n+1, anim_names[n]))

        print("playing animation by name: anim_poked_giggle")
        robot.play_anim("anim_poked_giggle")

    print("------ finish testing animations ------")

if __name__ == "__main__":
    main()
