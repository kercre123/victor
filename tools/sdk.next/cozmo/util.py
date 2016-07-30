__ALL__ = ['Angle', 'degrees', 'radians']

import collections
import math
import re


# from https://stackoverflow.com/questions/1175208/elegant-python-function-to-convert-camelcase-to-snake-case
_first_cap_re = re.compile('(.)([A-Z][a-z]+)')
_all_cap_re = re.compile('([a-z0-9])([A-Z])')
def _uncamelcase(name):
    s1 = _first_cap_re.sub(r'\1_\2', name)
    return _all_cap_re.sub(r'\1_\2', s1).lower()


ImageBox = collections.namedtuple('ImageBox', 'top_left_x top_left_y width height')
ImageBox.__doc__ += ': Defines a bounding box within an image frame.'


class Angle:
    '''Represents an angle.

    Use the :func:`degrees` or :func:`radians` convenience methods to generate
    an Angle instance.
    '''

    __slots__ = ('_radians')

    def __init__(self, radians=None, degrees=None):
        if radians is None and degrees is None:
            raise ValueError("Expected either the degrees or radians keyword argument")
        if radians and degrees:
            raise ValueError("Expected either the degrees or radians keyword argument, not both")

        if degrees:
            radians = degrees * math.pi / 180
        self._radians = radians

    def __repr__(self):
        return "<%s %.1f radians (%1.f degrees)>" % (self.__class__.__name__, self.radians, self.degrees)

    def __add__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported operand for + - expected Angle")
        self._radians += other.radians

    def __sub__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported operand for + - expected Angle")
        self._radians -= other.radians

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for + - expected number")
        self._radians *= other

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for + - expected number")
        self._radians /= other

    @property
    def radians(self):
        return self._radians

    @property
    def degrees(self):
        return self._radians / math.pi * 180


def degrees(degrees):
    '''Returns an Angle instance set to the specified number of degrees.'''
    return Angle(degrees=degrees)

def radians(radians):
    '''Returns an Angle instance set to the specified number of radians.'''
    return Angle(radians=radians)
