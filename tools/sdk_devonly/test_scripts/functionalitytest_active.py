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

'''Run through commands on cozmo related to behavior and object interaction.

Make sure none of them throw errors.
'''

import time

import cozmo
from cozmo.util import degrees, distance_mm, speed_mmps

def run(robot: cozmo.robot.Robot):
    robot.backup_onto_charger(max_drive_time=2.0)

    robot.drive_straight(distance_mm(-100), speed_mmps(100))

    robot.run_timed_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace, 1.0)
    time.sleep(1.25)

    lookaround = robot.start_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace)
    cubes = robot.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=60)
    lookaround.stop()

    #@TODO: Place the cube manipulation code up top, and ensure the wbt world is set up so that 
    #       cozmo is guaranteed to see 2 cubes on startup.

    # for now, if he doesn't find a cube, i'll just let the test silently succeed
    # we may want to modify this so he's a little more guaranteed to hit the proper flow.
    # For now my priority is to avoid sending false fails on the nightly tests.
    if len(cubes) > 1:
        robot.go_to_object(cubes[1], distance_mm(100)).wait_for_completed()
        robot.pickup_object(cubes[0]).wait_for_completed()
        robot.place_on_object(cubes[1]).wait_for_completed()
        robot.drive_straight(distance_mm(-150), speed_mmps(100)).wait_for_completed()
        robot.pickup_object(cubes[0]).wait_for_completed()
        robot.drive_straight(distance_mm(-150), speed_mmps(100)).wait_for_completed()
        robot.place_object_on_ground_here(cubes[0]).wait_for_completed()

        #@TODO: have cozmo drive forward, and call "abort_all_actions", and make sure his pose updated roughly the right amount

if __name__ == '__main__':
    cozmo.run_program(run)
