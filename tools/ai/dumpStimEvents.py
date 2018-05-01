#!/usr/bin/env python3

'''
Script to read emotion events and dump an easier-to-parse list of events
'''

REL_EVENTS_PATH='../../resources/config/engine/emotionevents'

import os
import sys
import collections
import csv

## the default json package doesn't support comments, so load demjson
scriptPath = os.path.dirname(os.path.realpath(__file__))
sys.path.append(scriptPath + "/behavior/demjson")
# TODO: move demjson somewhere else?
import demjson

def getEvents(dirname):
    ret = []
    
    for root, dirs, files in os.walk(dirname):
        for filename in files:
            if os.path.splitext(filename)[1] == '.json':
                full_path = os.path.join(root, filename)

                try:
                    J = demjson.decode_file(full_path)
                except demjson.JSONException as e:
                    print("Error: could not read json file {}: {}".format(full_path, e))
                    return None

                if "emotionEvents" not in J:
                    print("Error: no 'emotionEvents' field in file {}".format(full_path))
                    return None

                for event in J["emotionEvents"]:
                    for emo in event["emotionAffectors"]:
                        if emo["emotionType"] == "Stimulated":
                            if "repetitionPenalty" in event:
                                cooldown = "custom"
                            else:
                                cooldown = "default"
                            ret.append( {"name": event["name"], "stim": emo["value"], "cooldown":cooldown,
                                         "file":filename} )
    return ret

eventsDir = os.path.join(scriptPath, REL_EVENTS_PATH)

events = getEvents(eventsDir)

if events:
    writer = csv.DictWriter(sys.stdout, fieldnames=events[0].keys())
    writer.writeheader()
    for row in events:
        writer.writerow(row)
