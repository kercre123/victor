# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['EvtRobotReady',
           'GoToPose', 'DriveOffChargerContacts', 'PickupObject',
           'PlaceOnObject', 'PlaceObjectOnGroundHere', 'SayText', 'SetHeadAngle',
           'SetLiftHeight', 'TurnInPlace', 'TurnTowardsFace',
           'MIN_HEAD_ANGLE', 'MAX_HEAD_ANGLE',
           'Cozmo']


import asyncio

from . import logger, logger_protocol
from . import action
from . import anim
from . import behavior
from . import camera
from . import conn
from . import event
from . import lights
from . import objects
from . import util
from . import world

from ._clad import _clad_to_engine_iface, _clad_to_engine_cozmo, _clad_to_game_cozmo


#### Events

class EvtRobotReady(event.Event):
    '''Generated when the robot has been initialized and is ready for commands'''
    robot = "Cozmo object representing the robot to command"


#### Constants


MIN_HEAD_ANGLE = util.degrees(-25)
MAX_HEAD_ANGLE = util.degrees(44.5)


#### Actions

class GoToPose(action.Action):
    '''Represents the go to pose action in progress.

    Returned by :meth:`~cozmo.robot.Cozmo.go_to_pose`
    '''
    def __init__(self, pose, **kw):
        super().__init__(**kw)
        self.pose = pose

    def _repr_values(self):
        return "pose=%s" % (self.pose)

    def _encode(self):
        return _clad_to_engine_iface.GotoPose(x_mm=self.pose.position.x, y_mm=self.pose.position.y,
                                              rad=self.pose.rotation.angle_z.radians)

class DriveOffChargerContacts(action.Action):
    '''Represents the drive off charger contacts action in progress.

    Returned by :meth:`~cozmo.robot.Cozmo.drive_off_charger_contacts`
    '''
    def __init__(self, **kw):
        super().__init__(**kw)

    def _repr_values(self):
        return ""

    def _encode(self):
        return _clad_to_engine_iface.DriveOffChargerContacts()

class PickupObject(action.Action):
    '''Represents the pickup object action in progress.

    Returned by :meth:`~cozmo.robot.Cozmo.pickup_object`
    '''

    def __init__(self, obj, use_pre_dock_pose=True, **kw):
        super().__init__(**kw)
        #: The object (e.g. an instance of :class:`cozmo.objects.LightCube`) that was picked up
        self.obj = obj
        #: A bool that is true when Cozmo needs to go to a pose before attempting to navigate to the object
        self.use_pre_dock_pose = use_pre_dock_pose

    def _repr_values(self):
        return "object=%s" % (self.obj,)

    def _encode(self):
        return _clad_to_engine_iface.PickupObject(objectID=self.obj.object_id, usePreDockPose=self.use_pre_dock_pose)


class PlaceOnObject(action.Action):
    '''Tracks the state of the "place on object" action.

    return by :meth:`~cozmo.robot.Cozmo.place_on_object`
    '''

    def __init__(self, obj, use_pre_dock_pose=True, **kw):
        super().__init__(**kw)
        #: The object (e.g. an instance of :class:`cozmo.objects.LightCube`) that the held object will be placed on
        self.obj = obj
        #: A bool that is true when Cozmo needs to go to a pose before attempting to navigate to the object
        self.use_pre_dock_pose = use_pre_dock_pose

    def _repr_values(self):
        return "object=%s use_pre_dock_pose=%s" % (self.obj, self.use_pre_dock_pose)

    def _encode(self):
        return _clad_to_engine_iface.PlaceOnObject(objectID=self.obj.object_id, usePreDockPose=self.use_pre_dock_pose)


class PlaceObjectOnGroundHere(action.Action):
    '''Tracks the state of the "place object on ground here" action.

    Returned by :meth:`~cozmo.robot.Cozmo.place_object_on_ground_here`
    '''

    def __init__(self, obj, **kw):
        super().__init__(**kw)
        #: The object (eg. an instance of :class:`cozmo.objects.LightCube`) that is being put down
        self.obj = obj

    def _repr_values(self):
        return "object=%s" % (self.obj,)

    def _encode(self):
        return _clad_to_engine_iface.PlaceObjectOnGroundHere()


