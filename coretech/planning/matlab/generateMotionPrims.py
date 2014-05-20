
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

prim.generateAngles()

# # test
# for i in range(16):
#     a = prim.actions["long straight"].generate(i, prim)
#     print "%d: (%d, %d, %d) len: %f" % (i, a.endPose.x, a.endPose.y, a.endPose.theta,
#                                         sqrt(a.endPose.x**2 + a.endPose.y**2))

# startPose = Pose(0, 0, 0)
# for x in range(10):
#     for y in range(10):
#         endPose = Pose(x, y, 1)
#         res = prim.computeTurn(startPose, endPose)
#         if res["l"] > 0.0 and res["r"] > 0.0:
#             score = res["l"]**2 + res["r"]**2
#             print x, y, sqrt(float(x*x) + float(y*y)), res, score

startPose = Pose(0, 0, 0)
print prim.findTurn(startPose, 1, 8.0)
print prim.findTurn(startPose, 4, 4.0)
print prim.findTurn(Pose(0, 0, 1), 1, 5.0)
