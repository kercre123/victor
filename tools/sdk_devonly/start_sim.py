#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import argparse
import cozmo
import time

'''Dev-only script, starts engine and connects to a sim robot (for running with a sim robot in Webots with no app)
'''


_is_engine_loaded = False
_options = None

def handle_engine_loading_state(evt, *, msg):
  if msg.ratioComplete == 1.0:
    print("Engine Finished Loading!")
    global _is_engine_loaded
    _is_engine_loaded = True


def handle_robot_found(evt, **kw):
    print("Robot found - assume engine must already have been loaded!")
    global _is_engine_loaded
    _is_engine_loaded = True


def start_sim(coz_conn):
    global _options
    "start sim - starts the engine and connects to a sim robot"
    
    coz_conn.add_event_handler(cozmo._clad._MsgEngineLoadingDataStatus, handle_engine_loading_state)
    coz_conn.add_event_handler(cozmo.conn.EvtRobotFound, handle_robot_found)
    
    start_engine_msg = cozmo._clad._clad_to_engine_iface.StartEngine()
    coz_conn.send_msg(start_engine_msg)
    
    time_waiting_for_engine_load = 0
    max_time_waiting_for_engine_load = 60.0 if _options and _options.is_real_robot else 6.0
    global _is_engine_loaded
    while (not _is_engine_loaded) and (time_waiting_for_engine_load < max_time_waiting_for_engine_load):
      sleep_time = 0.5
      time.sleep(sleep_time)
      time_waiting_for_engine_load += sleep_time
      if not _is_engine_loaded and coz_conn._primary_robot and coz_conn._primary_robot.is_ready:
        print("We have a robot, must have missed the event - assuming engine was already loaded")
        _is_engine_loaded = True

    if _is_engine_loaded:
      print("Took %s seconds to load engine" % time_waiting_for_engine_load)
    else:
      print("Timed out after %s seconds waiting to load engine" % time_waiting_for_engine_load)

    is_simulated = 0 if _options and _options.is_real_robot else 1

    if is_simulated != 0:
      ip_address = "127.0.0.1\0\0\0\0\0\0\0".encode('ascii')
    else:
      ip_address = "172.31.1.1\0\0\0\0\0\0".encode('ascii')

    connect_robot_msg = cozmo._clad._clad_to_engine_iface.ConnectToRobot(ipAddress=ip_address, robotID=1, isSimulated=is_simulated)
    coz_conn.send_msg(connect_robot_msg)


def run(coz_conn):
  
    start_sim(coz_conn)
    coz = coz_conn.wait_for_robot()
    cozmo.logger.info("Done")


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--real-robot',
                        dest='is_real_robot',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Run with a real robot instead of a sim')
    _options = parser.parse_args()
    cozmo.setup_basic_logging()
    cozmo.connect(run)