class SayText(action.Action):
    '''Tracks the progress of a say text robot action.

    Returned by :meth:`~cozmo.robot.Cozmo.say_text`
    '''

    def __init__(self, text, play_excited_animation, use_cozmo_voice, duration_scalar, voice_pitch, **kw):
        super().__init__(**kw)
        self.text = text

        # Note: play_event must be an AnimationTrigger that supports text-to-speech being generated

        if play_excited_animation:
            self.play_event = _clad_to_engine_cozmo.AnimationTrigger.OnSawNewNamedFace
        else:
            # Alternatively using AnimationTrigger.Count will force no animation at all
            self.play_event = _clad_to_engine_cozmo.AnimationTrigger.SdkTextToSpeech

        if use_cozmo_voice:
            self.say_style = _clad_to_engine_cozmo.SayTextVoiceStyle.CozmoProcessing
        else:
            # default male human voice
            self.say_style = _clad_to_engine_cozmo.SayTextVoiceStyle.UnProcessed

        self.duration_scalar = duration_scalar
        self.voice_pitch = voice_pitch

    def _repr_values(self):
        return "text=%s style=%s event=%s duration=%s pitch=%s" % (self.text, self.say_style, self.play_event, self.duration_scalar, self.voice_pitch)

    def _encode(self):
        return _clad_to_engine_iface.SayText(text=self.text, playEvent=self.play_event, voiceStyle=self.say_style, durationScalar=self.duration_scalar, voicePitch=self.voice_pitch)

class SetHeadAngle(action.Action):
    '''Represents the Set Head Angle action in progress.
       Returned by :meth:`~cozmo.robot.Cozmo.set_head_angle`
    '''

    def __init__(self, angle, max_speed, accel, duration, **kw):
        super().__init__(**kw)

        if angle < MIN_HEAD_ANGLE:
            logger.info("Clamping head angle from %s to min %s" % (angle, MIN_HEAD_ANGLE))
            self.angle = MIN_HEAD_ANGLE
        elif angle > MAX_HEAD_ANGLE:
            logger.info("Clamping head angle from %s to max %s" % (angle, MAX_HEAD_ANGLE))
            self.angle = MAX_HEAD_ANGLE
        else:
            self.angle = angle

        self.max_speed = max_speed
        self.accel = accel
        self.duration = duration

    def _repr_values(self):
        return "angle=%s max_speed=%s accel=%s duration=%s" % (self.angle, self.max_speed, self.accel, self.duration)

    def _encode(self):
        return _clad_to_engine_iface.SetHeadAngle(angle_rad=self.angle.radians, max_speed_rad_per_sec=self.max_speed, accel_rad_per_sec2=self.accel, duration_sec=self.duration)

class SetLiftHeight(action.Action):
    '''Represents the Set Lift Height action in progress.
       Returned by :meth:`~cozmo.robot.Cozmo.set_lift_height`
    '''

    def __init__(self, height, max_speed, accel, duration, **kw):
        super().__init__(**kw)

        min_lift_height_mm = 32.0 # TODO: fix engine to expose these directly
        max_lift_height_mm = 92.0
        if (height < 0.0):
            self.lift_height_mm = min_lift_height_mm
        elif (height > 1.0):
            self.lift_height_mm = max_lift_height_mm
        else:
            self.lift_height_mm = min_lift_height_mm + (height * (max_lift_height_mm - min_lift_height_mm))

        self.max_speed = max_speed
        self.accel = accel
        self.duration = duration

    def _repr_values(self):
        return "height=%s max_speed=%s accel=%s duration=%s" % (self.lift_height_mm, self.max_speed, self.accel, self.duration)

    def _encode(self):
        return _clad_to_engine_iface.SetLiftHeight(height_mm=self.lift_height_mm, max_speed_rad_per_sec=self.max_speed, accel_rad_per_sec2=self.accel, duration_sec=self.duration)

class TurnInPlace(action.Action):
    '''Tracks the progress of a turn in place robot action.

    Returned by :meth:`~cozmo.robot.Cozmo.turn_in_place`
    '''

    def __init__(self, angle, **kw):
        super().__init__(**kw)
        #: The angle to turn
        self.angle = angle

    def _repr_values(self):
        return "angle=%s" % (self.angle,)

    def _encode(self):
        return _clad_to_engine_iface.TurnInPlace(
            angle_rad = self.angle.radians,
            isAbsolute = 0)


