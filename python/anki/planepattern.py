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
from numpy import *
from numpy.linalg import inv
import pdb

class PlanePattern(object):
    occlusionGridSize = {'x':80, 'y':80}

    hogBlockSize = {'x':80, 'y':80}
    hogCellSize = {'x':80, 'y':80}
    hogNumBins = 5

    def __init__(self, templateImage):
        assert type(templateImage).__module__ == np.__name__, 'Must be a Numpy array'
        assert len(templateImage.shape) == 2, 'Must be 2D'

        self.templateImage = templateImage
        self.detector = cv2.ORB()
        self.matcher = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
        self.keypoints, self.descriptors = self.detector.detectAndCompute(self.templateImage, None)
       
        self.templateDescriptors = {}       
        self.templateDescriptors['hog'] = self.__computeBlockDescriptor(templateImage, 'hog')
        self.templateDescriptors['raw'] = self.__computeBlockDescriptor(templateImage, 'raw')

    def __computeBlockDescriptor(self, image, descriptorType):
        
        if descriptorType.lower() == 'hog':
            hog = cv2.HOGDescriptor(
                _winSize=(self.occlusionGridSize['x'], self.occlusionGridSize['y']),
                _blockSize=(self.hogBlockSize['x'], self.hogBlockSize['y']),
                _blockStride=(self.hogCellSize['x'], self.hogCellSize['y']),
                _cellSize=(self.hogCellSize['x'], self.hogCellSize['y']),
                _nbins=self.hogNumBins)

        blockDescriptor = []

        for y in range(0, image.shape[0], PlanePattern.occlusionGridSize['y']):
            blockDescriptorLine = []
            for x in range(0, image.shape[1], PlanePattern.occlusionGridSize['x']):
                curBlock = image[y:(y+PlanePattern.occlusionGridSize['y']), x:(x+PlanePattern.occlusionGridSize['x'])]
                
                if descriptorType.lower() == 'hog':                
                    des = hog.compute(curBlock)
                elif descriptorType.lower() == 'raw':  
                    des = curBlock.flatten()

                blockDescriptorLine.append(des)

            blockDescriptor.append(blockDescriptorLine)

        return blockDescriptor

    def match(self, queryImage, bruteForceMatchThreshold, displayMatches=False):
        assert type(queryImage).__module__ == np.__name__, 'queryImage must be a numpy array'
        assert len(queryImage.shape) == 2,'queryImage must be 2D'
        assert queryImage.shape == self.templateImage.shape, 'template and query must be the same size'

        queryKeypoints, queryDescriptors = self.detector.detectAndCompute(queryImage, None)

        # Compute matches between query and template

        matches = self.matcher.match(queryDescriptors, self.descriptors)
        matches = sorted(matches, key = lambda x:x.distance)

        matched_template = []
        matched_query = []
        for i,m in enumerate(matches):
            if m.distance > bruteForceMatchThreshold:
                break;

            matched_template.append(self.keypoints[m.trainIdx])
            matched_query.append(queryKeypoints[m.queryIdx])

        H = eye(3)
        numInliers = 0

        # Compute the homography between query and template

        if len(matched_template) >= 4:
            matchedArray_original = zeros((len(matched_template),2))
            matchedArray_query = zeros((len(matched_template),2))

            for i in range(0,len(matched_template)):
                matchedArray_original[i,:] = matched_template[i].pt
                matchedArray_query[i,:] = matched_query[i].pt

            try:
                H = cv2.findHomography( matchedArray_query, matchedArray_original, cv2.RANSAC )
                numInliers = H[1].sum()
                H = H[0];
            except:
                H = eye(3)

        # Compute the occlusion map

        warpedQueryImage = cv2.warpPerspective(queryImage, H, queryImage.shape[::-1])
        queryDescriptor = self.__computeBlockDescriptor(warpedQueryImage, 'hog')

        occlusionMap = zeros((len(queryDescriptor), len(queryDescriptor[0])))
        for y, (templateLine, queryLine) in enumerate(zip(self.templateDescriptors['hog'], queryDescriptor)):
            for x, (templateElement, queryElement) in enumerate(zip(templateLine, queryLine)):
                #occlusionMap[y,x] = cv2.compareHist(templateElement, queryElement, cv2.cv.CV_COMP_CORREL)
                occlusionMap[y,x] = abs(templateElement-queryElement).sum() / len(templateElement)

        occlusionMap = pow(occlusionMap, 0.5)

        occlusionMap *= 255

        occlusionMap = occlusionMap.astype('uint8')

        # Optionally, display the result

        if displayMatches:
            # Draw the image with keypoints
            imgKeypoints1 = cv2.drawKeypoints(self.templateImage, matched_template)
            imgKeypoints2 = cv2.drawKeypoints(queryImage, matched_query)

            imgKeypointsBoth = concatenate((imgKeypoints1,imgKeypoints2), axis=1)
            imgKeypointsBoth = imgKeypointsBoth.copy()

            # Draw the warped quadrilateral
            originalPoints = matrix(
                [(0,0),
                 (queryImage.shape[1],0),
                 (queryImage.shape[1],queryImage.shape[0]),
                 (0,queryImage.shape[0])])


            originalPoints = concatenate((originalPoints, ones((4,1))), axis=1).transpose()

            warpedPoints = inv(H) * originalPoints
            warpedPoints = warpedPoints[0:2,:] / tile(warpedPoints[2,:], (2,1))
            warpedPoints[0,:] += imgKeypoints1.shape[1]
            warpedPoints = warpedPoints.astype('int32').transpose().tolist()
            warpedPoints = [tuple(x) for x in warpedPoints]

            if numInliers >= 4:
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
            cv2.imshow('warpedQueryImage', warpedQueryImage)
            warpedQueryImage

            # Draw the occlusion image
            occlusionMap = cv2.resize(occlusionMap, queryImage.shape[::-1], interpolation=cv2.INTER_NEAREST)
            cv2.imshow('occlusionMapU8', occlusionMap)

        return H, numInliers


