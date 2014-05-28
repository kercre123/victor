
import json

from MotionPrimitives import *

prim = MotionPrimitiveSet()
prim.minRadius = 1.0
# prim.sampleLength = 0.01 # stupidly high for nice looking plots
prim.sampleLength = 0.5

longLen = 5

prim.addAction("short straight", 0, 1)
prim.addAction("long straight", 0, longLen)
prim.addAction("backup", 0, -1, 5.0)
prim.addAction("slight left", 1, longLen)
prim.addAction("slight right", -1, longLen)
prim.addAction("hard left", 3, longLen)
prim.addAction("hard right", -3, longLen)
prim.addAction("inplace left", 1, 0)
prim.addAction("inplace right", -1, 0)

prim.createPrimitives()

import pprint
# pprint.pprint(prim.primitivesPerAngle)

for motion in prim.primitivesPerAngle[8]:
    print motion
    pprint.pprint(motion.intermediatePoses)

#exit(0)

from pylab import *

figure() # plot each angle
for angle in range(16):
    ax = subplot(4,4,angle+1, aspect='equal')
    ax.xaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])
    ax.yaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])
    ax.grid()
    for motion in prim.primitivesPerAngle[angle]:
        # print motion
        # pprint.pprint(motion.intermediatePoses)
        X = [p.x_mm for p in motion.intermediatePoses]
        Y = [p.y_mm for p in motion.intermediatePoses]
        ax.plot(X, Y, 'b-')
        color = 'ro'
        if X[-1] == 0 and Y[-1] == 0:
            color = 'go'
        ax.plot(X[-1], Y[-1], color)
    

figure() # plot 3 and all in the 4th corner
# for angle in range(3):
#     ax = subplot(2,2,angle+1, aspect='equal')
#     ax.xaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])
#     ax.yaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])
#     ax.grid()
#     for motion in prim.primitivesPerAngle[angle]:
#         # print motion
#         # pprint.pprint(motion.intermediatePoses)
#         X = [p.x_mm for p in motion.intermediatePoses]
#         Y = [p.y_mm for p in motion.intermediatePoses]
#         ax.plot(X, Y, 'b-')
#         ax.plot(X[-1], Y[-1], 'ro')

# ax = subplot(2, 2, 4, aspect='equal')
ax = subplot(1, 1, 1, aspect='equal')
ax.grid()
ax.xaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])
ax.yaxis.set_ticks([float(x) for x in range(-longLen-2, longLen+3)])

for angle in range(prim.numAngles):
    for motion in prim.primitivesPerAngle[angle]:
        # print motion
        # pprint.pprint(motion.intermediatePoses)
        X = [p.x_mm for p in motion.intermediatePoses]
        Y = [p.y_mm for p in motion.intermediatePoses]
        ax.plot(X, Y, 'b-')
        #ax.plot(X[-1], Y[-1], 'ro')


show()
