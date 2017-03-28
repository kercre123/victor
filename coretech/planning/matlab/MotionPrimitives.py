from collections import namedtuple
from math import *
from fractions import gcd
import numpy
import json
import copy

debugAngle = None

class MotionPrimitiveSet:
    "A set of motion primitives for use in the xytheta lattice planner"

    def __init__(self):
        self.numAngles = 16
        self.resolution_mm = 1.0

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

    def addAction(self, name, angleOffset, length, targetRadius = None, costFactor = 1.0, backwardsCostFactor = 0.0):
        """Add an action to the primitive set.
        * angleOffset is number of discrete angles to transition (signed int)
        * length is approximate length of the action in number of cells (signed int)
          The length may be adjusted to fit the grid
        * costFactor is optional and is a multiple of the cost of the action
        * backwardsCostFactor is optional. If set to 0.0 (the default) this action is normal
            otherwise, the action will be duplicated as a backwards action with the specified
            cost factor."""

        if name in self.actions:
            raise DuplicateActionError(name)
        self.actions[name] = Action(name, angleOffset, length, targetRadius, self.numActions, costFactor, backwardsCostFactor)
        self.numActions += 1

    def createPrimitives(self):
        "After calling addAction to add all the actions, this will generate the motion primitives"
        self.generateAngles()

        for angleIdx in range(len(self.angles)):
            # initialize empty list for primitives
            self.primitivesPerAngle[angleIdx] = [None for i in range(self.numActions)]
            for action in self.actions.values():
                self.primitivesPerAngle[angleIdx][action.index] = action.generate(angleIdx, self)

        # now if there are any backwards actions, go through and copy them over from the opposite angles
        newActions = {}
        for action in self.actions.values():
            if action.backwardsCostFactor > 1e-5:
                backwardsAction = copy.deepcopy(action)
                backwardsAction.isReverseAction = True
                backwardsAction.name = "backwards " + backwardsAction.name
                backwardsAction.costFactor = action.backwardsCostFactor
                backwardsAction.index = len(self.actions) + len(newActions)
                newActions[backwardsAction.name] = backwardsAction
                assert(backwardsAction.index == self.numActions)
                self.numActions += 1

                # copy primitives over as well, changing only the ending angle
                for angleIdx in range(len(self.angles)):
                    oppositeAngle = (angleIdx + (self.numAngles / 2)) % self.numAngles
                    backwardsPrim = copy.deepcopy(self.primitivesPerAngle[oppositeAngle][action.index])
                    newTheta = fixTheta(angleIdx - backwardsAction.angleOffset, self.numAngles)
                    backwardsPrim.endPose = Pose(backwardsPrim.endPose.x, backwardsPrim.endPose.y, newTheta)
                    backwardsPrim.actionIndex = backwardsAction.index
                    backwardsPrim.l *= -1
                    if backwardsPrim.arc:
                        newStartRad = fixTheta_rads(backwardsPrim.arc.startRad)
                        newThetaRads = backwardsPrim.arc.sweepRad
                        backwardsPrim.arc = Arc(backwardsPrim.arc.centerPt_x_mm,
                                                backwardsPrim.arc.centerPt_y_mm,
                                                backwardsPrim.arc.radius_mm,
                                                newStartRad,
                                                newThetaRads)
                    newPoses = []
                    for pose in backwardsPrim.intermediatePoses:
                        newPoses.append(Pose_mm(pose.x_mm, pose.y_mm, fixTheta_rads(pose.theta_rads + pi)))
                    backwardsPrim.intermediatePoses = newPoses
                    self.primitivesPerAngle[angleIdx].append(backwardsPrim)

        self.actions.update(newActions)

    def dumpJson(self, filename):
        J = self.createDict()
        outfile = open(filename, 'w')
        json.dump(J, outfile, indent=2, separators=(',', ': '), sort_keys=True)
        outfile.close()
        
    def createDict(self):
        J = {}
        J["angle_definitions"] = self.angles
        J["num_angles"] = self.numAngles
        J["resolution_mm"] = self.resolution_mm

        J["actions"] = [None for i in range(self.numActions)]
        for name in self.actions:
            action = self.actions[name]
            J["actions"][action.index] = self.actions[name].createDict()

        J["angles"] = []
        for angle, anglePrims in enumerate(self.primitivesPerAngle):
            J["angles"].append({"starting_angle": angle})
            J["angles"][-1]["prims"] = []
            
            for prim in anglePrims:
                J["angles"][-1]["prims"].append(prim.createDict())
        return J

    def plotEachAction(self, longLen):
        "creates a matplotlib plot with a subplot for each action, using arrows. This function doesn't really work..."
        import pylab

        pylab.figure()
        # set up titles any ylabels for plot
        for name in self.actions:
            ax = pylab.subplot(self.numActions, self.numAngles, self.numAngles * self.actions[name].index + 1)
            ax.set_ylabel(name)

        for i, rads in enumerate(self.angles):
            ax = pylab.subplot(self.numActions, self.numAngles, i + 1)
            ax.set_title("%+6.3f" % rads)

        for angle in range(self.numAngles):
            for actionID in range(self.numActions):
                motion = self.primitivesPerAngle[angle][actionID]
                # pylab.figure()
                # ax = pylab.subplot(111, aspect='equal')
                ax = pylab.subplot(self.numActions, self.numAngles, self.numAngles*actionID + angle + 1,  aspect='equal')
                ax.xaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
                ax.yaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
                ax.grid()
                X = [p.x_mm for p in motion.intermediatePoses]
                Y = [p.y_mm for p in motion.intermediatePoses]
                U = [0.5*cos(p.theta_rads) for p in motion.intermediatePoses]
                V = [0.5*sin(p.theta_rads) for p in motion.intermediatePoses]
                #ax.plot(X, Y, 'k-')
                ax.quiver(X,Y,U,V, angles='xy', scale_units='xy', scale=1)
                color = 'ro'
                if X[-1] == 0 and Y[-1] == 0:
                    color = 'go'
                ax.plot(X[-1], Y[-1], color)
        pylab.draw()
                

    def plotEachPrimitive(self, longLen):
        "creates a matplotlib plot with a subplot for each primitive"
        import pylab

        pylab.figure() # plot each angle
        for angle in range(16):
            ax = pylab.subplot(4,4,angle+1, aspect='equal')
            ax.xaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
            ax.yaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
            ax.grid()
            for motion in self.primitivesPerAngle[angle]:
                # print motion
                # pprint.pprint(motion.intermediatePoses)
                X = [p.x_mm for p in motion.intermediatePoses]
                Y = [p.y_mm for p in motion.intermediatePoses]
                ax.plot(X, Y, 'b-')
                color = 'ro'
                if X[-1] == 0 and Y[-1] == 0:
                    color = 'go'
                ax.plot(X[-1], Y[-1], color)
        pylab.draw()

    def plotPrimitives(self, longLen, pretty=False, style='b-', xoff=0.0, yoff=0.0, doDraw=True, linewidth=1):
        "creates a single plot with all primitives superimposed"
        import pylab
        if doDraw:
            pylab.figure()

        ax = pylab.subplot(1, 1, 1, aspect='equal')
        if not pretty:
            ax.grid()
            ax.xaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
            ax.yaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])

        if pretty:
            pylab.axis('off')

        for angle in range(self.numAngles):
            for motion in self.primitivesPerAngle[angle]:
                # print motion
                # pprint.pprint(motion.intermediatePoses)
                X = [p.x_mm + xoff for p in motion.intermediatePoses]
                Y = [p.y_mm + yoff for p in motion.intermediatePoses]

                ax.plot(X, Y, style, linewidth=linewidth)
                #ax.plot(X[-1], Y[-1], 'ro')

        if doDraw:
            pylab.draw()

    def plotRawActions(self, longLen, pretty=False, style='b-', xoff=0.0, yoff=0.0, doDraw=True, linewidth=1):
        "creates a single plot with all 'desired' primitives on top of each other, not the actual primitives"
        import pylab
        if doDraw:
            pylab.figure()

        ax = pylab.subplot(1, 1, 1, aspect='equal')
        if not pretty:
            ax.grid()
            ax.xaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])
            ax.yaxis.set_ticks([float(x) * self.resolution_mm for x in range(-longLen-2, longLen+3)])

        if pretty:
            pylab.axis('off')

        for action in self.actions.values():
            if action.length <= 0:
                # skip backups and turn in place actions
                continue

            if action.angleOffset == 0:
                X = pylab.arange(0, action.length * self.resolution_mm, self.sampleLength)
                Y = pylab.zeros(pylab.size(X))
                pts = pylab.array([X, Y]).T
            else:
                if action.angleOffset > 0:
                    ySign = 1.0
                else:
                    ySign = -1.0

                dTheta = action.length * self.resolution_mm / action.targetRadius
                T = pylab.linspace(-pi/2, -pi/2 + dTheta, 50)
                X = action.targetRadius * pylab.cos(T)
                Y = (action.targetRadius * pylab.sin(T) + action.targetRadius) * ySign
                pts = pylab.array([ X, Y] ).T


            for theta in pylab.linspace(0, 2*pi, self.numAngles+1):
                R = pylab.array([[cos(theta),sin(theta)],[-sin(theta),cos(theta)]])
                pts_rot = pylab.dot(pts,R)
                
                ax.plot(*pts_rot.T, linestyle=style[1], color=style[0], linewidth=linewidth)

        if doDraw:
            pylab.draw()

            
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
        # math from http://sbpl.net/node/53 All math here is in cells
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

        x0 = startPose.x
        y0 = startPose.y
        x1 = endPose.x
        y1 = endPose.y
        A = numpy.matrix([[c0, s0 - s1], [s0, c1 - c0]])
        B = numpy.matrix([[x1-x0], [y1-y0]])

        X = A.I * B

        # index 0 is l, 1 is r, both in cells
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


    def findTurn(self, startPose, deltaTheta, targetRadius_mm):

        targetRadius = targetRadius_mm / self.resolution_mm

        #TEMP
        naturalTurn = GetExpectedXY(self.angles[startPose.theta], deltaTheta)
        naturalRadius = naturalTurn[2]
        factor = round(targetRadius / naturalRadius)
        # targetRadius = naturalRadius * factor

        if startPose.theta == debugAngle:
            print "finding turn from",startPose,"dtheta =",deltaTheta,"targetRad =",targetRadius

        newAngle = startPose.theta + deltaTheta
        newAngle = fixTheta(newAngle, self.numAngles) # between 0 and numAngles

        # constrain the solution so that the turn ends in the quadrant
        # that the ending angle is in
        quads = self.quadrant(self.angles[startPose.theta])
        quads.union(self.quadrant(self.angles[newAngle]))
        
        searchSize = 15

        bestScore = 99999.9
        best = None
        for x in range(startPose.x - searchSize - 1, startPose.x + searchSize):
            for y in range(startPose.y - searchSize - 1, startPose.y + searchSize):
                if self.inQuadrant(x,y,quads):
                    endPose = Pose(x, y, newAngle)
                    turn = self.computeTurn(startPose, endPose)
                    signCorrect = (deltaTheta > 0) == (turn[1] < 0)
                    if turn[0] >= -0.1 and turn[0] < 2.5 and abs(turn[1]) > 0.1 and signCorrect:

                        chordLength = sqrt(float(x)**2 + float(y)**2)
                        # chord length is 2 r sin(theta/2)
                        deltaTheta_rads = self.angles[endPose.theta] - self.angles[startPose.theta]
                        effectiveRadius = chordLength / (2*sin(deltaTheta_rads/2))

                        if startPose.theta == debugAngle:
                            print "     cl = %f, dtheta = %f, effective r = %f, real r = %f" % (
                                chordLength, deltaTheta_rads, effectiveRadius, turn[1])

                        # score = abs(turn[0]) - abs(turn[1])
                        # score = abs(abs(turn[1]) - targetRadius) + 0.1*turn[0]
                        score = abs(abs(effectiveRadius) - targetRadius) + 0.5*turn[0]

                        if startPose.theta == debugAngle:
                            print "ok : (%3d, %3d): l=%f, r=%f, score=%f" % (
                                x, y, turn[0], turn[1], score )

                        if score < bestScore:
                            bestScore = score
                            best = [x, y, turn]
                    else:
                        if startPose.theta == debugAngle:
                            print "bad: (%3d, %3d): l=%f, r=%f" % (
                                    x, y, turn[0], turn[1] )

        if not best:
            print "failed to find turn to angle",startPose.theta
            print "quads:", quads
            return None
        else:
            # compute arc parameters, in mm
            l = best[2][0] * self.resolution_mm
            r = best[2][1] * self.resolution_mm
            theta0 = self.angles[startPose.theta]
            theta1 = self.angles[newAngle]

            # compute the center of the circle (x_c, y_c), in mm
            x_c = l*cos(theta0) + r*sin(theta0)
            y_c = l*sin(theta0) - r*cos(theta0)

            # from the center, comptue the starting angle
            startRads = theta0 + pi/2

            # fix radius
            if r < 0:
                r = -r
                startRads = fixTheta_rads(startRads - pi)

            # how many radians we move through during the arc (+ is CCW)
            sweepRads = theta1 - theta0
            sweepRads = fixTheta_rads(sweepRads)

            arc = Arc(x_c, y_c, r, startRads, sweepRads)

            if startPose.theta == debugAngle:
                print "turn found:",best, l, bestScore


            return [best[0], best[1], l, arc]
        

