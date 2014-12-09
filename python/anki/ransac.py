# -*- coding: utf-8 -*-
"""
Created on Wed Nov 19 16:23:00 2014

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
import random
import pdb
from anki.geometry import Quadrilateral

numFailures = {'nonConvex':0, 'quadOppositeRatio':0, 'quadAdjacentRatio':0, 'quadArea':0}

def isHomographyValid(
        homography,
        reprojectionThreshold,
        templateSize,
        quadOppositeSideRatioRange,
        quadAdjacentSideRatioRange):
    """
    Use some heuristics to determine if the homography is reasonable
    """

    return True

    templateQuad = array([[0,0], [templateSize[0],0], [templateSize[0],templateSize[1]], [0,templateSize[1]]])
    templateQuad = concatenate((templateQuad, ones((4,1))), axis=1).transpose()

    # Check if warped quad is convex
    warpedTemplateQuad = homography * templateQuad
    warpedTemplateQuad = (warpedTemplateQuad[0:2,:] / tile(warpedTemplateQuad[2,:], (2,1))).transpose()
    warpedTemplateQuad = Quadrilateral(warpedTemplateQuad)

#    if not warpedTemplateQuad.isConvex():
#        numFailures['nonConvex'] += 1
#        return False

    # Check ratios of sides, to make sure it's reasonable
    # Check the quad area
    warpedTemplateQuadCorners = warpedTemplateQuad.computeClockwiseCorners()
    sideLengths = []
    for i1 in range(0,4):
        i2 = (i1+1) % 4
        dist = array(warpedTemplateQuadCorners[i1]) - array(warpedTemplateQuadCorners[i2])
        dist = sqrt(pow(dist[0],2) + pow(dist[1],2))
        sideLengths.append(dist)

    quadOppositeSideRatios = []
    quadAdjacentSideRatios = []
    for iBase in range(0,4):
        iOpposite = (iBase+2) % 4
        iAdjacent = (iBase+1) % 4

        opposite1 = sideLengths[iBase] / sideLengths[iOpposite]
        opposite2 = sideLengths[iOpposite] / sideLengths[iBase]

        adjacent1 = sideLengths[iBase] / sideLengths[iAdjacent]
        adjacent2 = sideLengths[iAdjacent] / sideLengths[iBase]

        quadOppositeSideRatios.append(max(opposite1, opposite2))
        quadAdjacentSideRatios.append(max(adjacent1, adjacent2))

    if min(quadOppositeSideRatios) < min(quadOppositeSideRatioRange):
        numFailures['quadOppositeRatio'] += 1
        return False

    if max(quadOppositeSideRatios) > max(quadOppositeSideRatioRange):
        numFailures['quadOppositeRatio'] += 1
        return False

    if min(quadAdjacentSideRatios) < min(quadAdjacentSideRatioRange):
        numFailures['quadAdjacentRatio'] += 1
        return False

    if max(quadAdjacentSideRatios) > max(quadAdjacentSideRatioRange):
        numFailures['quadAdjacentRatio'] += 1
        return False

    return True

def computeHomography(
        queryKeypoints,
        templateKeypoints,
        numIterations,
        reprojectionThreshold,
        templateSize,
        quadOppositeSideRatioRange,
        quadAdjacentSideRatioRange):

    assert type(queryKeypoints).__module__[:5] == np.__name__, 'Must be a numpy array'
    assert type(templateKeypoints).__module__[:5] == np.__name__, 'Must be a numpy array'
    assert len(queryKeypoints.shape) == 2, 'Must be 2D'
    assert queryKeypoints.shape == templateKeypoints.shape, 'Must be the same size'
    assert queryKeypoints.shape[1] == 2, 'Must be nx2'
    assert queryKeypoints.shape[0] >= 4, 'Need at least 4 points'
    assert reprojectionThreshold >= 0, 'reprojectionThreshold must be positive'
    assert numIterations > 0, 'iterations must be greater than zero'
    assert len(templateSize) == 2

    queryKeypoints = matrix(queryKeypoints)
    templateKeypoints = matrix(templateKeypoints)

    queryKeypointsWith1 = concatenate((queryKeypoints, ones((queryKeypoints.shape[0],1))), axis=1).transpose()
    #templateKeypointsWith1 = concatenate((templateKeypoints, ones((templateKeypoints.shape[0],1))), axis=1).transpose()

    numKeypoints = queryKeypoints.shape[0]

    bestH = eye(3)
    bestHInlierIndexes = []

    for key in numFailures.keys():
        numFailures[key] = 0

    for iteration in range(0, numIterations):
        sampledIndexes = random.sample(range(0,numKeypoints), 4)

        templateSubset = array(queryKeypoints[sampledIndexes, :]).copy()
        querySubset = array(templateKeypoints[sampledIndexes, :]).copy()

        try:
            H = cv2.findHomography(templateSubset, querySubset, 0)
            H = matrix(H[0]);
        except:
            continue

        # Check validity heuristics
        if not isHomographyValid(H, reprojectionThreshold, templateSize, quadOppositeSideRatioRange, quadAdjacentSideRatioRange):
            continue

        # Count the number of inliers
        warpedQuery = H * queryKeypointsWith1
        warpedQuery = (warpedQuery[0:2,:] / tile(warpedQuery[2,:], (2,1))).transpose()

        distance = sqrt(power(warpedQuery[:,0] - templateKeypoints[:,0],2) + power(warpedQuery[:,1] - templateKeypoints[:,1],2))

        inlierIndexes = nonzero(distance <= reprojectionThreshold)[0].tolist()[0]

        #print('Iteration ' + str(iteration) + ' has ' + str(len(inlierIndexes)) + ' inliers')

        if len(inlierIndexes) > len(bestHInlierIndexes):
            bestH = H
            bestHInlierIndexes = inlierIndexes

        # TODO: check for early termination

#    if len(bestHInlierIndexes) >= 4:
#        templateSubset = array(queryKeypoints[bestHInlierIndexes, :]).copy()
#        querySubset = array(templateKeypoints[bestHInlierIndexes, :]).copy()
#
#        #pdb.set_trace()
#        bestH = cv2.findHomography(templateSubset, querySubset, 0)
#        bestH = matrix(bestH[0]);
#
##        if not isHomographyValid(bestH, reprojectionThreshold, templateSize, quadOppositeSideRatioRange, quadAdjacentSideRatioRange):
##            print('Total estimation failure')
##            bestH = eye(3)
##            bestHInlierIndexes = []

    print(numFailures)

    return bestH, bestHInlierIndexes






