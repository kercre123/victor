#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

"""This is a script to drive up to one block, pick it up, and put it on another block.
This is a special case script where the first block must be in view at the beginning
And then once he finishes picking up the first block he MUST be able to see the second or
else it will not work."""

cozmo = CozmoInterface(False, 0)

cozmo.StartSim()

cozmo.UnlockAll()

time.sleep(5)

print("Trying to align!")

state = cozmo.GetState()
if len(state.cubeOrder):
    cozmo.PickupObject(state.cubeOrder[0])
print("After lifting, driving backwards now")
if len(state.cubeOrder) > 1:
    cozmo.PlaceOnObject(state.cubeOrder[1])


cozmo.Stop()
time.sleep(1)
print("Bye!")
cozmo.Shutdown()