class DuplicateActionError(Exception):
    def __init__(self, value):
        self.value = value
    def __repr__(self):
        return "Action named '%s' already defined" % self.value

class Action:
    "A type of action that will create a primitive from every starting angle."
    
    def __init__(self, name, angleOffset, length, targetRadius, index, costFactor = 1.0, backwardsCostFactor = 0.0):
        self.name = name
        self.length = length # length in cells
        self.angleOffset = angleOffset
        self.targetRadius = targetRadius
        self.costFactor = costFactor
        self.index = index
        self.backwardsCostFactor = backwardsCostFactor
        self.isReverseAction = False

    def __repr__(self):
        return "%s (%d): %+d angles, %+d length, costFactor=%f" % (
            self.name,
            self.index,
            self.angleOffset,
            self.length,
            self.costFactor)

    def createDict(self):
        J = {}
        J["name"] = self.name
        J["index"] = self.index
        J["extra_cost_factor"] = self.costFactor
        if self.isReverseAction:
            J["reverse_action"] = True
        return J

    def generate(self, startingAngle, primSet):
        "Turn the action into a real motion primtive"

        if self.length == 0:
            # turn in place action
            endPose = Pose(0, 0, startingAngle + self.angleOffset)
            ret = MotionPrimitive(self.index, endPose, 0.0)
            if self.angleOffset > 0:
                ret.turnInPlaceDirection = +1.0
            else:
                ret.turnInPlaceDirection = -1.0

        elif self.angleOffset == 0:
            # find the closest multiple of the angle cell that matches the length
            x_target_cell = primSet.angleCells[startingAngle][0]
            y_target_cell = primSet.angleCells[startingAngle][1]
            angleCellLength_cell = sqrt(x_target_cell**2 + y_target_cell**2)
            numLengths = round(self.length / angleCellLength_cell)
            if numLengths <= 0 and self.length > 0:
                numLengths = 1
            elif numLengths >= 0 and self.length < 0:
                numLengths = -1
            x = x_target_cell * numLengths
            y = y_target_cell * numLengths
            endPose = Pose(x, y, startingAngle)
            realLength = sqrt(float(x**2 + y**2)) * primSet.resolution_mm
            if self.length < 0:
                realLength = -realLength
            ret = MotionPrimitive(self.index, endPose, realLength)

        else:
            startPose = Pose(0, 0, startingAngle)
            # straight followed by an arc
            if not self.targetRadius:
                print "ERROR: no target radius for turn!"
            turn = primSet.findTurn(startPose, self.angleOffset, self.targetRadius)
            if not turn:
                print "Error: turn could not be found from angle %d for action '%s'" % (startingAngle, self)
                return None
            endPose = Pose(turn[0], turn[1], startingAngle + self.angleOffset)
            l = turn[2]
            arc = turn[3]
            ret = MotionPrimitive(self.index, endPose, l)
            ret.arc = arc

        # clean up primitive
        ret.endPose = fixPose(ret.endPose, primSet.numAngles)
        assert ret.endPose.theta < primSet.numAngles

        ret.sample(primSet.angles[startingAngle], primSet)

        return ret
            
