#!/usr/bin/env python3
from . import color

class ColorProfile:
    '''A Color profile send to be used with messages involving LEDs.

    Args:
        red_multiplier (float): Scaling value for the brightness of red LEDs
        green_multiplier (float): Scaling value for the brightness of green LEDs
        blue_multiplier (float): Scaling value for the brightness of blue LEDs
    '''

    def __init__(self, red_multiplier, green_multiplier, blue_multiplier):
        self._red_multiplier = red_multiplier
        self._green_multiplier = green_multiplier
        self._blue_multiplier = blue_multiplier

    def augment_color(self, original_color):
        rgb = [
            (original_color.int_color >> 24) & 0xff,
            (original_color.int_color >> 16) & 0xff,
            (original_color.int_color >> 8) & 0xff
            ]

        rgb[0] = int(self._red_multiplier * rgb[0])
        rgb[1] = int(self._green_multiplier * rgb[1])
        rgb[2] = int(self._blue_multiplier * rgb[2])

        result_int_code = (rgb[0] << 24) | (rgb[1] << 16) | (rgb[2] << 8) | 0xff
        return color.Color(result_int_code)

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


#: :class:`ColorProfile`:  Color profile to get the maximum possible brightness out of each LED.
max_color_profile = ColorProfile(red_multiplier=1.0, green_multiplier=1.0, blue_multiplier=1.0)

#: :class:`ColorProfile`:  Color profile balanced so that a max color value more closely resembles pure white.
#TODO: Balance this more carefully once robots with proper color pipe hardware becomes available
white_balanced_backpack_profile = ColorProfile(red_multiplier=1.0, green_multiplier=0.825, blue_multiplier=0.81)

#: :class:`ColorProfile`:  Color profile balanced so that a max color value more closely resembles pure white.
#TODO: Balance this more carefully once robots with proper color pipe hardware becomes available
white_balanced_cube_profile = ColorProfile(red_multiplier=1.0, green_multiplier=0.95, blue_multiplier=0.7)

class Light:
    '''Lights are used with LightCubes and Vector's backpack.

    Lights may either be "on" or "off", though in practice any colors may be
    assigned to either state (including no color/light).
    '''

    def __init__(self, on_color=color.off, off_color=color.off, on_period_ms=250,
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
        if not isinstance(color, color.Color):
            raise TypeError("Must specify a Color")
        self._on_color = color

    @property
    def off_color(self):
        ''':class:`Color`: The Color shown when the light is off.'''
        return self._off_color

    @off_color.setter
    def off_color(self, color):
        if not isinstance(color, color.Color):
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
    

def package_request_params(lights, color_profile):
    merged_params = {}
    for light in lights:
        for attr_name in vars(light):
            attr_name = attr_name[1:]
            attr_val = getattr(light, attr_name)
            if isinstance(attr_val, color.Color):
                attr_val = color_profile.augment_color(attr_val).int_color
            merged_params.setdefault(attr_name, []).append(attr_val)
    return merged_params

#: :class:`Light`: A steady green colored LED light.
green_light = Light(on_color=color.green)

#: :class:`Light`: A steady red colored LED light.
red_light = Light(on_color=color.red)

#: :class:`Light`: A steady blue colored LED light.
blue_light = Light(on_color=color.blue)

#: :class:`Light`: A steady cyan colored LED light.
cyan_light = Light(on_color=color.cyan)

#: :class:`Light`: A steady magenta colored LED light.
magenta_light = Light(on_color=color.magenta)

#: :class:`Light`: A steady yellow colored LED light.
yellow_light = Light(on_color=color.yellow)

#: :class:`Light`: A steady white colored LED light.
white_light = Light(on_color=color.white)

#: :class:`Light`: A steady off (non-illuminated LED light).
off_light = Light(on_color=color.off)
