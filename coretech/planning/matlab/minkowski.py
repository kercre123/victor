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
    
def projectsLeft(vec, targetVec):
    "returns true if the target vector is to the 'left' of the line specified by vec"
    length = np.linalg.norm(vec)
    perpUnitVec = np.array( [ -vec[1] / length , vec[0] / length ] )
    dot = perpUnitVec.dot(targetVec)

    return dot > 0.0

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

        # the last one is redundant
        return Polygon(newPoints[:-1, :])

    def findCenterForCircles(self):
        "returns a center point that seems good for the circumscribing and inscribing circles"

        # 


        # return np.matrix([[ self.center[0,0] + 20.0, self.center[0,1] + 10]])
        return self.points.mean(0)

    def computeCircumscribingCircle(self):
        # for now, just keep the center. We could do MUCH better than this in the future
        ret = {"center": self.findCenterForCircles()}
        maxRadius = 0
        for point in self.points:
            radius = np.linalg.norm(point - ret["center"])
            if radius > maxRadius:
                maxRadius = radius
        ret["radius"] = maxRadius

        return ret

    def computeInscribingCircle(self):
        # for now, just keep the center. We could do MUCH better than this in the future
        ret = {"center": self.findCenterForCircles()}

        minRadius = 9999999.9
        for idx in range(len(self.angles)):
            # compute the distance from the edge to the center
            vec = self.getEdgeVector(idx)
            length = np.linalg.norm(vec)
            perpUnitVec = np.array( [ vec[1] / length , -vec[0] / length ] )

            if abs(perpUnitVec.dot(vec)) > 1e-5:
                print "ERROR! math is wrong!!"
                return None

            centerVec = np.squeeze(np.asarray(ret["center"] - self.points[idx]))

            radius = perpUnitVec.dot(centerVec)
            if radius < minRadius:
                minRadius = radius

        ret["radius"] = minRadius

        return ret

    def sortEdgesForCompute_old(self, inscribed, circumscribed):
        "return the edge indcides in a nice order for checking if a point is inside, based on using"
        "circumscribing and inscribing circles"

        # compute distances to center as ratio of the circle's radii
        edges = []

        for idx in range(len(self.angles)):
            # compute the distance from the edge to the center
            vec = self.getEdgeVector(idx)
            length = np.linalg.norm(vec)
            perpUnitVec = np.array( [ vec[1] / length , -vec[0] / length ] )
            centerVec = np.squeeze(np.asarray(self.center - self.points[idx]))
            dist = perpUnitVec.dot(centerVec)

            # percentage based on the radii
            score = (dist - inscribed["radius"]) / (circumscribed["radius"] - inscribed["radius"])

            edges.append((score, self.angles[idx], idx))

        # now select edges in order of increasing score, but not with angles that are too close
        edges.sort()

        ret = []

        # how close angles can be to be next to eachother in the list
        minAngleDist = 0.4

        lastAngle = edges[0][1]
        ret.append(edges[0][2])
        edges = edges[1:]

        while edges:
            found = False
            for edge in edges:
                dist = (edge[1] - lastAngle) % ( 2*pi )
                if dist > minAngleDist:
                    lastAngle = edge[1]
                    ret.append(edge[2])
                    edges.remove(edge)
                    found = True
                    break

            if not found:
                lastAngle = edges[0][1]
                ret.append(edges[0][2])
                edges.remove(edges[0])                

        return ret

    def sortEdgesForCompute(self, inscribed, circumscribed):
        "return the edge indcides in a nice order for checking if a point is inside, based on using"
        "circumscribing and inscribing circles"

        numTestPoints = 16

        # create some test points on the outer circle
        testPointsX = [circumscribed["radius"] * cos(t) + circumscribed["center"][0,0]
                       for t in np.linspace(0, 2*pi, numTestPoints+1)[:-1] ]
        testPointsY = [circumscribed["radius"] * sin(t) + circumscribed["center"][0,1]
                       for t in np.linspace(0, 2*pi, numTestPoints+1)[:-1]]

        # add some points between the two circles
        midRadius = 0.3*circumscribed["radius"] + 0.7*inscribed["radius"]
        testPointsX.extend( [ midRadius * cos(t) + circumscribed["center"][0,0]
                              for t in np.linspace(0, 2*pi, numTestPoints+1)[:-1]] )
        testPointsY.extend( [ midRadius * sin(t) + circumscribed["center"][0,1]
                              for t in np.linspace(0, 2*pi, numTestPoints+1)[:-1]] )

        # pylab.plot(testPointsX, testPointsY, 'cd')

        testPoints = np.array( zip(testPointsX, testPointsY) )
        testPointHit = [False] * len(testPoints)

        def GetNewHits(edgeIdx):
            "helper function to get the new hits"
            ret = [False] * len(testPoints)

            edgeVec = self.getEdgeVector(edgeIdx)
            for testIdx in range(len(testPoints)):
                if testPointHit[testIdx]:
                    continue
                testVec = testPoints[testIdx] - np.squeeze(np.asarray(self.points[edgeIdx]))
                if projectsLeft( edgeVec, testVec ):
                    ret[testIdx] = True

            return ret


        ret = []

        # keep going until we are out of edges
        edges = range(len(self.angles))

        while edges:

            # iterate over each edge, and check the number of new hits
            maxHits = -1
            maxHitIdx = None
            maxHitHits = None
            for idx in edges:
                newHits = GetNewHits(idx)
                numNewHits = sum(newHits)
                if numNewHits > maxHits:
                    maxHits = numNewHits
                    maxHitIdx = idx
                    maxHitHits = newHits

            print "adding edge %d with %d new hits" % (maxHitIdx, maxHits)

            ret.append(maxHitIdx)
            edges.remove(maxHitIdx)
            testPointHit = [ a or b for (a,b) in zip(testPointHit, maxHitHits) ]

        print "hit %d total points" % sum(testPointHit)

        return ret

        


