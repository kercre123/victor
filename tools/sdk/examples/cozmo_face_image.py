#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

'''Display images on Cozmo's face (lcd screen)
'''

import sys
import time

try:
    from PIL import Image
except ImportError:
    sys.exit("Cannot import from PIL: Do `pip3 install Pillow` to install")

import cozmo


def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''

    coz = coz_conn.wait_for_robot()

    # move head fully up. and lift down to the bottom, to make it easy to see Cozmo's face
    coz.set_lift_height(0.0).wait_for_completed()
    coz.set_head_angle(cozmo.robot.MAX_HEAD_ANGLE).wait_for_completed()

    # load an image and resize it to Cozmo's screen dimensions
    anki_logo = Image.open("ankilogo.png")
    anki_logo = anki_logo.resize(cozmo.lcd_face.dimensions(), Image.BICUBIC)

    # convert the image to the format used bt the lcd screen
    anki_logo = cozmo.lcd_face.convert_image_to_screen_data(anki_logo, invert_image=True)

    # display the image on Cozmo's face for duration_s seconds
    # (currently clamped at 30 seconds internally to prevent burn-in)
    duration_s = 5.0
    coz.display_lcd_face_image(anki_logo, duration_s * 1000.0)
    time.sleep(duration_s)


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