Pose = namedtuple('Pose', ['x', 'y', 'theta'])
Pose_mm = namedtuple('Pose_mm', ['x_mm', 'y_mm', 'theta_rads'])
Arc = namedtuple('Arc', ['centerPt_x_mm', 'centerPt_y_mm', 'radius_mm', 'startRad', 'sweepRad'])

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

    def __init__(self, actionIndex, endPose=None, l=None):
        self.endPose = endPose
        self.l = l # length in mm
        self.intermediatePoses = []
        self.actionIndex = actionIndex
        self.arc = None
        self.turnInPlaceDirection = 0

    def __repr__(self):
        if self.arc:
            return "%2u: l:%+7.3f <Arc: x_c:%+5.2f y_c:%+5.2f r:%5.2f start:%+6.3f sweep:%+6.3f> --> %s" % (
                self.actionIndex,
                self.l,
                self.arc.centerPt_x_mm,
                self.arc.centerPt_y_mm,
                self.arc.radius_mm,
                self.arc.startRad,
                self.arc.sweepRad,
                self.endPose)
        else:
            return "%15s: l:%+7.3f --> %s" % (self.actionIndex, self.l, self.endPose)

    def createDict(self):
        J = {}
        J["action_index"] = self.actionIndex
        J["end_pose"] = self.endPose._asdict()
        J["straight_length_mm"] = self.l
        if self.arc:
            J["arc"] = self.arc._asdict()
        J["intermediate_poses"] = []
        for pose in self.intermediatePoses:
            J["intermediate_poses"].append(pose._asdict())
        if self.turnInPlaceDirection != 0:
            J["turn_in_place_direction"] = self.turnInPlaceDirection
        return J


    def sample(self, startRads, primSet):
        "fill in intermediatePoses with samples every sampleLength mm"

        # simulate linear portion, if needed
        if abs(self.l) > 1e-6 or self.arc:
            sampleLen = primSet.sampleLength
            if self.l < 0:
                sampleLen = -sampleLen
            for t in numpy.arange(0.0, self.l, sampleLen):
                x = t*cos(startRads)
                y = t*sin(startRads)
                pose = Pose_mm(x, y, startRads)
                self.intermediatePoses.append(pose)
        else:
            # simulate turn in place, using a factor of the angular resolution of the map
            endRads = primSet.angles[self.endPose.theta]
            step = self.turnInPlaceDirection * 2*pi / (4*primSet.numAngles)
            if abs(step) < 1e-8:
                print "ERROR: step too low! step = %f, turnInPlaceDir = %f, numAngles = %u" % (
                    step, self.turnInPlaceDirection, primSet.numAngles)
                raise RuntimeError("bug!")
            theta = startRads
            while abs(endRads - theta) > abs(step):
                theta = fixTheta_rads(theta + step)
                pose = Pose_mm(0.0, 0.0, theta)
                self.intermediatePoses.append(pose)
            

        #simulate arc, if there is one
        if self.arc:
            # compute the step size in radians
            radStep = primSet.sampleLength / self.arc.radius_mm
            if self.arc.sweepRad < 0:
                radStep = -radStep

            for t in numpy.arange(0.0, self.arc.sweepRad, radStep):
                theta = self.arc.startRad + t
                x_mm = self.arc.centerPt_x_mm + self.arc.radius_mm * cos(theta)
                y_mm = self.arc.centerPt_y_mm + self.arc.radius_mm * sin(theta)
                pose = Pose_mm(x_mm, y_mm, theta + pi/2)
                self.intermediatePoses.append(fixPose_mm(pose))

        self.intermediatePoses.append(Pose_mm(self.endPose.x * primSet.resolution_mm,
                                              self.endPose.y * primSet.resolution_mm,
                                              primSet.angles[self.endPose.theta]))


# TEMP TODO: this function should actually return a set of radii that
# get you close to a cell. E.g. the current thing, and then any factor
# higher up to maybe 3 or 4. these should each be different sets, so
# you know you'd need a radius near each of those things?

def GetExpectedXY(theta0, deltaTheta):
    "return the (x,y,radius) that gets you closest to a cell border"
    theta1 = theta0 + deltaTheta
    x = sin(theta0) - sin(theta1)
    y = -cos(theta0) + cos(theta1)
    #print "raw (%f, %f)" % (x, y)

    if abs(x) < 1e-6:
        r = 1.0 / abs(y)
    elif abs(y) < 1e-6:
        r = 1.0 / abs(x)
    else:
        r = 1.0 / min(abs(x), abs(y))

    if deltaTheta > 0:
        r = -r
    x = x*r
    y = y*r

    # print "can get from %f to %f landing on (%f, %f) with a radius of %f" % (
    #     theta0, theta1, x, y, r)

    return (x,y,r)

