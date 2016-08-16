#!/usr/bin/env python3

"""
OpenCV cv2 implementation of IImageViewer
"""
__author__ = "Mark Wesley"


from iImageViewer import IImageViewer
import cv2
import numpy as np


class ImageViewerCv(IImageViewer):

    def __init__(self, imageScale = 2, windowName = "CozmoView"):
        IImageViewer.__init__(self)
        self.scale = imageScale
        self.windowName = windowName

    def __del__(self):
        pass

    def DisplayImage(self, nextImage, displayTime=50):

        arr = np.array(nextImage)
        # Numpy's non-empty check
        if arr is None:
            return None

        if self.scale != 1:
            width  = nextImage.width
            height = nextImage.height

            resized = cv2.resize(arr,(self.scale*width, self.scale*height), interpolation = cv2.INTER_CUBIC)
        else:
            resized = arr

        cv2.imshow(self.windowName, resized)
        return cv2.waitKey(displayTime)

