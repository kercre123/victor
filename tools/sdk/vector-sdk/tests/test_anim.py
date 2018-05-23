#!/usr/bin/env python3

import asyncio
import os
from pathlib import Path
import sys
import time
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

CUSTOM_ANIM_FOLDER = Path("test_assets", "custom_animations")
async def main(robot):
    print("------ testing animations ------")

    print("receiving all loaded animations")
    await asyncio.sleep(2.0)
    anim_names = await robot.get_anim_names()
    if len(anim_names) == 0:
        print("Error: no animations loaded")
    else:
        for n in range(0, len(anim_names)):
            s = anim_names[n].decode("utf_8")
            s = s[s.find("anim_"):]
            anim_names[n] = s
            print("%d: %s" % (n+1, anim_names[n])),

    print("playing animation by name: anim_poked_giggle")
    await robot.play_anim("anim_poked_giggle")
    await asyncio.sleep(2.0)

    print("uploading a custom pounce animation")
    await robot.transfer_file(robot.FileType.Animation, CUSTOM_ANIM_FOLDER / "anim_custom_pounce.json")
    await asyncio.sleep(2.0)

    print("playing the custom pounce animaiton")
    await robot.play_anim("anim_custom_pounce")
    await asyncio.sleep(2.0)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)