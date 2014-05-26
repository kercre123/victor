
import json

from MotionPrimitives import *

prim = MotionPrimitiveSet()

longLen = 7

prim.addAction("short straight", 0, 1)
prim.addAction("long straight", 0, longLen)
prim.addAction("backup", 0, -1, 5.0)
prim.addAction("slight left", 1, longLen)
prim.addAction("slight right", -1, longLen)
prim.addAction("inplace left", 1, 0)
prim.addAction("inplace right", -1, 0)

prim.createPrimitives()

import pprint
pprint.pprint(prim.primitivesPerAngle)
