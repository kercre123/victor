#!/usr/bin/env python3


class Color:
    '''A Color to be used with a Light or OLED.

    Either int_color or rgb may be used to specify the actual color.
    Any alpha components (from int_color) are ignored - all colors are fully opaque.

    Args:
        int_color (int): A 32 bit value holding the binary RGBA value (where A
            is ignored and forced to be fully opaque).
        rgb (tuple): A tuple holding the integer values from 0-255 for (reg, green, blue)
        name (str): A name to assign to this color
    '''

    def __init__(self, int_color=None, rgb=None, name=None):
        self.name = name
        self._int_color = 0
        if int_color is not None:
            self._int_color = int_color | 0xff
        elif rgb is not None:
            self._int_color = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8) | 0xff

    @property
    def int_color(self):
        '''int: The encoded integer value of the color.'''
        return self._int_color

    @property
    def rgb565_bytepair(self):
        '''bytes[]: two bytes representing an int16 color with rgb565 encoding

        This format reflects the robot's oled color range, and performing this
        conversion will reduce network traffic when sending oled data.
        '''

        red5 = ((self._int_color >> 24) & 0xff) >> 3
        green6 = ((self._int_color >> 16) & 0xff) >> 2
        blue5 = ((self._int_color >> 8) & 0xff) >> 3

        green3_hi = green6 >> 3
        green3_low = green6 & 0x07

        int_565_color_lowbyte = (green3_low << 5) | blue5
        int_565_color_highbyte = (red5 << 3) | green3_hi

        return [int_565_color_highbyte, int_565_color_lowbyte]


#: :class:`Color`: Green color instance.
green = Color(name="green", int_color=0x00ff00ff)

#: :class:`Color`: Red color instance.
red = Color(name="red", int_color=0xff0000ff)

#: :class:`Color`: Blue color instance.
blue = Color(name="blue", int_color=0x0000ffff)

#: :class:`Color`: Cyan color instance.
cyan = Color(name="cyan", int_color=0x00ffffff)

#: :class:`Color`: Magenta color instance.
magenta = Color(name="magenta", int_color=0xff00ffff)

#: :class:`Color`: Yellow color instance.
yellow = Color(name="yellow", int_color=0xffff00ff)

#: :class:`Color`: White color instance.
white = Color(name="white", int_color=0xffffffff)

#: :class:`Color`: instance representing no color (LEDs off).
off = Color(name="off")
