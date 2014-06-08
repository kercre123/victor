import json

from MotionPrimitives import *

prim = MotionPrimitiveSet()
prim.minRadius = 15.0

# how often to sample (in mm)
#prim.sampleLength = 0.01 # stupidly high for nice looking plots
prim.sampleLength = 0.5

prim.resolution_mm = 10.0

# how long the long straights should be in cells (approximately)
longLen = 5

# 0.0 to disable
backwardsFactor = 0.0

prim.addAction("short straight", 0, 1, backwardsCostFactor = backwardsFactor)
prim.addAction("long straight", 0, longLen)
prim.addAction("slight left", 1, longLen, backwardsCostFactor = backwardsFactor)
prim.addAction("slight right", -1, longLen, backwardsCostFactor = backwardsFactor)
prim.addAction("hard left", 3, longLen, 1.0)
prim.addAction("hard right", -3, longLen, 1.0)
prim.addAction("inplace left", 1, 0, 2.0)
prim.addAction("inplace right", -1, 0, 2.0)

prim.createPrimitives()

import pprint
# pprint.pprint(prim.primitivesPerAngle)

# for motion in prim.primitivesPerAngle[1]:
#     print motion
#     pprint.pprint(motion.intermediatePoses)
# exit(0)

prim.dumpJson("newPrim.json")
#exit(0)

from pylab import *

prim.plotEachPrimitive(longLen)
prim.plotPrimitives(longLen)

## this one doesn't really work
# #prim.plotEachAction(longLen)
show()

