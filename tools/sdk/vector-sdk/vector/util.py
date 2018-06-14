# Copyright (c) 2018 Anki, Inc.

'''
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['setup_basic_logging', 'get_class_logger']

import logging
import os
import sys

MODULE_LOGGER = logging.getLogger(__name__)

def setup_basic_logging(custom_handler=None, general_log_level=None, target=sys.stderr):
    '''Helper to perform basic setup of the Python logging machinery.
    Args:
        general_log_level (str): 'DEBUG', 'INFO', 'WARN', 'ERROR' or an equivalent
            constant from the :mod:`logging` module.  If None then a
            value will be read from the COZMO_LOG_LEVEL environment variable.
        target (object): The stream to send the log data to; defaults to stderr
    '''
    if general_log_level is None:
        general_log_level = os.environ.get('VICTOR_LOG_LEVEL', logging.DEBUG)

    handler = custom_handler
    if handler is None:
        handler = logging.StreamHandler(stream=target)
        formatter = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
        handler.setFormatter(formatter)

    vector_logger = logging.getLogger('vector')
    if not vector_logger.handlers:
        vector_logger.addHandler(handler)
        vector_logger.setLevel(general_log_level)

def get_class_logger(module, obj):
    '''Helper to create logger for a given class (and module)'''
    return logging.getLogger(".".join([module, type(obj).__name__]))


class Vector3:
    '''Represents a 3D Vector (type/units aren't specified)
    Args:
        x (float): X component
        y (float): Y component
        z (float): Z component
    '''

    __slots__ = ('_x', '_y', '_z')

    def __init__(self, x, y, z):
        self._x = x
        self._y = y
        self._z = z

    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    @property
    def z(self):
        return self._z   

    def __repr__(self):
        return (f"<{self.__class__.__name__} x: {self.x:.2} y: {self.y:.2} z: {self.z:.2}>")


class Quaternion:
    '''Represents the rotation of an object in the world.'''

    __slots__ = ('_q0', '_q1', '_q2', '_q3')

    def __init__(self, q0, q1, q2, q3):
        self._q0 = q0
        self._q1 = q1
        self._q2 = q2
        self._q3 = q3

    @property
    def q0(self):
        return self._q0

    @property
    def q1(self):
        return self._q1

    @property
    def q2(self):
        return self._q2

    @property
    def q3(self):
        return self._q3
    
    def __repr__(self):
        return (f"<{self.__class__.__name__} q0: {self.q0:.2} q1: {self.q1:.2} q2: {self.q2:.2} q3: {self.q3:.2}>")


class Position(Vector3):
    '''Represents the position of an object in the world.
    A position consists of its x, y and z values in millimeters.
    Args:
        x (float): X position in millimeters
        y (float): Y position in millimeters
        z (float): Z position in millimeters
    '''
    __slots__ = ()


class Pose:
    '''Represents the current pose (position and orientation) of vector'''

    __slots__ = ('_position', '_rotation', '_origin_id')

    def __init__(self, x, y, z, q0, q1, q2, q3,
                 origin_id=-1):
        self._position = Position(x, y, z)
        self._rotation = Quaternion(q0, q1, q2, q3)
        self._origin_id = origin_id

    @property
    def position(self):
        return self._position

    @property
    def rotation(self):
        return self._rotation
    
    @property
    def origin_id(self):
        return self._origin_id
    
    def __repr__(self):
        return (f"<{self.__class__.__name__}: {self._position} {self._rotation} <Origin Id: {self._origin_id}>>")



