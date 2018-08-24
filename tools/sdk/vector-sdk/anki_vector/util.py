# Copyright (c) 2018 Anki, Inc.

'''
Utility functions and classes for the Vector SDK
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['Angle',
           'Component',
           'Pose',
           'Position',
           'Quaternion',
           'Vector3',
           'Distance',
           'Speed',
           'degrees',
           'radians',
           'angle_z_to_quaternion',
           'distance_mm',
           'distance_inches',
           'speed_mmps',
           'setup_basic_logging',
           'get_class_logger']

import argparse
import logging
import math
import os
from pathlib import Path

MODULE_LOGGER = logging.getLogger(__name__)


# TODO: Update this using the login credentials when they're available
def parse_test_args(parser: argparse.ArgumentParser = None):
    '''
    Provides the command line interface for all the tests

    :param parser: To add new arguments,
         pass an argparse parser with the new options
         already defined. Leave empty to use the defaults.
    '''
    if parser is None:
        parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--esn", nargs='?', default=os.environ.get('ANKI_ROBOT_SERIAL', None))
    parser.add_argument("-n", "--name", nargs='?', default=os.environ.get('VECTOR_ROBOT_NAME', None))
    parser.add_argument("-i", "--ip", nargs='?', default=os.environ.get('VECTOR_ROBOT_IP', None))
    parser.add_argument("-c", "--cert_file", nargs='?', default=os.environ.get('VECTOR_ROBOT_CERT', None))
    parser.add_argument("--port", nargs='?', default="443")
    args = parser.parse_args()
    if args.port == "8443":
        args.name = os.environ.get('VECTOR_ROBOT_NAME_MAC', args.name)
        args.ip = os.environ.get('VECTOR_ROBOT_IP_MAC', args.ip)
        args.cert_file = os.environ.get('VECTOR_ROBOT_CERT_MAC', args.cert_file)

    if args.esn is None:
        if args.name is None or args.ip is None or args.cert_file is None:
            parser.error('The following arguments are required: name, ip, cert_file '
                        'or they may be set with the environment variables: '
                        'VECTOR_ROBOT_NAME, VECTOR_ROBOT_IP, VECTOR_ROBOT_CERT '
                        'respectively')

        cert = Path(args.cert_file)
        args.cert = cert.resolve()
    return args


def setup_basic_logging(custom_handler: logging.Handler = None,
                        general_log_level: str = None,
                        target: object = None):
    '''Helper to perform basic setup of the Python logging machinery.

    :param custom_handler: provide an external logger for custom logging locations
    :param general_log_level: 'DEBUG', 'INFO', 'WARN', 'ERROR' or an equivalent
            constant from the :mod:`logging` module.  If None then a
            value will be read from the VECTOR_LOG_LEVEL environment variable.
    :param target: The stream to send the log data to; defaults to stderr
    '''
    if general_log_level is None:
        general_log_level = os.environ.get('VICTOR_LOG_LEVEL', logging.DEBUG)

    handler = custom_handler
    if handler is None:
        handler = logging.StreamHandler(stream=target)
        formatter = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
        handler.setFormatter(formatter)

    vector_logger = logging.getLogger('anki_vector')
    if not vector_logger.handlers:
        vector_logger.addHandler(handler)
        vector_logger.setLevel(general_log_level)


def get_class_logger(module: str, obj: object) -> logging.Logger:
    '''Helper to create logger for a given class (and module)

    :param module: The name of the module to which the object belongs.
    :param obj: the object that owns the logger.
    '''
    return logging.getLogger(".".join([module, type(obj).__name__]))


def angle_z_to_quaternion(angle_z):
    '''This function converts an angle in the z axis (Euler angle z component) to a quaternion.

    Args:
        angle_z (:class:`anki_vector.util.Angle`): The z axis angle.

    Returns:
        q0, q1, q2, q3 (float, float, float, float): A tuple with all the members
            of a quaternion defined by angle_z.
    '''

    # Define the quaternion to be converted from a Euler angle (x,y,z) of 0,0,angle_z
    # These equations have their original equations above, and simplified implemented
    # q0 = cos(x/2)*cos(y/2)*cos(z/2) + sin(x/2)*sin(y/2)*sin(z/2)
    q0 = math.cos(angle_z.radians / 2)
    # q1 = sin(x/2)*cos(y/2)*cos(z/2) - cos(x/2)*sin(y/2)*sin(z/2)
    q1 = 0
    # q2 = cos(x/2)*sin(y/2)*cos(z/2) + sin(x/2)*cos(y/2)*sin(z/2)
    q2 = 0
    # q3 = cos(x/2)*cos(y/2)*sin(z/2) - sin(x/2)*sin(y/2)*cos(z/2)
    q3 = math.sin(angle_z.radians / 2)
    return q0, q1, q2, q3


class Vector2:
    '''Represents a 2D Vector (type/units aren't specified)

    Args:
        x (float): X component
        y (float): Y component
    '''

    __slots__ = ('_x', '_y')

    def __init__(self, x, y):
        self._x = x
        self._y = y

    def set_to(self, rhs):
        """Copy the x and y components of the given Vector2 instance.

        Args:
            rhs (:class:`Vector2`): The right-hand-side of this assignment - the
                source Vector2 to copy into this Vector2 instance.
        """
        self._x = rhs.x
        self._y = rhs.y

    @property
    def x(self):
        '''float: The x component.'''
        return self._x

    @property
    def y(self):
        '''float: The y component.'''
        return self._y

    @property
    def x_y(self):
        '''tuple (float, float): The X, Y elements of the Vector2 (x,y)'''
        return self._x, self._y

    def __repr__(self):
        return "<%s x: %.2f y: %.2f>" % (self.__class__.__name__, self.x, self.y)

    def __add__(self, other):
        if not isinstance(other, Vector2):
            raise TypeError("Unsupported operand for + expected Vector2")
        return Vector2(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        if not isinstance(other, Vector2):
            raise TypeError("Unsupported operand for - expected Vector2")
        return Vector2(self.x - other.x, self.y - other.y)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return Vector2(self.x * other, self.y * other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return Vector2(self.x / other, self.y / other)


class Vector3:
    '''Represents a 3D Vector (type/units aren't specified)

    :param x: X component
    :param y: Y component
    :param z: Z component
    '''

    __slots__ = ('_x', '_y', '_z')

    def __init__(self, x: float, y: float, z: float):
        self._x = x
        self._y = y
        self._z = z

    def set_to(self, rhs):
        """Copy the x, y and z components of the given Vector3 instance.

        Args:
            rhs (:class:`Vector3`): The right-hand-side of this assignment - the
                source Vector3 to copy into this Vector3 instance.
        """
        self._x = rhs.x
        self._y = rhs.y
        self._z = rhs.z

    @property
    def x(self):
        '''float: The x component.'''
        return self._x

    @property
    def y(self):
        '''float: The y component.'''
        return self._y

    @property
    def z(self):
        '''float: The z component.'''
        return self._z

    @property
    def magnitude_squared(self):
        '''float: The magnitude of the Vector3 instance'''
        return self._x**2 + self._y**2 + self._z**2

    @property
    def magnitude(self):
        '''float: The magnitude of the Vector3 instance'''
        return math.sqrt(self.magnitude_squared)

    @property
    def normalized(self):
        ''':class:`Vector3`: Returns a Vector3 instance with the same direction and unit magnitude'''
        mag = self.magnitude
        if mag == 0:
            return Vector3(0, 0, 0)
        return Vector3(self._x / mag, self._y / mag, self._z / mag)

    def dot(self, other):
        ''':class:`Vector3`: Returns the dot product of this and another Vector3 instance'''
        if not isinstance(other, Vector3):
            raise TypeError("Unsupported argument for dot product, expected Vector3")
        return self._x * other.x + self._y * other.y + self._z * other.z

    def cross(self, other):
        ''':class:`Vector3`: Returns the cross product of this and another Vector3 instance'''
        if not isinstance(other, Vector3):
            raise TypeError("Unsupported argument for cross product, expected Vector3")

        return Vector3(
            self._y * other.z - self._z * other.y,
            self._z * other.x - self._x * other.z,
            self._x * other.y - self._y * other.x)

    @property
    def x_y_z(self):
        '''tuple (float, float, float): The X, Y, Z elements of the Vector3 (x,y,z)'''
        return self._x, self._y, self._z

    def __repr__(self):
        return f"<{self.__class__.__name__} x: {self.x:.2} y: {self.y:.2} z: {self.z:.2}>"

    def __add__(self, other):
        if not isinstance(other, Vector3):
            raise TypeError("Unsupported operand for +, expected Vector3")
        return Vector3(self.x + other.x, self.y + other.y, self.z + other.z)

    def __sub__(self, other):
        if not isinstance(other, Vector3):
            raise TypeError("Unsupported operand for -, expected Vector3")
        return Vector3(self.x - other.x, self.y - other.y, self.z - other.z)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return Vector3(self.x * other, self.y * other, self.z * other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return Vector3(self.x / other, self.y / other, self.z / other)


class Angle:
    '''Represents an angle.

    Use the :func:`degrees` or :func:`radians` convenience methods to generate
    an Angle instance.

    :param radians: The number of radians the angle should represent
        (cannot be combined with ``degrees``)
    :param degrees: The number of degress the angle should represent
        (cannot be combined with ``radians``)
    '''

    __slots__ = ('_radians')

    def __init__(self, radians: float = None, degrees: float = None):  # pylint: disable=redefined-outer-name
        if radians is None and degrees is None:
            raise ValueError("Expected either the degrees or radians keyword argument")
        if radians and degrees:
            raise ValueError("Expected either the degrees or radians keyword argument, not both")

        if degrees is not None:
            radians = degrees * math.pi / 180
        self._radians = float(radians)

    @property
    def radians(self):  # pylint: disable=redefined-outer-name
        '''float: The angle in radians.'''
        return self._radians

    @property
    def degrees(self):  # pylint: disable=redefined-outer-name
        '''float: The angle in degrees.'''
        return self._radians / math.pi * 180

    def __repr__(self):
        return f"<{self.__class__.__name__} Radians: {self.radians:.2} Degrees: {self.degrees:.2}>"

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


def degrees(degrees: float):  # pylint: disable=redefined-outer-name
    '''Returns an :class:`anki_vector.util.Angle` instance set to the specified number of degrees.'''
    return Angle(degrees=degrees)


def radians(radians: float):  # pylint: disable=redefined-outer-name
    '''Returns an :class:`anki_vector.util.Angle` instance set to the specified number of radians.'''
    return Angle(radians=radians)


class Matrix44:
    """A 4x4 Matrix for representing the rotation and/or position of an object in the world.

    Can be generated from a :class:`Quaternion` for a pure rotation matrix, or
    combined with a position for a full translation matrix, as done by
    :meth:`Pose.to_matrix`.
    """
    __slots__ = ('m00', 'm10', 'm20', 'm30',
                 'm01', 'm11', 'm21', 'm31',
                 'm02', 'm12', 'm22', 'm32',
                 'm03', 'm13', 'm23', 'm33')

    def __init__(self,
                 m00, m10, m20, m30,
                 m01, m11, m21, m31,
                 m02, m12, m22, m32,
                 m03, m13, m23, m33):
        self.m00 = m00
        self.m10 = m10
        self.m20 = m20
        self.m30 = m30

        self.m01 = m01
        self.m11 = m11
        self.m21 = m21
        self.m31 = m31

        self.m02 = m02
        self.m12 = m12
        self.m22 = m22
        self.m32 = m32

        self.m03 = m03
        self.m13 = m13
        self.m23 = m23
        self.m33 = m33

    def __repr__(self):
        return ("<%s: "
                "%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f "
                "%.1f %.1f %.1f %.1f %.1f %.1f %.1f %.1f>" % (
                    self.__class__.__name__, *self.in_row_order))

    @property
    def tabulated_string(self):
        """str: A multi-line string formatted with tabs to show the matrix contents."""
        return ("%.1f\t%.1f\t%.1f\t%.1f\n"
                "%.1f\t%.1f\t%.1f\t%.1f\n"
                "%.1f\t%.1f\t%.1f\t%.1f\n"
                "%.1f\t%.1f\t%.1f\t%.1f" % self.in_row_order)

    @property
    def in_row_order(self):
        """tuple of 16 floats: The contents of the matrix in row order."""
        return self.m00, self.m01, self.m02, self.m03,\
            self.m10, self.m11, self.m12, self.m13,\
            self.m20, self.m21, self.m22, self.m23,\
            self.m30, self.m31, self.m32, self.m33

    @property
    def in_column_order(self):
        """tuple of 16 floats: The contents of the matrix in column order."""
        return self.m00, self.m10, self.m20, self.m30,\
            self.m01, self.m11, self.m21, self.m31,\
            self.m02, self.m12, self.m22, self.m32,\
            self.m03, self.m13, self.m23, self.m33

    @property
    def forward_xyz(self):
        """tuple of 3 floats: The x,y,z components representing the matrix's forward vector."""
        return self.m00, self.m01, self.m02

    @property
    def left_xyz(self):
        """tuple of 3 floats: The x,y,z components representing the matrix's left vector."""
        return self.m10, self.m11, self.m12

    @property
    def up_xyz(self):
        """tuple of 3 floats: The x,y,z components representing the matrix's up vector."""
        return self.m20, self.m21, self.m22

    @property
    def pos_xyz(self):
        """tuple of 3 floats: The x,y,z components representing the matrix's position vector."""
        return self.m30, self.m31, self.m32

    def set_forward(self, x, y, z):
        """Set the x,y,z components representing the matrix's forward vector.

        Args:
            x (float): The X component.
            y (float): The Y component.
            z (float): The Z component.
        """
        self.m00 = x
        self.m01 = y
        self.m02 = z

    def set_left(self, x, y, z):
        """Set the x,y,z components representing the matrix's left vector.

        Args:
            x (float): The X component.
            y (float): The Y component.
            z (float): The Z component.
        """
        self.m10 = x
        self.m11 = y
        self.m12 = z

    def set_up(self, x, y, z):
        """Set the x,y,z components representing the matrix's up vector.

        Args:
            x (float): The X component.
            y (float): The Y component.
            z (float): The Z component.
        """
        self.m20 = x
        self.m21 = y
        self.m22 = z

    def set_pos(self, x, y, z):
        """Set the x,y,z components representing the matrix's position vector.

        Args:
            x (float): The X component.
            y (float): The Y component.
            z (float): The Z component.
        """
        self.m30 = x
        self.m31 = y
        self.m32 = z


class Quaternion:
    '''Represents the rotation of an object in the world.'''

    __slots__ = ('_q0', '_q1', '_q2', '_q3')

    def __init__(self, q0=None, q1=None, q2=None, q3=None, angle_z=None):
        is_quaternion = q0 is not None and q1 is not None and q2 is not None and q3 is not None

        if not is_quaternion and angle_z is None:
            raise ValueError("Expected either the q0 q1 q2 and q3 or angle_z keyword arguments")
        if is_quaternion and angle_z:
            raise ValueError("Expected either the q0 q1 q2 and q3 or angle_z keyword argument,"
                             "not both")
        if angle_z is not None:
            if not isinstance(angle_z, Angle):
                raise TypeError("Unsupported type for angle_z expected Angle")
            q0, q1, q2, q3 = angle_z_to_quaternion(angle_z)

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
        q0, q1, q2, q3 = self.q0_q1_q2_q3
        return Angle(radians=math.atan2(2 * (q1 * q2 + q0 * q3), 1 - 2 * (q2**2 + q3**2)))

    @property
    def q0_q1_q2_q3(self):
        '''tuple of float: Contains all elements of the quaternion (q0,q1,q2,q3)'''
        return self._q0, self._q1, self._q2, self._q3

    def to_matrix(self, pos_x=0.0, pos_y=0.0, pos_z=0.0):
        """Convert the Quaternion to a 4x4 matrix representing this rotation.

        A position can also be provided to generate a full translation matrix.

        Args:
            pos_x (float): The x component for the position.
            pos_y (float): The y component for the position.
            pos_z (float): The z component for the position.

        Returns:
            :class:`cozmo.util.Matrix44`: A matrix representing this Quaternion's
            rotation, with the provided position (which defaults to 0,0,0).
        """
        # See https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
        q0q0 = self.q0 * self.q0
        q1q1 = self.q1 * self.q1
        q2q2 = self.q2 * self.q2
        q3q3 = self.q3 * self.q3

        q0x2 = self.q0 * 2.0  # saves 2 multiplies
        q0q1x2 = q0x2 * self.q1
        q0q2x2 = q0x2 * self.q2
        q0q3x2 = q0x2 * self.q3
        q1x2 = self.q1 * 2.0  # saves 1 multiply
        q1q2x2 = q1x2 * self.q2
        q1q3x2 = q1x2 * self.q3
        q2q3x2 = 2.0 * self.q2 * self.q3

        m00 = (q0q0 + q1q1 - q2q2 - q3q3)
        m01 = (q1q2x2 + q0q3x2)
        m02 = (q1q3x2 - q0q2x2)

        m10 = (q1q2x2 - q0q3x2)
        m11 = (q0q0 - q1q1 + q2q2 - q3q3)
        m12 = (q0q1x2 + q2q3x2)

        m20 = (q0q2x2 + q1q3x2)
        m21 = (q2q3x2 - q0q1x2)
        m22 = (q0q0 - q1q1 - q2q2 + q3q3)

        return Matrix44(m00, m10, m20, pos_x,
                        m01, m11, m21, pos_y,
                        m02, m12, m22, pos_z,
                        0.0, 0.0, 0.0, 1.0)

    def __repr__(self):
        return (f"<{self.__class__.__name__} q0: {self.q0:.2} q1: {self.q1:.2}"
                f" q2: {self.q2:.2} q3: {self.q3:.2} {self.angle_z}>")


class Position(Vector3):
    '''Represents the position of an object in the world.

    A position consists of its x, y and z values in millimeters.

    :param x: X position in millimeters
    :param y: Y position in millimeters
    :param z: Z position in millimeters
    '''
    __slots__ = ()


class Pose:
    '''Represents the current pose (position and orientation) of the robot'''

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
        return (f"<{self.__class__.__name__}: {self._position}"
                f" {self._rotation} <Origin Id: {self._origin_id}>>")

    def define_pose_relative_this(self, new_pose):
        '''Creates a new pose such that new_pose's origin is now at the location of this pose.

        Args:
            new_pose (:class:`anki_vector.util.Pose`): The pose which origin is being changed.

        Returns:
            A :class:`anki_vector.util.pose` object for which the origin was this pose's origin.
        '''

        if not isinstance(new_pose, Pose):
            raise TypeError("Unsupported type for new_origin, must be of type Pose")
        x, y, z = self.position.x_y_z
        angle_z = self.rotation.angle_z
        new_x, new_y, new_z = new_pose.position.x_y_z
        new_angle_z = new_pose.rotation.angle_z

        cos_angle = math.cos(angle_z.radians)
        sin_angle = math.sin(angle_z.radians)
        res_x = x + (cos_angle * new_x) - (sin_angle * new_y)
        res_y = y + (sin_angle * new_x) + (cos_angle * new_y)
        res_z = z + new_z
        res_angle = angle_z + new_angle_z
        return Pose(res_x,
                    res_y,
                    res_z,
                    angle_z=res_angle,
                    origin_id=self._origin_id)

    @property
    def is_valid(self):
        '''bool: Returns True if this is a valid, usable pose.'''
        return self.origin_id >= 0

    def is_comparable(self, other_pose):
        '''Are these two poses comparable.

        Poses are comparable if they're valid and having matching origin IDs.

        Args:
            other_pose (:class:`anki_vector.util.Pose`): The other pose to compare against.
        Returns:
            bool: True if the two poses are comparable, False otherwise.
        '''
        return (self.is_valid and other_pose.is_valid and
                (self.origin_id == other_pose.origin_id))

    def to_matrix(self):
        """Convert the Pose to a Matrix44.

        Returns:
            :class:`anki_vector.util.Matrix44`: A matrix representing this Pose's
            position and rotation.
        """
        return self.rotation.to_matrix(*self.position.x_y_z)


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


class Distance:
    '''Represents a distance.

    The class allows distances to be returned in either millimeters or inches.

    Use the :func:`distance_inches` or :func:`distance_mm` convenience methods to generate
    a Distance instance.

    Args:
        distance_mm (float): The number of millimeters the distance should
            represent (cannot be combined with ``distance_inches``).
        distance_inches (float): The number of inches the distance should
            represent (cannot be combined with ``distance_mm``).
    '''

    __slots__ = ('_distance_mm')

    def __init__(self, distance_mm=None, distance_inches=None):  # pylint: disable=redefined-outer-name
        if distance_mm is None and distance_inches is None:
            raise ValueError("Expected either the distance_mm or distance_inches keyword argument")
        if distance_mm and distance_inches:
            raise ValueError("Expected either the distance_mm or distance_inches keyword argument, not both")

        if distance_inches is not None:
            distance_mm = distance_inches * 25.4
        self._distance_mm = distance_mm

    def __repr__(self):
        return "<%s %.2f mm (%.2f inches)>" % (self.__class__.__name__, self.distance_mm, self.distance_inches)

    def __add__(self, other):
        if not isinstance(other, Distance):
            raise TypeError("Unsupported operand for + expected Distance")
        return distance_mm(self.distance_mm + other.distance_mm)

    def __sub__(self, other):
        if not isinstance(other, Distance):
            raise TypeError("Unsupported operand for - expected Distance")
        return distance_mm(self.distance_mm - other.distance_mm)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return distance_mm(self.distance_mm * other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return distance_mm(self.distance_mm / other)

    @property
    def distance_mm(self):  # pylint: disable=redefined-outer-name
        '''float: The distance in millimeters'''
        return self._distance_mm

    @property
    def distance_inches(self):  # pylint: disable=redefined-outer-name
        '''float: The distance in inches'''
        return self._distance_mm / 25.4


def distance_mm(distance_mm):  # pylint: disable=redefined-outer-name
    '''Returns an :class:`anki_vector.util.Distance` instance set to the specified number of millimeters.'''
    return Distance(distance_mm=distance_mm)


def distance_inches(distance_inches):  # pylint: disable=redefined-outer-name
    '''Returns an :class:`anki_vector.util.Distance` instance set to the specified number of inches.'''
    return Distance(distance_inches=distance_inches)


class Speed:
    '''Represents a speed.

    This class allows speeds to be measured  in millimeters per second.

    Use :func:`speed_mmps` convenience methods to generate
    a Speed instance.

    Args:
        speed_mmps (float): The number of millimeters per second the speed
            should represent.
    '''

    __slots__ = ('_speed_mmps')

    def __init__(self, speed_mmps=None):  # pylint: disable=redefined-outer-name
        if speed_mmps is None:
            raise ValueError("Expected speed_mmps keyword argument")
        self._speed_mmps = speed_mmps

    def __repr__(self):
        return "<%s %.2f mmps>" % (self.__class__.__name__, self.speed_mmps)

    def __add__(self, other):
        if not isinstance(other, Speed):
            raise TypeError("Unsupported operand for + expected Speed")
        return speed_mmps(self.speed_mmps + other.speed_mmps)

    def __sub__(self, other):
        if not isinstance(other, Speed):
            raise TypeError("Unsupported operand for - expected Speed")
        return speed_mmps(self.speed_mmps - other.speed_mmps)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return speed_mmps(self.speed_mmps * other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return speed_mmps(self.speed_mmps / other)

    @property
    def speed_mmps(self):  # pylint: disable=redefined-outer-name
        '''float: The speed in millimeters per second (mmps).'''
        return self._speed_mmps


def speed_mmps(speed_mmps):  # pylint: disable=redefined-outer-name
    '''Returns an :class:`anki_vector.util.Speed` instance set to the specified millimeters per second speed'''
    return Speed(speed_mmps=speed_mmps)


class Component:
    def __init__(self, robot):
        self.logger = get_class_logger(__name__, self)
        self._robot = robot

    @property
    def robot(self):
        return self._robot

    @property
    def interface(self):
        return self._robot.conn.interface
