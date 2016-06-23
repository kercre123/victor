#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

""" This is a script to have cozmo use behaviors to look around for blocks for 30 seconds and then
decide to execute certain behaviors based on the amount of blocks he sees. 
If he sees 0 blocks he will play an angry animation and stop. If he sees 1 block he will
try to roll that block to its upright position. If it is currently upright he will not do anything
If he sees 2 blocks or more, he will try to stack the blocks, but only if they are currently upright"""

cozmo = CozmoInterface(False, 0)

cozmo.StartSim()

cozmo.UnlockAll()
print("Looking around for blocks, 30 seconds")
cozmo.LookAround()
numCubes = cozmo.WaitUntilSeeBlocks(3, timeout=30)
cozmo.StopBehavior()
time.sleep(1)
print("Done with LookAround, numCubes = " + str(numCubes))
if numCubes == 0:
    print("Playing angry!")
    cozmo.PlayAnimation("anim_speedTap_loseSession_03")
if numCubes == 1:
    print("Playing RollBlock") 
    cozmo.RollBlock()
    time.sleep(60)
    cozmo.StopBehavior()
if numCubes > 1:
    print("Playing StackBlocks")
    cozmo.StackBlocks()
    time.sleep(60)
    cozmo.StopBehavior()
time.sleep(1)
print("Behavior done, playing majorWin")
cozmo.PlayAnimation("majorWin")
cozmo.Stop()
time.sleep(1)
print("Bye!")
cozmo.Shutdown()
