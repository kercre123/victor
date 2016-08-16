#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

cozmo = CozmoInterface()

cozmo.WaitUntilSeeFaces(1)

while True:
   state = cozmo.GetState()

   firstFace = state.GetFace(0)

   if state.IsObjectVisible(firstFace):

       cozmo.SetBackpackLEDs("LED_BLUE")
   else:
       cozmo.SetBackpackLEDs("LED_OFF")

   time.sleep(0.02)

cozmo.Shutdown()
