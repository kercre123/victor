#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

'''Cozmo Alarm Clock

Use Cozmo's face to display the current time
Play an alarm (Cozmo tells you to wake up) at a set time

NOTE: This is an example program, we take no responsibility if it fails to wake you up on time
'''

from contextlib import contextmanager
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


def convert_to_time_int(in_value, time_unit):
    '''Convert in_value to an int and ensure it's in the valid range for that time unit

    (e.g. 0..23 for hours)'''

    max_for_time_unit = {'hours': 23, 'minutes': 59, 'seconds': 59}
    max_val = max_for_time_unit[time_unit]

    try:
        int_val = int(in_value)
    except ValueError:
        raise ValueError("%s value '%s' is not an int" % (time_unit, in_value))

    if int_val < 0:
        raise ValueError("%s value %s is negative" % (time_unit, int_val))

    if int_val > max_val:
        raise ValueError("%s value %s exceeded %s" % (time_unit, int_val, max_val))

    return int_val


def extract_time_from_args():
    ''' Extract a (24-hour-clock) user-specified time from the command-line

    Supports colon and space separators - e.g. all 3 of "11 22 33", "11:22:33" and "11 22:33"
    would map to the same time.
    The seconds value is optional, defaults to 0 if not provided.'''

    # split sys.argv further for any args that contain a ":"
    split_time_args = []
    for i in range(1, len(sys.argv)):
        arg = sys.argv[i]
        split_args = arg.split(':')
        for split_arg in split_args:
            split_time_args.append(split_arg)

    if len(split_time_args) >= 2:
        try:
            hours = convert_to_time_int(split_time_args[0], 'hours')
            minutes = convert_to_time_int(split_time_args[1], 'minutes')
            seconds = 0
            if len(split_time_args) >= 3:
                seconds = convert_to_time_int(split_time_args[2], 'seconds')

            return datetime.time(hours, minutes, seconds)
        except ValueError as e:
            print("ValueError %s" % e)

    # Default to no alarm
    return None


def backup_onto_charger(robot):
    '''Attempts to reverse robot onto its charger

    Assumes charger is directly behind cozmo
    Keep driving straight back until until charger is in contact
    '''

    robot.drive_wheels(-30, -30)
    time_waited = 0.0
    while time_waited < 3.0 and not robot.is_on_charger:
        sleep_time_s = 0.1
        time.sleep(sleep_time_s)
        time_waited += sleep_time_s

    robot.stop_all_motors()


@contextmanager
def perform_operation_off_charger(robot):
    '''Perform a block of code with robot off the charger

    Ensure robot is off charger before yielding
    yield - (at which point any code in the caller's with block will run).
    If Cozmo started on the charger then return them back afterwards'''

    was_on_charger = robot.is_on_charger
    robot.drive_off_charger_contacts().wait_for_completed()

    yield robot

    if was_on_charger:
        backup_onto_charger(robot)


def get_in_position(robot):
    '''If necessary, Move Cozmo's Head and Lift to make it easy to see Cozmo's face'''
    if (robot.lift_height.distance_mm > 45) or (robot.head_angle.degrees < 40):
        with perform_operation_off_charger(robot):
            robot.set_lift_height(0.0).wait_for_completed()
            robot.set_head_angle(cozmo.robot.MAX_HEAD_ANGLE).wait_for_completed()


def alarm_clock(robot):
    '''The core of the alarm_clock program'''

    alarm_time = extract_time_from_args()
    if alarm_time:
        print("Alarm set for %s" % alarm_time)
    else:
        print("No Alarm time provided")

    get_in_position(robot)

    was_before_alarm_time = False
    last_displayed_time = ""

    while True:
        # Check the current time, and see if it's time to play the alarm

        current_time = datetime.datetime.now().time()

        do_alarm = False
        if alarm_time:
            is_before_alarm_time = current_time < alarm_time
            do_alarm = was_before_alarm_time and not is_before_alarm_time  # did we just cross the alarm time
            was_before_alarm_time = is_before_alarm_time

        if do_alarm:
            # Speak The Time (off the charger as it's an animation)
            with perform_operation_off_charger(robot):
                short_time_string = str(current_time.hour) + ":" + str(current_time.minute)
                robot.say_text("Wake up lazy human! it's " + short_time_string).wait_for_completed()
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


def run(sdk_conn):
    '''The run method runs once the Cozmo SDK is connected.'''

    robot = sdk_conn.wait_for_robot()

    try:
        alarm_clock(robot)

    except KeyboardInterrupt:
        print("")
        print("Exit requested by user")


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.robot.Robot.drive_off_charger_on_connect = False  # Cozmo can stay on his charger for this example
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
