"""
Animation Manager to store a list of available animations
"""
__author__ = "Alec Solder"

import sys
import os
    
CLAD_DIR  = os.path.join("generated", "cladPython")
sys.path.insert(0, CLAD_DIR)

# ================================================================================    
# Animation Manager
# ================================================================================
def GetMatchingNameFromList(nameList, text):
    "Returns a list of names that start with text from nameList"
    matchingNames = []
    for name in nameList:
        if name.startswith(text):
            matchingNames.append(name)
    return matchingNames


class AnimationManager:
    "AnimationManager"

    def __init__(self):
        self.animationNames = set()
        self.animationGroupNames = set()

    def GetMatchingAnimNames(self,text):
        return GetMatchingNameFromList(self.animationNames,text)

    def InAnimationNames(self,name):
        return name in self.animationNames

    def InAnimationGroupNames(self,name):
        return name in self.animationGroupNames

    def UpdateAnimations(self, msg):
        self.animationNames.add(msg.animName)

    def UpdateAnimationGroups(self, msg):
        self.animationGroupNames.add(msg.animGroupName)

    def PrintAnimations(self):
        sys.stdout.write("Animations:" + os.linesep)
        if len(self.animationNames) == 0:
            sys.stdout.write("Animation List empty" + os.linesep)
            sys.stdout.write("You should use getAnimations to populate list before playAnimations")
        for anim in self.animationNames:
            sys.stdout.write("  " + anim + os.linesep)

    def PrintAnimationGroups(self):
        sys.stdout.write("Animation Groups:" + os.linesep)
        if len(self.animationGroupNames) == 0:
            sys.stdout.write("Animation Group List empty" + os.linesep)
            sys.stdout.write("You should use getAnimationGroups to populate list before playAnimationGroup")
        for animGroup in self.animationGroupNames:
            sys.stdout.write("  " + animGroup + os.linesep)

