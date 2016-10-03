#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

'''Cozmo Alarm Clock

Use Cozmo's face to display the current time
Play an alarm (Cozmo tells you to wake up) at a set time

NOTE: This is an example program, we take no responsibility if it fails to wake you up on time
'''

import datetime
import sys
import time

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    sys.exit("Cannot import from PIL: Do `pip3 install Pillow` to install")

import cozmo


def make_text_image(text_to_draw, x, y, font=None):
    '''Make a Pillow.Image with the current time printed on it

    Args:
        text_to_draw (string): the text to draw to the image
        x (int): x pixel location
        y (int): y pixel location
        font (PIL.ImageFont): the font to use

    Returns:
        :class:(`PIL.Image.Image`): a Pillow image with the text drawn on it
    '''

    # make a blank image for the text, initialized to opaque black
    txt = Image.new('RGBA', cozmo.lcd_face.dimensions(), (0, 0, 0, 255))

    # get a drawing context
    dc = ImageDraw.Draw(txt)

    # draw the text
    dc.text((x, y), text_to_draw, fill=(255, 255, 255, 255), font=font)

    return txt


# get a font - location depends on OS so try a couple of options
# failing that the default of None will just use a default font
_clock_font = None
try:
    _clock_font = ImageFont.truetype("arial.ttf", 24)
except IOError:
    try:
        _clock_font = ImageFont.truetype("/Library/Fonts/Arial.ttf", 24)
    except IOError:
        pass


def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''

    robot = sdk_conn.wait_for_robot()

    # move head fully up. and lift down to the bottom, to make it easy to see Cozmo's face
    # (this will only do anything if Cozmo is off the charger)
    robot.set_lift_height(0.0).wait_for_completed()
    robot.set_head_angle(cozmo.robot.MAX_HEAD_ANGLE).wait_for_completed()

    alarm_time = datetime.time(8, 30)  # request an alarm for 08:30 (uses 24-hour clock)

    was_before_alarm_time = False
    last_displayed_time = ""

    try:
        while True:
            # Check the current time, and see if it's time to play the alarm

            current_time = datetime.datetime.now().time()
            is_before_alarm_time = current_time < alarm_time
            do_alarm = was_before_alarm_time and not is_before_alarm_time  # did we just cross the alarm time
            was_before_alarm_time = is_before_alarm_time

            if do_alarm:

                # Play the alarm, moving cozmo off the contacts if necessary (so that we can play an animation)

                was_on_charger = robot.is_on_charger
                robot.drive_off_charger_contacts().wait_for_completed()
                short_time_string = str(current_time.hour) + ":" + str(current_time.minute)
                robot.say_text("Wake up lazy human! it's " + short_time_string).wait_for_completed()

                # reverse back onto the charger (if we just drove him off) so that he can go back to charging

                if was_on_charger:
                    robot.drive_wheels(-50, -50, duration=2)
            else:

                # See if the displayed time needs updating

                current_time_string = time.strftime("%I:%M:%S")
                if current_time_string != last_displayed_time:

                    # Create the updated image with this time
                    text_image = make_text_image(time.strftime("%I:%M:%S"), 15, 2, _clock_font)
                    lcd_face_data = cozmo.lcd_face.convert_image_to_screen_data(text_image)

                    # display for 1 second
                    robot.display_lcd_face_image(lcd_face_data, 1000.0)
                    last_displayed_time = current_time_string

            # only sleep for a fraction of a second to ensure we update the seconds as soon as they change
            time.sleep(0.1)

    except KeyboardInterrupt:
        print("")
        print("Exit requested by user")


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.robot.Robot.drive_off_charger_on_connect = False  # Cozmo can stay on his charger for this example
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A konnection error occurred: %s" % e)
