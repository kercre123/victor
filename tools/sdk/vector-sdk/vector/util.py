# Copyright (c) 2018 Anki, Inc.

'''
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['Angle', 'Pose', 'Position', 'Quaternion', 'Vector3', 'setup_basic_logging', 'get_class_logger']

import logging
import math
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

def angle_z_to_quaternion(angle_z):
    '''This function converts an angle in the z axis (Euler angle z component) to a quaternion.
    
    Args:
        angle_z (:class:`vector.util.Angle`): The z axis angle.
    
    Returns:
        q0,q1,q2,q3 (float, float, float, float): A tuple with all the members
            of a quaternion defined by angle_z.
    '''

    #Define the quaternion to be converted from a Euler angle (x,y,z) of 0,0,angle_z
    #These equations have their original equations above, and simplified implemented
    # q0 = cos(x/2)*cos(y/2)*cos(z/2) + sin(x/2)*sin(y/2)*sin(z/2)
    q0 = math.cos(angle_z.radians/2)
    # q1 = sin(x/2)*cos(y/2)*cos(z/2) - cos(x/2)*sin(y/2)*sin(z/2)
    q1 = 0
    # q2 = cos(x/2)*sin(y/2)*cos(z/2) + sin(x/2)*cos(y/2)*sin(z/2)
    q2 = 0
    # q3 = cos(x/2)*cos(y/2)*sin(z/2) - sin(x/2)*sin(y/2)*cos(z/2)
    q3 = math.sin(angle_z.radians/2)
    return q0,q1,q2,q3


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

    @property
    def x_y_z(self):
        '''tuple (float, float, float): The X, Y, Z elements of the Vector3 (x,y,z)'''
        return self._x, self._y, self._z  

    def __repr__(self):
        return (f"<{self.__class__.__name__} x: {self.x:.2} y: {self.y:.2} z: {self.z:.2}>")


class Angle:
    '''Represents an angle.
    
    Use the :func:`degrees` or :func:`radians` convenience methods to generate
    an Angle instance.
    
    Args:
        radians (float): The number of radians the angle should represent
            (cannot be combined with ``degrees``)
        degrees (float): The number of degress the angle should represent
            (cannot be combined with ``radians``)
    '''

    __slots__ = ('_radians')

    def __init__(self, radians=None, degrees=None):
        if radians is None and degrees is None:
            raise ValueError("Expected either the degrees or radians keyword argument")
        if radians and degrees:
            raise ValueError("Expected either the degrees or radians keyword argument, not both")

        if degrees is not None:
            radians = degrees * math.pi / 180
        self._radians = float(radians)

    @property
    def radians(self):
        '''float: The angle in radians.'''
        return self._radians

    @property
    def degrees(self):
        '''float: The angle in degrees.'''
        return self._radians / math.pi * 180

    def __repr__(self):
        return (f"<{self.__class__.__name__} Radians: {self.radians:.2} Degrees: {self.degrees:.2}>")

    def __add__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported type for + expected Angle")
        return Angle(radians=(self.radians + other.radians))

    def __sub__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported type for - expected Angle")
        return Angle(radians=(self.radians - other.radians))

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported type for * expected number")
        return Angle(radians=(self.radians * other))


class Quaternion:
    '''Represents the rotation of an object in the world.'''

    __slots__ = ('_q0', '_q1', '_q2', '_q3')

    def __init__(self, q0=None, q1=None, q2=None, q3=None, angle_z=None):
        is_quaternion = not (q0 is None) and not (q1 is None) and not (q2 is None) and not (q3 is None)

        if not is_quaternion and angle_z is None:
            raise ValueError("Expected either the q0 q1 q2 and q3 or angle_z keyword arguments")
        if is_quaternion and angle_z:
            raise ValueError("Expected either the q0 q1 q2 and q3 or angle_z keyword argument, not both")
        if angle_z is not None:
            if not isinstance(angle_z, Angle):
                raise TypeError("Unsupported type for angle_z expected Angle")
            q0,q1,q2,q3 = angle_z_to_quaternion(angle_z)

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

    @property
    def angle_z(self):
        '''class:`Angle`: The z Euler component of the object's rotation.
        
        Defined as the rotation in the z axis.
        '''
        q0,q1,q2,q3 = self.q0_q1_q2_q3
        return Angle(radians=math.atan2(2*(q1*q2+q0*q3), 1-2*(q2**2+q3**2)))

    @property
    def q0_q1_q2_q3(self):
        '''tuple of float: Contains all elements of the quaternion (q0,q1,q2,q3)'''
        return self._q0,self._q1,self._q2,self._q3
    
    def __repr__(self):
        return (f"<{self.__class__.__name__} q0: {self.q0:.2} q1: {self.q1:.2} q2: {self.q2:.2} q3: {self.q3:.2} {self.angle_z}>")


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

    def __init__(self, x, y, z, q0=None, q1=None, q2=None, q3=None,
                 angle_z=None, origin_id=-1):
        self._position = Position(x, y, z)
        self._rotation = Quaternion(q0, q1, q2, q3, angle_z)
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

    def define_pose_relative_this(self, new_pose):
        '''Creates a new pose such that new_pose's origin is now at the location of this pose.
        
        Args:
            new_pose (:class:`vector.util.Pose`): The pose which origin is being changed.
        
        Returns:
            A :class:`vector.util.pose` object for which the origin was this pose's origin.
        '''

        if not isinstance(new_pose, Pose):
            raise TypeError("Unsupported type for new_origin, must be of type Pose")
        x,y,z = self.position.x_y_z
        angle_z = self.rotation.angle_z
        new_x,new_y,new_z = new_pose.position.x_y_z
        new_angle_z = new_pose.rotation.angle_z

        cos_angle = math.cos(angle_z.radians)
        sin_angle = math.sin(angle_z.radians)
        res_x = x + (cos_angle * new_x) - (sin_angle * new_y)
        res_y = y + (sin_angle * new_x) + (cos_angle * new_y)
        res_z = z + new_z
        res_angle = angle_z + new_angle_z
        return Pose(res_x, res_y, res_z, angle_z=res_angle, origin_id=self._origin_id)


class ImageRect:
    '''Image co-ordinates and size'''

    __slots__ = ('_x_top_left', '_y_top_left', '_width', '_height')

    def __init__(self, x_top_left, y_top_left, width, height):
        self._x_top_left = x_top_left
        self._y_top_left = y_top_left
        self._width = width
        self._height = height

    @property
    def x_top_left(self):
        return self._x_top_left

    @property
    def y_top_left(self):
        return self._y_top_left

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height
    
    
    
    
