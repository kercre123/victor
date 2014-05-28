from collections import namedtuple
from math import *
from fractions import gcd
import numpy

class MotionPrimitiveSet:
    "A set of motion primitives for use in the xytheta lattice planner"

    def __init__(self):
        self.numAngles = 16
        self.resolution_mm = 1.0
        self.minRadius = 8.0

        # dictionary of properties for each action
        self.actions = {}

        # the angle in radians (index is discrete angle index)
        self.angles = []

        # the exact coordinates that define each angle
        self.angleCells = []

        # motion primitives, first index will be angle, second will be action id (integer)
        self.primitivesPerAngle = []

        # distance between samples for intermediatePoints
        self.sampleLength = 0.5

        self.numActions = 0

    def addAction(self, name, angleOffset, length, costFactor = 1.0):
        """Add an action to the primitive set.
        * angleOffset is number of discrete angles to transition (signed int)
        * length is approximate length of the action in number of cells (signed int)
          The length may be adjusted to fit the grid
        * costFactor is optional and is a multiple of the cost of the action"""

        if name in self.actions:
            raise DuplicateActionError(name)
        self.actions[name] = Action(name, angleOffset, length, self.numActions, costFactor)
        self.numActions += 1

    def createPrimitives(self):
        "After calling addAction to add all the actions, this will generate the motion primitives"
        self.generateAngles()

        for angleIdx in range(len(self.angles)):
            # initialize empty list for primitives
            self.primitivesPerAngle[angleIdx] = [None for i in range(self.numActions)]
            for action in self.actions.values():
                self.primitivesPerAngle[angleIdx][action.index] = action.generate(angleIdx, self)

    def generateAngles(self):
        "create numAngles worth of grid-aligned angles, as close as possible to 2*pi / numAngles radians each"

        self.primitivesPerAngle = [None for i in range(self.numAngles)]
        self.angles = []
        self.angleCells = []

        if self.numAngles == 4:
            self.angles = [i * pi/2 for i in range(4)]
        else:
            # create a square with a point at each cell corner that has the desired number of points
            if self.numAngles % 8 != 0:
                raise ValueError("number of angles must be 4 or a factor of 8")
            l = self.numAngles / 8
            x = l
            y = 0
            direction = "up"
            for i in range(self.numAngles):
                theta = atan2(y, x)
                self.angles.append(theta)

                denom = gcd(abs(x), abs(y))
                self.angleCells.append((x / denom, y / denom))
                
                # print i, x, y, theta

                if direction == "up":
                    if y == l:
                        direction = "left"
                        x = x - 1
                    else:
                        y = y + 1
                elif direction == "left":
                    if x == -l:
                        direction = "down"
                        y = y - 1
                    else:
                        x = x - 1
                elif direction == "down":
                    if y == -l:
                        direction = "right"
                        x = x + 1
                    else:
                        y = y - 1
                elif direction == "right":
                    if x == l:
                        direction = "up"
                        y = y + 1
                    else:
                        x = x + 1
                else:
                    raise RuntimeError("invalid direction '%s'!" % direction)

        print "generated %d discrete angles" % len(self.angles)

    def computeTurn(self, startPose, endPose):
        # math from http://sbpl.net/node/53
        try:
            c0 = cos(self.angles[startPose.theta])
            s0 = sin(self.angles[startPose.theta])
        except IndexError:
            print "index error! startPose = '%s', len(self.angles) = %d" % (startPose, len(self.angles))
            raise
        try:
            c1 = cos(self.angles[endPose.theta])
            s1 = sin(self.angles[endPose.theta])
        except IndexError:
            print "index error! endPose = '%s', len(self.angles) = %d" % (endPose, len(self.angles))
            raise

        x0 = startPose.x * self.resolution_mm
        y0 = startPose.y * self.resolution_mm
        x1 = endPose.x * self.resolution_mm
        y1 = endPose.y * self.resolution_mm
        A = numpy.matrix([[c0, s1 - s0], [s0, c0-c1]])
        B = numpy.matrix([[x1-x0], [y1-y0]])

        X = A.I * B

        # index 0 is l, 1 is r
        return X.reshape(-1).tolist()[0]

    def quadrant(self, rads):
        if abs(rads - 0.0)  < 1e-6:
            return set([1,4])
        elif abs(rads - pi/2)  < 1e-6:
            return set([1,2])
        elif abs(rads - pi)  < 1e-6:
            return set([2,3])
        elif abs(rads - -pi/2)  < 1e-6:
            return set([3,4])

        elif 0.0 < rads < pi/2:
            return set([1])
        elif pi/2 < rads < pi:
            return set([2])
        elif 0.0 > rads > -pi/2:
            return set([4])
        else:
            return set([3])

    def inQuadrant(self, x, y, quadSet):
        if x == 0 and y == 0:
            return True
        if x == 0 and y > 0:
            return 1 in quadSet or 2 in quadSet
        if x == 0 and y < 0:
            return 3 in quadSet or 4 in quadSet

        if x > 0 and y == 0:
            return 1 in quadSet or 4 in quadSet
        if x < 0 and y == 0:
            return 2 in quadSet or 3 in quadSet

        if x > 0 and y > 0:
            return 1 in quadSet
        if x > 0 and y < 0:
            return 4 in quadSet
        if x < 0 and y > 0:
            return 2 in quadSet
        if x < 0 and y < 0:
            return 3 in quadSet
        
        raise RuntimeError("bug!")


    def findTurn(self, startPose, deltaTheta):
        newAngle = startPose.theta + deltaTheta
        newAngle = fixTheta(newAngle, self.numAngles) # between 0 and numAngles

        # constrain the solution so that the turn ends in the quadrant
        # that the ending angle is in
        quads = self.quadrant(self.angles[startPose.theta])
        quads.union(self.quadrant(self.angles[newAngle]))
        
        bestScore = 99999.9
        best = None
        for x in range(startPose.x - 9, startPose.x + 10):
            for y in range(startPose.y - 9, startPose.y + 10):
                if self.inQuadrant(x,y,quads):
                    endPose = Pose(x, y, newAngle)
                    turn = self.computeTurn(startPose, endPose)
                    if abs(turn[1]) > self.minRadius and turn[0] >= 0.0:
                        score = 10.0*turn[0] + turn[1]**2
                        if score < bestScore:
                            bestScore = score
                            best = [x, y, turn]

        if not best:
            print "failed to find turn to angle",startPose.theta
            print "quads:", quads
            return None
        else:
            # compute arc parameters
            l = best[2][0]
            r = best[2][1]
            theta0 = self.angles[startPose.theta]
            theta1 = self.angles[newAngle]

            # compute the center of the circle (x_c, y_c)
            x_c = l*cos(theta0) - r*sin(theta0)
            y_c = l*sin(theta0) + r*cos(theta0)

            # from the center, comptue the starting angle
            startRads = theta0 - pi/2

            # how many radians we move through during the arc (+ is CCW)
            sweepRads = theta1 - theta0
            sweepRads = fixTheta_rads(sweepRads)

            arc = Arc(x_c, y_c, r, startRads, sweepRads)

            return [best[0], best[1], l, arc]
        return best
        

