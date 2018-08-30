# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Vector's OLED screen that displays his face - related functions and values.

Copyright (c) 2018 Anki, Inc.
"""

from . import sync, color, util
from .messaging import protocol

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['dimensions', 'convert_pixels_to_screen_data',
           'convert_image_to_screen_data', 'OledComponent']


SCREEN_WIDTH = 184
SCREEN_HEIGHT = 96


def dimensions():
    """Return the dimension (width, height) of the oled screen.

    Returns:
        A tuple of ints (width, height)
    """
    return SCREEN_WIDTH, SCREEN_HEIGHT


def convert_pixels_to_screen_data(pixel_data, image_width, image_height):
    """Convert a sequence of pixel data to the correct format to display on Vector's face.

    Args:
        pixel_data (list): sequence of triplets representing rgb values, should be ints from 0-255
        image_width (int): width of the image defined by the pixel_data
        image_height (int): height of the image defined by the pixel_data

    Returns:
        A :class:`bytes` object representing all of the pixels (16bit color in rgb565 format)

    Raises:
        ValueError: Invalid Dimensions
        ValueError: Bad image_width
        ValueError: Bad image_height
    """

    if len(pixel_data) != (image_width * image_height):
        raise ValueError('Invalid Dimensions: len(pixel_data) {0} != image_width={1} * image_height={2} (== {3})'. format(len(pixel_data),
                                                                                                                          image_width,
                                                                                                                          image_height,
                                                                                                                          image_width *
                                                                                                                          image_height))

    # @TODO: We should decide on a resampling approach and have this function automatically rescale images
    #  We should either enforce the aspect ratio, or have options to:
    #  - automatically crop to the proper aspect ratio
    #  - stretch to fit
    #  - shrink to fit with margins some default color
    if image_width != SCREEN_WIDTH:
        raise ValueError('Bad image_width: image_width {0} must be the resolution width: {1}'. format(image_width, SCREEN_WIDTH))

    if image_height != SCREEN_HEIGHT:
        raise ValueError('Bad image_height: image_height {0} must be the resolution height: {1}'. format(image_width, SCREEN_HEIGHT))

    color_565_data = []
    for color_tuple in pixel_data:
        color_object = color.Color(rgb=color_tuple)
        color_565_data.extend(color_object.rgb565_bytepair)

    return bytes(color_565_data)


def convert_image_to_screen_data(image):
    """ Convert an image into the correct format to display on Vector's face.

    Args:
        image (:class:`~PIL.Image.Image`): The image to display on Vector's face

    Returns:
        A :class:`bytes` object representing all of the pixels (16bit color in rgb565 format)
    """
    image_data = image.getdata()

    return convert_pixels_to_screen_data(image_data, image.width, image.height)


class OledComponent(util.Component):
    @sync.Synchronizer.wrap
    async def set_oled_with_screen_data(self, image_data, duration_sec, interrupt_running=True):
        if not isinstance(image_data, bytes):
            raise ValueError("set_oled_with_screen_data expected bytes")
        if len(image_data) != 35328:
            raise ValueError("set_oled_with_screen_data expected 35328 bytes - (2 bytes each for 17664 pixels)")

        # Generate the message
        message = protocol.DisplayFaceImageRGBRequest()
        # Create byte array at the oled resolution
        message.face_data = image_data
        message.duration_ms = int(1000 * duration_sec)
        message.interrupt_running = interrupt_running

        return await self.interface.DisplayFaceImageRGB(message)

    def set_oled_to_color(self, solid_color, duration_sec, interrupt_running=True):
        image_data = bytes(solid_color.rgb565_bytepair * 17664)
        return self.set_oled_with_screen_data(image_data, duration_sec, interrupt_running)
