# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['Color', 'Light',
    'green', 'red', 'blue', 'white', 'off',
    'green_light', 'red_light', 'blue_light', 'white_light', 'off_light']

import copy

from . import logger


class Color:
    '''A Color to be used with a Light.'''

    def __init__(self, int_color=None, rgba=None, rgb=None, name=None):
        '''Initialize a Color.

        One of int_color, rgba or rgb may be used to specify the actual color.
        Args:
            int_color (int): A 32 bit value holding the binary RGBA value
            rgba (tuple): A tuple holding the integer values from 0-255 for (red, green, blue, alpha)
            rgb (tuple): A tuple holding the integer values from 0-255 for (reg, green, blue) - Alpha defaults to 255
            name (str): A name to assign to this color
        '''
        self.name = name
        self._int_color = 0
        if int_color is not None:
            self._int_color = int_color
        elif rgb is not None:
            self._int_color = (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | 0xff
        elif rgba is not None:
            self._int_color = (rgba[0] << 24) | (rgba[1] << 16) | (rgba[2] << 8) | rgba[3]

    @property
    def int_color(self):
        '''The encoded integer value of the color.'''
        return self._int_color

    def with_alpha(self, alpha):
        """Return the same color with a different alpha value.

        Args:
            alpha (int): A value from 0-255
        Returns:
            A new :class:`Color` instance.
        """
        if not 0 < alpha < 255:
            raise ValueError("Illegal value for alpha")
        return Color(int_color=(self._int_color & 0xffffff00) | alpha)


green = Color(name="green", int_color=0x00ff00ff)
red = Color(name="red", int_color=0xff0000ff)
blue = Color(name="blue", int_color=0x0000ffff)
white = Color(name="white", int_color=0xffffffff)
off = Color(name="off")


class Light:
    '''Lights are used with LightCubes and Cozmo's backpack.'''

    def __init__(self, on_color=off, off_color=off, on_period_ms=250,
            off_period_ms=0, transition_on_period_ms=0, transition_off_period_ms=0):
        self._on_color = on_color
        self._off_color = off_color
        self._on_period_ms = on_period_ms
        self._off_period_ms = off_period_ms
        self._transition_on_period_ms = transition_on_period_ms
        self._transition_off_period_ms = transition_off_period_ms

    @property
    def on_color(self):
        return self._on_color

    @on_color.setter
    def on_color(self, color):
        if not isinstance(color, Color):
            raise TypeError("Must specify a Color")
        self._on_color = color

    @property
    def off_color(self):
        return self._off_color

    @off_color.setter
    def off_color(self, color):
        if not isinstance(color, Color):
            raise TypeError("Must specify a Color")
        self._off_color = color

    @property
    def on_period_ms(self):
        return self._on_period_ms

    @on_period_ms.setter
    def on_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._on_period_ms = ms

    @property
    def off_period_ms(self):
        return self._off_period_ms

    @off_period_ms.setter
    def off_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._off_period_ms = ms

    @property
    def transition_on_period_ms(self):
        return self._transition_on_period_ms

    @transition_on_period_ms.setter
    def transition_on_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._transition_on_period_ms = ms

    @property
    def transition_off_period_ms(self):
        return self._transition_off_period_ms

    @transition_off_period_ms.setter
    def transition_off_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._transition_off_period_ms = ms

    def flash(self, on_period_ms=250, off_period_ms=250, off_color=off):
        '''Convenience function to make a flashing version of an existing Light instance.'''
        flasher = copy.copy(self)
        flasher.on_period_ms = on_period_ms
        flasher.off_period_ms = off_period_ms
        flasher.off_color=off_color
        return flasher

def _set_light(msg, idx, light):
    # For use with clad light messages specifically. 
    if not isinstance(light, Light):
        raise TypeError("Expected a lights.Light")
    msg.onColor[idx] = light.on_color.int_color
    msg.offColor[idx] = light.off_color.int_color
    msg.onPeriod_ms[idx] = light.on_period_ms
    msg.offPeriod_ms[idx] = light.off_period_ms
    msg.transitionOnPeriod_ms[idx] = light.transition_on_period_ms
    msg.transitionOffPeriod_ms[idx] = light.transition_off_period_ms   

#There is a glitch so it will always flash unless on_color==off_color 
#ticket is COZMO-3319
green_light = Light(on_color=green, off_color=green)
red_light = Light(on_color=red, off_color=red)
blue_light = Light(on_color=blue, off_color=blue)
white_light = Light(on_color=white, off_color=white)
off_light = Light(on_color=off, off_color=off)
