# -*- coding: utf-8 -*-
"""
Created on Thu Dec  4 10:54:57 2014

@author: pbarnum
"""

import cv2
import os

def saveStereo(leftImageRaw, rightImageRaw, leftImageRectified, rightImageRectified, baseFilename):
    if "saveIndex" not in saveStereo.__dict__:
        saveStereo.saveIndex = 0

    while True:
        leftImageFilename = baseFilename + 'left_' + str(saveStereo.saveIndex) + '.png'
        rightImageFilename = baseFilename + 'right_' + str(saveStereo.saveIndex) + '.png'
        leftRectifiedImageFilename = baseFilename + 'leftRectified_' + str(saveStereo.saveIndex) + '.png'
        rightRectifiedImageFilename = baseFilename + 'rightRectified_' + str(saveStereo.saveIndex) + '.png'
        saveStereo.saveIndex += 1

        if (not os.path.isfile(leftImageFilename)) and\
           (not os.path.isfile(rightImageFilename)) and\
           (not os.path.isfile(leftRectifiedImageFilename)) and\
           (not os.path.isfile(rightRectifiedImageFilename)):
                break

    savedString = 'Saved to '

    if leftImageRaw is not None:
        cv2.imwrite(leftImageFilename, leftImageRaw)
        savedString += leftImageFilename + ' '

    if rightImageRaw is not None:
        cv2.imwrite(rightImageFilename, rightImageRaw)
        #savedString += rightImageFilename + ' '

    if leftImageRectified is not None :
        cv2.imwrite(leftRectifiedImageFilename, leftImageRectified)
        #savedString += leftRectifiedImageFilename + ' '

    if rightImageRectified is not None:
        cv2.imwrite(rightRectifiedImageFilename, rightImageRectified)
        #savedString += rightRectifiedImageFilename + ' '

    print(savedString)
