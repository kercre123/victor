#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math


cozmo = CozmoInterface()

#######################################################

# cozmo.DriveWheels(50,50, duration = 3)

#######################################################

# for i in range(4):
#     cozmo.SayText(str(i+1))

#######################################################

# for i in range(4):
#     cozmo.DriveWheels(50,50,50,50, duration = 3)
#     cozmo.TurnInPlace(degrees = 90)

#######################################################

# cozmo.WaitUntilSeeBlocks(2)
#
# cozmo.PickupObject(0)
# cozmo.PlaceOnObject(1)

#######################################################

# cozmo.PlayAnimationTrigger("AdmireStackAttemptGrabThirdCube")
# AdmireStackAttemptGrabThirdCube moves a lot, yells, funny
# MajorWin no sound
# MajorFail no eyes?
# OnWiggle move a little
# StackBlocksSuccess move a little
# SimonPointLeftBig turns

#######################################################





cozmo.Shutdown()
