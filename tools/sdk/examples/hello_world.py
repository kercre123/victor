#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo

'''Simplest "Hello World" Cozmo example program
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    coz.say_text("Hello World").wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
