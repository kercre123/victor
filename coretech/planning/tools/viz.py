import json
import argparse

from math import *

parser = argparse.ArgumentParser(description='plots environments and paths')
parser.add_argument('context', metavar='context.json',
                    help='input planner context json format')
parser.add_argument('--path', default=None, metavar='path.txt',
                    help='Path to path.txt from standlone planner output')
parser.add_argument('--mprim', default='mprim.json', metavar='mprim.json',
                    help='path to json format motion primitives')

args = parser.parse_args()

J = None

try:
    with open(args.context, 'r') as contextFile:
        J = json.load(contextFile)
except IOError:
    print "could not read context from '%s'" % args.context
    parser.print_help()
    exit(-1)


def ParseState_c( stateJ ):
    x = stateJ["x_mm"]
    y = stateJ["y_mm"]
    theta = stateJ["theta_rads"]

    return (x,y,theta)

def ParsePoly( polyJ ):
    poly = []
    for j in polyJ:
        x = j["x"]
        y = j["y"]
        poly.append( (x, y) )

    return poly

def DrawArrow(state, color):
    plt.plot(state[0], state[1], '%so' % color[0] )

    arrowLen = 10.0
    lineX = [state[0], state[0] + arrowLen * cos( state[2] )]
    lineY = [state[1], state[1] + arrowLen * sin( state[2] )]
    plt.plot(lineX, lineY, '%s-' % color[0])

def DrawPoly(poly, color):
    polyX = [ p[0] for p in poly ]
    polyX.append( polyX[0] )
    polyY = [ p[1] for p in poly ]
    polyY.append( polyY[0] )
    plt.plot(polyX, polyY, '%s-' % color[0])

def DrawMap(obs, start, goal):
    plt.figure()

    if start:
        DrawArrow(start, 'r')
    if goal:
        DrawArrow(goal, 'g')

    for poly in obs:
        DrawPoly(poly, 'k')

def ParsePath(pathFilename):
    p = []
    for line in open(pathFilename, 'r'):
        s = line.strip().split(' ')
        if len(s) == 3:
            p.append(s)

    return p

def DrawPath(path, style):
    X = [p[0] for p in path]
    Y = [p[1] for p in path]
    plt.plot(X, Y, style)
        

try:
    start = ParseState_c( J["start"] )
    goal = ParseState_c( J["goals"][0] )

    obs = []

    # just get the obstacles at angle 0
    for obsJ in J["env"]["obstacles"]["angles"][0]["obstacles"]:
        obs.append( ParsePoly( obsJ["poly"] ) )

    import pylab as plt

    DrawMap(obs, start, goal)

    if args.path:
        path = ParsePath(args.path)
        DrawPath(path, 'b--')

    plt.show()
        

except KeyError as e:
    print "could not parse context json correctly, key error:",e
    exit(-1)


