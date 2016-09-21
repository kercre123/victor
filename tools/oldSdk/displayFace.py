#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math
import numpy as np
from PIL import Image

cozmo = CozmoInterface()

# pic = Image.open("bently.png")
# while True:
# cozmo.DisplayFaceImage(pic)
    # time.sleep(.1)
cozmo.DisplayProceduralFace(faceAngle = 45)

time.sleep(10)

cozmo.Shutdown()
