# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''
The main robot class for managing the Vector SDK
'''

__all__ = ['Robot', 'AsyncRobot', 'MIN_HEAD_ANGLE', 'MAX_HEAD_ANGLE']

import asyncio
import configparser
import functools
import logging
from pathlib import Path

from . import (animation, backpack, behavior, camera, connection,
               events, exceptions, faces, motors,
               oled_face, photos, proximity, sync, util,
               viewer, world)
from .messaging import protocol

MODULE_LOGGER = logging.getLogger(__name__)

# Constants

#: The minimum angle the robot's head can be set to
MIN_HEAD_ANGLE = util.degrees(-22)

#: The maximum angle the robot's head can be set to
MAX_HEAD_ANGLE = util.degrees(45)


class Robot:
    """The Robot object is responsible for managing the state and connections
    to a Vector, and is typically the entry-point to running the sdk.

    The majority of the robot will not work until it is properly connected
    to Vector. There are two ways to get connected:

    1. Using :code:`with`: it works just like opening a file, and will close when
    the :code:`with` block's indentation ends

    .. code-block:: python

        # Create the robot connection
        with Robot("00e20115") as robot:
            # Run your commands (for example play animation)
            robot.play_animation("anim_blackjack_victorwin_01")

    2. Using :func:`connect` and :func:`disconnect` to explicitly open and close the connection:
    it allows the robot's connection to continue in the context in which it started.

    .. code-block:: python

        # Create a Robot object
        robot = Robot("00e20115")
        # Connect to the Robot
        robot.connect()
        # Run your commands (for example play animation)
        robot.play_animation("anim_blackjack_victorwin_01")
        # Disconnect from the Robot
        robot.disconnect()

    :param serial: Vector's serial number. Used to identify which Vector configuration file to load.
    :param ip: Vector's IP Address. (optional)
    :param port: The port on which Vector is listening. Defaults to :code:`443`
    :param config: A custom :class:`dict` to override values in Vector's configuration. (optional)
                   Example: :code:`{"cert": "/path/to/file.cert", "name": "Vector-A1B2", "guid": "<secret_key>"}`
                   Where :code:`cert` is the certificate to identify Vector, :code:`name` is the name on Vector's face
                   when his backpack is double clicked on the charger, and :code:`guid` is the authorization token
                   that identifies the SDK user. Note: Never share your guid with anyone.
    :param loop: The async loop on which the Vector commands will execute. Defaults to :code:`None`
    :param default_logging: Disable default logging. Defaults to :code:`False`
    :param behavior_timeout: The time to wait for control of the robot before failing. Defaults to :code:`10`
    :param enable_vision_mode: Turn on face detection. Defaults to :code:`False`
    :param enable_camera_feed: Turn camera feed on/off. Defaults to :code:`True`
    :param show_viewer: Render camera feed on/off. Defaults to :code:`False`"""

    def __init__(self,
                 serial: str = None,
                 ip: str = None,
                 port: str = "443",
                 config: dict = None,
                 loop: asyncio.BaseEventLoop = None,
                 default_logging: bool = True,
                 behavior_timeout: int = 10,
                 cache_animation_list: bool = True,
                 enable_vision_mode: bool = False,
                 enable_camera_feed: bool = True,
                 show_viewer: bool = False):

        if default_logging:
            util.setup_basic_logging()
        self.logger = util.get_class_logger(__name__, self)
        self.is_async = False
        self.is_loop_owner = False
        self._original_loop = None
        self.loop = loop
        config = config if config is not None else {}

        if serial is not None:
            config = {**self._read_configuration(serial), **config}

        self._name = config["name"]
        self._ip = ip if ip is not None else config["ip"]
        self._cert_file = config["cert"]
        self._guid = config["guid"]

        if self._name is None or self._ip is None or self._cert_file is None or self._guid is None:
            raise ValueError("Robot requires a serial and for Vector to be logged in (using the app then configure.py).\n"
                             "You may also provide the values necessary for connection through the config parameter. ex: "
                             '{"name":"Vector-A1B2", "ip":"192.168.43.48", "cert":"/path/to/cert_file", "guid":"<secret_key>"}')

        self.conn = connection.Connection(self._name, ':'.join([self._ip, port]), self._cert_file, self._guid)
        self.events = events.EventHandler()
        # placeholders for components before they exist
        self._anim: animation.AnimationComponent = None
        self._backpack: backpack.BackpackComponent = None
        self._behavior: behavior.BehaviorComponent = None
        self._camera: camera.CameraComponent = None
        self._faces: faces.FaceComponent = None
        self._motors: motors.MotorComponent = None
        self._oled: oled_face.OledComponent = None
        self._photos: photos.PhotographComponent = None
        self._proximity: proximity.ProximityComponent = None
        self._viewer: viewer.ViewerComponent = None
        self._world: world.World = None

        self.behavior_timeout = behavior_timeout
        self.enable_vision_mode = enable_vision_mode
        self.cache_animation_list = cache_animation_list
        # Robot state/sensor data
        self._pose: util.Pose = None
        self._pose_angle_rad: float = None
        self._pose_pitch_rad: float = None
        self._left_wheel_speed_mmps: float = None
        self._right_wheel_speed_mmps: float = None
        self._head_angle_rad: float = None
        self._lift_height_mm: float = None
        self._accel: util.Vector3 = None
        self._gyro: util.Vector3 = None
        self._carrying_object_id: float = None
        self._carrying_object_on_top_id: float = None
        self._head_tracking_object_id: float = None
        self._localized_to_object_id: float = None
        self._last_image_time_stamp: float = None
        self._status: float = None
        self.pending = []

        self._enable_camera_feed = enable_camera_feed
        self._show_viewer = show_viewer

    @staticmethod
    def _read_configuration(serial: str) -> dict:
        """Open the default conf file, and read it into a :class:`configparser.ConfigParser`

        :param serial: Vector's serial number
        """
        home = Path.home() / ".anki-vector"
        conf_file = str(home / "sdk_config.ini")
        parser = configparser.ConfigParser()
        parser.read(conf_file)
        return parser[serial]

    @property
    def robot(self) -> 'Robot':
        return self

    @property
    def anim(self) -> animation.AnimationComponent:
        if self._anim is None:
            raise exceptions.VectorNotReadyException("AnimationComponent is not yet initialized")
        return self._anim

    @property
    def backpack(self) -> backpack.BackpackComponent:
        if self._backpack is None:
            raise exceptions.VectorNotReadyException("BackpackComponent is not yet initialized")
        return self._backpack

    @property
    def behavior(self) -> behavior.BehaviorComponent:
        return self._behavior

    @property
    def camera(self) -> camera.CameraComponent:
        """:class:`anki_vector.camera.CameraComponent`: The camera instance used to control
        Vector's camera feed

        .. code-block:: python

            with anki_vector.Robot("00e20115") as robot:
                image = Image.fromarray(robot.camera.latest_image)
                image.show()
        """
        if self._camera is None:
            raise exceptions.VectorNotReadyException("CameraComponent is not yet initialized")
        return self._camera

    @property
    def faces(self) -> faces.FaceComponent:
        if self._faces is None:
            raise exceptions.VectorNotReadyException("FaceComponent is not yet initialized")
        return self._faces

    @property
    def motors(self) -> motors.MotorComponent:
        if self._motors is None:
            raise exceptions.VectorNotReadyException("MotorComponent is not yet initialized")
        return self._motors

    @property
    def oled(self) -> oled_face.OledComponent:
        if self._oled is None:
            raise exceptions.VectorNotReadyException("OledComponent is not yet initialized")
        return self._oled

    @property
    def photos(self) -> photos.PhotographComponent:
        if self._photos is None:
            raise exceptions.VectorNotReadyException("PhotographyComponent is not yet initialized")
        return self._photos

    @property
    def proximity(self) -> proximity.ProximityComponent:
        '''Component containing state related to object proximity detection
        '''
        return self._proximity

    @property
    def viewer(self) -> viewer.ViewerComponent:
        """:class:`anki_vector.viewer.ViewerComponent`: The viewer instance used to render
        Vector's camera feed

        .. code-block:: python

            with anki_vector.Robot("00e20115", show_viewer=True) as robot:
                robot.loop.run_until_complete(utilities.delay_close(5))
                robot.viewer.stop_video()
        """
        if self._viewer is None:
            raise exceptions.VectorNotReadyException("ViewerComponent is not yet initialized")
        return self._viewer

    @property
    def world(self) -> world.World:
        if self._world is None:
            raise exceptions.VectorNotReadyException("WorldComponent is not yet initialized")
        return self._world

    @property
    def pose(self) -> util.Pose:
        ''':class:`anki_vector.util.Pose`: The current pose (position and orientation) of Vector'''
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
    def accel(self):
        ''':class:`anki_vector.util.Vector3`: The current accelerometer reading (x, y, z)'''
        return self._accel

    @property
    def gyro(self):
        ''':class:`anki_vector.util.Vector3`: The current gyroscope reading (x, y, z)'''
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

    # TODO For Cozmo, this was named robot.camera.image_stream_enabled. Rename?
    @property
    def enable_camera_feed(self) -> bool:
        """The camera feed enabled/disabled

        :getter: Returns whether the camera feed is enabled
        :setter: Enable/disable the camera feeed

        .. code-block:: python

            with anki_vector.Robot("00e20115", enable_camera_feed=True) as robot:
                robot.loop.run_until_complete(utilities.delay_close(5))
                robot.enable_camera_feed = False
                robot.loop.run_until_complete(utilities.delay_close(5))
        """
        return self._enable_camera_feed

    @enable_camera_feed.setter
    def enable_camera_feed(self, enable) -> None:
        self._enable_camera_feed = enable
        if self.enable_camera_feed:
            self.camera.init_camera_feed()

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
        self._accel = util.Vector3(msg.accel.x, msg.accel.y, msg.accel.z)
        self._gyro = util.Vector3(msg.gyro.x, msg.gyro.y, msg.gyro.z)
        self._carrying_object_id = msg.carrying_object_id
        self._carrying_object_on_top_id = msg.carrying_object_on_top_id
        self._head_tracking_object_id = msg.head_tracking_object_id
        self._localized_to_object_id = msg.localized_to_object_id
        self._last_image_time_stamp = msg.last_image_time_stamp
        self._status = msg.status
        self._proximity.on_proximity_update(msg.prox_data)

    def connect(self, timeout: int = 10) -> None:
        """Start the connection to Vector

        .. code-block:: python

            robot = Robot("00e20115")
            robot.connect()
            robot.play_animation("anim_blackjack_victorwin_01")
            robot.disconnect()

        :param timeout: The time to allow for a connection before a
            :class:`anki_vector.exceptions.VectorTimeoutException` is raised.
        """
        if self.loop is None:
            self.logger.debug("Creating asyncio loop")
            self.is_loop_owner = True
            self._original_loop = asyncio.get_event_loop()
            self.loop = asyncio.new_event_loop()
            asyncio.set_event_loop(self.loop)

        self.conn.connect(self.loop, timeout=timeout)
        self.events.start(self.conn, self.loop)

        # Initialize components
        self._anim = animation.AnimationComponent(self)
        self._backpack = backpack.BackpackComponent(self)
        self._behavior = behavior.BehaviorComponent(self)
        self._camera = camera.CameraComponent(self)
        self._faces = faces.FaceComponent(self)
        self._motors = motors.MotorComponent(self)
        self._oled = oled_face.OledComponent(self)
        self._photos = photos.PhotographComponent(self)
        self._proximity = proximity.ProximityComponent(self)
        self._viewer = viewer.ViewerComponent(self)
        self._world = world.World(self)

        if self.cache_animation_list:
            # Load animations so they are ready to play when requested
            anim_request = self._anim.load_animation_list()
            if isinstance(anim_request, sync.Synchronizer):
                anim_request.wait_for_completed()

        # Start camera feed
        if self.enable_camera_feed:
            self.camera.init_camera_feed()

        # Start rendering camera feed
        if self._show_viewer:
            self.viewer.show_video()

        # Enable face detection, to allow Vector to add faces to its world view
        self._faces.enable_vision_mode(enable=self.enable_vision_mode)

        # Subscribe to a callback that updates the robot's local properties
        self.events.subscribe("robot_state", self._unpack_robot_state)

    def disconnect(self) -> None:
        """Close the connection with Vector

        .. code-block:: python

            robot = Robot("00e20115")
            robot.connect()
            robot.play_animation("anim_blackjack_victorwin_01")
            robot.disconnect()
        """
        if self.is_async:
            for task in self.pending:
                task.wait_for_completed()

        vision_mode = self._faces.enable_vision_mode(enable=False)
        if isinstance(vision_mode, sync.Synchronizer):
            vision_mode.wait_for_completed()

        # Stop rendering video
        self.viewer.stop_video()
        # Shutdown camera feed
        self.camera.close_camera_feed()

        self.events.close()
        self.conn.close()
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
    async def get_battery_state(self) -> protocol.BatteryStateResponse:
        """Check the current state of the battery.

        .. code-block:: python

            battery_state = robot.get_battery_state()
        """
        get_battery_state_request = protocol.BatteryStateRequest()
        return await self.conn.interface.BatteryState(get_battery_state_request)

    @sync.Synchronizer.wrap
    async def get_version_state(self) -> protocol.VersionStateResponse:
        """Get the versioning information of the Robot

        .. code-block:: python

            version_state = robot.get_version_state()
        """
        get_version_state_request = protocol.VersionStateRequest()
        return await self.conn.interface.VersionState(get_version_state_request)

    @sync.Synchronizer.wrap
    async def get_network_state(self) -> protocol.NetworkStateResponse:
        """Get the network information of the Robot

        .. code-block:: python

            network_state = robot.get_version_state()
        """
        get_network_state_request = protocol.NetworkStateRequest()
        return await self.conn.interface.NetworkState(get_network_state_request)

    @sync.Synchronizer.wrap
    async def say_text(self, text: str, use_vector_voice: bool = True, duration_scalar: float = 1.0) -> protocol.SayTextResponse:
        '''Have Vector say text!

        :param text: The words for Vector to say.
        :param use_vector_voice: Whether to use Vector's robot voice
                (otherwise, he uses a generic human male voice).
        :param duration_scalar: Adjust the relative duration of the
                generated text to speech audio.

        :return: object that provides the status and utterance state
        '''
        say_text_request = protocol.SayTextRequest(text=text,
                                                   use_vector_voice=use_vector_voice,
                                                   duration_scalar=duration_scalar)
        return await self.conn.interface.SayText(say_text_request)


class AsyncRobot(Robot):
    """The AsyncRobot object is just like the Robot object, but allows multiple commands
    to be executed at the same time. To achieve this, all function calls also
    return a :class:`sync.Synchronizer`

    1. Using :code:`with`: it works just like opening a file, and will close when
    the :code:`with` block's indentation ends

    .. code-block:: python

        # Create the robot connection
        with AsyncRobot("00e20115") as robot:
            # Run your commands (for example play animation)
            robot.play_animation("anim_blackjack_victorwin_01").wait_for_completed()

    2. Using :func:`connect` and :func:`disconnect` to explicitly open and close the connection:
    it allows the robot's connection to continue in the context in which it started.

    .. code-block:: python

        # Create a Robot object
        robot = AsyncRobot("00e20115")
        # Connect to the Robot
        robot.connect()
        # Run your commands (for example play animation)
        robot.play_animation("anim_blackjack_victorwin_01").wait_for_completed()
        # Disconnect from the Robot
        robot.disconnect()

    :param serial: Vector's serial number. Used to identify which Vector configuration file to load.
    :param ip: Vector's IP Address. (optional)
    :param port: The port on which Vector is listening. Defaults to :code:`443`
    :param config: A custom :class:`dict` to override values in Vector's configuration. (optional)
                   Example: :code:`{"cert": "/path/to/file.cert", "name": "Vector-A1B2", "guid": "<secret_key>"}`
                   Where :code:`cert` is the certificate to identify Vector, :code:`name` is the name on Vector's face
                   when his backpack is double clicked on the charger, and :code:`guid` is the authorization token
                   that identifies the SDK user. Note: Never share your guid with anyone.
    :param loop: The async loop on which the Vector commands will execute. Defaults to :code:`None`
    :param default_logging: Disable default logging. Defaults to :code:`False`
    :param behavior_timeout: The time to wait for control of the robot before failing. Defaults to :code:`10`
    :param enable_vision_mode: Turn on face detection. Defaults to :code:`False`
    :param enable_camera_feed: Turn camera feed on/off. Defaults to :code:`True`
    :param show_viewer: Render camera feed on/off. Defaults to :code:`False`"""

    @functools.wraps(Robot.__init__)
    def __init__(self, *args, **kwargs):
        super(AsyncRobot, self).__init__(*args, **kwargs)
        self.is_async = True

    def add_pending(self, task):
        self.pending += [task]

    def remove_pending(self, task):
        self.pending = [x for x in self.pending if x is not task]
