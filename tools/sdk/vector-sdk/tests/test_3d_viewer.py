#!/usr/bin/env python3

"""
Test 3d viewer
"""

import asyncio
import anki_vector
from anki_vector import opengl_viewer

async def my_function(robot):
    await robot.anim.play_animation("anim_blackjack_victorwin_01")
    await asyncio.sleep(30)

def main():
    """main execution"""
    args = anki_vector.util.parse_command_args()

    with anki_vector.Robot(args.serial) as robot:
        viewer = opengl_viewer.OpenGLViewer(robot=robot)
        viewer.run(my_function)

if __name__ == "__main__":
    main()
