#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Hello World

Make Cozmo say 'Hello World' in this simple Cozmo SDK example program.
'''

import sys

import cozmo

def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''
    coz = coz_conn.wait_for_robot()
    coz.say_text("Hello World").wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()

    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
