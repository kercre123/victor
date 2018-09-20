#!/usr/bin/env python3

'''
Script to map animation triggers to groups. Prints groups that match the trigger map for any triggers
passed in as arugments on the command line
'''

REL_MAP_PATH='../../resources/assets/cladToFileMaps/AnimationTriggerMap.json'

import os
import sys
import json

scriptPath = os.path.dirname(os.path.realpath(__file__))
mapPath = os.path.join(scriptPath, REL_MAP_PATH)

triggerMap = {}


with open(mapPath, 'r') as infile:
    J = json.load(infile)
    for pair in J:
        trigger = pair["CladEvent"]
        group = pair["AnimName"]
        triggerMap[trigger] = group

# print("loaded {} mappings".format(len(triggerMap)))

for arg in sys.argv[1:]:
    if arg in triggerMap:
        print(triggerMap[arg])
    else:
        print("ERROR: {} not found in map".format(arg))
