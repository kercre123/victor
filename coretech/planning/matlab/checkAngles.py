import json

from MotionPrimitives import *

prim = MotionPrimitiveSet()

prim.generateAngles()

from pprint import *
pprint(prim.angles)

for offset in [1,2,3,4]:

    radii = set()
    
    for angleIdx in range(len(prim.angles)):
        nextAngleIdx = (angleIdx + offset) % len(prim.angles)
        delta = prim.angles[nextAngleIdx] - prim.angles[angleIdx]
        p = GetExpectedXY(prim.angles[angleIdx], delta)
        radii.add(abs(round(p[2], 4)))

    print "radii for turning %+d angles:" % offset
    pprint(radii)
    for factor in [1,2,3]:
        print "",(factor*max(radii)),
    print ""


p = GetExpectedXY(prim.angles[1], prim.angles[2] - prim.angles[1])
print "+3 from 1", p

print p[0] / abs(p[2]), p[1] / abs(p[2])
