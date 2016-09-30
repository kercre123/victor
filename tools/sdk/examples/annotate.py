#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

'''Display a GUI window showing an annotated camera view.

Note:
    This example requires Python to have Tkinter installed to display the GUI.
    It also requires the Pillow and numpy python packages to be pip installed.

The :class:`cozmo.world.World` object collects raw images from Cozmo's camera
and makes them available as a property (:attr:`~cozmo.world.World.latest_image`)
and by generating :class:`cozmo.world.EvtNewCamerImages` events as they come in.

Each image is an instance of :class:`cozmo.world.CameraImage` which provides
access both to the raw camera image, and to a scalable annotated image which
can show where Cozmo sees faces and objects, along with any other information
your program may wish to display.

This example uses the tkviewer to display the annotated camera on the screen
and adds a couple of custom annotations of its own using two different methods.
'''


import asyncio
import sys
import time

try:
    from PIL import ImageDraw, ImageFont
except ImportError:
    sys.exit('run `pip3 install Pillow numpy` to run this example')

import cozmo


# Define an annotator using the annotator decorator
@cozmo.annotate.annotator
def clock(image, scale, annotator=None, world=None, **kw):
    d = ImageDraw.Draw(image)
    bounds = (0, 0, image.width, image.height)
    text = cozmo.annotate.ImageText(time.strftime("%H:%m:%S"),
            position=cozmo.annotate.TOP_LEFT)
    text.render(d, bounds)

# Define another decorator as a subclass of Annotator
class Battery(cozmo.annotate.Annotator):
    def apply(self, image, scale):
        d = ImageDraw.Draw(image)
        bounds = (0, 0, image.width, image.height)
        batt = self.world.robot.battery_voltage
        text = cozmo.annotate.ImageText('BATT %.1fv' % batt, color='green')
        text.render(d, bounds)


async def run(coz_conn):
    coz = await coz_conn.wait_for_robot()
    coz.world.image_annotator.add_static_text('text', 'Coz-Cam', position=cozmo.annotate.TOP_RIGHT)
    coz.world.image_annotator.add_annotator('clock', clock)
    coz.world.image_annotator.add_annotator('battery', Battery)

    await asyncio.sleep(2)

    print("Turning off all annotations for 2 seconds")
    coz.world.image_annotator.annotation_enabled = False
    await asyncio.sleep(2)

    print('Re-enabling all annotations')
    coz.world.image_annotator.annotation_enabled = True

    # Disable the face annotator after 10 seconds
    await asyncio.sleep(10)
    print("Disabling face annotations (light cubes still annotated)")
    coz.world.image_annotator.disable_annotator('faces')

    # Shutdown the program after 100 seconds
    await asyncio.sleep(100)


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect_with_tkviewer(run)
