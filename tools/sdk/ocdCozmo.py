#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

cozmo = CozmoInterface()

cozmo.WaitUntilSeeBlocks(3, timeout = 30)

state = cozmo.GetState()
initialRad = []
for cube in state.lightCubes:
    initialRad.append(cube.topFaceOrientation_rad)

minRotation = .4
while True:
    state = cozmo.GetState()
    for i in range(len(state.lightCubes)):
        cube = state.lightCubes[i]
        if abs(cube.topFaceOrientation_rad - initialRad[i]) > minRotation:
            cozmo.PlayAnimationTrigger("MajorFail")
            #MajorFail Shiver PounceFail
            break


cozmo.Shutdown()
