# Copyright (c) 2018 Anki, Inc.

'''
'''

__all__ = ['MIN_HEAD_ANGLE', 'MAX_HEAD_ANGLE', 'Robot', 'AsyncRobot']

import asyncio
import logging
import os
import sys

import aiogrpc
from . import util
from . import actions
from . import lights
from . import events
from . import world
from .messaging import protocol
from .messaging import client


module_logger = logging.getLogger(__name__)

#### Constants

#: The minimum angle the robot's head can be set to
MIN_HEAD_ANGLE = util.degrees(-22)

#: The maximum angle the robot's head can be set to
MAX_HEAD_ANGLE = util.degrees(45)


# TODO Remove this when the proto file is using an enum instead of a number
class SDK_PRIORITY_LEVEL:
    '''Enum used to specify the priority level that the program requests to run at'''

    # Runs below level "MandatoryPhysicalReactions" and "LowBatteryFindAndGoToHome", and above "TriggerWordDetected".
    SDK_HIGH_PRIORITY  = 0 


class Robot:
    def __init__(self, ip, cert_file, port="443",
                 loop=None, is_async=False, default_logging=True):
        if cert_file is None:
            raise Exception("Must provide a cert file")
        with open(cert_file, 'rb') as cert:
            self.trusted_certs = cert.read()
        self.is_async = is_async
        if default_logging:
            util.setup_basic_logging()
        self.logger = util.get_class_logger(__name__, self)
        self.is_loop_owner = False
        if loop is None:
            self.logger.debug("Creating asyncio loop")
            self.is_loop_owner = True
            loop = asyncio.get_event_loop()
        self.loop = loop
        self.address = ':'.join([ip, port])
        self.channel = None
        self.connection = None
        self.events = events.EventHandler(self.loop)
        self._world = world.World()
        # Robot state/sensor data
        self._pose:util.Pose = None
        self._pose_angle_rad:float = None
        self._pose_pitch_rad:float = None
        self._left_wheel_speed_mmps:float = None
        self._right_wheel_speed_mmps:float = None
        self._head_angle_rad:float = None
        self._lift_height_mm:float = None
        self._battery_voltage:float = None
        self._accel:util.Vector3 = None
        self._gyro:util.Vector3 = None
        self._carrying_object_id:float = None
        self._carrying_object_on_top_id:float = None
        self._head_tracking_object_id:float = None
        self._localized_to_object_id:float = None
        self._last_image_time_stamp:float = None
        self._status:float = None
        self._game_status:float = None

    @property
    def world(self):
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
        ''':class:`cozmo.util.Vector3`: The current accelerometer reading (x, y, z)'''
        return self._accel

    @property
    def gyro(self):
        ''':class:`cozmo.util.Vector3`: The current gyroscope reading (x, y, z)'''
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
    def unpack_robot_state(self, _, msg):
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

    # TODO Refactor connect and disconnect out of robot class and into class like cozmo.conn
    def connect(self):
        credentials = aiogrpc.ssl_channel_credentials(root_certificates=self.trusted_certs)
        self.logger.info("Connecting to {}".format(self.address))
        self.channel = aiogrpc.secure_channel(self.address, credentials)
        self.connection = client.ExternalInterfaceStub(self.channel)

        # TODO Add a default, configurable timeout to request_control
        # so that the Python script (such as a tutorial or user script)
        # doesn't potentially wait forever for the SDK behavior to be
        # activated, just in case. This means that the caller to the method
        # that calls request_control needs to return a value to the user's
        # script indicating that the script should not proceed and will fail
        # to control the robot.
        self.request_control()

        self.events.start(self.connection)
        # Enable face detection, to allow Vector to add faces to its world view
        self.enable_vision_mode(enable=True)
        # Subscribe to a callback that updates the robot's local properties
        self.events.subscribe("robot_state", self.unpack_robot_state)
        # Subscribe to a callback that updates the world view
        self.events.subscribe("robot_observed_face", self.world.add_update_face_to_world_view)
        # Subscribe to a callback that updates a face's id
        self.events.subscribe("robot_changed_observed_face_id", self.world.update_face_id)

    def disconnect(self, wait_for_tasks=True):
        if self.is_async and wait_for_tasks:
            for task in self.pending:
                task.wait_for_completed()

        try:
            self.release_control().wait_for_completed()
        except AttributeError as err:
            pass

        self.events.close()
        if self.channel:
            self.loop.run_until_complete(self.channel.close())
        if self.is_loop_owner:
            self.loop.close()

    @actions._as_actionable
    async def request_control(self, priority_level=SDK_PRIORITY_LEVEL.SDK_HIGH_PRIORITY):
        sdk_activation_request = protocol.SDKActivationRequest(slot=priority_level, enable=True)
        result = await self.connection.SDKBehaviorActivation(sdk_activation_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result 

    @actions._as_actionable
    async def release_control(self, priority_level=SDK_PRIORITY_LEVEL.SDK_HIGH_PRIORITY):
        sdk_activation_request = protocol.SDKActivationRequest(slot=priority_level, enable=False)
        result = await self.connection.SDKBehaviorActivation(sdk_activation_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result 
    
    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    # Animations
    # @TODO: Refactor all animation code out into class like Cozmo SDK's cozmo.anim
    # @TODO: In cozmo we fetched and stored the animation list locally in the robot on startup, we should probably do that here too
    @actions._as_actionable
    async def get_anim_names(self, enable_diagnostics=False):

        req = protocol.ListAnimationsRequest()
        result = await self.connection.ListAnimations(req)

        self.logger.debug('(status: {0}, number_of_animations:{1}'.format(result.status, len(result.animation_names)))

        return [a.name for a in result.animation_names]

    @actions._as_actionable
    async def play_anim(self, animation_name, loop_count=1, ignore_body_track=True, ignore_head_track=True, ignore_lift_track=True):
        '''Starts an animation playing on a robot.

        Warning: Specific animations may be renamed or removed in future updates of the app.
            If you want your program to work more reliably across all versions
            we recommend using :meth:`play_anim_trigger` instead.

        Args:
            animation_name (str): The name of the animation to play.
            loop_count (int): Number of times to play the animation.
        '''
        anim = protocol.Animation(name=animation_name)
        req = protocol.PlayAnimationRequest(animation=anim, loops=loop_count) # TODO: add body tracks
        result = await self.connection.PlayAnimation(req)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    # Low level motor functions
    @actions._as_actionable
    async def set_wheel_motors(self, left_wheel_speed, right_wheel_speed, left_wheel_accel=0.0, right_wheel_accel=0.0):
        motors = protocol.DriveWheelsRequest(left_wheel_mmps=left_wheel_speed,
                                             right_wheel_mmps=right_wheel_speed,
                                             left_wheel_mmps2=left_wheel_accel,
                                             right_wheel_mmps2=right_wheel_accel)
        result = await self.connection.DriveWheels(motors)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_wheel_motors_turn(self, turn_speed, turn_accel=0.0, curvature_radius_mm=0):
        drive_arc_request = protocol.DriveArcRequest(speed=turn_speed, accel=turn_accel, curvature_radius_mm=1)
        result = await self.connection.DriveArc(drive_arc_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_head_motor(self, speed):
        set_head_request = protocol.MoveHeadRequest(speed_rad_per_sec=speed)
        result = await self.connection.MoveHead(set_head_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_lift_motor(self, speed):
        set_lift_request = protocol.MoveLiftRequest(speed_rad_per_sec=speed)
        result = await self.connection.MoveLift(set_lift_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def go_to_pose(self, pose, relative_to_robot=False, motion_prof_map={}):
        '''Tells Vector to drive to the specified pose and orientation.

        If relative_to_robot is set to True, the given pose will assume the
        robot's pose as its origin.

        Since the robot understands position by monitoring its tread movement,
        it does not understand movement in the z axis. This means that the only
        applicable elements of pose in this situation are position.x position.y
        and rotation.angle_z.

        Args:
            pose: (:class:`vector.util.Pose`): The destination pose.
            relative_to_robot (bool): Whether the given pose is relative to
                the robot's pose.
            motion_prof_map (dict): Provide custom speed, acceleration and deceleration 
                values with which the robot goes to the given pose.
                speed_mmps (float)
                accel_mmps2 (float)
                decel_mmps2 (float)
                point_turn_speed_rad_per_sec (float)
                point_turn_accel_rad_per_sec2 (float)
                point_turn_decel_rad_per_sec2 (float)
                dock_speed_mmps (float)
                dock_accel_mmps2 (float)
                dock_decel_mmps2 (float)
                reverse_speed_mmps (float)
                is_custom (bool)
        
        Returns:
            A :class:`vector.messaging.external_interface_pb2.GoToPoseResult` object which 
            provides an action result.
        '''
        default_motion_profile = {
                            "speed_mmps": 100.0,
                            "accel_mmps2": 200.0,
                            "decel_mmps2": 500.0,
                            "point_turn_speed_rad_per_sec": 2.0,
                            "point_turn_accel_rad_per_sec2": 10.0,
                            "point_turn_decel_rad_per_sec2": 10.0,
                            "dock_speed_mmps": 60.0,
                            "dock_accel_mmps2": 200.0,
                            "dock_decel_mmps2": 500.0,
                            "reverse_speed_mmps": 80.0,
                            "is_custom": 0
                        }
        if relative_to_robot and self.pose:
            pose = self.pose.define_pose_relative_this(pose)
        default_motion_profile.update(motion_prof_map)
        motion_prof = protocol.PathMotionProfile(**default_motion_profile)
        go_to_pose_request = protocol.GoToPoseRequest(x_mm=pose.position.x, 
                                                      y_mm=pose.position.y, 
                                                      rad=pose.rotation.angle_z.radians,
                                                      motion_prof=motion_prof)
        result = await self.connection.GoToPose(go_to_pose_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    # Movement actions
    @actions._as_actionable
    async def drive_straight(self, distance, speed, should_play_anim=False):
        '''Tells Vector to drive in a straight line
        
        Vector will drive for the specified distance (forwards or backwards)
        
        Args:
            distance (vector.util.Distance): The distance to drive
                (>0 for forwards, <0 for backwards)
            speed (vector.util.Speed): The speed to drive at
                (should always be >0, the abs(speed) is used internally)
            should_play_anim (bool): Whether to play idle animations
                whilst driving (tilt head, hum, animated eyes, etc.)
        
        Returns:
           A :class:`protocol.DriveStraightResponse` object which provides the result description.
        '''
        drive_straight_request = protocol.DriveStraightRequest(speed_mmps=speed.speed_mmps,
                                                                dist_mm=distance.distance_mm,
                                                                should_play_animation=should_play_anim)
        result = await self.connection.DriveStraight(drive_straight_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def turn_in_place(self, angle, speed=util.Angle(0.0), accel=util.Angle(0.0), angle_tolerance=util.Angle(0.0), is_absolute=0):
        '''Turn the robot around its current position.
        
        Args:
            angle (vector.util.Angle): The angle to turn. Positive
                values turn to the left, negative values to the right.
            speed (vector.util.Angle): Angular turn speed (per second).
            accel (vector.util.Angle): Acceleration of angular turn
                (per second squared).
            angle_tolerance (vector.util.Angle): angular tolerance
                to consider the action complete (this is clamped to a minimum
                of 2 degrees internally).
            is_absolute (bool): True to turn to a specific angle, False to
                turn relative to the current pose.
        
        Returns:
            A :class:`protocol.TurnInPlaceResponse` object which provides the result description.
        '''
        turn_in_place_request = protocol.TurnInPlaceRequest(angle_rad=angle.radians,
                                                            speed_rad_per_sec=speed.radians,
                                                            accel_rad_per_sec2=accel.radians,
                                                            tol_rad=angle_tolerance.radians,
                                                            is_absolute=is_absolute)
        result = await self.connection.TurnInPlace(turn_in_place_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_head_angle(self, angle, accel=10.0, max_speed=10.0, duration=0.0):
        '''Tell Vector's head to turn to a given angle.
        
        Args:
            angle: (:class:`cozmo.util.Angle`): Desired angle for
                Vector's head. (:const:`MIN_HEAD_ANGLE` to
                :const:`MAX_HEAD_ANGLE`).
            max_speed (float): Maximum speed of Vector's head in radians per second.
            accel (float): Acceleration of Vector's head in radians per second squared.
            duration (float): Time for Vector's head to turn in seconds. A value
                of zero will make Vector try to do it as quickly as possible.
        
        Returns:
            A :class:`protocol.SetHeadAngleResponse` object which provides the result description.
        '''
        set_head_angle_request = protocol.SetHeadAngleRequest(angle_rad=angle.radians,
                                                            max_speed_rad_per_sec=max_speed,
                                                            accel_rad_per_sec2=accel,
                                                            duration_sec=duration)
        result = await self.connection.SetHeadAngle(set_head_angle_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_lift_height(self, height, accel=10.0, max_speed=10.0, duration=0.0):
        '''Tell Vector's lift to move to a given height
        
        Args:
            height (float): desired height for Vector's lift 0.0 (bottom) to
                1.0 (top) (we clamp it to this range internally).
            max_speed (float): Maximum speed of Vector's lift in radians per second.
            accel (float): Acceleration of Vector's lift in radians per
                second squared.
            duration (float): Time for Vector's lift to move in seconds. A value
                of zero will make Vector try to do it as quickly as possible.
        
        Returns:
            A :class:`protocol.SetLiftHeightResponse` object which provides the result description.
        '''
        set_lift_height_request = protocol.SetLiftHeightRequest(height_mm=height,
                                                                max_speed_rad_per_sec=max_speed,
                                                                accel_rad_per_sec2=accel,
                                                                duration_sec=duration)
        result = await self.connection.SetLiftHeight(set_lift_height_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    # Meet Victor
    @actions._as_actionable
    async def meet_victor(self, name):
        req = protocol.AppIntentRequest(intent='intent_meet_victor', param=name) # TODO: add body tracks
        result = await self.connection.AppIntent(req)
        self.logger.debug(result)
        return result

    # TODO Refactor all face code out into class like cozmo.faces
    @actions._as_actionable
    async def cancel_face_enrollment(self):
        req = protocol.CancelFaceEnrollmentRequest()
        result = await self.connection.CancelFaceEnrollment(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def request_enrolled_names(self):
        req = protocol.RequestEnrolledNamesRequest()
        result = await self.connection.RequestEnrolledNames(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def update_enrolled_face_by_id(self, face_id, old_name, new_name):
        '''Update the name enrolled for a given face.

        Args:
            face_id (int): The ID of the face to rename.
            old_name (string): The old name of the face (must be correct, otherwise message is ignored).
            new_name (string): The new name for the face.
        '''
        req = protocol.UpdateEnrolledFaceByIDRequest(faceID=face_id, oldName=old_name, newName=new_name)
        result = await self.connection.UpdateEnrolledFaceByID(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def erase_enrolled_face_by_id(self, face_id):
        '''Erase the enrollment (name) record for the face with this ID.

        Args:
            face_id (int): The ID of the face to erase.
        '''
        req = protocol.EraseEnrolledFaceByIDRequest(faceID = face_id)
        result = await self.connection.EraseEnrolledFaceByID(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def erase_all_enrolled_faces(self):
        '''Erase the enrollment (name) records for all faces.
        '''
        req = protocol.EraseAllEnrolledFacesRequest()
        result = await self.connection.EraseAllEnrolledFaces(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def set_face_to_enroll(self, name, observedID=0, saveID=0, saveToRobot=True, sayName=False, useMusic=False):
        req = protocol.SetFaceToEnrollRequest(name=name, observedID=observedID, saveID=saveID, saveToRobot=saveToRobot, sayName=sayName, useMusic=useMusic)
        result = await self.connection.SetFaceToEnroll(req)
        self.logger.debug(result)
        return result

    @actions._as_actionable
    async def enable_vision_mode(self, enable, mode=protocol.VisionMode.Value("VISION_MODE_DETECTING_FACES")):
        '''Edit the vision mode
        
        Args:
            enable (bool): Enable/Disable the mode specified
            mode (messaging.protocol.VisionMode): Specifies the vision mode to edit
        '''
        enable_vision_mode_request = protocol.EnableVisionModeRequest(mode=mode, enable=enable)
        result = await self.connection.EnableVisionMode(enable_vision_mode_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    # Vector Display
    @actions._as_actionable
    async def set_backpack_lights(self, light1, light2, light3, backpack_color_profile=lights.white_balanced_backpack_profile):
        '''Set the lights on Vector's backpack.

        The light descriptions below are all from Vector's perspective.

        Args:
            light1 (:class:`vector.lights.Light`): The front backpack light
            light2 (:class:`vector.lights.Light`): The center backpack light
            light3 (:class:`vector.lights.Light`): The rear backpack light
        '''
        params = lights.package_request_params((light1, light2, light3), backpack_color_profile)
        set_backpack_lights_request = protocol.SetBackpackLEDsRequest(**params)

        result = await self.connection.SetBackpackLEDs(set_backpack_lights_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    def set_all_backpack_lights(self, light, color_profile=lights.white_balanced_backpack_profile):
        '''Set the lights on Vector's backpack to the same color.

        Args:
            light (:class:`vector.lights.Light`): The lights for Vector's backpack.
        '''
        light_arr = [ light ] * 3
        return self.set_backpack_lights(*light_arr, color_profile)

    # TODO Refactor all cube code into class like cozmo.objects
    async def set_cube_light_corners( self, cube_id, light1, light2, light3, light4, color_profile=lights.white_balanced_cube_profile ):
        message = _clad_message.SetAllActiveObjectLEDs(
            objectID=cube_id,
            offset=[0,0,0,0])
        for i, light in enumerate( (light1, light2, light3, light4) ):
            if light is not None:
                lights._set_light(message, i, light, color_profile)

        innerWrappedMessage = _clad_message.Cubes(SetAllActiveObjectLEDs=message)
        outerWrappedMessage = _clad_message.ExternalComms(Cubes=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_cube_lights( self, cube_id, light, color_profile=lights.white_balanced_cube_profile ):
        await self.set_cube_light_corners( cube_id, *[light, light, light, light], color_profile)

    # TODO Factor all OLED code out into class like cozmo.oled_face
    @actions._as_actionable
    async def set_oled_with_screen_data(self, image_data, duration_sec, interrupt_running=True):
        if not isinstance(image_data, bytes):
            raise ValueError("set_oled_with_screen_data expected bytes")
        if len(image_data) != 35328:
            raise ValueError("set_oled_with_screen_data expected 35328 bytes - (2 bytes each for 17664 pixels)")

        # generate the message
        message = protocol.DisplayFaceImageRGBRequest()
        # create byte array at the oled resolution
        message.face_data = image_data
        message.duration_ms = int(1000 * duration_sec)
        message.interrupt_running = interrupt_running

        result = await self.connection.DisplayFaceImageRGB(message)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    def set_oled_to_color(self, color, duration_sec, interrupt_running=True):

        image_data = bytes(color.rgb565_bytepair * 17664)
        return self.set_oled_with_screen_data(image_data, duration_sec, interrupt_running)

    # TODO Refactor into Behavior class like Cozmo
    @actions._as_actionable
    async def drive_off_charger(self):
        drive_off_charger_request = protocol.DriveOffChargerRequest()
        result = await self.connection.DriveOffCharger(drive_off_charger_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result 


    # TODO Refactor into Behavior class like Cozmo
    @actions._as_actionable
    async def drive_on_charger(self):
        drive_on_charger_request = protocol.DriveOnChargerRequest()
        result = await self.connection.DriveOnCharger(drive_on_charger_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result 

    @actions._as_actionable
    async def get_robot_stats(self):
        get_robot_stats_request = protocol.RobotStatsRequest()
        result = await self.connection.RobotStats(get_robot_stats_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

class AsyncRobot(Robot):
    def __init__(self, *args, **kwargs):
        super(AsyncRobot, self).__init__(*args, **kwargs, is_async=True)
        self.pending = []

    def add_pending(self, task):
        self.pending += [task]

    def remove_pending(self, task):
        self.pending = [x for x in self.pending if x is not task]
