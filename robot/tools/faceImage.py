#!/usr/bin/env python3
"""
Python tool for putting an image on the robot's face
"""

import sys, os, time
if sys.version_info.major < 3:
    sys.stderr.write("Python 2 is depricated." + os.linesep)
from PIL import Image
import robotInterface
Anki = robotInterface.Anki

SCREEN_WIDTH = 128
SCREEN_HEIGHT = 64

def loadImage(filePathName, invert=False):
    "Load an image and return a face keyframe message"
    img = Image.open(filePathName)
    if img.size[0] > SCREEN_WIDTH:
        raise ValueError("Image too wide: {:d} > {:d}".format(img.size[0], SCREEN_WIDTH))
    elif img.size[1] > SCREEN_HEIGHT:
        raise ValueError("Image to tall: {:d} > {:d}".format(img.size[1], SCREEN_HEIGHT))
    else:
        hPadding = (SCREEN_WIDTH  - img.size[0])//2
        vPadding = (SCREEN_HEIGHT - img.size[1])//2
        bitmap = [1]*(SCREEN_WIDTH*SCREEN_HEIGHT)
        for y in range(img.size[1]):
            for x in range(img.size[0]):
                bitmap[(hPadding + x)*SCREEN_HEIGHT + (vPadding + y)] = 1 if img.getpixel((x,y)) > 128 else 0
        if invert: bitmap = [0 if b else 1 for b in bitmap]
        packed = [0]*(SCREEN_WIDTH*SCREEN_HEIGHT//8)
        for c in range(len(packed)):
            packed[c] = (bitmap[c*8 + 0] << 0) | (bitmap[c*8 + 1]) << 1 | (bitmap[c*8 + 2] << 2) | (bitmap[c*8 + 3] << 3) | (bitmap[c*8 + 4] << 4) | (bitmap[c*8 + 5] << 5) | (bitmap[c*8 + 6] << 6) | (bitmap[c*8 + 7] << 7)
        return Anki.Cozmo.AnimKeyFrame.FaceImage(packed)

def SendFaceImage(*args):
    img = loadImage(sys.argv[1], (len(sys.argv) > 2 and sys.argv[2] == '-i'))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animStartOfAnimation=Anki.Cozmo.AnimKeyFrame.StartOfAnimation()))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animFaceImage=img))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animEndOfAnimation=Anki.Cozmo.AnimKeyFrame.EndOfAnimation()))

if __name__ == "__main__":
    robotInterface.Init()
    robotInterface.SubscribeToConnect(SendFaceImage)
    robotInterface.Connect()
    try:
        time.sleep(3600)
    except KeyboardInterrupt:
        pass
