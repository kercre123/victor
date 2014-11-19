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
    occlusionGridSize = {'x':40, 'y':40}

    hogBlockSize = {'x':40, 'y':40}
    hogCellSize = {'x':40, 'y':40}
    hogNumBins = 3

    #sparseLk = {'numPointsPerDimension':10, 'blockSizes':[(15,15),(7,7)]}
    sparseLk = {'numPointsPerDimension':10, 'blockSizes':[(15,15)]}

    def __init__(self, templateImage):
        assert type(templateImage).__module__ == np.__name__, 'Must be a Numpy array'
        assert len(templateImage.shape) == 2, 'Must be 2D'

        templateImage = cv2.equalizeHist(templateImage)

        self.templateImage = templateImage
        self.detector = cv2.ORB()
        self.matcher = cv2.BFMatcher(cv2.NORM_HAMMING, crossCheck=True)
        self.keypoints, self.descriptors = self.detector.detectAndCompute(self.templateImage, None)

        self.templateDescriptors = {}
        self.templateDescriptors['hog'] = self.__computeBlockDescriptor(templateImage, 'hog')
        self.templateDescriptors['raw'] = self.__computeBlockDescriptor(templateImage, 'raw')

        self.sparseLkPoints = []
        for y in linspace(0, templateImage.shape[0], PlanePattern.sparseLk['numPointsPerDimension']):
            for x in linspace(0, templateImage.shape[1], PlanePattern.sparseLk['numPointsPerDimension']):
                self.sparseLkPoints.append((x,y))

        self.sparseLkPoints = array(self.sparseLkPoints).reshape(-1,1,2).astype('float32')

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
                    des = curBlock.flatten().astype('float32')
                    des /= len(des)

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
            # First, use the sparse feature matches to compute a rough homography alignment
            matchedArray_original = zeros((len(matched_template),2))
            matchedArray_query = zeros((len(matched_template),2))

            for i in range(0,len(matched_template)):
                matchedArray_original[i,:] = matched_template[i].pt
                matchedArray_query[i,:] = matched_query[i].pt

            try:
                H1 = cv2.findHomography( matchedArray_query, matchedArray_original, cv2.RANSAC )
                numInliers = H1[1].sum()
                H1 = H1[0];
            except:
                H1 = eye(3)

            H = H1

            #print(H)

            # Iterate a few times
            for iteration in range(0,len(PlanePattern.sparseLk['blockSizes'])):
                warpedQueryImage1 = cv2.warpPerspective(queryImage, H, queryImage.shape[::-1])
                warpedQueryImage1 = cv2.equalizeHist(warpedQueryImage1)

                #cv2.imshow('warpedQueryImageB' + str(iteration), warpedQueryImage1)

                # Second, use sparse translational LK to get a good alignment

                nextPoints, status, err = cv2.calcOpticalFlowPyrLK(
                    self.templateImage,
                    warpedQueryImage1,
                    self.sparseLkPoints,
                    None, None, None, PlanePattern.sparseLk['blockSizes'][iteration], maxLevel=3)

                validInds = nonzero(status==1)[0]

                #pdb.set_trace()

                #TODO: pick the right value
                if len(validInds) < 5:
                    break

                try:
                    H2 = cv2.findHomography( nextPoints.reshape(-1,2)[validInds,:], self.sparseLkPoints.reshape(-1,2)[validInds,:], cv2.RANSAC )
                    numInliers = H2[1].sum()
                    H2 = H2[0];
                except:
                    H2 = eye(3)
                    break

                H = array(matrix(H2)*matrix(H))
                H = H / H[2,2]

            #pdb.set_trace()

        #print(H)
        #print(' ')

        # Compute the occlusion map

        warpedQueryImage2 = cv2.warpPerspective(queryImage, H, queryImage.shape[::-1])
        warpedQueryImage2 = cv2.equalizeHist(warpedQueryImage2)

        #descriptorType = 'hog'
        descriptorType = 'raw'

        queryDescriptor = self.__computeBlockDescriptor(warpedQueryImage2, descriptorType)

        occlusionMap = zeros((len(queryDescriptor), len(queryDescriptor[0])))
        for y, (templateLine, queryLine) in enumerate(zip(self.templateDescriptors[descriptorType], queryDescriptor)):
            for x, (templateElement, queryElement) in enumerate(zip(templateLine, queryLine)):
                #occlusionMap[y,x] = cv2.compareHist(templateElement, queryElement, cv2.cv.CV_COMP_CORREL)
                occlusionMap[y,x] = abs(templateElement-queryElement).sum()

        if descriptorType == 'hog':
            occlusionMap = pow(occlusionMap, 0.5)
            occlusionMap *= 255

        assert occlusionMap.max() < 255.1, 'oops'

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
            cv2.imshow('matches', imgKeypointsBoth)
            cv2.imshow('templateImage', self.templateImage)
            cv2.imshow('warpedQueryImage', warpedQueryImage2)

            # Draw the occlusion image
            occlusionMap = cv2.resize(occlusionMap, queryImage.shape[::-1], interpolation=cv2.INTER_NEAREST)
            cv2.imshow('occlusionMapU8', occlusionMap)

        return H, numInliers


