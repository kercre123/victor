#!/usr/bin/env python3

"""
Test 3d viewer
"""

import asyncio
import anki_vector
from anki_vector import opengl_viewer
from anki_vector.objects import CustomObjectTypes, CustomObjectMarkers


async def my_function(robot):  # pylint: disable=unused-argument
    await asyncio.sleep(20)
    # @TODO: 3d viewer needs to shut down when this function completes
    print("done")


def main():
    """main execution"""
    args = anki_vector.util.parse_command_args()

    with anki_vector.Robot(args.serial, show_viewer=True, enable_camera_feed=True, enable_vision_mode=True) as robot:

        robot.world.define_custom_cube(custom_object_type=CustomObjectTypes.CustomType00,
                                       marker=CustomObjectMarkers.Circles2,
                                       size_mm=100.0,
                                       marker_width_mm=100.0, marker_height_mm=100.0)

        robot.world.connect_cube()

        viewer = opengl_viewer.OpenGLViewer(robot=robot)
        viewer.run(my_function)


if __name__ == "__main__":
    main()
