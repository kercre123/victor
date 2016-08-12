__ALL__ = ['Angle', 'degrees', 'radians', 
           'Pose', 'pose_quaternion', 'pose_angle_z', 
           'Position', 'position_x_y_z',
           'Rotation', 'rotation_quaternion', 'rotation_z_angle']

import collections
import math
import re
from ._clad import _clad_to_engine_anki

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

        if degrees is not None:
            radians = degrees * math.pi / 180
        self._radians = radians

    def __repr__(self):
        return "<%s %.2f radians (%.2f degrees)>" % (self.__class__.__name__, self.radians, self.degrees)

    def __add__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported operand for + expected Angle")
        return radians(self.radians + other.radians)

    def __sub__(self, other):
        if not isinstance(other, Angle):
            raise TypeError("Unsupported operand for - expected Angle")
        return radians(self.radians - other.radians)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return radians(self.radians * other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return radians(self.radians / other)

    @property
    def radians(self):
        '''The angle in radians.'''
        return self._radians

    @property
    def degrees(self):
        '''The angle in degrees.'''
        return self._radians / math.pi * 180

def degrees(degrees):
    '''Returns an :class:`cozmo.util.Angle` instance set to the specified number of degrees.'''
    return Angle(degrees=degrees)

def radians(radians):
    '''Returns an :class:`cozmo.util.Angle` instance set to the specified number of radians.'''
    return Angle(radians=radians)


class Pose:
    '''Represents where an object is in the world

    Use the :func:'pose_quaternion' to return pose in the form of
    position and rotation defined by a quaternion

    Use the :func:'pose_angle_z' to return pose in the form of
    position and rotation defined by rotation about the z axis
    '''
        
    __slots__ = ('_position', '_rotation', '_origin_id')

    def __init__(self, x, y, z, q0=None, q1=None, q2=None, q3=None, angle_z=None, origin_id=0):
        self._position = Position(x,y,z)
        self._rotation = Rotation(q0,q1,q2,q3,angle_z)
        self._origin_id = origin_id

    def __repr__(self):
        return "<%s %s %s origin_id=%d>" % (self.__class__.__name__, self.position, self.rotation, self.origin_id)

    def __add__(self, other):
        if not isinstance(other, Pose):
            raise TypeError("Unsupported operand for + expected Pose")
        pos = self.position + other.position
        rot = self.rotation + other.rotation
        return pose_quaternion(pos.x, pos.y, pos.z, rot.q0, rot.q1, rot.q2, rot.q3)

    def __sub__(self, other):
        if not isinstance(other, Pose):
            raise TypeError("Unsupported operand for - expected Pose")
        pos = self.position - other.position
        rot = self.rotation - other.rotation
        return pose_quaternion(pos.x, pos.y, pos.z, rot.q0, rot.q1, rot.q2, rot.q3)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")  
        pos = self.position * other
        rot = self.rotation * other
        return pose_quaternion(pos.x, pos.y, pos.z, rot.q0, rot.q1, rot.q2, rot.q3)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")  
        pos = self.position / other
        rot = self.rotation / other
        return pose_quaternion(pos.x, pos.y, pos.z, rot.q0, rot.q1, rot.q2, rot.q3)

    def define_pose_relative_this(self, new_pose):
        '''Creates a new pose such that new_pose's origin is now at the location of this pose.

        Args:
            new_pose (:class:`cozmo.util.Pose`): The pose which origin is being changed.
        Returns:
            A :class:`cozmo.util.pose` object for which the origin was this pose's origin.
        '''

        if not isinstance(new_pose, Pose):
                raise TypeError("Unsupported type for new_origin, must be of type Pose")
        x,y,z = self.position.x_y_z
        angle_z = self.rotation.angle_z
        new_x,new_y,new_z = new_pose.position.x_y_z
        new_angle_z = new_pose.rotation.angle_z

        res_x = x + math.cos(angle_z.radians)*new_x - math.sin(angle_z.radians)*new_y
        res_y = y + math.sin(angle_z.radians)*new_x + math.cos(angle_z.radians)*new_y
        res_z = z + new_z
        res_angle = angle_z + new_angle_z
        return Pose(res_x, res_y, res_z, angle_z=res_angle)

    def encode_pose(self):
        x, y, z = self.position.x_y_z
        q0, q1, q2, q3 = self.rotation.q0_q1_q2_q3
        return _clad_to_engine_anki.PoseStruct3d(x, y, z, q0, q1, q2, q3, self.origin_id)

    @property
    def position(self):
        '''The :class:`cozmo.util.Position` component of this pose.'''
        return self._position

    @property
    def rotation(self):
        '''The :class:`cozmo.util.Rotation` component of this pose.'''
        return self._rotation

    @property
    def origin_id(self):
        '''An int which represents which coordinate frame this pose is in.'''
        return self._origin_id

    @origin_id.setter
    def origin_id(self, value):
        '''Lets this be changed later in case it was not originally defined.'''
        if not isinstance(value, int):
            raise TypeError("The type of origin_id must be int")
        self._origin_id = value


def pose_quaternion(x, y, z, q0, q1, q2, q3, origin_id=0):
    '''Returns a :class:`cozmo.util.Pose` instance set to the pose given in quaternion format.'''
    return Pose(x, y, z, q0=q0, q1=q1, q2=q2, q3=q3, origin_id=origin_id)

def pose_z_angle(x, y, z, angle_z, origin_id=0):
    '''Returns a :class:`cozmo.util.Pose` instance set to the pose given in z angle format.'''
    return Pose(x, y, z, angle_z=angle_z, origin_id=origin_id)

class Rotation:
    '''Represents the rotation of an object in the world. Can be generated with
    quaternion to define it's rotation in 3d space, or with only a z axis rotation
    to define things limited to the x-y plane like Cozmo.

    Use the :func:'rotation_quaternion' to return rotation defined by a quaternion.

    Use the :func:'rotation_angle_z' to return rotation defined by an angle in the z axis.
    '''

    __slots__ = ('_q0', '_q1', '_q2', '_q3')

    def __init__(self, q0=None, q1=None, q2=None, q3=None, angle_z=None):
        is_quaternion = not (q0 is None) and not (q1 is None) and not (q2 is None) and not (q3 is None)

        if not is_quaternion and angle_z is None:
            raise ValueError("Expected either the q0 q1 q2 and q3 or angle_z keyword arguments")
        if is_quaternion and angle_z:
            raise ValueError("Expected either the q0 q1 q2 and q3 or or angle_z keyword argument, not both")
        if angle_z is not None:
            if not isinstance(angle_z, Angle):
                raise TypeError("Unsupported type for angle_z expected Angle")
            q0,q1,q2,q3 = angle_z_to_quaternion(angle_z)

        self._q0, self._q1, self._q2, self._q3 = q0, q1, q2, q3

    def __repr__(self):
        return ("<%s q0: %.2f q1: %.2f q2: %.2f q3: %.2f (angle_z: %s)>" %
            (self.__class__.__name__, self.q0, self.q1, self.q2, self.q3, self.angle_z))

    #These are only for angle_z because quaternion addition/subtraction is not relevant here
    def __add__(self, other):
        if not isinstance(other, Rotation):
            raise TypeError("Unsupported operand for + expected Rotation")
        return rotation_z_angle(self.angle_z + other.angle_z)

    def __sub__(self, other):
        if not isinstance(other, Rotation):
            raise TypeError("Unsupported operand for - expected Rotation")
        return rotation_z_angle(self.angle_z - other.angle_z)

    def __mul__(self, other):
        if not isinstance(other, (int,float)):
            raise TypeError("Unsupported operand for * expected number")
        return rotation_z_angle(self.angle_z * other)

    def __truediv__(self, other):
        if not isinstance(other, (int,float)):
            raise TypeError("Unsupported operand for / expected number")
        return rotation_z_angle(self.angle_z / other)

    @property
    def q0(self):
        '''The q0 (w) value of the quaterinon defining this rotation.'''
        return self._q0

    @property
    def q1(self):
        '''The q1 (i) value of the quaterinon defining this rotation.'''
        return self._q1

    @property
    def q2(self):
        '''The q2 (j) value of the quaterinon defining this rotation.'''
        return self._q2

    @property
    def q3(self):
        '''The q3 (k) value of the quaterinon defining this rotation.'''
        return self._q3

    @property
    def q0_q1_q2_q3(self):
        '''A tuple containing all elements of the quaternion (q0,q1,q2,q3)'''
        return self._q0,self._q1,self._q2,self._q3

    @property
    def angle_z(self):
        '''The z euler component of the object's rotation. Defined as the rotation in the z axis.'''
        q0,q1,q2,q3 = self.q0_q1_q2_q3
        return Angle(radians=math.atan2(2*(q1*q2+q0*q3), 1-2*(q2**2+q3**2)))

def rotation_quaternion(q0, q1, q2, q3):
    '''Returns a rotation instance set by a quaternion'''
    return Rotation(q0=q0, q1=q1, q2=q2, q3=q3)

def rotation_z_angle(angle_z):
    '''Returns a rotation instance set by an angle in the z axis'''
    return Rotation(angle_z=angle_z)

def angle_z_to_quaternion(angle_z):
    '''This function converts an angle in the z axis (eueler angler z component) to a quaternion.

    Args:
        angle_z (:class:`cozmo.util.Angle`): The z axis angle. 

    Returns:
        q0,q1,q2,q3 (foat, float, float, float): A tuple with all the members of a quaternion defined by angle_z.
    '''

    #Define the quaternion to be converted from a euler angle (x,y,z) of 0,0,angle_z
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
    
class Position:
    '''Represents the rotation of an object in the world. Can be generated with
    quaternion to define it's rotation in 3d space, or with only a z axis rotation
    for simple rotation of things like cozmo.
    '''
    
    __slots__ = ('_x', '_y', '_z')

    def __init__(self, x, y, z):
        self._x = x
        self._y = y
        self._z = z

    @property
    def x(self):
        '''The x value of this position in mm.'''
        return self._x
    
    @property
    def y(self):
        '''The y value of this position in mm.'''
        return self._y
    
    @property
    def z(self):
        '''The z value of this position in mm.'''
        return self._z

    @property
    def x_y_z(self):
        '''A tuple containing all elements of the position (x,y,z)'''
        return self._x,self._y,self._z
    
    def __repr__(self):
        return "<%s x: %.2f y: %.2f z: %.2f>" % (self.__class__.__name__, self.x, self.y, self.z)

    def __add__(self, other):
        if not isinstance(other, Position):
            raise TypeError("Unsupported operand for + expected Position")
        return Position(self.x+other.x, self.y+other.y, self.z+other.z)

    def __sub__(self, other):
        if not isinstance(other, Position):
            raise TypeError("Unsupported operand for - expected Position")
        return Position(self.x-other.x, self.y-other.y, self.z-other.z)

    def __mul__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for * expected number")
        return Position(self.x*other, self.y*other, self.z*other)

    def __truediv__(self, other):
        if not isinstance(other, (int, float)):
            raise TypeError("Unsupported operand for / expected number")
        return Position(self.x/other, self.y/other, self.z/other)
