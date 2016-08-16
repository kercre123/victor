#!/usr/bin/env python3

"""
Interface for any GUI image viewer capable of consuming and displaying images from imageManager.py
"""
__author__ = "Mark Wesley"


class IImageViewer:
    "Abstract Base Interface for all Image Viewers"

    def __init__(self):
        pass

    def DisplayImage(self, nextImage):
        "Display nextImage at the next convenient moment"
        raise NotImplementedError
