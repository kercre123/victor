import pylab
import numpy as np
from math import *
from copy import deepcopy


def fixAngle(theta):
    return (theta + pi) % (2*pi) - pi

class Polygon:
    "A polygon defined by points"

    def __init__(self, points = [], center = None):
        "creates a polygon with points an n x 2 numpy matrix of points (or something that can be converted to such), CW order"
        self.points = np.matrix(points)

        self.center = self.points.mean(0)

        self.angles = [self._computeAngle( x, (x+1) % self.points.shape[0] ) for x in range(self.points.shape[0])]

        self._sort()

        if center != None:
            self.center = np.matrix(center)


    def _computeAngle(self, point0, point1):
        return fixAngle(atan2(self.points[point1,1] - self.points[point0,1],
                              self.points[point1,0] - self.points[point0,0]) + pi/2)

    def _sort(self):
        "sorts this object by angles"

        # return

        maxAngle = max(self.angles)
        maxIdx = self.angles.index(maxAngle)

        self.angles = self.angles[maxIdx:] + self.angles[:maxIdx]
        
        newPoints = self.points.tolist()
        newPoints = newPoints[maxIdx:] + newPoints[:maxIdx]
        self.points = np.matrix(newPoints)

    def offset(self, vec):
        "returns a new polygon which has been offset by vec"
        return Polygon(self.points + vec, self.center + vec)
        

    def plot(self, style = 'r-'):

        if len(self.points) == 0:
            return

        X = self.points[:,0].tolist()
        X.append([self.points[0,0]])
        Y = self.points[:,1].tolist()
        Y.append([self.points[0,1]])

        pylab.plot(self.points[0,0], self.points[0,1], 'k*')
        pylab.plot(X, Y, style)

        if self.center != None:
            pylab.plot(self.center[0,0], self.center[0,1], 'g+')

    def findSumStartingPoint_old(self, robot):
        "find the (x,y) point to start the minkowsky sum"

        # find the point on the robot that would be in contact with
        # the first edge in self. the first edge is roughly the "top"
        # of the polygon, so we want the point on robot which is
        # furthest below this edge

        edgeVec = self.points[1,:] - self.points[0,:]
        perpVec = np.matrix([-edgeVec[0,1], edgeVec[0,0]])

        contactPoint = robot.points.dot(perpVec.T).argmin()

        # note: edge case where angles are the same is irrelevant,
        # because you can pick them in either order and get the same
        # result

        # now put the robots contactPoint against the obstacles
        # starting point, and return the center of the robot
        return robot.center - robot.points[contactPoint] + self.points[0]


    def findSumStartingPoint(self, robot):
        "find the (x,y) point to start the minkowsky difference"

        # put the start of robot on top of the start of self, then return center of robot
        robotCenterVec = robot.center - robot.points[0]

        return self.points[0] + robotCenterVec
        
    def getEdgeVector(self ,idx):
        edgePt0 = self.points[idx].tolist()[0]
        edgePt1 = self.points[ (idx + 1) % self.points.shape[0] ].tolist()[0]
        return np.array(edgePt1) - np.array(edgePt0)

    def reverseAngles(self):
        self.angles = [ fixAngle(a + pi) for a in self.angles]
        self._sort()

    def negative(self):
        "returns the negative of the polygon where all angles are reversed and object is re-sorted"
        neg = deepcopy(self)
        neg.reverseAngles()
        return neg

    def expandCSpace(self, robot):
        "expands the C-space around this polygon with robot around robot.center  usin ga Minkowski difference"

        negRobot = robot.negative()

        start = self.findSumStartingPoint(negRobot)

        # now make the new points array, starting from start and
        # merging the two sets of points based on angle

        selfIdx = 0
        robotIdx = 0

        newPoints = [np.array(start.tolist()[0])]            

        while selfIdx < len(self.angles) and robotIdx < len(negRobot.angles):
            if self.angles[selfIdx] > negRobot.angles[robotIdx]:
                edgeVec = self.getEdgeVector(selfIdx)
                newPoints.append(newPoints[-1].tolist() + edgeVec)
                selfIdx += 1
            else:
                edgeVec = negRobot.getEdgeVector(robotIdx)
                newPoints.append(newPoints[-1].tolist() - edgeVec)
                robotIdx += 1

        while selfIdx < len(self.angles):
            edgeVec = self.getEdgeVector(selfIdx)
            newPoints.append(newPoints[-1].tolist() + edgeVec)
            selfIdx += 1

        while robotIdx < len(negRobot.angles):
            edgeVec = negRobot.getEdgeVector(robotIdx)
            newPoints.append(newPoints[-1].tolist() - edgeVec)
            robotIdx += 1

        newPoints = np.matrix(newPoints)
        return Polygon(newPoints)


# do a crappy little test

# obstacleP = [(-6.0, 3.0), (-6.3, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
obstacleP = [(-6.0, 3.0), (-5.8, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
# obstacleP = [(-6.0, 3.0), (-6.0, 6.0), (-3.0, 6.0), (-3.0, 3.0)]
obstacle = Polygon(obstacleP)

# robotP = [(0.0, 1.0), (1.0, -1.0), (-1.0, -1.0)]
robotP = [(1.0, 1.0), (1.2, 0.8), (1.4, -0.5), (0.3, -1.1), (-1.0, -0.8), (-1.4, 0.0), (-1.0, 0.4)]
robotC = (0.0, 0.5)
robot = Polygon(robotP, robotC)

print "robot:", robot.angles
print "negative robot:", robot.negative().angles
print "obstacle:", obstacle.angles


start = obstacle.findSumStartingPoint(robot.negative())
robotStart = robot.offset(start - robot.center)

cspace = obstacle.expandCSpace(robot)

doplot = True

if doplot:
    pylab.figure()
    ax = pylab.subplot(1, 1, 1, aspect='equal')

    obstacle.plot('b-')
    robot.plot('r-')
    # robotStart.plot('r--')

    cspace.plot('m-')

    pylab.show()



