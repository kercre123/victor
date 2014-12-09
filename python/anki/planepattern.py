# -*- coding: utf-8 -*-
"""
Created on Mon Nov 17 13:20:16 2014

Holds an image of a planar pattern, and matches an input image to it

@author: pbarnum
"""

# Use python 3 types by default (uses "future" package)
# WARNING: missing unicode_literals, because it conflicts with opencv
from __future__ import (absolute_import, division,
                        print_function)
from future.builtins import *

import cv2
import numpy as np
from numpy import *
from numpy.linalg import inv
import pdb
from anki.ransac import computeHomography
from anki.geometry import Quadrilateral

import matplotlib.pyplot as plt
import matplotlib.cm as cm

class PlanePattern(object):
    occlusionGridSize = {'x':40, 'y':40}

    hogBlockSize = {'x':40, 'y':40}
    hogCellSize = {'x':40, 'y':40}
    hogNumBins = 3

    #sparseLk = {'numPointsPerDimension':10, 'blockSizes':[(15,15),(7,7)]}
    #sparseLk = {'numPointsPerDimension':50, 'blockSizes':[(15,15)]}
    sparseLk = {'numPointsPerDimension':25, 'blockSizes':[]}

    useSIFTmatching = False

    def __init__(self, templateImage):
        assert type(templateImage).__module__[:5] == np.__name__, 'Must be a Numpy array'
        assert len(templateImage.shape) == 2, 'Must be 2D'

        templateImage = cv2.equalizeHist(templateImage)

        self.templateImage = templateImage

        if self.useSIFTmatching:
            self.detector = cv2.SIFT()

            index_params = {'algorithm':0, 'trees':5}
            search_params = {'checks':50}
            self.matcher = cv2.FlannBasedMatcher(index_params, search_params)
        else:
            # Default parameters, same at self.detector = cv2.ORB()
            # self.detector = cv2.ORB(nfeatures=500, scaleFactor=1.2, nlevels=8, edgeThreshold=31, firstLevel=0, WTA_K=2, patchSize=31)

            #self.detector = cv2.ORB(nfeatures=400, scaleFactor=1.2, nlevels=8, edgeThreshold=31, firstLevel=0, WTA_K=2, patchSize=31)
            self.detector = cv2.ORB(nfeatures=1000, scaleFactor=1.2, nlevels=8, edgeThreshold=31, firstLevel=0, WTA_K=2)


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

    def match(self, queryImage, bruteForceMatchThreshold, preMatchHomographies=[eye(3)], displayMatches=False):
        assert type(queryImage).__module__[:5] == np.__name__, 'queryImage must be a numpy array'
        assert len(queryImage.shape) == 2, 'queryImage must be 2D'
        assert queryImage.shape == self.templateImage.shape, 'template and query must be the same size'

        # Compute matches between query and template

        matched_template = []
        matched_query = []

        numMatchedPoints = []

        for preMatchHomography in preMatchHomographies:
            assert type(preMatchHomography).__module__[:5] == np.__name__, 'Must be a numpy array'

            if all(preMatchHomography == eye(3)) == True:
                queryImageWarped = queryImage
            else:
                queryImageWarped = cv2.warpPerspective(queryImage, preMatchHomography, queryImage.shape[::-1])

            inversePreMatchHomography = inv(preMatchHomography)

            queryImageWarped = cv2.equalizeHist(queryImageWarped)
            queryKeypoints, queryDescriptors = self.detector.detectAndCompute(queryImageWarped, None)

            cur_matched_template = []
            cur_matched_query = []

            if self.useSIFTmatching:
                matches = self.matcher.knnMatch(queryDescriptors, self.descriptors, k=2)

                # Need to draw only good matches, so create a mask
                matchesMask = [[0,0] for i in xrange(len(matches))]

                # ratio test as per Lowe's paper
                for i,(m,n) in enumerate(matches):
                    if m.distance < 0.7*n.distance:
                        matchesMask[i]=[1,0]

                        cur_matched_template.append(self.keypoints[m.trainIdx])
                        cur_matched_query.append(queryKeypoints[m.queryIdx])
            else:
                matches = self.matcher.match(queryDescriptors, self.descriptors)
                matches = sorted(matches, key = lambda x:x.distance)

                for i,m in enumerate(matches):
                    if m.distance > bruteForceMatchThreshold:
                        break;

                    cur_matched_template.append(self.keypoints[m.trainIdx])
                    cur_matched_query.append(queryKeypoints[m.queryIdx])

            #print('Matched ' + str(len(cur_matched_template)) + ' points')
            numMatchedPoints.append(len(cur_matched_template))

            # Inverse warp all points to their non-preMatchHomography positions
            for (template, query) in zip(cur_matched_template, cur_matched_query):
                queryPoint = inversePreMatchHomography * matrix([query.pt[0], query.pt[1], 1]).transpose()
                queryPoint = queryPoint / queryPoint[2]

                warpedQuery = cv2.KeyPoint(x=queryPoint.tolist()[0][0],
                                           y=queryPoint.tolist()[1][0],
                                           _size=query.size,
                                           _angle=query.angle,
                                           _response=query.response,
                                           _octave=query.octave,
                                           _class_id=query.class_id)

                matched_template.append(template)
                matched_query.append(warpedQuery)

        # Remove points that were detected multiple times
        matched_template_good = []
        matched_query_good = []
        closenessThreshold = 2
        for i1, (template1,query1) in enumerate(zip(matched_template,matched_query)):
            closeMatchId = -1
            for i2, (template2,query2) in enumerate(zip(matched_template,matched_query)):
                if i1 == i2:
                    continue

                if sqrt(pow(query1.pt[0] - query2.pt[0], 2) + pow(query1.pt[1] - query2.pt[1], 2)) < closenessThreshold:
                    closeMatchId = i2
                    break

            if closeMatchId == -1 or closeMatchId > i1:
                matched_template_good.append(template1)
                matched_query_good.append(query1)

        #pdb.set_trace()
        print('Matched ' + str(numMatchedPoints) + ' = ' + str(array(numMatchedPoints).sum()) + ' and kept ' + str(len(matched_template_good)))

        matched_template = matched_template_good
        matched_query = matched_query_good

        H = eye(3)
        originalH = eye(3)
        numInliers = 0

        # Compute the homography between query and template

        lkErrorMap = 255*ones(queryImage.shape, 'uint8')

        if len(matched_template) >= 4:
            # First, use the sparse feature matches to compute a rough homography alignment
            matchedArray_original = zeros((len(matched_template),2))
            matchedArray_query = zeros((len(matched_template),2))

            for i in range(0,len(matched_template)):
                matchedArray_original[i,:] = matched_template[i].pt
                matchedArray_query[i,:] = matched_query[i].pt

            useOpenCvRansac = True

            #try:
            if useOpenCvRansac:
                H1 = cv2.findHomography( matchedArray_query, matchedArray_original, cv2.RANSAC, ransacReprojThreshold = 10)
                numInliers = H1[1].sum()
                H1 = H1[0];
            else:
                # TODO: pick reasonable thresholds
                H1, inliners = computeHomography(
                        matchedArray_query,
                        matchedArray_original,
                        numIterations=1000,
                        reprojectionThreshold=1,
                        templateSize=self.templateImage.shape,
                        quadOppositeSideRatioRange=[0.9, 3],
                        quadAdjacentSideRatioRange=[0.9, 3])

                numInliers = len(inliners)

            print('numInliers = ' + str(numInliers))
            print(H1)
            #except:
               # pdb.set_trace()
               # H1 = eye(3)

            H = H1
            originalH = H.copy()

            # Iterate a few times
            for iteration in range(0,len(PlanePattern.sparseLk['blockSizes'])):
                warpedQueryImage1 = cv2.warpPerspective(queryImage, H, queryImage.shape[::-1])
                warpedQueryImage1 = cv2.equalizeHist(warpedQueryImage1)

                warpedTemplateSparseLkPoints = (H) * concatenate((matrix(self.sparseLkPoints), ones((self.sparseLkPoints.shape[0],1))), axis=1).transpose()
                warpedTemplateSparseLkPoints = (warpedTemplateSparseLkPoints[0:2,:] / tile(warpedTemplateSparseLkPoints[2,:], (2,1))).transpose()

                validTemplateSparseLkPoints = []
                for i in range(0, warpedTemplateSparseLkPoints.shape[0]):
                    if (warpedTemplateSparseLkPoints[i,0] > PlanePattern.sparseLk['blockSizes'][iteration][0]/2) and\
                       (warpedTemplateSparseLkPoints[i,0] < (queryImage.shape[1]-PlanePattern.sparseLk['blockSizes'][iteration][0]/2)) and\
                       (warpedTemplateSparseLkPoints[i,1] > PlanePattern.sparseLk['blockSizes'][iteration][1]/2) and\
                       (warpedTemplateSparseLkPoints[i,1] < (queryImage.shape[0]-PlanePattern.sparseLk['blockSizes'][iteration][1]/2)):
                           validTemplateSparseLkPoints.append(i)
                           #pdb.set_trace()

                print(len(validTemplateSparseLkPoints))

                validTemplateSparseLkPoints = self.sparseLkPoints[validTemplateSparseLkPoints,:,:]

                #plt.ion()
                #plt.scatter(validTemplateSparseLkPoints[:,:,0], validTemplateSparseLkPoints[:,:,1])
                #plt.show()

                #pdb.set_trace()

                #cv2.imshow('warpedQueryImageB' + str(iteration), warpedQueryImage1)

                # Second, use sparse translational LK to get a good alignment

                nextPoints, status, err = cv2.calcOpticalFlowPyrLK(
                    self.templateImage,
                    warpedQueryImage1,
                    #self.sparseLkPoints,
                    validTemplateSparseLkPoints,
                    None, None, None, PlanePattern.sparseLk['blockSizes'][iteration], maxLevel=3)

                validInds = nonzero(status==1)[0]

                #err[nonzero(status==0)[0]] = 255
                #lkErrorMap = err.astype('uint8').reshape(self.sparseLk['numPointsPerDimension'], self.sparseLk['numPointsPerDimension'])
                #lkErrorMap = cv2.resize(lkErrorMap, queryImage.shape[::-1], interpolation=cv2.INTER_NEAREST)

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
            occlusionMap *= 255

        occlusionMap = 255*pow(occlusionMap/255, 0.5)

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

            if numInliers >= 4:
                quadColor = (0,255,0)
            else:
                quadColor = (0,0,255)

            warpedPoints = H * originalPoints
            warpedPoints = warpedPoints[0:2,:] / tile(warpedPoints[2,:], (2,1))
            warpedQuad = Quadrilateral(warpedPoints)
            warpedQuad.draw(imgKeypointsBoth, color=quadColor, thickness=3, offset=(0,0))

            warpedPoints = originalH * originalPoints
            warpedPoints = warpedPoints[0:2,:] / tile(warpedPoints[2,:], (2,1))
            warpedQuad = Quadrilateral(warpedPoints)
            warpedQuad.draw(imgKeypointsBoth, color=(255,0,0), thickness=3, offset=(0,0))

            #pdb.set_trace()