class DuplicateActionError(Exception):
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return "Action named '%s' already defined" % self.value

class Action:
    "A type of action that will create a primitive from every starting angle."
    
    def __init__(self, name, angleOffset, length, index, costFactor = 1.0):
        self.name = name
        self.length = length
        self.angleOffset = angleOffset
        self.costFactor = costFactor
        self.index = index

    def __repr__(self):
        return "%s: %+d angles, %+d length, costFactor=%f" % (
            self.name,
            self.angleOffset,
            self.length,
            self.costFactor)

    def generate(self, startingAngle, primSet):
        "Turn the action into a real motion primtive"

        if self.length == 0:
            # turn in place action
            endPose = Pose(0, 0, startingAngle + self.angleOffset)
            ret = MotionPrimitive(self.name, endPose, 0.0)
            # TODO: intermediate poses

        elif self.angleOffset == 0:
            # find the closest multiple of the angle cell that matches the length
            angleCellLength = sqrt(primSet.angleCells[startingAngle][0]**2 + primSet.angleCells[startingAngle][1]**2)
            numLengths = round(self.length / angleCellLength)
            if numLengths <= 0 and self.length > 0:
                numLengths = 1
            elif numLengths >= 0 and self.length < 0:
                numLengths = -1
            x = primSet.angleCells[startingAngle][0] * numLengths
            y = primSet.angleCells[startingAngle][1] * numLengths
            endPose = Pose(x, y, startingAngle)
            realLength = sqrt(float(x**2 + y**2))
            ret = MotionPrimitive(self.name, endPose, realLength)

        else:
            startPose = Pose(0, 0, startingAngle)
            # straight followed by an arc
            turn = primSet.findTurn(startPose, self.angleOffset)
            if not turn:
                print "Error: turn could not be found from angle %d for action '%s'" % (startingAngle, self)
                return None
            endPose = Pose(turn[0], turn[1], startingAngle + self.angleOffset)
            l = turn[2]
            arc = turn[3]
            ret = MotionPrimitive(self.name, endPose, l)
            ret.arc = arc

        # clean up primitive
        ret.endPose = fixPose(ret.endPose, primSet.numAngles)
        assert ret.endPose.theta < primSet.numAngles

        ret.sample(primSet.angles[startingAngle], primSet)

        return ret
            
