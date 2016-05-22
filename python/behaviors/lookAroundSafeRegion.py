import matplotlib.pyplot as plt
from math import *

center = [34.7, -12.9]
r = 40.0
cliff = [20.4, 15.33]
cliffTheta = -1.33 + pi

fig = plt.figure()
ax = fig.add_subplot(111,aspect='equal')
ax.set_xlim([-100.0, 100.0])
ax.set_ylim([-100.0, 100.0])

def plotCircle(center, r, color='g'):
    ax.plot( [center[0]], [center[1]], color+'+' )
    c = plt.Circle(center, r, color=color, fill=False)
    fig.gca().add_artist(c)

def plotCliff(cliff, cliffTheta):
    arrowLength = 5.0
    ax.arrow(cliff[0], cliff[1],
             arrowLength*cos(cliffTheta), arrowLength*sin(cliffTheta),
             head_width=2.0, head_length=2.0)


def plotNewSafe(center, r, cliff, cliffTheta):
    dx = center[0] - cliff[0]
    dy = center[1] - cliff[1]
    rotX = dx*cos(cliffTheta) - dy*sin(cliffTheta)
    rotY = dx*sin(cliffTheta) + dy*cos(cliffTheta)
    operand = pow(rotY,2) - (pow(rotX,2) + pow(rotY,2) - pow(r,2))
    if operand < 0:
        print "ERROR: negative operand",operand
        return
    sr = sqrt(operand)
    ans1 = -rotY + sr
    ans2 = -rotY - sr
    if ans1 > 0 and ans2 > 0:
        shiftD = min(ans1, ans2)
    elif ans1 > 0:
        shiftD = ans1
    elif ans2 > 0:
        shiftD = ans2
    else:
        print "ERROR: no positive shift!", ans1, ans2
        return

    print shiftD
    
    c1x = center[0] - shiftD * cos(cliffTheta)
    c1y = center[1] - shiftD * sin(cliffTheta)

    plotCircle([c1x, c1y], r, 'r')
    

plotCircle(center, r, 'g')
plotCliff(cliff, cliffTheta)

plotNewSafe(center, r, cliff, cliffTheta)

plt.show()
