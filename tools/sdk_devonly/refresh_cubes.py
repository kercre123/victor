#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo
import time
import os, json
import functools

from cozmo import _clad

'''Dev-only script, refresh cubes 
'''

_blocksConnected = True

#Block pool data messages come back once everything is connected
#and when we send out the get block pool message
#This will check if enough devices are connected and if so set the
#_blocksConnected flag so we can refresh again
def handle_block_pool_data_message(evt, *, msg):
    global _blocksConnected
    _blocksConnected = len(msg.blockData) >= 4
    print("Block pool items: ", len(msg.blockData))
    for block in msg.blockData:
        print(block)

def run(sdk_conn):
    global _blocksConnected
    robot = sdk_conn.wait_for_robot()
    robot.add_event_handler(_clad._MsgBlockPoolDataMessage, handle_block_pool_data_message)

    reset_block_msg = cozmo._clad._clad_to_engine_iface.BlockPoolResetMessage()
    get_block_pool_msg = cozmo._clad._clad_to_engine_iface.GetBlockPoolMessage()

    #Loop checking if blocks are connected and once they are connected refresh them
    while True:
        while not _blocksConnected:
            sdk_conn.send_msg(get_block_pool_msg)
            time.sleep(1)
        print("Refreshing Blockpool")
        _blocksConnected = False
        sdk_conn.send_msg(reset_block_msg)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
