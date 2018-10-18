#!/usr/bin/env python3

"""
Test 3d viewer
"""

import asyncio
import anki_vector

async def my_function(robot):
    await robot.play_animation("anim_blackjack_victorwin_01")
    asyncio.sleep(30)

with anki_vector.Robot() as robot:
    viewer = anki_vector.opengl_viewer.OpenGLViewer(robot=robot)
    viewer.run(my_function)
