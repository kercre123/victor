#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo turn toward a face.

This script shows off the turn_towards_face action. It will wait for a face
and then constantly turn towards it to keep it in frame.
'''

import asyncio
import sys
import time

import cozmo


def follow_faces(robot):
    '''The core of the follow_faces program'''

    face_to_follow = None

    while True:
        turn_action = None
        if face_to_follow:
            # start turning towards the face
            turn_action = robot.turn_towards_face(face_to_follow)

        if not (face_to_follow and face_to_follow.is_visible):
            # find a visible face, timeout if nothing found after a short while
            try:
                face_to_follow = robot.world.wait_for_observed_face(timeout=30)
            except asyncio.TimeoutError:
                print("Didn't find a face - exiting!")
                return

        if turn_action:
            # Complete the turn action if one was in progress
            turn_action.wait_for_completed()

        time.sleep(.1)


def run(sdk_conn):
    '''The run method runs once the Cozmo SDK is connected.'''
    robot = sdk_conn.wait_for_robot()

    try:
        follow_faces(robot)

    except KeyboardInterrupt:
        print("")
        print("Exit requested by user")


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect_with_tkviewer(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
