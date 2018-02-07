#!/usr/bin/python

import argparse
import os

singingBehaviorConfigsPath = os.getcwd() + '/../../resources/config/engine/behaviors/singing/'
unlockCladPath = os.getcwd() + '/../../clad/src/clad/types/unlockTypes.clad'
behaviorStringsPath = os.getcwd() + '/../../unity/Cozmo/Assets/StreamingAssets/LocalizedStrings/en-US/BehaviorStrings.json'

parser = argparse.ArgumentParser(description="Adds a new song for Cozmo to sing\nEx: addSong.py TwinkleTwinkle 120 Cozmo_Sings_Twinkle_Twinkle \"Cozmo is singing Twinkle Twinkle\"")
parser.add_argument('song_name', help='The name of the song (one word, camelCase)')
parser.add_argument('song_bpm', help='The bpm of this song (80, 100, 120)')
parser.add_argument('audio_switch_name', help='The name of the audio switch for this song. Should match enums in audioSwitchTypes.clad')
parser.add_argument('in_app_text', help='Text that displays in app while Cozmo is singing this song (needs to be in quotes)')
options = parser.parse_args()

if not os.path.isfile(behaviorStringsPath):
    print(behaviorStringsPath + " does not exist")
    exit()

if not os.path.isfile(unlockCladPath):
    print(unlockCladPath + " does not exist")
    exit()

behaviorName = "Singing_" + options.song_name
behaviorNameWithQuotes = "\"Singing_" + options.song_name + "\""

if os.path.isfile(behaviorName):
    print(behaviorName + " already exists")
    exit()

# Create behavior config file and populate

songBehaviorConfigFile = open(singingBehaviorConfigsPath + behaviorName + ".json", "w")

songBehaviorConfigFile.write("{\n")
songBehaviorConfigFile.write("  \"behaviorClass\":\"Singing\",\n")
songBehaviorConfigFile.write("  \"behaviorID\":" + behaviorNameWithQuotes + ",\n")
songBehaviorConfigFile.write("  \"requiredUnlockId\":" + behaviorNameWithQuotes + ",\n")
songBehaviorConfigFile.write("  \"audioSwitchGroup\":" + "\"Cozmo_Sings_" + options.song_bpm + "Bpm\",\n")
songBehaviorConfigFile.write("  \"audioSwitch\":" + "\"" + options.audio_switch_name + "\"\n")
songBehaviorConfigFile.write("}")

songBehaviorConfigFile.close()


# Add to unlockTypes.clad
# Looks for an existing unlock type of "Singing_Reserved_*" and replaces it

unlockClad = open(unlockCladPath, "r+")
unlockToReplace = ""
for line in unlockClad:
    if "Singing_Reserved_" in line:
        trigger = line[2:]
        trigger = trigger[:-11]

        unlockToReplace = trigger
        break;

if unlockToReplace is not "":
    unlockClad.seek(0)

    data = unlockClad.read()
    data = data.replace(unlockToReplace, behaviorName)

    unlockClad.seek(0)
    unlockClad.write(data)
    unlockClad.truncate()

    unlockClad.close()
else:
    print("Failed to find existing \"Singing_Reserved_*\" entry in unlockTypes.clad")



# Add to BehaviorStrings.json

behaviorStrings = open(behaviorStringsPath, "r+")

behaviorStrings.seek(0)

data = behaviorStrings.read()

data = data[:-2]

data += ",\n"
data += "  \"behavior.singing." + options.song_name + "\": {\n"
data += "    \"translation\": \"" + options.in_app_text + "\"\n"
data += "  }\n}"

behaviorStrings.seek(0)
behaviorStrings.write(data)
behaviorStrings.truncate()

behaviorStrings.close()
