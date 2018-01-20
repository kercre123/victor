#!/usr/bin/env python3

# Copyright (c) 2017 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Run through a list of exposed sdk calls, being as exhaustive as possible.
This should help to surface any bad clad ids, or other internal problems which
would be useful to catch at the PR test level.
'''

import time
import numpy as np
from PIL import Image

import cozmo
from cozmo.objects import CustomObject, CustomObjectMarkers, CustomObjectTypes
from cozmo.util import degrees, distance_mm, speed_mmps, Pose
from cozmo.objects import LightCube1Id, LightCube2Id, LightCube3Id

async def run_anim_test(robot: cozmo.robot.Robot):
    action = robot.play_anim(name="anim_poked_giggle")
    action.abort()
    
    action = robot.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    action.abort()

    robot.set_idle_animation(cozmo.anim.Triggers.MajorWin)
    robot.clear_idle_animation()
    
async def run_behavior_test(robot: cozmo.robot.Robot):
    #@TODO: research why this fails intermittently
    #robot.start_freeplay_behaviors()
    #robot.stop_freeplay_behaviors()

    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace)
    behavior.stop()
    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.KnockOverCubes)
    behavior.stop()
    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.FindFaces)
    behavior.stop()
    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.PounceOnMotion)
    behavior.stop()
    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.RollBlock)
    behavior.stop()
    behavior = robot.start_behavior(cozmo.behavior.BehaviorTypes.StackBlocks)
    behavior.stop()

async def run_camera_test(robot: cozmo.robot.Robot):
    camera = robot.camera
    config = camera.config

    print("camera.config.focal_length: "+ str(config.focal_length))
    print("camera.config.center: "+ str(config.center))
    print("camera.config.fov_x: "+ str(config.fov_x))
    print("camera.config.fov_y: "+ str(config.fov_y))
    print("camera.config.min_exposure_time_ms: "+ str(config.min_exposure_time_ms))
    print("camera.config.max_exposure_time_ms: "+ str(config.max_exposure_time_ms))
    print("camera.config.min_gain: "+ str(config.min_gain))
    print("camera.config.max_gain: "+ str(config.max_gain))

    camera.set_manual_exposure( (config.min_exposure_time_ms + config.max_exposure_time_ms)/2, (config.min_gain + config.max_gain)/2 )
    camera.enable_auto_exposure()
    print("camera.is_auto_exposure_enabled: "+ str(camera.is_auto_exposure_enabled))

    print("camera.gain: "+ str(camera.gain))
    print("camera.exposure_ms: "+ str(camera.exposure_ms))

    camera.image_stream_enabled = True
    print("camera.image_stream_enabled: "+ str(camera.image_stream_enabled))

    camera.color_image_enabled = True
    print("camera.color_image_enabled: "+ str(camera.color_image_enabled))

async def run_face_test(robot: cozmo.robot.Robot):
    #generate a really simple (empty black) image
    face_dimensions = cozmo.oled_face.SCREEN_WIDTH, cozmo.oled_face.SCREEN_HALF_HEIGHT
    img = Image.new('1', face_dimensions)
    convertedImage = cozmo.oled_face.convert_pixels_to_screen_data(bytes(np.array(img)), face_dimensions[0],  face_dimensions[1])

    #display for some time
    duration = 0.1
    robot.display_oled_face_image(convertedImage, duration * 1000.0)

async def run_define_objects_test(robot: cozmo.robot.Robot):
    # (These are all lifted out of 09_custom_objects)
    #
    # define a unique cube (44mm x 44mm x 44mm) (approximately the same size as a light cube)
    # with a 30mm x 30mm Diamonds2 image on every face
    cube_obj = await robot.world.define_custom_cube(CustomObjectTypes.CustomType00,
                                              CustomObjectMarkers.Diamonds2,
                                              44,
                                              30, 30, True)

    # define a unique cube (88mm x 88mm x 88mm) (approximately 2x the size of a light cube)
    # with a 50mm x 50mm Diamonds3 image on every face
    big_cube_obj = await robot.world.define_custom_cube(CustomObjectTypes.CustomType01,
                                              CustomObjectMarkers.Diamonds3,
                                              88,
                                              50, 50, True)

    # define a unique wall (150mm x 120mm (x10mm thick for all walls)
    # with a 50mm x 30mm Circles2 image on front and back
    wall_obj = await robot.world.define_custom_wall(CustomObjectTypes.CustomType02,
                                              CustomObjectMarkers.Circles2,
                                              150, 120,
                                              50, 30, True)

    # define a unique box (60mm deep x 140mm width x100mm tall)
    # with a different 30mm x 50mm image on each of the 6 faces
    box_obj = await robot.world.define_custom_box(CustomObjectTypes.CustomType03,
                                            CustomObjectMarkers.Hexagons2,  # front
                                            CustomObjectMarkers.Circles3,   # back
                                            CustomObjectMarkers.Circles4,   # top
                                            CustomObjectMarkers.Circles5,   # bottom
                                            CustomObjectMarkers.Triangles2, # left
                                            CustomObjectMarkers.Triangles3, # right
                                            60, 140, 100,
                                            30, 50, True)

    if ((cube_obj is not None) and (big_cube_obj is not None) and
            (wall_obj is not None) and (box_obj is not None)):
        print('All objects defined successfully!')
    else:
        raise Exception('Issue creating custom objects')

async def run_low_level_test(robot: cozmo.robot.Robot):
    robot.enable_all_reaction_triggers(False)
    robot.set_robot_volume(1.0)
    robot.abort_all_actions()
    robot.enable_facial_expression_estimation(False)

    await robot.drive_wheels(l_wheel_speed=10, r_wheel_speed=10)
    robot.stop_all_motors()

    robot.move_head(1.0)
    robot.move_lift(1.0)
    robot.stop_all_motors()

    robot.set_backpack_lights( cozmo.lights.white_light, cozmo.lights.white_light, cozmo.lights.white_light, cozmo.lights.white_light, cozmo.lights.white_light )
    robot.set_center_backpack_lights( cozmo.lights.blue_light )
    robot.set_center_backpack_lights( cozmo.lights.off_light )
    robot.set_all_backpack_lights( cozmo.lights.green_light )
    robot.set_all_backpack_lights( cozmo.lights.red_light )
    robot.set_backpack_lights_off()

    robot.set_head_light(True)
    robot.set_head_light(False)

    robot.set_head_angle(cozmo.util.degrees(0), in_parallel=True)
    robot.set_lift_height(0, in_parallel=True)
    robot.abort_all_actions()

async def run_driving_test(robot: cozmo.robot.Robot):
    action = robot.go_to_pose(Pose(100, 100, 0, angle_z=degrees(45)), relative_to_robot=True)
    action.abort()

    action = robot.turn_in_place(degrees(90))
    action.abort()

    action = robot.drive_straight(distance_mm(10), speed_mmps(10))
    action.abort()

    action = robot.drive_off_charger_contacts()
    action.abort()

    await robot.wait_for_all_actions_completed()

    action = robot.say_text("success!")
    action.abort()

async def run_cubes_test(robot: cozmo.robot.Robot):
    await robot.world.connect_to_cubes()

    cube1 = robot.world.get_light_cube(LightCube1Id)  # looks like a paperclip
    cube2 = robot.world.get_light_cube(LightCube2Id)  # looks like a lamp / heart
    cube3 = robot.world.get_light_cube(LightCube3Id)  # looks like the letters 'ab' over 'T'

    if cube1 is not None and cube2 is not None and cube3 is not None:
        cube1.set_light_corners(cozmo.lights.red_light, cozmo.lights.blue_light, cozmo.lights.green_light, cozmo.lights.off_light)
        cube2.set_lights(cozmo.lights.blue_light)
        cube3.set_lights(cozmo.lights.green_light)

        cube1.set_lights_off()
        cube2.set_lights_off()
        cube3.set_lights_off()

        print("cube1.battery_percentage: "+ str(cube1.battery_percentage))
        print("cube1.battery_str: "+ cube1.battery_str)

async def run_property_test(robot: cozmo.robot.Robot):
    print( "robot.is_ready: " + str(robot.is_ready) )
    print( "robot.anim_names count: " + str(len(robot.anim_names)) )
    print( "robot.pose: " + repr(robot.pose) )
    print( "robot.is_moving: " + str(robot.is_moving) )
    print( "robot.is_carrying_block: " + str(robot.is_carrying_block) )
    print( "robot.is_picking_or_placing: " + str(robot.is_picking_or_placing) )
    print( "robot.is_picked_up: " + str(robot.is_picked_up) )
    print( "robot.is_falling: " + str(robot.is_falling) )
    print( "robot.is_animating: " + str(robot.is_animating) )
    print( "robot.is_animating_idle: " + str(robot.is_animating_idle) )
    print( "robot.is_pathing: " + str(robot.is_pathing) )
    print( "robot.is_lift_in_pos: " + str(robot.is_lift_in_pos) )
    print( "robot.is_head_in_pos: " + str(robot.is_head_in_pos) )
    print( "robot.is_anim_buffer_full: " + str(robot.is_anim_buffer_full) )
    print( "robot.is_on_charger: " + str(robot.is_on_charger) )
    print( "robot.is_charging: " + str(robot.is_charging) )
    print( "robot.is_cliff_detected: " + str(robot.is_cliff_detected) )
    print( "robot.are_wheels_moving: " + str(robot.are_wheels_moving) )
    print( "robot.is_localized: " + str(robot.is_localized) )
    print( "robot.pose_angle: " + str(robot.pose_angle) )
    print( "robot.pose_pitch: " + str(robot.pose_pitch) )
    print( "robot.head_angle: " + str(robot.head_angle) )
    print( "robot.current_behavior: " + str(robot.current_behavior) )
    print( "robot.is_behavior_running: " + str(robot.is_behavior_running) )
    print( "robot.is_freeplay_mode_active: " + str(robot.is_freeplay_mode_active) )
    print( "robot.has_in_progress_actions: " + str(robot.has_in_progress_actions) )
    print( "robot.serial: " + str(robot.serial) )

async def run(robot: cozmo.robot.Robot):
    startTime = time.time()
    await run_anim_test(robot)
    await run_behavior_test(robot)
    await run_camera_test(robot)
    await run_face_test(robot)
    await run_define_objects_test(robot)
    await run_low_level_test(robot)
    await run_driving_test(robot)
    await run_cubes_test(robot)
    await run_property_test(robot)

    elapsedTime = time.time() - startTime
    print("smoke test finished in " + str(int(elapsedTime * 1000.0)) + "ms");

if __name__ == '__main__':
    cozmo.run_program(run)
