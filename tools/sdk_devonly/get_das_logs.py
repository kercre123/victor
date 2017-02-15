#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo
import time
import os, json

from cozmo import _clad

'''Dev-only script, get das logs
'''

_dasData = dict()

#File in a dictionary for each file on the unit
def handle_json_das_log_message(evt, *, msg):
    if msg.fileName in _dasData:
        _dasData[msg.fileName] += msg.jsonData
    else:
        _dasData[msg.fileName] = msg.jsonData

#Once we have all the messages write them to thier given files in the das_logs directory
def handle_json_das_log_all_sent_message(evt, *, msg):
    if not os.path.exists("das_logs"):
        os.mkdir("das_logs")
    for fileName, jsonData in _dasData.items():
        file = open("das_logs/"+fileName, 'w')
        jsonParsed = json.loads(jsonData)
        prettyJson = json.dumps(jsonParsed, sort_keys=True, indent=4)
        file.write(prettyJson)
    os._exit(0)

def run(coz_conn):
    # GetJsonDasLogsMessage is now ignored when sent from an SDK connection,
    # but for internal engine builds we can disable that via a console var
    msg = cozmo._clad._clad_to_engine_iface.SetDebugConsoleVarMessage(varName="AllowBannedSdkMessages", tryValue="1")
    coz_conn.send_msg(msg)

    get_json_das_logs = cozmo._clad._clad_to_engine_iface.GetJsonDasLogsMessage()
    coz_conn.add_event_handler(_clad._MsgJsonDasLogMessage, handle_json_das_log_message)
    coz_conn.add_event_handler(_clad._MsgJsonDasLogAllSentMessage, handle_json_das_log_all_sent_message)
    coz_conn.send_msg(get_json_das_logs)

    #Give a 10 minute time out for this request to come back,
    #it should never take more than a minute or two
    time.sleep(600)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
