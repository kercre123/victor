from collections import namedtuple
from math import *
from fractions import gcd
import numpy

class MotionPrimitiveSet:
    "A set of motion primitives for use in the xytheta lattice planner"

    def __init__(self):
        self.numAngles = 16
        self.resolution_mm = 1.0
        self.actions = {}
        self.angles = []
        self.angleCells = []

    def addAction(self, name, angleOffset, length, costFactor = 1.0):
        """Add an action to the primitive set.
        * angleOffset is number of discrete angles to transition (signed int)
        * length is approximate length of the action in number of cells (signed int)
          The length may be adjusted to fit the grid
        * costFactor is optional and is a multiple of the cost of the action"""

        if name in self.actions:
            raise DuplicateActionError(name)
        self.actions[name] = Action(name, angleOffset, length, costFactor)

    def generateAngles(self):
        "create numAngles worth of grid-aligned angles, as close as possible to 2*pi / numAngles radians each"

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

                denom = gcd(x, y)
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

    def computeTurn(self, startPose, endPose):
        # math from http://sbpl.net/node/53
        c0 = cos(self.angles[startPose.theta])
        s0 = sin(self.angles[startPose.theta])
        c1 = cos(self.angles[endPose.theta])
        s1 = sin(self.angles[endPose.theta])
        x0 = startPose.x * self.resolution_mm
        y0 = startPose.y * self.resolution_mm
        x1 = endPose.x * self.resolution_mm
        y1 = endPose.y * self.resolution_mm
        A = numpy.matrix([[c0, s1 - s0], [s0, c0-c1]])
        B = numpy.matrix([[x1-x0], [y1-y0]])

        X = A.I * B

        # index 0 is l, 1 is r
        return X.reshape(-1).tolist()[0]

    def findTurn(self, startPose, deltaTheta, minR):
        bestScore = 99999.9
        best = None
        for x in range(startPose.x, startPose.x+10):
            for y in range(startPose.y, startPose.y+10):
                endPose = Pose(x, y, startPose.theta + deltaTheta)
                turn = self.computeTurn(startPose, endPose)
                if turn[1] > minR and turn[0] >= 0.0:
                    score = turn[0]**2 + turn[1]**2
                    if score < bestScore:
                        bestScore = score
                        best = [x, y, turn]

        return best
        

class DuplicateActionError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return "Action named '%s' already defined" % self.value

class Action:
    "A type of action that will create a primitive from every starting angle."
    
    def __init__(self, name, angleOffset, length, costFactor = 1.0):
        self.name = name
        self.length = length
        self.angleOffset = angleOffset
        self.costFactor = costFactor

    def __str__(self):
        return "%s: %+d angles, %+d length, costFactor=%f" % (
            self.name,
            self.angleOffset,
            self.length,
            self.costFactor)

    def generate(self, startingAngle, primSet):
        "Turn the action into a real motion primtive"

        if self.length == 0:
            # turn in place action
            ret = MotionPrimitive(self.name, 0, 0, startingAngle + self.angleOffset)
            # TODO: intermediate poses

        elif self.angleOffset == 0:
            # find the closest multiple of the angle cell that matches the length
            angleCellLength = sqrt(primSet.angleCells[startingAngle][0]**2 + primSet.angleCells[startingAngle][1]**2)
            numLengths = round(self.length / angleCellLength)
            x = primSet.angleCells[startingAngle][0] * numLengths
            y = primSet.angleCells[startingAngle][1] * numLengths
            ret = MotionPrimitive(self.name, x, y, startingAngle)

        else:
            return NotImplementedError("sorry, not done yet")

        # clean up primitive
        if ret.endPose.theta < 0:
            ret.endPose.theta += primSet.numAngles
        if ret.endPose.theta >= primSet.numAngles:
            ret.endPose.theta -= primSet.numAngles

        return ret
            
Pose = namedtuple('Pose', ['x', 'y', 'theta'])
Pose_mm = namedtuple('Pose_mm', ['x_mm', 'y_mm', 'theta_mm'])

class MotionPrimitive:
    "A primitive for a given starting angle and action"

    def __init__(self, actionName, x, y, theta):
        self.endPose = Pose(x, y, theta)
        self.intermediatePoses = []
        self.actionName = actionName