# do a crappy little test

# obstacleP = [(-6.0, 3.0), (-6.3, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
# obstacleP = [(-6.0, 3.0), (-5.8, 5.9), (-2.7, 6.2), (-3.1, 2.8)]
# obstacleP = [(-6.0, 3.0), (-6.0, 6.0), (-3.0, 6.0), (-3.0, 3.0)]
obstacleP = [(-162.156311, 135.594849), (-179.01123, 177.167496), (-138.097122, 193.755432), (-121.242203, 152.182785)]
obstacle = Polygon(obstacleP)

# robotP = [(0.0, 1.0), (1.0, -1.0), (-1.0, -1.0)]
# robotP = [(1.0, 1.0), (1.2, 0.8), (1.4, -0.5), (0.3, -1.1), (-1.0, -0.8), (-1.4, 0.0), (-1.0, 0.4)]
robotP = [( -55.900002, -27.100000), ( -55.900002, 27.100000), ( 22.099998, 27.100000), ( 22.099998, -27.100000)]
# robotP = [(-0.2, 0.2), (0.2, 0.2), (0.2, -1.6), (-0.2, -1.6) ]
# robotC = (0.0, 0.5)
robotC = (0.0, 0.0)
robot = Polygon(robotP, robotC)

print "robot:", robot.angles
print "negative robot:", robot.negative().angles
print "obstacle:", obstacle.angles


start = obstacle.findSumStartingPoint(robot.negative())
robotStart = robot.offset(start - robot.center)

cspace = obstacle.expandCSpace(robot)

print "cspace:", cspace.points

circumscribed = cspace.computeCircumscribingCircle()
inscribed = cspace.computeInscribingCircle()

edgeOrder = cspace.sortEdgesForCompute(inscribed, circumscribed)
print "edge order:", edgeOrder

doplot = True

if doplot:
    pylab.figure()
    ax = pylab.subplot(1, 1, 1, aspect='equal')

    obstacle.plot('b-')
    robot.plot('r-')
    # robotStart.plot('r--')

    cspace.plot('m-')

    X = [circumscribed["radius"] * cos(t) + circumscribed["center"][0,0] for t in np.arange(0, 2*pi, 0.05)]
    Y = [circumscribed["radius"] * sin(t) + circumscribed["center"][0,1] for t in np.arange(0, 2*pi, 0.05)]
    pylab.plot(X, Y, 'm--')

    X = [inscribed["radius"] * cos(t) + inscribed["center"][0,0] for t in np.arange(0, 2*pi, 0.05)]
    Y = [inscribed["radius"] * sin(t) + inscribed["center"][0,1] for t in np.arange(0, 2*pi, 0.05)]
    pylab.plot(X, Y, 'm--')

    for idx in range(len(edgeOrder)):
        # edge edgeOrder[idx] is the idx'th most important one
        nextPoint = ( edgeOrder[idx] + 1 ) % len(cspace.points)
        point0 = cspace.points[edgeOrder[idx]]
        point1 = cspace.points[nextPoint]
        centerPt = 0.5 * (point0 + point1)
        pylab.text(centerPt[0,0], centerPt[0,1], "%d" % idx)

    pylab.show()



