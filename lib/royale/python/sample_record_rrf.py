#!/usr/bin/python3

# Copyright (C) 2018 Infineon Technologies & pmdtechnologies ag
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.

"""This sample shows how to shows how to record data to an .rrf file.

This sample uses Royale's feature of stopping after a given number of
frames are captured, therefore the --frames argument is required.
"""

import argparse
import queue
import roypy
from sample_camera_info import print_camera_info
from roypy_sample_utils import CameraOpener, add_camera_opener_options
from roypy_platform_utils import PlatformHelper

class MyListener (roypy.IRecordStopListener):
    """A simple listener, in which waitForStop() blocks until onRecordingStopped has been called."""
    def __init__ (self):
        super (MyListener, self).__init__()
        self.queue = queue.Queue()

    def onRecordingStopped (self, frameCount):
        self.queue.put (frameCount)

    def waitForStop (self):
        frameCount = self.queue.get()
        print ("Stopped after capturing {frameCount} frames".format (frameCount=frameCount))

def main ():
    platformhelper = PlatformHelper()
    parser = argparse.ArgumentParser (usage = __doc__)
    add_camera_opener_options (parser)
    parser.add_argument ("--frames", type=int, required=True, help="duration to capture data (number of frames)")
    parser.add_argument ("--output", type=str, required=True, help="filename to record to")
    parser.add_argument ("--skipFrames", type=int, default=0, help="frameSkip argument for the API method")
    parser.add_argument ("--skipMilliseconds", type=int, default=0, help="msSkip argument for the API method")
    options = parser.parse_args()

    opener = CameraOpener (options)
    cam = opener.open_camera ()

    print_camera_info (cam)

    l = MyListener()
    cam.registerRecordListener(l)
    cam.startCapture()
    cam.startRecording (options.output, options.frames, options.skipFrames, options.skipMilliseconds);

    seconds = options.frames * (options.skipFrames + 1) / cam.getFrameRate()
    if options.skipMilliseconds:
        timeForSkipping = options.frames * options.skipMilliseconds / 1000
        seconds = int (max (seconds, timeForSkipping))

    print ("Capturing with the camera running at {rate} frames per second".format (rate=cam.getFrameRate()))
    print ("This is expected to take around {seconds} seconds".format (seconds=seconds))
    
    l.waitForStop()

    cam.stopCapture()

if (__name__ == "__main__"):
    main()