class TurnTowardsFace(action.Action):
    '''Tracks the progress of a turn towards face robot action.

    Returned by :meth:`~cozmo.robot.Cozmo.turn_towards_face`
    '''

    def __init__(self, face, **kw):
        super().__init__(**kw)
        #: The face to turn towards
        self.face = face

    def _repr_values(self):
        return "face=%s" % (self.face)

    def _encode(self):
        return _clad_to_engine_iface.TurnTowardsFace(
            faceID=self.face.face_id)


class Cozmo(event.Dispatcher):
    """The interface to a Cozmo robot.

    A robot has access to:

    * A :class:`~cozmo.world.World` object (:attr:`cozmo.robot.Cozmo.world`), 
        which tracks the state of the world the robot knows about

    * A :class:`~cozmo.camera.Camera` object (:attr:`cozmo.robot.Cozmo.camera`), 
        which provides access to Cozmo's camera

    * An Animations object, controlling the playing of animations on the robot

    * A Behaviors object, starting and ending robot behaviors such as looking around

    Robots are instantiated by the :class:`~cozmo.conn.CozmoConnection` object
    and emit a :class:`EvtRobotReady` when it has been configured and is
    ready to be commanded.
    """

    # action factories
    _action_dispatcher_factory = action._ActionDispatcher
    turn_in_place_factory = TurnInPlace
    turn_towards_face_factory = TurnTowardsFace
    pickup_object_factory = PickupObject
    place_on_object_factory = PlaceOnObject
    go_to_pose_factory = GoToPose
    place_object_on_ground_here_factory = PlaceObjectOnGroundHere
    say_text_factory = SayText
    set_head_angle_factory = SetHeadAngle
    set_lift_height_factory = SetLiftHeight
    drive_off_charger_contacts_factory = DriveOffChargerContacts

    # other factories
    animation_factory = anim.Animation
    animation_trigger_factory = anim.AnimationTrigger
    behavior_factory = behavior.Behavior
    camera_factory = camera.Camera
    world_factory = world.World

    # other attributes
    drive_off_charger_on_connect = True  # Required for most movement actions


    def __init__(self, conn, robot_id, is_primary, **kw):
        super().__init__(**kw)
        self.conn = conn
        self._is_ready = False
        self.robot_id = robot_id
        self._pose = None
        self.is_primary = is_primary

        #: :class:`cozmo.camera.Camera` Provides access to the robot's camera
        self.camera = self.camera_factory(self, dispatch_parent=self)

        #: :class:`cozmo.world.World` Tracks state information about Cozmo's world.
        self.world = self.world_factory(self.conn, self, dispatch_parent=self)

        self._action_dispatcher = self._action_dispatcher_factory(self)

        # send all received events to the world and action dispatcher
        self._add_child_dispatcher(self._action_dispatcher)
        self._add_child_dispatcher(self.world)


    #### Private Methods ####

    def _initialize(self):
        # Perform all steps necessary to initialize the robot and trigger
        # an EvtRobotReady event when complete.
        async def _init():
            # TODO: reset the robot state
            self.stop_all_motors()
            self.enable_reactionary_behaviors(False)

            # Ensure the SDK has full control of cube lights
            self._set_cube_light_state(False)

            await self.world.delete_all_custom_objects()
            self._reset_behavior_state()

            # wait for animations to load
            await self.conn.anim_names.wait_for_loaded()

            self._is_ready = True
            logger.info("Robot initialized OK")
            self.dispatch_event(EvtRobotReady, robot=self)
        asyncio.ensure_future(_init(), loop=self._loop)

    def _reset_behavior_state(self):
        msg = _clad_to_engine_iface.ExecuteBehavior(
                behaviorType=_clad_to_engine_cozmo.BehaviorType.NoneBehavior)
        self.conn.send_msg(msg)

    def _set_cube_light_state(self, enable):
        msg = _clad_to_engine_iface.EnableLightStates(enable=enable, objectID=-1)
        self.conn.send_msg(msg)

    #### Properties ####

    @property
    def is_ready(self):
        """is_ready is True if the robot has been initialized and is ready to accept commands."""
        return self._is_ready

    @property
    def anim_names(self):
        '''Returns a set of all the available animation names (an alias of cozmo.conn.anim_names)

        Generally animation triggers are preferred over explict animation names:
        See :class:`cozmo.anim.Triggers` for available animation triggers.
        '''
        return self.conn.anim_names

    @property
    def pose(self):
        """Pose is the current pose of cozmo relative to where he started when the engine was initialized.

        Returns:
            A :class:`cozmo.util.Pose` object.
        """
        return self._pose

    @property
    def is_moving(self):
        ''':bool: Is Cozmo currently moving anything (head, lift or wheels/treads)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_MOVING) != 0

    @property
    def is_carrying_block(self):
        ''':bool: Is Cozmo currently carrying a block'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_CARRYING_BLOCK) != 0

    @property
    def is_picking_or_placing(self):
        ''':bool: Is Cozmo picking or placing something'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_PICKING_OR_PLACING) != 0

    @property
    def is_picked_up(self):
        ''':bool: Is Cozmo currently picked up (in the air)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_PICKED_UP) != 0

    @property
    def is_falling(self):
        ''':bool: Is Cozmo currently falling'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_FALLING) != 0

    @property
    def is_animating(self):
        ''':bool: Is Cozmo currently playing an animation'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_ANIMATING) != 0

    @property
    def is_animating_idle(self):
        ''':bool: Is Cozmo currently playing an idle animation'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_ANIMATING_IDLE) != 0

    @property
    def is_pathing(self):
        ''':bool: Is Cozmo currently traversing a path'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_PATHING) != 0

    @property
    def is_lift_in_pos(self):
        ''':bool: Is Cozmo's lift in the desired position (False if still trying to move there)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.LIFT_IN_POS) != 0

    @property
    def is_head_in_pos(self):
        ''':bool: Is Cozmo's head in the desired position (False if still trying to move there)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.HEAD_IN_POS) != 0

    @property
    def is_anim_buffer_full(self):
        ''':bool: Is Cozmo's animation buffer full (on robot)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_ANIM_BUFFER_FULL) != 0

    @property
    def is_on_charger(self):
        ''':bool: Is Cozmo currently on the charger'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_ON_CHARGER) != 0

    @property
    def is_charging(self):
        ''':bool: Is Cozmo currently charging'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.IS_CHARGING) != 0

    @property
    def is_cliff_detected(self):
        ''':bool: Has Cozmo detected a cliff (in front)'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.CLIFF_DETECTED) != 0

    @property
    def are_wheels_moving(self):
        ''':bool: Are Cozmo's wheels/treads currently moving'''
        return (self._robot_status_flags & _clad_to_game_cozmo.RobotStatusFlag.ARE_WHEELS_MOVING) != 0

    @property
    def is_localized(self):
        ''':bool: Is Cozmo Localized (i.e. knows where he is, and has both treads on the ground)'''
        return (self._game_status_flags & _clad_to_game_cozmo.GameStatusFlag.IsLocalized) != 0

    @property
    def pose_angle(self):
        ''':class:`cozmo.util.Angle` Cozmo's pose angle (heading in X-Y plane)'''
        return self._pose_angle

    @property
    def pose_pitch(self):
        ''':class:`cozmo.util.Angle` Cozmo's pose pitch (angle up/down)'''
        return self._pose_pitch

    @property
    def head_angle(self):
        ''':class:`cozmo.util.Angle` Cozmo's head angle (up/down)'''
        return self._head_angle

    #### Private Event Handlers ####

    #def _recv_default_handler(self, event, **kw):
    #    msg = kw.get('msg')
    #    logger_protocol.debug("Robot received unhandled internal event_name=%s  kw=%s", event.event_name, kw)

    def recv_default_handler(self, event, **kw):
        logger.debug("Robot received unhandled public event=%s", event)

    def _recv_msg_processed_image(self, _, *, msg):
        pass

    def _recv_msg_image_chunk(self, evt, *, msg):
        self.camera.dispatch_event(evt)

    def _recv_msg_robot_state(self, evt, *, msg):
        self._pose = util.Pose(x=msg.pose.x, y=msg.pose.y, z=msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3)
        self._pose_angle = util.radians(msg.poseAngle_rad) # heading in X-Y plane
        self._pose_pitch = util.radians(msg.posePitch_rad)
        self._head_angle = util.radians(msg.headAngle_rad)
        self.left_wheel_speed  = util.speed_mmps(msg.leftWheelSpeed_mmps)
        self.right_wheel_speed = util.speed_mmps(msg.rightWheelSpeed_mmps)
        self.lift_height       = util.distance_mm(msg.liftHeight_mm)
        self.battery_voltage           = msg.batteryVoltage
        self.carrying_object_id        = msg.carryingObjectID      # int_32 will be -1 if not carrying object
        self.carrying_object_on_top_id = msg.carryingObjectOnTopID # int_32 will be -1 if no object on top of object being carried
        self.head_tracking_object_id   = msg.headTrackingObjectID  # int_32 will be -1 if head is not tracking to any object
        self.localized_to_object_id    = msg.localizedToObjectID   # int_32 Will be -1 if not localized to any object
        self.last_image_time           = msg.lastImageTimeStamp
        self._robot_status_flags       = msg.status     # uint_16 as bitflags - See _clad_to_game_cozmo.RobotStatusFlag
        self._game_status_flags        = msg.gameStatus # uint_8  as bitflags - See _clad_to_game_cozmo.GameStatusFlag

        if msg.robotID != self.robot_id:
            logger.error("robot id changed mismatch (msg=%s, self=%s)", msg.robotID, self.robot_id )

    #### Public Event Handlers ####


    #### Commands ####

    def enable_reactionary_behaviors(self, should_enable):
        '''Enable or disable Cozmo's responses to being handled or observing the world.

        Args:
            should_enable (bool): True if the robot should react to its environment.
        '''
        msg = _clad_to_engine_iface.EnableReactionaryBehaviors(enabled=should_enable)
        self.conn.send_msg(msg)


    ### Low-Level Commands ###

    async def drive_wheels(self, l_wheel_velocity, r_wheel_velocity,
                                 l_wheel_acc=None, r_wheel_acc=None, duration=None):
        '''Tell Cozmo to directly move his treads.

        Args:
            l_wheel_velocity (float): Velocity of the left tread (in millimeters per second)
            r_wheel_velocity (float): Velocity of the right tread (in millimeters per second)
            l_wheel_acc (float): Acceleration of left tread (in millimeters per second squared)
            None value defaults this to the same as l_wheel_velocity
            r_wheel_acc (float): Acceleration of right tread (in millimeters per second squared)
            None value defaults this to the same as r_wheel_velocity
            duration (float): Time for the robot to drive. Will call :meth:`~cozmo.robot.stop_all_motors`
            after this duration has passed
        '''
        if l_wheel_acc is None:
            l_wheel_acc = l_wheel_velocity
        if r_wheel_acc is None:
            r_wheel_acc = r_wheel_velocity

        msg = _clad_to_engine_iface.DriveWheels(lwheel_speed_mmps=l_wheel_velocity,
                                                rwheel_speed_mmps=r_wheel_velocity,
                                                lwheel_accel_mmps2=l_wheel_acc,
                                                rwheel_accel_mmps2=r_wheel_acc)

        self.conn.send_msg(msg)
        if duration:
            await asyncio.sleep(duration, loop=self._loop)
            self.stop_all_motors()

    def stop_all_motors(self):
        '''Tell Cozmo to stop all motors'''
        msg = _clad_to_engine_iface.StopAllMotors()
        self.conn.send_msg(msg)


    def move_head(self, velocity):
        '''Tell Cozmo's head motor to move with a certain velocity.

        Positive velocity for up, negative velocity for down. Measured in radians per second.

        Args:
            velocity (float): Motor velocity for Cozmo's head.
        '''
        msg = _clad_to_engine_iface.MoveHead(speed_rad_per_sec=velocity)
        self.conn.send_msg(msg)

    def move_lift(self, velocity):
        '''Tell Cozmo's lift motor to move with a certain velocity.

        Positive velocity for up, negative velocity for down.  Measured in radians per second.

        Args:
            velocity (float): Motor velocity for Cozmo's lift.
        '''
        msg = _clad_to_engine_iface.MoveLift()
        msg = _clad_to_engine_iface.MoveLift(speed_rad_per_sec=velocity)
        self.conn.send_msg(msg)

    def say_text(self, text, play_excited_animation=False, use_cozmo_voice=True, duration_scalar=1.8, voice_pitch=0.0):
        '''Have Cozmo say text!

        Args:
            text (string): The words for Cozmo to say
            play_excited_animation (bool): Whether to also play an excited animation while speaking (moves Cozmo a lot)
            use_cozmo_voice (bool): Whether to use Cozmo's robot voice (otherwise, he uses a generic human male voice)
            duration_scalar (float): Adjust the relative duration of the generated text to speech audio
            voice_pitch (float): Adjust the pitch of Cozmo's robot voice [-1.0, 1.0]
        Returns:
            A :class:`cozmo.robot.SayText` action object which can be queried to see when it is complete
        '''

        action = self.say_text_factory(text=text, play_excited_animation=play_excited_animation,
                                       use_cozmo_voice=use_cozmo_voice, duration_scalar=duration_scalar,
                                       voice_pitch=voice_pitch, conn=self.conn,
                                       robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def set_backpack_lights(self, light1, light2, light3, light4, light5):
        '''Set the lights on Cozmo's backpack.

        Args:
            light1-5 (class:'cozmo.lights.light'): The lights for Cozmo's backpack
        '''
        msg = _clad_to_engine_iface.SetBackpackLEDs(robotID=self.robot_id)
        for i, light in enumerate( (light1, light2, light3, light4, light5) ):
            if light is not None:
                lights._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_all_backpack_lights(self, light):
        '''Set the lights on Cozmo's backpack to the same color.

        Args:
            light (class:'cozmo.lights.light'): The lights for Cozmo's backpack
        '''
        light_arr = [ light ] * 5
        self.set_backpack_lights(*light_arr)

    def set_backpack_lights_off(self):
        '''Set the lights on Cozmo's backpack to off.'''
        light_arr = [ lights.off_light ] * 5
        self.set_backpack_lights(*light_arr)

    def set_head_angle(self, angle, accel=10.0, max_speed=10.0, duration=0.0):
        '''Tell Cozmo's head to turn to a given angle.
        Args:
            angle: (:class:`cozmo.util.Angle`) - desired angle in radians for Cozmo's head. (MIN_HEAD_ANGLE to MAX_HEAD_ANGLE)
            accel (float): Acceleration of Cozmo's head in radians per second squared
            max_speed (float): Maximum speed of Cozmo's head in radians per second
            duration (float): Time for Cozmo's head to turn in seconds
        Returns:
            A :class:`cozmo.robot.SetHeadAngle` action object which can be queried to see when it is complete
        '''
        action = self.set_head_angle_factory(angle=angle, max_speed=max_speed, accel=accel, duration=duration, conn=self.conn, robot=self, dispatch_parent=self)

        self._action_dispatcher._send_single_action(action)
        return action

    def set_lift_height(self, height, accel=1.0, max_speed=1.0, duration=10.0):
        '''Tell Cozmo's lift to move to a given height
        Args:
            height (float): desired height for Cozmo's lift 0.0 (bottom) to 1.0 (top) (we clamp it to this range internally)
            accel (float): Acceleration of Cozmo's lift in radians per second squared
            max_speed (float): Maximum speed of Cozmo's lift in radians per second
            duration (float): Time for Cozmo's lift to move in seconds
        Returns:
            A :class:`cozmo.robot.SetLiftHeight` action object which can be queried to see when it is complete
        '''
        action = self.set_lift_height_factory(height=height, max_speed=max_speed, accel=accel, duration=duration, conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    ## Animation Commands ##

    def play_anim(self, name, loop_count=1):
        '''Starts an animation playing on a robot.

        Returns an Animation object as soon as the request to play the animation
        has been sent.  Call the wait_for_completed method on the animation
        if you wish to wait for completion (or listen for the
        :class:`cozmo.anim.EvtAnimationCompleted` event).

        Args:
            name (str): The name of the animation to play
            loop_count (int): Number of times to play the animation
        Returns:
            A :class:`cozmo.anim.Animation` action object which can be queried to see when it is complete
        Raises:
            :class:`ValueError` if supplied an invalid animation name.
        '''
        if name not in self.conn.anim_names:
            raise ValueError('Unknown animation name "%s"' % name)
        action = self.animation_factory(name, loop_count,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def play_anim_trigger(self, trigger, loop_count=1):
        """Starts an animation trigger playing on a robot.

        As noted in the Triggers class, playing a trigger requests that an
        animation of a certain class starts playing, rather than an exact
        animation name as influenced by the robot's mood, and other factors.

        Args:
            trigger (object): An attribute of the :class:`cozmo.anim.Triggers` class
            loop_count (int): Number of times to play the animation
        Returns:
            A :class:`cozmo.anim.AnimationTrigger` action object which can be queried to see when it is complete
        Raises:
            :class:`ValueError` if supplied an invalid animation trigger.
        """
        if not isinstance(trigger, anim._AnimTrigger):
            raise TypeError("Invalid trigger supplied")

        action = self.animation_trigger_factory(trigger, loop_count,
            conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    # Cozmo's Face animation commands

    def display_lcd_face_image(self, screen_data, duration_ms):
        ''' Display a bitmap image on Cozmo's LCD face screen

        Args:
            screen_data (:class:`bytes`): a sequence of pixels (8 pixels per byte) (from e.g. :func:`cozmo.lcd_face.convert_pixels_to_screen_data`)
            duration_ms (float): time to keep displaying this image on Cozmo's face (clamped to 30 seconds in engine)
        '''
        msg = _clad_to_engine_iface.DisplayFaceImage(faceData=screen_data, duration_ms=duration_ms)
        self.conn.send_msg(msg)

    ## Behavior Commands ##

    def start_behavior(self, behavior_type):
        '''Starts executing a behavior.

        Call the stop method on the behavior object at some point in the future
        to terminate execution.

        Args:
            behavior_type (:class:`cozmo.behavior._BehaviorType) - An attribute of
                :class:`cozmo.behavior.BehaviorTypes`.
        Returns:
            :class:`cozmo.behavior.Behavior`
        Raises:
            :class:`TypeError` if an invalid behavior type is supplied.
        '''
        if not isinstance(behavior_type, behavior._BehaviorType):
            raise TypeError('Invalid behavior supplied')
        b = self.behavior_factory(self, behavior_type, is_active=True, dispatch_parent=self)
        msg = _clad_to_engine_iface.ExecuteBehaviorByExecutableType(
                behaviorType=behavior_type.id)
        self.conn.send_msg(msg)
        return b

    async def run_timed_behavior(self, behavior_type, active_time):
        '''Executes a behavior for a set number of seconds.

        This call blocks and stops the behavior after active_time seconds.

        Args:
            behavior_type (:class:`cozmo.behavior._BehaviorType): An attribute of
                :class:`cozmo.behavior.BehaviorTypes`.
            active_time (float): specifies the time to execute in seconds
        Raises:
            :class:`TypeError` if an invalid behavior type is supplied.
        '''
        b = self.start_behavior(behavior_type)
        await asyncio.sleep(active_time, loop=self._loop)
        b.stop()


    ## Object Commands ##

    def pickup_object(self, obj, use_pre_dock_pose=True):
        '''Ask Cozmo to pickup this object.

        Args:
            obj (:class:`cozmo.objects.ObservableObject`): The target object to pick up
            where obj.pickupable is true
            use_pre_dock_pose (bool): whether or not to try to immediately pick up an object or
            relocate and then try
        Returns:
            A :class:`cozmo.robot.PickupObject` action object which can be queried to see when it is complete
        Raises:
            :class:`cozmo.exceptions.RobotBusy` if another action is already running
            :class:`cozmo.exceptions.NotPickupable` if object type can't be picked up
        '''
        if not obj.pickupable:
            raise exceptions.NotPickupable('Cannot pickup this type of object')

        # TODO: Check with the World to see if Cozmo is already holding an object.
        logger.info("Sending pickup object request for object=%s", obj)
        action = self.pickup_object_factory(obj=obj, use_pre_dock_pose=use_pre_dock_pose,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def place_on_object(self, obj, use_pre_dock_pose=True):
        '''Asks Cozmo to place the currently held object onto target object.

        Args:
            obj (:class:`cozmo.objects.ObservableObject`): The target object to place current held
            object on, where obj.place_objects_on_this is true
            use_pre_dock_pose (bool): Whether or not to try to immediately place on the object or
            relocate and then try
        Returns:
            A :class:`cozmo.robot.PlaceOnObject` action object which can be queried to see when it is complete
        Raises:
            :class:`cozmo.exceptions.RobotBusy` if another action is already running
            :class:`cozmo.exceptions.CannotPlaceObjectsOnThis` if the object cannot have objects
            placed on it.
        '''
        if not obj.place_objects_on_this:
            raise exceptions.CannotPlaceObjectsOnThis('Cannot place objects on this type of object')

        # TODO: Check with the World to see if Cozmo is already holding an object.
        logger.info("Sending place on object request for target object=%s", obj)
        action = self.place_on_object_factory(obj=obj, use_pre_dock_pose=use_pre_dock_pose,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def place_object_on_ground_here(self, obj):
        '''Ask Cozmo to place the object he is carrying on the ground at the current location.

        Returns:
            A :class:`cozmo.robot.PlaceObjectOnGroundHere` action object which can be queried to see when it is complete
        Raises:
            :class:`cozmo.exceptions.RobotBusy` if another action is already running
        '''
        # TODO: Check whether Cozmo is known to be holding the object in question
        logger.info("Sending place down here request for object=%s", obj)
        action = self.place_object_on_ground_here_factory(obj=obj,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    ## Interact with seen Face Commands ##

    def turn_towards_face(self, face):
        '''Tells Cozmo to turn towards this face.

        Args:
            face: (:class:`cozmo.faces.Face`): The face Cozmo will turn towards
        Returns:
            A :class:`cozmo.robot.TurnTowardsFace` action object which can be queried to see when it is complete
        '''
        action = self.turn_towards_face_factory(face=face,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action


    ## Robot Driving Commands ##

    def go_to_pose(self, pose, relative_to_robot=False):
        '''Tells Cozmo to drive to the specified pose and orientation.

        If relative_to_robot is set to true, the given pose will assume the robot's pose
        as its origin.
        Since Cozmo understands position by monitoring his tread movement, he does not
        understand movement in the z axis. This means that the only applicable elements of
        pose in this situation are position.x position.y and rotation.angle_z.

        Args:
            pose: (:class:`cozmo.util.Pose`): The destination pose
            relative_to_robot (bool): Whether the given pose is relative to the robot's pose.
        Returns:
            A :class:`cozmo.robot.GoToPose` action object which can be queried to see when it is complete
        '''
        if relative_to_robot:
            pose = self.pose.define_pose_relative_this(pose)
        action = self.go_to_pose_factory(pose=pose,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def turn_in_place(self, angle):
        '''Turn the robot around its current position.

        Args:
            angle: (:class:`cozmo.util.Angle`) - The angle to turn
        Returns:
            A :class:`cozmo.robot.TurnInPlace` action object which can be queried to see when it is complete
        '''
        # TODO: add support for absolute vs relative positioning, speed & accel options
        action = self.turn_in_place_factory(angle=angle,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def drive_off_charger_contacts(self):
        ''' Tells Cozmo to drive forward slightly to get off the charger contacts

        All motor movement is disabled while Cozmo is on the charger to
        prevent hardware damage. This command is the one exception and provides
        a way to drive forward a little to disconnect from the charger contacts
        and thereby re-enable all other commands.

        Returns:
           A :class:`cozmo.robot.DriveOffChargerContacts` action object which can be queried to see when it is complete
        '''
        action = self.drive_off_charger_contacts_factory(conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    def set_robot_volume(self, robot_volume):
        '''Set the volume for the speaker in the robot

        Args:
            robot_volume: (:float:) - The new volume (0.0 = mute, 1.0 = max)
        '''
        msg = _clad_to_engine_iface.SetRobotVolume(robotId=self.robot_id, volume=robot_volume)
        self.conn.send_msg(msg)

        
        
