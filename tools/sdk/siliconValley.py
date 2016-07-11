#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

cozmo = CozmoInterface()

prevLen = 0
prevSet = set()
inFrame = []
while True:
    state = cozmo.GetState()
    diff = [item for item in set(state.faceOrder) if item not in prevSet]
    if diff: cozmo.EnrollNamedFace(input("Please type face name: "), state.FaceID(diff[0]))
    for i in range(state.numFaces):
        face = state.GetFace(i)
        if not state.IsObjectVisible(face) and face.faceID in inFrame:
            inFrame.remove(face.faceID)
        elif state.IsObjectVisible(face) and (face.faceID not in inFrame) and face.name:
            cozmo.SayText(face.name)
            inFrame.append(face.faceID)
    prevSet = set(state.faceOrder)
    time.sleep(.1)

cozmo.Shutdown()
