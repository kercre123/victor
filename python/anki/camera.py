# -*- coding: utf-8 -*-
"""
Created on Mon Nov 17 14:45:00 2014

@author: pbarnum
"""

# Use python 3 types by default (uses "future" package)
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from future.builtins import *

import cv2
import numpy as np

class Camera(object):
    def __init__(self, cameraMatrix, distortionCoefficients, imageSize):
        assert type(cameraMatrix).__module__[:5] == np.__name__, 'Must be a Numpy array'
        assert type(distortionCoefficients).__module__[:5] == np.__name__, 'Must be a Numpy array'

        self.cameraMatrix = cameraMatrix
        self.distortionCoefficients = distortionCoefficients
        self.imageSize = imageSize

        self.mapX, self.mapY = cv2.initUndistortRectifyMap(cameraMatrix, distortionCoefficients, np.eye(3), cameraMatrix, imageSize[::-1], cv2.CV_32FC1)

    def undistort(self, image, convertToGray=True):
        assert type(image).__module__[:5] == np.__name__, 'Must be a Numpy array'

        if convertToGray and (len(image.shape) == 3):
            image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)

        image = cv2.remap(image, self.mapX, self.mapY, cv2.INTER_LINEAR)

        return image
