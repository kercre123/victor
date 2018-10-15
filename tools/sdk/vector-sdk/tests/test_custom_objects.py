#!/usr/bin/env python3

"""
"""

import anki_vector  # pylint: disable=wrong-import-position
from anki_vector.util import Pose, degrees
from anki_vector.objects import CustomObjectMarkers, CustomObjectTypes
import asyncio

def wait(robot, t):
    robot.loop.run_until_complete(asyncio.wait_for(asyncio.sleep(t), float(t) + 2.5))

def main():
    """main execution"""
    args = anki_vector.util.parse_command_args()

    with anki_vector.Robot(args.serial) as robot:
        wait(robot, 1)

        print('testing creation of custom box...')
        box = robot.world.define_custom_box(custom_object_type=CustomObjectTypes.CustomType00,
                                            marker_front= CustomObjectMarkers.Circles2,
                                            marker_back=  CustomObjectMarkers.Circles3,
                                            marker_top=   CustomObjectMarkers.Circles4,
                                            marker_bottom=CustomObjectMarkers.Circles5,
                                            marker_left=  CustomObjectMarkers.Triangles2,
                                            marker_right= CustomObjectMarkers.Triangles3,
                                            depth_mm=20.0, width_mm=20.0, height_mm=20.0,
                                            marker_width_mm=10.0, marker_height_mm=10.0)
        if box:
            print("custom box created successfully")
        wait(robot, 1)
        robot.world.delete_custom_objects()


        print('testing creation of custom cube...')
        cube = robot.world.define_custom_cube(custom_object_type=CustomObjectTypes.CustomType00,
                                              marker=CustomObjectMarkers.Circles2,
                                              size_mm=20.0,
                                              marker_width_mm=10.0, marker_height_mm=10.0)
        if cube:
            print("custom cube created successfully")
        wait(robot, 1)
        robot.world.delete_custom_objects()


        print('testing creation of custom wall...')
        wall = robot.world.define_custom_wall(custom_object_type=CustomObjectTypes.CustomType00,
                                              marker=CustomObjectMarkers.Circles2,
                                              width_mm=20.0, height_mm=20.0,
                                              marker_width_mm=10.0, marker_height_mm=10.0)
        if wall:
            print("custom wall created successfully")
        wait(robot, 1)
        robot.world.delete_custom_objects()


        fixed_object = robot.world.create_custom_fixed_object(Pose(100, 0, 0, angle_z=degrees(0)),
                                                              10, 100, 100, relative_to_robot=True)
        if fixed_object:
            print("fixed custom object created successfully")

        robot.behavior.go_to_pose(Pose(200, 0, 0, angle_z=degrees(0)), relative_to_robot=True)
        robot.world.delete_custom_objects()

        print('objects: {0}'.format(robot.world.all_objects))

if __name__ == "__main__":
    main()
