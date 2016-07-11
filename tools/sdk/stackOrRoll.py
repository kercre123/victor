#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

""" This is a script to have cozmo use behaviors to look around for blocks for 30 seconds and then
decide to execute certain behaviors based on the amount of blocks he sees. 
If he sees 0 blocks he will play an angry animation and stop. If he sees 1 block he will
try to roll that block to its upright position. If it is currently upright he will not do anything
If he sees 2 blocks or more, he will try to stack the blocks, but only if they are currently upright"""

cozmo = CozmoInterface()

cozmo.LookAround()
numCubes = cozmo.WaitUntilSeeBlocks(2, timeout=20)
cozmo.StopBehavior()

if numCubes == 0:
    cozmo.PlayAnimationTrigger("CubePounceLoseSession")

if numCubes == 1:
    cozmo.RollBlock(duration = 60)
    cozmo.PlayAnimationTrigger("MajorWin")

if numCubes > 1:
    cozmo.StackBlocks(duration = 60)
    cozmo.PlayAnimationTrigger("MajorWin")

cozmo.Shutdown()
