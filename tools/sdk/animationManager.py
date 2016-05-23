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

class AnimationManager:
    "AnimationManager"

    def __init__(self):
        self.animationNames = set()

    def UpdateAnimations(self, msg):
        self.animationNames.add(msg.animName)

    def PrintAnimations(self):
        sys.stdout.write("Animations:" + os.linesep)
        if len(self.animationNames) == 0:
            sys.stdout.write("Animation List empty" + os.linesep)
            sys.stdout.write("You should use getAnimations to populate list before playAnimations")
        for anim in self.animationNames:
            sys.stdout.write("  " + anim + os.linesep)

