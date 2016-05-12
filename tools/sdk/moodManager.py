"""
MoodManager for querying and modifying Cozmo's mood
"""
__author__ = "Mark Wesley"

import sys
import os
    
CLAD_DIR  = os.path.join("generated", "cladPython")
sys.path.insert(0, CLAD_DIR)

try:
    from clad.types.emotionTypes import Anki
except Exception as e:
    sys.stdout.write("Exception = " + str(e)  + os.linesep)
    sys.exit("Can't import Anki.Cozmo.EmotionType libraries!{linesep}\t* Are you running from the base cozmo-engine directory?{linesep}".format(linesep=os.linesep))
    
# ================================================================================    
# MoodManager
# ================================================================================


def ExtractCladEnumNames(cladEnum):
    enumNames = [None] * cladEnum.Count 
    attrs = dir(cladEnum)
    for enumName in attrs:
        if not enumName.startswith("__") and enumName != "Count":
            enumValue = getattr(cladEnum, enumName)
            enumNames[enumValue] = enumName
    return enumNames


class MoodManager:
    "MoodManager"

    def __init__(self):
        self.emotionNames = ExtractCladEnumNames(Anki.Cozmo.EmotionType)
        self.emotionValues = [0.0] * Anki.Cozmo.EmotionType.Count
        
    def UpdateMoodState(self, msg):
        for i in range(0, Anki.Cozmo.EmotionType.Count):
            self.emotionValues[i] = msg.emotionValues[i]
            
    def EmotionNameToType(self, emotionName):
        try:
            emotionType = getattr(Anki.Cozmo.EmotionType, emotionName)
            return emotionType
        except Exception as e:
            sys.stderr.write("[EmotionNameToType] Exception: " + str(e) + os.linesep)
            return None  
            
    def GetEmotionValue(self, emotionType):
        return self.emotionValues[emotionType]
        
    def GetEmotionValueByName(self, emotionName):    
        emotionType = self.EmotionNameToType(emotionName)
        return self.GetEmotionValue(emotionType)
    
    def PrintEmotions(self):
        sys.stdout.write("Emotions:" + os.linesep)
        for i in range(0, Anki.Cozmo.EmotionType.Count):
            sys.stdout.write("  " + self.emotionNames[i] + " = " + str(self.emotionValues[i]) + os.linesep)
            
    