#            cv2.line(imgKeypointsBoth, warpedPoints[0], warpedPoints[1], quadColor, 3)
#            cv2.line(imgKeypointsBoth, warpedPoints[1], warpedPoints[2], quadColor, 3)
#            cv2.line(imgKeypointsBoth, warpedPoints[2], warpedPoints[3], quadColor, 3)
#            cv2.line(imgKeypointsBoth, warpedPoints[3], warpedPoints[0], quadColor, 3)

            # Draw keypoint matches
            for key1, key2 in zip(matched_template, matched_query):
                pt1 = (int(round(key1.pt[0])), int(round(key1.pt[1])))
                pt2 = (int(round(key2.pt[0]))+imgKeypoints1.shape[1], int(round(key2.pt[1])))

                cv2.line(imgKeypointsBoth, pt1, pt2, (255,0,0))

            # Show the drawn image
            cv2.imshow('matches', imgKeypointsBoth)
            #cv2.imshow('templateImage', self.templateImage)
            cv2.imshow('warpedQueryImage', warpedQueryImage2)
            #cv2.imshow('lkErrorMap', lkErrorMap)

            # Draw the occlusion image
            occlusionMap = cv2.resize(occlusionMap, queryImage.shape[::-1], interpolation=cv2.INTER_NEAREST)
            #cv2.imshow('occlusionMapU8', occlusionMap)

        return H, numInliers


