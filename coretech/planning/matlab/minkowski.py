import pylab
import numpy as np
from math import *
from copy import deepcopy


def fixAngle(theta):
    return (theta + pi) % (2*pi) - pi

def angDist(fromAngle, toAngle):
    "returns positive angular distance"
    ret = toAngle - fromAngle
    while ret < 0:
        ret += 2*pi
    return ret
    

class Polygon:
    "A polygon defined by points"

    def __init__(self, points = [], center = None):
        "creates a polygon with points an n x 2 numpy matrix of points (or something that can be converted to such), CW order"
        self.points = np.matrix(points)

        self.center = self.points.mean(0)

        self.angles = [self._computeAngle( x, (x+1) % self.points.shape[0] ) for x in range(self.points.shape[0])]

        if center != None:
            self.center = np.matrix(center)


    def _computeAngle(self, point0, point1):
        return fixAngle(atan2(self.points[point1,1] - self.points[point0,1],
                              self.points[point1,0] - self.points[point0,0]))

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

    def findSumStartingPoint(self, robot, selfIdx = 0, robotIdx = 0):
        "find the (x,y) point to start the minkowsky difference"

        # put the start of robot on top of the start of self, then return center of robot
        robotCenterVec = robot.center - robot.points[robotIdx]

        return self.points[selfIdx] + robotCenterVec
        
    def getEdgeVector(self ,idx):
        edgePt0 = self.points[idx].tolist()[0]
        edgePt1 = self.points[ (idx + 1) % self.points.shape[0] ].tolist()[0]
        return np.array(edgePt1) - np.array(edgePt0)

    def reverseAngles(self):
        self.angles = [ fixAngle(a + pi) for a in self.angles]

    def negative(self):
        "returns the negative of the polygon where all angles are reversed and object is re-sorted"
        neg = deepcopy(self)
        neg.reverseAngles()
        return neg

    def expandCSpace(self, robot):
        "expands the C-space around this polygon with robot around robot.center using a Minkowski difference"

        selfMin = 0
        startingAngle = self.angles[0]

        robotMin = 0

        minNegRobotVal = 100.0
        for idx in range(len(robot.angles)):
            dist = angDist(robot.angles[idx] + pi, startingAngle)
            if dist < minNegRobotVal:
                minNegRobotVal = dist
                robotMin = idx

        print "robotMin:", robotMin

        start = self.findSumStartingPoint(robot, selfIdx = selfMin, robotIdx = robotMin)

        # now make the new points array, starting from start and
        # merging the two sets of points based on angle

        selfIdx = selfMin
        selfNum = 0
        robotIdx = robotMin
        robotNum = 0

        newPoints = [np.array(start.tolist()[0])]            

        while selfNum < len(self.angles) and robotNum < len(robot.angles):
            if angDist(self.angles[selfIdx], startingAngle) < angDist(robot.angles[robotIdx] + pi, startingAngle):
                edgeVec = self.getEdgeVector(selfIdx)
                newPoints.append(newPoints[-1].tolist() + edgeVec)
                selfNum += 1
                selfIdx = (selfIdx + 1) % len(self.angles)
            else:
                edgeVec = robot.getEdgeVector(robotIdx)
                newPoints.append(newPoints[-1].tolist() - edgeVec)
                robotNum += 1
                robotIdx = (robotIdx + 1) % len(robot.angles)

        while selfNum < len(self.angles):
            edgeVec = self.getEdgeVector(selfIdx)
            newPoints.append(newPoints[-1].tolist() + edgeVec)
            selfNum += 1
            selfIdx = (selfIdx + 1) % len(self.angles)

        while robotNum < len(robot.angles):

            edgeVec = robot.getEdgeVector(robotIdx)
            newPoints.append(newPoints[-1].tolist() - edgeVec)
            robotNum += 1
            robotIdx = (robotIdx + 1) % len(robot.angles)

        newPoints = np.matrix(newPoints)
        return Polygon(newPoints)


# do a crappy little test

# obstacleP = [(-6.0, 3.0), (-6.3, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
# obstacleP = [(-6.0, 3.0), (-5.8, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
# obstacleP = [(-6.0, 3.0), (-6.0, 6.0), (-3.0, 6.0), (-3.0, 3.0)]
obstacleP = [(-162.156311, 135.594849), (-179.01123, 177.167496), (-138.097122, 193.755432), (-121.242203, 152.182785)]
obstacle = Polygon(obstacleP)

# robotP = [(0.0, 1.0), (1.0, -1.0), (-1.0, -1.0)]
# robotP = [(1.0, 1.0), (1.2, 0.8), (1.4, -0.5), (0.3, -1.1), (-1.0, -0.8), (-1.4, 0.0), (-1.0, 0.4)]
robotP = [( -55.900002, -27.100000), ( -55.900002, 27.100000), ( 22.099998, 27.100000), ( 22.099998, -27.100000)]
# robotC = (0.0, 0.5)
robotC = (0.0, 0.0)
robot = Polygon(robotP, robotC)

print "robot:", robot.angles
print "negative robot:", robot.negative().angles
print "obstacle:", obstacle.angles


start = obstacle.findSumStartingPoint(robot.negative())
robotStart = robot.offset(start - robot.center)

cspace = obstacle.expandCSpace(robot)

print "cspace:", cspace.angles

doplot = True

if doplot:
    pylab.figure()
    ax = pylab.subplot(1, 1, 1, aspect='equal')

    obstacle.plot('b-')
    robot.plot('r-')
    # robotStart.plot('r--')

    cspace.plot('m-')

    pylab.show()



