#!/usr/bin/env python3

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    await robot.play_anim("anim_poked_giggle")

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)