Pose = namedtuple('Pose', ['x', 'y', 'theta'])
Pose_mm = namedtuple('Pose_mm', ['x_mm', 'y_mm', 'theta_rads'])
Arc = namedtuple('Arc', ['centerPt_x', 'centerPt_y', 'radius', 'startRad', 'sweepRad'])

def fixTheta_rads(theta):
    scaled = theta
    while scaled > pi:
        scaled -= 2*pi
    while scaled < -pi:
        scaled += 2*pi
    return scaled

def fixTheta(theta, numAngles):
    scaled = theta
    while scaled >= numAngles:
        scaled -= numAngles
    while scaled < 0:
        scaled += numAngles
    return scaled

def fixPose(oldPose, numAngles):
    return Pose(oldPose.x, oldPose.y, fixTheta(oldPose.theta, numAngles))

def fixPose_mm(oldPose):
    return Pose_mm(oldPose.x_mm, oldPose.y_mm, fixTheta_rads(oldPose.theta_rads))

class MotionPrimitive:
    "A primitive for a given starting angle and action"

    def __init__(self, actionName, endPose, l):
        self.endPose = endPose
        self.l = l
        self.intermediatePoses = []
        self.actionName = actionName
        self.arc = None

    def __repr__(self):
        if self.arc:
            return "%15s: l:%+7.3f <Arc: x_c:%+5.2f y_c:%+5.2f r:%5.2f start:%+6.3f sweep:%+6.3f> --> %s" % (
                self.actionName,
                self.l,
                self.arc.centerPt_x,
                self.arc.centerPt_y,
                self.arc.radius,
                self.arc.startRad,
                self.arc.sweepRad,
                self.endPose)
        else:
            return "%15s: l:%+7.3f --> %s" % (self.actionName, self.l, self.endPose)

    def sample(self, startRads, primSet):
        "fill in intermediatePoses with samples every sampleLength mm"

        # simulate linear portion, if needed
        if self.l > 0.0:
            for t in numpy.arange(0.0, self.l, primSet.sampleLength):
                x = t*cos(startRads)
                y = t*sin(startRads)
                pose = Pose_mm(x, y, startRads)
                self.intermediatePoses.append(pose)

        #simulate arc, if there is one
        if self.arc:
            # compute the step size in radians
            radStep = primSet.sampleLength / self.arc.radius

            for t in numpy.arange(0.0, self.arc.sweepRad, radStep):
                theta = self.arc.startRad + t
                x = self.arc.centerPt_x + self.arc.radius * cos(theta)
                y = self.arc.centerPt_y + self.arc.radius * sin(theta)
                pose = Pose_mm(x, y, theta + pi/2)
                self.intermediatePoses.append(fixPose_mm(pose))

        self.intermediatePoses.append(Pose_mm(self.endPose.x, self.endPose.y, primSet.angles[self.endPose.theta]))
