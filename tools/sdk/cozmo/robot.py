__all__ = ['EvtRobotReady',
           'PickupObject', 'PlaceObjectOnGroundHere', 'TurnInPlace', 'GoToPose'
           'Cozmo']


import asyncio

from . import logger, logger_protocol
from . import action
from . import anim
from . import behavior
from . import conn
from . import event
from . import lights
from . import objects
from . import util
from . import world

from ._clad import _clad_to_engine_iface, _clad_to_engine_cozmo


#### Events

class EvtRobotReady(event.Event):
    '''Generated when the robot has been initialized and is ready for commands'''
    robot = "Cozmo object representing the robot to command"


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

class PickupObject(action.Action):
    '''Represents the pickup object action in progress.

    Returned by :meth:`~cozmo.robot.Cozmo.pickup_object`
    '''

    def __init__(self, obj, use_pre_dock_pose=True, **kw):
        super().__init__(**kw)
        #: The object (eg. an instance of :class:`cozmo.objects.LightCube`) that was picked up
        self.obj = obj
        #: A bool that is true when cozmo needs to go to a pose before attempting to navigate to the object
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
        #: The object (eg. an instance of :class:`cozmo.objects.LightCube`) that the held object will be placed on
        self.obj = obj
        #: A bool that is true when cozmo needs to go to a pose before attempting to navigate to the object
        self.use_pre_dock_pose = use_pre_dock_pose

    def _repr_values(self):
        return "object=%s" % (self.obj)

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

    def __init__(self, text, play_excited_animation, use_cozmo_voice, **kw):
        super().__init__(**kw)
        self.text = text

        # Note: play_event must be an AnimationTrigger that supports text-to-speech being generated

        if play_excited_animation:
            self.play_event = _clad_to_engine_cozmo.AnimationTrigger.OnSawNewNamedFace
        else:
            # Count = No animation - will replace with SdkTextToSpeech when that works with text-to-speech
            self.play_event = _clad_to_engine_cozmo.AnimationTrigger.Count

        if use_cozmo_voice:
            self.say_style = _clad_to_engine_cozmo.SayTextStyle.Name_Normal
        else:
            # default male human voice
            self.say_style = _clad_to_engine_cozmo.SayTextStyle.Normal

    def _repr_values(self):
        return "text=%s" % (self.text)

    def _encode(self):
        return _clad_to_engine_iface.SayText(text=self.text, playEvent=self.play_event, style=self.say_style)

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

    * A World object, which tracks the state of the world the robot knows about

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

    # other factories
    animation_factory = anim.Animation
    animation_trigger_factory = anim.AnimationTrigger
    behavior_factory = behavior.Behavior
    world_factory = world.World


    def __init__(self, conn, robot_id, is_primary, **kw):
        super().__init__(**kw)
        self.conn = conn
        self._is_ready = False
        self.robot_id = robot_id
        self._pose = None
        self.is_primary = is_primary

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
            self.enable_reactionary_behaviors(False)
            await self.world.delete_all_custom_objects()
            self._reset_behavior_state()
            self.unlock_all_behaviors()
            # SetRobotImageSendMode(True)
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
    


    #### Private Event Handlers ####

    #def _recv_default_handler(self, event, **kw):
    #    msg = kw.get('msg')
    #    logger_protocol.debug("Robot received unhandled internal event_name=%s  kw=%s", event.event_name, kw)

    def recv_default_handler(self, event, **kw):
        logger.debug("Robot received unhandled public event=%s", event)

    def _recv_msg_processed_image(self, _, *, msg):
        pass

    def _recv_msg_robot_state(self, evt, *, msg):
        #TODO flesh out the rest of the params in the robotState message
        self._pose = util.Pose(x=msg.pose.x, y=msg.pose.y, z=msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3)
        self.last_image_time = msg.lastImageTimeStamp

    #### Public Event Handlers ####


    #### Commands ####

    def enable_reactionary_behaviors(self, should_enable):
        '''Enable or disable Cozmo's responses to being handled or observing the world.

        Args:
            should_enable (bool): True if the robot should react to its environment.
        '''
        msg = _clad_to_engine_iface.EnableReactionaryBehaviors(enabled=should_enable)
        self.conn.send_msg(msg)

    def set_robot_image_send_mode(self):
        '''Send this message when you want to enable Cozmo streaming images to the SDK.'''
        msg = _clad_to_engine_iface.SetRobotImageSendMode(
                robotID=1, mode=_clad_to_engine_cozmo.ImageSendMode.Stream)
        self.send_msg(msg)


    ### Low-Level Commands ###

    async def drive_wheels(self, l_wheel_velocity, r_wheel_velocity,
                                 l_wheel_acc=None, r_wheel_acc=None, duration=None):
        '''Tell Cozmo to directly move his treads.

        Args:
            l_wheel_velocity (float): Velocity of the left tread
            r_wheel_velocity (float): Velocity of the right tread
            l_wheel_acc (float): Acceleration of left tread, 
            None value defaults this to the same as l_wheel_velocity
            r_wheel_acc (float): Acceleration of right tread
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
        '''Tell cozmo to stop all motors'''
        msg = _clad_to_engine_iface.StopAllMotors()
        self.conn.send_msg(msg)


    def move_head(self, velocity):
        '''Tell Cozmo's head motor to move with a certain velocity.

        Positive velocity for up, negative velocity for down.

        Args:
            velocity (float): Motor velocity for Cozmo's head.
        '''
        msg = _clad_to_engine_iface.MoveHead(speed_rad_per_sec=velocity)
        self.conn.send_msg(msg)

    def move_lift(self, velocity):
        '''Tell Cozmo's lift motor to move with a certain velocity.

        Positive velocity for up, negative velocity for down.

        Args:
            velocity (float): Motor velocity for Cozmo's lift.
        '''
        msg = _clad_to_engine_iface.MoveLift()
        msg = _clad_to_engine_iface.MoveLift(speed_rad_per_sec=velocity)
        self.conn.send_msg('MoveLift', msg)

    def set_backpack_lights(self, light1, light2, light3, light4, light5):
        '''Set the lights on cozmo's back.
        
        Args:
            light1-5 (class:'cozmo.lights.light'): The lights for Cozmo's backpack
        '''
        msg = _clad_to_engine_iface.SetBackpackLEDs(robotID=self.robot_id)
        for i, light in enumerate( (light1, light2, light3, light4, light5) ):
            if light is not None:
                lights._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_all_backpack_lights(self, light):
        '''Set the lights on cozmo's back to the same color.
        
        Args:
            light (class:'cozmo.lights.light'): The lights for Cozmo's backpack
        '''
        light_arr = [ light ] * 5
        self.set_backpack_lights(*light_arr)

    def set_backpack_lights_off(self):
        '''Set the lights on cozmo's back to off.'''
        light_arr = [ lights.off_light ] * 5
        self.set_backpack_lights(*light_arr)

    def say_text(self, text, play_excited_animation=False, use_cozmo_voice=True):
        '''Have Cozmo say text!
        
        Args:
            text (string): The words for cozmo to say
        Returns:
            A class:'cozmo.robot.SayText' instance to track the action in progress.
        '''

        action = self.say_text_factory(text, conn=self.conn, robot=self, play_excited_animation=play_excited_animation,
                                       use_cozmo_voice=use_cozmo_voice, dispatch_parent=self)
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
            loop_ocunt (int): Number of times to play the animation
        Returns:
            :class:`cozmo.anim.Animation`
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
            loop_ocunt (int): Number of times to play the animation
        Returns:
            :class:`cozmo.anim.AnimationTrigger`
        Raises:
            :class:`ValueError` if supplied an invalid animation trigger.
        """
        if not isinstance(trigger, anim._AnimTrigger):
            raise TypeError("Invalid trigger supplied")

        action = self.animation_trigger_factory(trigger, loop_count,
            conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

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
        msg = _clad_to_engine_iface.ExecuteBehavior(
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

    def unlock_all_behaviors(self):
        '''Makes all behaviors available on the robot.'''
        umsg = _clad_to_engine_iface.RequestSetUnlock(unlocked=True)
        umsg.unlockID = _clad_to_engine_cozmo.UnlockId.All
        self.conn.send_msg(umsg)


    ## Object Commands ##

    def pickup_object(self, obj, use_pre_dock_pose=True):
        '''Ask Cozmo to pickup this object.

        Args:
            obj (:class:`cozmo.objects.ObservableObject`): The target object to pick up
            where obj.pickupable is true
            use_pre_dock_pose (bool): whether or not to try to immediately pick up an object or
            relocate and then try 
        Returns:
            A :class:`cozmo.robot.PickupObject` instance which can be
            observed to wait for the pickup to complete.
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
            A :class:`cozmo.robot.PlaceOnObject` instance which can be observed
            to wait for the place on object to complete.
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
        '''Ask Cozmo to place the object its carrying on the ground at the current location.

        Returns:
            A :class:`cozmo.robot.PlaceObjectOnGroundHere` instance which can be
            observed to wait for the pickup to complete.
        Raises:
            :class:`cozmo.exceptions.RobotBusy` if another action is already running
        '''
        # TODO: Check whether Cozmo is known to be holding the object in question
        logger.info("Sending place down here request for object=%s", obj)
        action = self.place_object_on_ground_here_factory(obj=obj,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action

    ## Face Commands ##

    def turn_towards_face(self, face):
        '''Tells Cozmo to turn towards this face.

        Args:
            face: (:class:`cozmo.faces.Face`): The face Cozmo will turn towards
        Returns:
            A :class:`cozmo.robot.TurnTowardsFace` action object
        '''
        action = self.turn_towards_face_factory(face=face,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action


    ## Robot Driving Commands ##

    def go_to_pose(self, pose, relative_to_robot=False):
        '''Tells Cozmo to drive to the specified pose and orientation.

        If relative_to_robot is set to true, the given pose will assume the robot's pose
        as it's origin.
        Since Cozmo understands position by monitoring his tread movement, he does not
        understand movment in the z axis. This means that the only applicable elements of 
        pose in this situation are position.x position.y and rotation.angle_z.

        Args:
            pose: (:class:`cozmo.util.Pose`): The destination pose
            relative_to_robot (bool): Whether the given pose is relative to the robot's pose.
        Returns:
            A :class:`cozmo.robot.GoToPose` action object
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
            A :class:`cozmo.robot.TurnInPlace` action object
        '''
        # TODO: This action does not return success on completion atm!  Get a 255 tag instead.
        # TODO: add support for absolute vs relative positioning, speed & accel options
        # See https://ankiinc.atlassian.net/browse/COZMO-3232
        action = self.turn_in_place_factory(angle=angle,
                conn=self.conn, robot=self, dispatch_parent=self)
        self._action_dispatcher._send_single_action(action)
        return action
