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

'''Run through commands on cozmo that we want to make sure function correctly.
This script is meant to test commands that are difficult to trap in the smoke test
do to their not executing in an instantaneous or context independent fashion.

Make sure nothing in this process throws errors.
'''

import sys
import asyncio

import cozmo
from cozmo.util import distance_mm, speed_mmps, degrees

class EvtTest:
    def evt_callback(self, evt, **kw):
        self.passed = True
    def __init__(self, robot, event_id, **kw):
        self.event_id = event_id
        self.passed = False
        robot.add_event_handler(event_id, self.evt_callback)

class EventsSmokeTest:
    robot = None
    faces_found = []

    # todo, create objects that register an event, and clear when its hit.
    # add them all to a set, and list out any that weren't thinged at the end
    async def handle_face_appeared(self, evt, **kw):
        if evt.face is not None and evt.face not in self.faces_found:
            self.faces_found.append(evt.face)

    async def test_events(self, conn, robot):
        robot.add_event_handler(cozmo.faces.EvtFaceAppeared, self.handle_face_appeared)

        # Camera events
        tests = [EvtTest(robot, cozmo.camera.EvtNewRawCameraImage), EvtTest(robot, cozmo.world.EvtNewCameraImage),
                 # Action events
                 EvtTest(robot, cozmo.action.EvtActionStarted), EvtTest(robot, cozmo.action.EvtActionCompleted),
                 # Behavior events
                 EvtTest(robot, cozmo.behavior.EvtBehaviorRequested), EvtTest(robot, cozmo.behavior.EvtBehaviorStarted),
                 EvtTest(robot, cozmo.behavior.EvtBehaviorStopped),
                 # Object events
                 # @TODO: How do I make objects come and go to triggere connectionChanges in webots?
                 #EvtTest(robot, cozmo.objects.EvtObjectConnectChanged),
                 EvtTest(robot, cozmo.objects.EvtObjectAppeared), EvtTest(robot, cozmo.objects.EvtObjectDisappeared),
                 EvtTest(robot, cozmo.objects.EvtObjectLocated), EvtTest(robot, cozmo.objects.EvtObjectObserved),
                 # @TODO: How do we move cubes in webots manually to trigger all the object appear/observe/move events?
                 #EvtTest(robot, cozmo.objects.EvtObjectMovingStarted),
                 #EvtTest(robot, cozmo.objects.EvtObjectMoving),
                 #EvtTest(robot, cozmo.objects.EvtObjectMovingStopped),
                 # @TODO: How do I simulate a touch in webots?
                 #EvtTest(robot, cozmo.objects.EvtObjectTapped),
                 # Face Events
                 EvtTest(robot, cozmo.faces.EvtFaceObserved), EvtTest(robot, cozmo.faces.EvtFaceAppeared),
                 EvtTest(robot, cozmo.faces.EvtFaceDisappeared),
                 EvtTest(robot, cozmo.faces.EvtErasedEnrolledFace),
                 # @TODO: This doesn't seem to be recieving its completion event - might be a bug
                 #EvtTest(robot, cozmo.faces.EvtFaceRenamed),
                 # Pet Events
                 EvtTest(robot, cozmo.pets.EvtPetAppeared), EvtTest(robot, cozmo.pets.EvtPetDisappeared),
                 EvtTest(robot, cozmo.pets.EvtPetObserved)]
        
        # ---------- currently not used - due to unsure how to set up ---------- #
        # EvtTest(robot, cozmo.faces.EvtFaceIdChanged),


        # ---------- perform stimula which will result in events ---------- #

        # look up at the pets and faces
        await robot.set_head_angle(degrees(20)).wait_for_completed()

        # really make sure cozmo has time to take in those faces...
        await asyncio.sleep(2.0)

        # start spamming camera events - to pass the camera tests
        robot.camera.image_stream_enabled = True

        # trigger action events
        await robot.drive_straight(distance_mm(-50), speed_mmps(100)).wait_for_completed()

        # trigger object located event
        conn._request_located_objects()

        await robot.turn_in_place(degrees(180.0)).wait_for_completed()
        await robot.turn_in_place(degrees(180.0)).wait_for_completed()

        robot.remove_event_handler(cozmo.faces.EvtFaceAppeared, self.handle_face_appeared)

        if len(self.faces_found) > 0:
            face = self.faces_found[0]
            if face.name:
                face.erase_enrolled_face()

            await face.name_face("faceguy").wait_for_completed()

            face.rename_face("faceguyrenamed")
            # give the event rename event some time to traverse
            await asyncio.sleep(2.0)
            face.erase_enrolled_face()

        # trigger behavior events
        await robot.run_timed_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace, 1.0)

        # give the events some time to filter through before verifying them
        await asyncio.sleep(2.0)

        testsPassed = 0
        for test in tests:
            if test.passed:
                testsPassed += 1
            else:
                print("test failed: " + str(test.event_id) + " never recieved")

        failedTests = len(tests) - testsPassed
        if failedTests > 0:
            raise Exception(str(failedTests) + "/" + str(len(tests)) + " Tests Failed")

    async def run(self, conn):
        self.robot = await conn.wait_for_robot()
        await self.test_events(conn, self.robot)

def main():
    cozmo.setup_basic_logging()
    try:
        est = EventsSmokeTest()
        cozmo.connect(est.run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)

if __name__ == '__main__':
    main()
