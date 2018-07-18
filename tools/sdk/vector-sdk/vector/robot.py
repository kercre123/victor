'''
The main robot class for managing the Vector SDK

Copyright (c) 2018 Anki, Inc.
'''

__all__ = ['MIN_HEAD_ANGLE', 'MAX_HEAD_ANGLE', 'Robot', 'AsyncRobot']

import asyncio
import logging

from . import (animation, backpack, behavior, connection,
               events, exceptions, faces, motors,
               oled_face, photos, sync, util, world)
from .messaging import protocol

MODULE_LOGGER = logging.getLogger(__name__)

# Constants

#: The minimum angle the robot's head can be set to
MIN_HEAD_ANGLE = util.degrees(-22)

#: The maximum angle the robot's head can be set to
MAX_HEAD_ANGLE = util.degrees(45)


class Robot:
    def __init__(self, ip, cert_file, port="443",
                 loop=None, default_logging=True, behavior_timeout=10):
        if default_logging:
            util.setup_basic_logging()
        self.logger = util.get_class_logger(__name__, self)
        self.is_async = False
        self.is_loop_owner = False
        self._original_loop = None
        self.loop = loop
        self.conn = connection.Connection(self, ':'.join([ip, port]), cert_file)
        self.events = events.EventHandler()
        # placeholders for components before they exist
        self._anim = None
        self._backpack = None
        self._behavior = behavior.BehaviorComponent(self)
        self._faces = None
        self._motors = None
        self._oled = None
        self._photos = None
        self._world = None

        self.behavior_timeout = behavior_timeout
        # Robot state/sensor data
        self._pose: util.Pose = None
        self._pose_angle_rad: float = None
        self._pose_pitch_rad: float = None
        self._left_wheel_speed_mmps: float = None
        self._right_wheel_speed_mmps: float = None
        self._head_angle_rad: float = None
        self._lift_height_mm: float = None
        self._battery_voltage: float = None
        self._accel: util.Vector3 = None
        self._gyro: util.Vector3 = None
        self._carrying_object_id: float = None
        self._carrying_object_on_top_id: float = None
        self._head_tracking_object_id: float = None
        self._localized_to_object_id: float = None
        self._last_image_time_stamp: float = None
        self._status: float = None
        self._game_status: float = None
        self.pending = []

    @property
    def robot(self):
        return self

    @property
    def anim(self):
        if self._anim is None:
            raise exceptions.VectorNotReadyException("AnimationComponent is not yet initialized")
        return self._anim

    @property
    def backpack(self):
        if self._backpack is None:
            raise exceptions.VectorNotReadyException("BackpackComponent is not yet initialized")
        return self._backpack

    @property
    def behavior(self):
        return self._behavior

    @property
    def faces(self):
        if self._faces is None:
            raise exceptions.VectorNotReadyException("FaceComponent is not yet initialized")
        return self._faces

    @property
    def motors(self):
        if self._motors is None:
            raise exceptions.VectorNotReadyException("MotorComponent is not yet initialized")
        return self._motors

    @property
    def oled(self):
        if self._oled is None:
            raise exceptions.VectorNotReadyException("OledComponent is not yet initialized")
        return self._oled

    @property
    def photos(self):
        if self._photos is None:
            raise exceptions.VectorNotReadyException("PhotographyComponent is not yet initialized")
        return self._photos

    @property
    def world(self):
        if self._world is None:
            raise exceptions.VectorNotReadyException("WorldComponent is not yet initialized")
        return self._world

    @property
    def pose(self):
        ''':class:`vector.util.Pose`: The current pose (position and orientation) of vector'''
        return self._pose

    @property
    def pose_angle_rad(self):
        '''Vector's pose angle (heading in X-Y plane).'''
        return self._pose_angle_rad

    @property
    def pose_pitch_rad(self):
        '''Vector's pose pitch (angle up/down).'''
        return self._pose_pitch_rad

    @property
    def left_wheel_speed_mmps(self):
        '''Vector's left wheel speed in mm/sec'''
        return self._left_wheel_speed_mmps

    @property
    def right_wheel_speed_mmps(self):
        '''Vector's right wheel speed in mm/sec'''
        return self._right_wheel_speed_mmps

    @property
    def head_angle_rad(self):
        '''Vector's head angle (up/down).'''
        return self._head_angle_rad

    @property
    def lift_height_mm(self):
        '''Height of Vector's lift from the ground.'''
        return self._lift_height_mm

    @property
    def battery_voltage(self):
        '''The current battery voltage'''
        return self._battery_voltage

    @property
    def accel(self):
        ''':class:`vector.util.Vector3`: The current accelerometer reading (x, y, z)'''
        return self._accel

    @property
    def gyro(self):
        ''':class:`vector.util.Vector3`: The current gyroscope reading (x, y, z)'''
        return self._gyro

    @property
    def carrying_object_id(self):
        '''The ID of the object currently being carried (-1 if none)'''
        return self._carrying_object_id

    @property
    def carrying_object_on_top_id(self):
        '''The ID of the object on top of the object currently being carried (-1 if none)'''
        return self._carrying_object_on_top_id

    @property
    def head_tracking_object_id(self):
        '''The ID of the object the head is tracking to (-1 if none)'''
        return self._head_tracking_object_id

    @property
    def localized_to_object_id(self):
        '''The ID of the object that the robot is localized to (-1 if none)'''
        return self._localized_to_object_id

    @property
    def last_image_time_stamp(self):
        '''The robot's timestamp for the last image seen.'''
        return self._last_image_time_stamp

    @property
    def status(self):
        return self._status

    @property
    def game_status(self):
        return self._game_status

    # Unpack streamed data to robot's internal properties
    def _unpack_robot_state(self, _, msg):
        self._pose = util.Pose(x=msg.pose.x, y=msg.pose.y, z=msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3,
                               origin_id=msg.pose.origin_id)
        self._pose_angle_rad = msg.pose_angle_rad
        self._pose_pitch_rad = msg.pose_pitch_rad
        self._left_wheel_speed_mmps = msg.left_wheel_speed_mmps
        self._right_wheel_speed_mmps = msg.right_wheel_speed_mmps
        self._head_angle_rad = msg.head_angle_rad
        self._lift_height_mm = msg.lift_height_mm
        self._battery_voltage = msg.battery_voltage
        self._accel = util.Vector3(msg.accel.x, msg.accel.y, msg.accel.z)
        self._gyro = util.Vector3(msg.gyro.x, msg.gyro.y, msg.gyro.z)
        self._carrying_object_id = msg.carrying_object_id
        self._carrying_object_on_top_id = msg.carrying_object_on_top_id
        self._head_tracking_object_id = msg.head_tracking_object_id
        self._localized_to_object_id = msg.localized_to_object_id
        self._last_image_time_stamp = msg.last_image_time_stamp
        self._status = msg.status
        self._game_status = msg.game_status

    def connect(self, timeout=10):
        if self.loop is None:
            self.logger.debug("Creating asyncio loop")
            self.is_loop_owner = True
            self._original_loop = asyncio.get_event_loop()
            self.loop = asyncio.new_event_loop()
            asyncio.set_event_loop(self.loop)

        self.conn.connect(timeout=timeout)

        self.events.start(self.conn, self.loop)

        # Initialize components
        self._anim = animation.AnimationComponent(self)
        self._backpack = backpack.BackpackComponent(self)
        self._faces = faces.FaceComponent(self)
        self._motors = motors.MotorComponent(self)
        self._oled = oled_face.OledComponent(self)
        self._photos = photos.PhotographComponent(self)
        self._world = world.World(self)

        # Enable face detection, to allow Vector to add faces to its world view
        self._faces.enable_vision_mode(enable=True)

        # Subscribe to a callback that updates the robot's local properties
        self.events.subscribe("robot_state", self._unpack_robot_state)
        # Subscribe to a callback that updates the world view
        self.events.subscribe("robot_observed_face",
                              self.world.add_update_face_to_world_view)
        # Subscribe to a callback that updates a face's id
        self.events.subscribe("robot_changed_observed_face_id",
                              self.world.update_face_id)

        # @TODO: these events subscriptions should be moved to objects.py rather than living on the robot

        # Subscribe to callbacks that is triggered when an object connects or disconnects
        self.events.subscribe("object_connection_state",
                              self.world.object_connection_state)
        # Subscribe to callbacks that is triggered when an object is in motion
        self.events.subscribe("object_moved",
                              self.world.object_moved)
        # Subscribe to callbacks that is triggered when an object stops moving
        self.events.subscribe("object_stopped_moving",
                              self.world.object_stopped_moving)
        # Subscribe to callbacks that is triggered when an object is rotated toward a new up axis
        self.events.subscribe("object_up_axis_changed",
                              self.world.object_up_axis_changed)
        # Subscribe to callbacks that is triggered when an object is tapped
        self.events.subscribe("object_tapped",
                              self.world.object_tapped)
        # Subscribe to callbacks that is triggered when the robot observes an object
        self.events.subscribe("robot_observed_object",
                              self.world.robot_observed_object)

    def disconnect(self, wait_for_tasks=True):
        if self.is_async and wait_for_tasks:
            for task in self.pending:
                task.wait_for_completed()

        self.events.close()
        self.loop.run_until_complete(self.conn.close())
        if self.is_loop_owner:
            try:
                self.loop.close()
            finally:
                self.loop = None
                if self._original_loop is not None:
                    asyncio.set_event_loop(self._original_loop)

    def __enter__(self):
        self.connect(self.behavior_timeout)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    @sync.Synchronizer.wrap
    async def get_battery_state(self):
        get_battery_state_request = protocol.BatteryStateRequest()
        return await self.conn.interface.BatteryState(get_battery_state_request)

    @sync.Synchronizer.wrap
    async def get_version_state(self):
        get_version_state_request = protocol.VersionStateRequest()
        return await self.conn.interface.VersionState(get_version_state_request)

    @sync.Synchronizer.wrap
    async def get_network_state(self):
        get_network_state_request = protocol.NetworkStateRequest()
        return await self.conn.interface.NetworkState(get_network_state_request)


class AsyncRobot(Robot):
    def __init__(self, *args, **kwargs):
        super(AsyncRobot, self).__init__(*args, **kwargs)
        self.is_async = True

    def add_pending(self, task):
        self.pending += [task]

    def remove_pending(self, task):
        self.pending = [x for x in self.pending if x is not task]
