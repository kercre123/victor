#!/usr/bin/env python3

class Color:
    '''A Color to be used with a Light.

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

#: :class:`Color`:  instance representing no color (LEDs off).
off = Color(name="off")


class BackpackColorProfile:
    '''A Color profile send to be used with messaged involving the robot's backpack LEDs.

    Args:
        red_multiplier (float): Scaling value for the brightness of backpack red LED's
        green_multiplier (float): Scaling value for the brightness of backpack green LED's
        blue_multiplier (float): Scaling value for the brightness of backpack blue LED's
    '''

    def __init__(self, red_multiplier, green_multiplier, blue_multiplier):
        self._red_multiplier = red_multiplier
        self._green_multiplier = green_multiplier
        self._blue_multiplier = blue_multiplier

    def augment_color(self, color):
        rgb = [
            (color.int_color >> 24) & 0xff,
            (color.int_color >> 16) & 0xff,
            (color.int_color >> 8) & 0xff
            ]

        rgb[0] = int(self._red_multiplier * rgb[0])
        rgb[1] = int(self._green_multiplier * rgb[1])
        rgb[2] = int(self._blue_multiplier * rgb[2])

        result_int_code = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8) | 0xff
        return Color(result_int_code)

    @property
    def red_multiplier(self):
        '''float: The multiplier used on the red channel.'''
        return self._red_multiplier

    @property
    def green_multiplier(self):
        '''float: The multiplier used on the red channel.'''
        return self._green_multiplier

    @property
    def blue_multiplier(self):
        '''float: The multiplier used on the red channel.'''
        return self._blue_multiplier


#: :class:`BackpackColorProfile`:  Color profile to get the maximum possible brightness out of each LED.
max_brightness_backpack_profile = BackpackColorProfile(red_multiplier=1.0, green_multiplier=1.0, blue_multiplier=1.0)

#: :class:`BackpackColorProfile`:  Color profile balanced so that a max color value more closely resembles pure white.
#TODO: Balance this more carefully once robots with proper color pipe hardware becomes available
white_balanced_backpack_profile = BackpackColorProfile(red_multiplier=1.0, green_multiplier=0.825, blue_multiplier=0.81)


class Light:
    '''Lights are used with LightCubes and Vector's backpack.

    Lights may either be "on" or "off", though in practice any colors may be
    assigned to either state (including no color/light).
    '''

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
        ''':class:`Color`: The Color shown when the light is on.'''
        return self._on_color

    @on_color.setter
    def on_color(self, color):
        if not isinstance(color, Color):
            raise TypeError("Must specify a Color")
        self._on_color = color

    @property
    def off_color(self):
        ''':class:`Color`: The Color shown when the light is off.'''
        return self._off_color

    @off_color.setter
    def off_color(self, color):
        if not isinstance(color, Color):
            raise TypeError("Must specify a Color")
        self._off_color = color

    @property
    def on_period_ms(self):
        '''int: The number of milliseconds the light should be "on" for for each cycle.'''
        return self._on_period_ms

    @on_period_ms.setter
    def on_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._on_period_ms = ms

    @property
    def off_period_ms(self):
        '''int: The number of milliseconds the light should be "off" for for each cycle.'''
        return self._off_period_ms

    @off_period_ms.setter
    def off_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._off_period_ms = ms

    @property
    def transition_on_period_ms(self):
        '''int: The number of milliseconds to take to transition the light to the on color.'''
        return self._transition_on_period_ms

    @transition_on_period_ms.setter
    def transition_on_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._transition_on_period_ms = ms

    @property
    def transition_off_period_ms(self):
        '''int: The number of milliseconds to take to transition the light to the off color.'''
        return self._transition_off_period_ms

    @transition_off_period_ms.setter
    def transition_off_period_ms(self, ms):
        if not 0 < ms < 2**32:
            raise ValueError("Invalid value")
        self._transition_off_period_ms = ms

def _set_light(msg, idx, light, profile):
    # For use with clad light messages specifically.
    if not isinstance(light, Light):
        raise TypeError("Expected a lights.Light")
    msg.onColor[idx] = profile.augment_color( light.on_color ).int_color
    msg.offColor[idx] = profile.augment_color( light.off_color ).int_color
    msg.onPeriod_ms[idx] = light.on_period_ms
    msg.offPeriod_ms[idx] = light.off_period_ms
    msg.transitionOnPeriod_ms[idx] = light.transition_on_period_ms
    msg.transitionOffPeriod_ms[idx] = light.transition_off_period_ms

#: :class:`Light`: A steady green colored LED light.
green_light = Light(on_color=green)

#: :class:`Light`: A steady red colored LED light.
red_light = Light(on_color=red)

#: :class:`Light`: A steady blue colored LED light.
blue_light = Light(on_color=blue)

#: :class:`Light`: A steady cyan colored LED light.
cyan_light = Light(on_color=cyan)

#: :class:`Light`: A steady magenta colored LED light.
magenta_light = Light(on_color=magenta)

#: :class:`Light`: A steady yellow colored LED light.
yellow_light = Light(on_color=yellow)

#: :class:`Light`: A steady white colored LED light.
white_light = Light(on_color=white)

#: :class:`Light`: A steady off (non-illuminated LED light).
off_light = Light(on_color=off)
