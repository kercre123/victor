#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

"""This is a script to drive up to one block, pick it up, and put it on another block.
This is a special case script where the first block must be in view at the beginning
And then once he finishes picking up the first block he MUST be able to see the second or
else it will not work."""

cozmo = CozmoInterface()

cozmo.WaitUntilSeeBlocks(1, timeout = 60)

cozmo.PickupObject(0)

cozmo.WaitUntilSeeBlocks(2, timeout = 60)

cozmo.PlaceOnObject(1)

cozmo.Shutdown()
