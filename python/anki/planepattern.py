# -*- coding: utf-8 -*-
"""
Created on Mon Nov 17 13:20:16 2014

Holds an image of a planar pattern, and matches an input image to it

@author: pbarnum
"""

# Use python 3 types by default (uses "future" package)
from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from future.builtins import *

import cv2
import numpy as np
from numpy.linalg import inv

class PlanePattern(object):
    def __init__(self, templateImage):
        assert type(templateImage).__module__ == np.__name__, "Must be a Numpy array"
        assert len(templateImage.shape) == 2, "Must be 2D"

        self.templateImage = templateImage
        self.detector = cv2.ORB()
        self.matcher = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
        self.keypoints, self.descriptors = self.detector.detectAndCompute(self.templateImage, None)

    def match(self, queryImage, bruteForceMatchThreshold, displayMatches=False):
        assert type(queryImage).__module__ == np.__name__, "queryImage must be a numpy array"
        assert len(queryImage.shape) == 2,"queryImage must be 2D"

        queryKeypoints, queryDescriptors = self.detector.detectAndCompute(queryImage, None)

        matches = self.matcher.match(queryDescriptors, self.descriptors)
        matches = sorted(matches, key = lambda x:x.distance)

        matched_template = []
        matched_query = []
        for i,m in enumerate(matches):
            if m.distance > bruteForceMatchThreshold:
                break;

            matched_template.append(self.keypoints[m.trainIdx])
            matched_query.append(queryKeypoints[m.queryIdx])

        H = np.eye(3)
        foundGoodMatch = False

        if len(matched_template) >= 4:
            matchedArray_original = np.zeros((len(matched_template),2))
            matchedArray_query = np.zeros((len(matched_template),2))

            for i in range(0,len(matched_template)):
                matchedArray_original[i,:] = matched_template[i].pt
                matchedArray_query[i,:] = matched_query[i].pt

            try:
                H = cv2.findHomography( matchedArray_query, matchedArray_original, cv2.RANSAC )
                H = H[0];
                foundGoodMatch = True
            except:
                H = np.eye(3)

        if displayMatches:
            # Draw the image with keypoints
            imgKeypoints1 = cv2.drawKeypoints(self.templateImage, matched_template)
            imgKeypoints2 = cv2.drawKeypoints(queryImage, matched_query)

            imgKeypointsBoth = np.concatenate((imgKeypoints1,imgKeypoints2), axis=1)
            imgKeypointsBoth = imgKeypointsBoth.copy()

            # Draw the warped quadrilateral
            originalPoints = np.matrix(
                [(0,0),
                 (queryImage.shape[1],0),
                 (queryImage.shape[1],queryImage.shape[0]),
                 (0,queryImage.shape[0])])


            originalPoints = np.concatenate((originalPoints, np.ones((4,1))), axis=1).transpose()

            warpedPoints = inv(H) * originalPoints
            warpedPoints = warpedPoints[0:2,:] / np.tile(warpedPoints[2,:], (2,1))
            warpedPoints[0,:] += imgKeypoints1.shape[1]
            warpedPoints = warpedPoints.astype('int32').transpose().tolist()
            warpedPoints = [tuple(x) for x in warpedPoints]

            if foundGoodMatch:
                quadColor = (0,255,0)
            else:
                quadColor = (0,0,255)

            cv2.line(imgKeypointsBoth, warpedPoints[0], warpedPoints[1], quadColor, 3)
            cv2.line(imgKeypointsBoth, warpedPoints[1], warpedPoints[2], quadColor, 3)
            cv2.line(imgKeypointsBoth, warpedPoints[2], warpedPoints[3], quadColor, 3)
            cv2.line(imgKeypointsBoth, warpedPoints[3], warpedPoints[0], quadColor, 3)

            # Draw keypoint matches
            for key1, key2 in zip(matched_template, matched_query):
                pt1 = (int(round(key1.pt[0])), int(round(key1.pt[1])))
                pt2 = (int(round(key2.pt[0]))+imgKeypoints1.shape[1], int(round(key2.pt[1])))

                cv2.line(imgKeypointsBoth, pt1, pt2, (255,0,0))

            # Show the drawn image
            cv2.imshow('query', imgKeypointsBoth)




