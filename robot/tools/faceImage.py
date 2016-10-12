#!/usr/bin/env python3
"""
Python tool for putting an image on the robot's face
"""

import sys, os, time, argparse
if sys.version_info.major < 3:
    sys.stderr.write("Python 2 is depricated." + os.linesep)
from PIL import Image
import robotInterface
Anki = robotInterface.Anki

SCREEN_WIDTH = 128
SCREEN_HEIGHT = 64

args = None

#converts rgba or rgb into greyscale.
def greyscale(pixel):
    try:
        r,g,b,a = pixel
    except (ValueError,TypeError):
        try:
            a = 1.0
            r,g,b = pixel
        except (ValueError,TypeError):
            r = g = b = pixel
    return (r+r+g+g+g+b)/6 * a

#divides brightness range (greyscale -> b/w)
def find_threshold(img):
        maxval = 0
        for y in range(img.size[1]):
            for x in range(img.size[0]):
                p = greyscale(img.getpixel((x,y)))
                maxval = max(maxval, p)
        return maxval/2


def loadImage(filePathName, invert=False):
    "Load an image and return a face keyframe message"
    img = Image.open(filePathName)
    if img.size[0] > SCREEN_WIDTH or img.size[1] > SCREEN_HEIGHT:
        img.thumbnail((SCREEN_WIDTH, SCREEN_HEIGHT))
    if img.size[0] > SCREEN_WIDTH:
        raise ValueError("Image too wide: {:d} > {:d}".format(img.size[0], SCREEN_WIDTH))
    elif img.size[1] > SCREEN_HEIGHT:
        raise ValueError("Image to tall: {:d} > {:d}".format(img.size[1], SCREEN_HEIGHT))
    else:
        threshold = find_threshold(img)
        
        hPadding = (SCREEN_WIDTH  - img.size[0])//2
        vPadding = (SCREEN_HEIGHT - img.size[1])//2
        bitmap = [1]*(SCREEN_WIDTH*SCREEN_HEIGHT)
        for y in range(img.size[1]):
            for x in range(img.size[0]):
                pixel = 1 if greyscale(img.getpixel((x,y))) > threshold else 0
                bitmap[(hPadding + x)*SCREEN_HEIGHT + (vPadding + y)] = pixel
                print(pixel,end="")
            print(end="\n")
        if invert: bitmap = [0 if b else 1 for b in bitmap]
        packed = [0]*(SCREEN_WIDTH*SCREEN_HEIGHT//8)
        for c in range(len(packed)):
            packed[c] = (bitmap[c*8 + 0] << 0) | (bitmap[c*8 + 1]) << 1 | (bitmap[c*8 + 2] << 2) | (bitmap[c*8 + 3] << 3) | (bitmap[c*8 + 4] << 4) | (bitmap[c*8 + 5] << 5) | (bitmap[c*8 + 6] << 6) | (bitmap[c*8 + 7] << 7)
        print("Sending Face image of {} bytes".format(len(packed)))
            
        return Anki.Cozmo.AnimKeyFrame.FaceImage(packed)

def SendFaceImage(conn):
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animStartOfAnimation=Anki.Cozmo.AnimKeyFrame.StartOfAnimation()))

    for fn in args.images:
        img = loadImage(fn, args.invert)
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence()))
        robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animFaceImage=img))
        time.sleep(1.0 / 15)

    robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(animEndOfAnimation=Anki.Cozmo.AnimKeyFrame.EndOfAnimation()))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Stream images to the robot.')
    parser.add_argument('images', metavar='Image', type=str, nargs='+',
                        help='list of images')
    parser.add_argument('-i', '--invert', dest='invert', action='store_true',
                        help='invert the images')

    args = parser.parse_args()
    robotInterface.Init()
    robotInterface.SubscribeToConnect(SendFaceImage)
    robotInterface.Connect()
    try:
        time.sleep(3600)
    except KeyboardInterrupt:
        pass
