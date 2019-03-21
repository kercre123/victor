#!/usr/bin/python3

# Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.

"""This sample opens a camera and displays information about the connected camera."""

import argparse
import roypy
from roypy_platform_utils import PlatformHelper

def main ():
    platformhelper = PlatformHelper()
    # Support a '--help' command-line option
    parser = argparse.ArgumentParser(usage = __doc__)
    parser.parse_args()

    # The rest of this function opens the first camera found
    c = roypy.CameraManager()
    l = c.getConnectedCameraList()

    print("Number of cameras connected: ", l.size())
    if l.size() == 0:
        raise RuntimeError ("No cameras connected")
    
    id = l[0]
    cam = c.createCamera(id)
    cam.initialize()
    print_camera_info (cam, id)
  
def print_camera_info (cam, id=None):
    """Display some details of the camera.
    
    This method can also be used from other Python scripts, and it works with .rrf recordings in
    addition to working with hardware.
    """
    print("====================================")
    print("        Camera information")
    print("====================================")
    
    if id:
        print("Id:              " + id)
    print("Type:            " + cam.getCameraName())
    print("Width:           " + str(cam.getMaxSensorWidth()))
    print("Height:          " + str(cam.getMaxSensorHeight()))
    print("Operation modes: " + str(cam.getUseCases().size()))

    listIndent = "    "
    noteIndent = "        "
    
    useCases = cam.getUseCases()
    for u in range(useCases.size()):
        print(listIndent + useCases[u])
        numStreams = cam.getNumberOfStreams(useCases[u])
        if (numStreams > 1):
            print(noteIndent + "this operation mode has " + str(numStreams) + " streams")

    camInfo = cam.getCameraInfo()
    
    print("CameraInfo items: " + str(camInfo.size()))
    for u in range(camInfo.size()):
        print(listIndent + str(camInfo[u]))
    
if (__name__ == "__main__"):
    main()
