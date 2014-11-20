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

def computeHomography(queryKeypoints, templateKeypoints, numIterations, reprojectionThreshold):
    assert type(queryKeypoints).__module__ == np.__name__, 'Must be a numpy array'
    assert type(templateKeypoints).__module__ == np.__name__, 'Must be a numpy array'
    assert len(queryKeypoints.shape) == 2, 'Must be 2D'
    assert queryKeypoints.shape == templateKeypoints.shape, 'Must be the same size'
    assert queryKeypoints.shape[1] == 2, 'Must be nx2'
    assert queryKeypoints.shape[0] >= 4, 'Need at least 4 points'
    assert reprojectionThreshold >= 0, 'reprojectionThreshold must be positive'
    assert numIterations > 0, 'iterations must be greater than zero'

    queryKeypoints = matrix(queryKeypoints)
    templateKeypoints = matrix(templateKeypoints)

    queryKeypointsWith1 = concatenate((queryKeypoints, ones((queryKeypoints.shape[0],1))), axis=1).transpose()
    #templateKeypointsWith1 = concatenate((templateKeypoints, ones((templateKeypoints.shape[0],1))), axis=1).transpose()

    numKeypoints = queryKeypoints.shape[0]

    bestH = eye(3)
    bestHInlierIndexes = []

    for iteration in range(0, numIterations):
        sampledIndexes = random.sample(range(0,numKeypoints), 4)

        templateSubset = array(queryKeypoints[sampledIndexes, :]).copy()
        querySubset = array(templateKeypoints[sampledIndexes, :]).copy()

        try:
            H = cv2.findHomography(templateSubset, querySubset, 0)
            H = matrix(H[0]);
            warpedQuery = H * queryKeypointsWith1
            warpedQuery = (warpedQuery[0:2,:] / tile(warpedQuery[2,:], (2,1))).transpose()
        except:
            continue

        distance = sqrt(power(warpedQuery[:,0] - templateKeypoints[:,0],2) + power(warpedQuery[:,1] - templateKeypoints[:,1],2))

        inlierIndexes = nonzero(distance <= reprojectionThreshold)[0].tolist()[0]

        #print('Iteration ' + str(iteration) + ' has ' + str(len(inlierIndexes)) + ' inliers')

#        if len(inlierIndexes) > 0:
#            pdb.set_trace()

        if len(inlierIndexes) > len(bestHInlierIndexes):
            bestH = H
            bestHInlierIndexes = inlierIndexes

        # TODO: check for early termination

    if len(bestHInlierIndexes) >= 4:
        templateSubset = array(queryKeypoints[bestHInlierIndexes, :]).copy()
        querySubset = array(templateKeypoints[bestHInlierIndexes, :]).copy()

        #pdb.set_trace()
        bestH = cv2.findHomography(templateSubset, querySubset, 0)
        bestH = matrix(bestH[0]);

    return bestH, bestHInlierIndexes






