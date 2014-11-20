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
import math

def cart2pol(x, y):
    if (x==0) and (y==0):
        rho = 0
        theta = 0
    else:
        rho = sqrt(x*x + y*y)
        theta = math.atan2(y, x)

    return rho, theta

class Quadrilateral(object):
    """
    A quad is a list of corners, [(x0,y0), (x1,y1), (x2,y2), (x3,y3)]
    """

    def __init__(self, corners):
        """
        Initialize either with a list of corners, or with a numpy array
        """

        if type(corners).__module__[:5] == np.__name__:
            assert (corners.shape == (4,2)) or (corners.shape == (2,4))

            self.corners = []
            if (corners.shape == (4,2)):
                for i in range(0,4):
                    self.corners.append((corners[i,0], corners[i,1]))
            else:
                for i in range(0,4):
                    self.corners.append((corners[0,i], corners[1,i]))
        else:
            assert len(corners) == 4
            self.corners = corners

    def computeCenter(self):
        center = [0,0]

        for i in range(0,4):
            center[0] += self.corners[i][0];
            center[1] += self.corners[i][1];

        center[0] /= 4;
        center[1] /= 4;

        return center;

    def computeClockwiseCorners(self):
        center = self.computeCenter()
        thetas = []

        for i in range(0,4):
            rho, theta = cart2pol(
                self.corners[i][0] - center[0],
                self.corners[i][1] - center[1])

            thetas.append(theta)

        # Sort with indexes
        indexes = [i[0] for i in sorted(enumerate(thetas), key=lambda x:x[1])]

        sortedQuad = [self.corners[i] for i in indexes]

        return sortedQuad

    def isConvex(self):
        """
        For checking convexity, the corners should be clockwise or counter-clockwise
        """
        for iCorner in range(0,4):
            corner1 = self.corners[iCorner]
            corner2 = self.corners[(iCorner+1) % 4]
            corner3 = self.corners[(iCorner+2) % 4]

            orientation = \
                ((corner2[1] - corner1[1]) * (corner3[0] - corner2[0])) - \
                ((corner2[0] - corner1[0]) * (corner3[1] - corner2[1]))

            if (orientation - 0.001) > 0:
                return False

        return True

    def draw(self, image, color, thickness=1, offset=(0,0)):

        for i1 in range(0,4):
            i2 = (i1+1) % 4

            pt1 = tuple((array(self.corners[i1]) + array(offset)).astype('int32').tolist())
            pt2 = tuple((array(self.corners[i2]) + array(offset)).astype('int32').tolist())

            #pdb.set_trace()
            retval, pt1, pt2 = cv2.clipLine((0, 0, image.shape[1], image.shape[0]), pt1, pt2)

            if retval:
                cv2.line(image, pt1, pt2, color, thickness)

        return image

    def __str__(self):
        return str(self.corners)


