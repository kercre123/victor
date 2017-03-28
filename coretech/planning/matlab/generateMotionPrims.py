import json

from MotionPrimitives import *

prim = MotionPrimitiveSet()

# how often to sample (in mm)
#prim.sampleLength = 0.01 # stupidly high for nice looking plots
prim.sampleLength = 0.5

prim.resolution_mm = 10.0

# how long the long straights should be in cells (approximately)
longLen = 5

# 0.0 to disable
backwardsFactor1 = 2.0
backwardsFactor2 = 0.0

slightRadius = 94.7
hardRadius = 34.142 # 68.284


prim.addAction("short straight", 0, 1, backwardsCostFactor = backwardsFactor1)
prim.addAction("long straight", 0, longLen)
prim.addAction("slight left", 1, longLen, targetRadius=slightRadius, backwardsCostFactor = backwardsFactor2)
prim.addAction("slight right", -1, longLen, targetRadius=slightRadius, backwardsCostFactor = backwardsFactor2)
prim.addAction("hard left", 2, longLen, costFactor = 1.0, targetRadius=hardRadius)
prim.addAction("hard right", -2, longLen, costFactor = 1.0, targetRadius=hardRadius)
prim.addAction("inplace left", 1, 0, costFactor = 2.0)
prim.addAction("inplace right", -1, 0, costFactor = 2.0)

from pylab import *
prim.plotRawActions(longLen)
draw()

# exit(0)

prim.createPrimitives()

import pprint
# pprint.pprint(prim.primitivesPerAngle)

# for motion in prim.primitivesPerAngle[1]:
#     print motion
#     # pprint.pprint(motion.intermediatePoses)
# # exit(0)

# exit(0)

# for action in prim.actions:
#     print action
#     for angle in prim.primitivesPerAngle:
#         for actionID in range(len(angle)):
#             if prim.actions[action].index == actionID and angle[actionID].arc:
#                 print "r =",angle[actionID].arc.radius_mm
                
        
# prim.dumpJson("newPrim.json")

#exit(0)

# from pylab import *

# prim.plotEachPrimitive(longLen)
# prim.plotPrimitives(longLen)

prim.plotPrimitives(longLen, pretty = False)
# savefig('prims.png', transparent=True)
# savefig('prims.svg', transparent=True)

def drawOverlayGrid():
    #silly pic of overlayed prims forming a grid
    figure()
    for x in range(-5,6):
        for y in range(-5,6):
            print x,y
            if x==0 and y==0:
                continue
            else:
                style = 'b-'
            prim.plotPrimitives(longLen, pretty = True, style=style, xoff=x*10, yoff=y*10, doDraw=False)

    prim.plotPrimitives(longLen, pretty = True, style='r-', xoff=0, yoff=0, doDraw=False, linewidth=3)

    draw()
    savefig('tiles5.png', transparent=True)
    savefig('tiles5.svg', transparent=True)

    axis([-50, 50, -50, 50])
    savefig('tiles5_zoom.png', transparent=True)
    savefig('tiles5_zoom.svg', transparent=True)

    axis([-40, 40, -22.5, 22.5])
    savefig('tiles5_zoom2.png', transparent=True)
    savefig('tiles5_zoom2.svg', transparent=True)

# drawOverlayGrid()

## this one doesn't really work
# #prim.plotEachAction(longLen)
show()

