#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

"""This is an example of a simple script for cozmo. He plays an angry animation,
drives forward for 3 seconds and then plays a success animation"""

cozmo = CozmoInterface()

cozmo.SetBackpackLEDs("LED_RED")

cozmo.PlayAnimationTrigger("CubePounceLoseSession")

cozmo.DriveWheels(50,50,50,50, duration = 3)

cozmo.PlayAnimationTrigger("ReactToCliff")

cozmo.SayText("Hello")

cozmo.Shutdown()
