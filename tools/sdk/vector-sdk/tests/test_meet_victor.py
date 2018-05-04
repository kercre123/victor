#!/usr/bin/env python3

import asyncio
import time

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    # Start Meet Victor. Puts Victor into behavior MeetVictor EnrollFace.
    # Must shake robot first to turn on behaviors.
    # Status messages for Meet Victor flow:
    #   MeetVictorStarted
    #   MeetVictorFaceScanStarted
    #   MeetVictorFaceScanComplete
    #   FaceEnrollmentCompleted
    await robot.meet_victor('michelle')
    await asyncio.sleep(10.0)

    # Cancel Meet Victor. Stops MeetVictor EnrollFace behavior.
    await robot.cancel_face_enrollment()

    # Returns list of names in EnrolledNamesResponse.
    await robot.request_enrolled_names()

    await robot.update_enrolled_face_by_id(0, 'michelle', 'not_michelle')

    await robot.erase_enrolled_face_by_id(0)

    await robot.erase_all_enrolled_faces()

    await robot.set_face_to_enroll('michelle')

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
