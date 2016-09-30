# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['World']

import asyncio
import time

from . import logger

from . import event
from . import faces
from . import objects
from . import world

from . import _clad
from ._clad import _clad_to_engine_iface, _clad_to_game_cozmo



class EvtNewCameraImage(event.Event):
    '''Dispatched when a new camera image is received and processed from the robot's camera.'''
    image = 'A :class:`CameraImage` object'


class World(event.Dispatcher):
    '''Represents the state of the world, as known to a Cozmo robot.'''

    #: Factory to generate LightCube objects.
    face_factory = faces.Face
    light_cube_factory = objects.LightCube
    custom_object_factory = objects.CustomObject

    def __init__(self, conn, robot, **kw):
        super().__init__(**kw)
        #: The active :class:`cozmo.conn.CozmoConnection`
        self.conn = conn

        #: The primary robot :class:`cozmo.robot.Cozmo`
        self.robot = robot

        self.custom_objects = {}

        #: The latest :class:`CameraImage` received, or None
        self.latest_image = None
        self.light_cubes = {}

        self._last_image_number = -1
        self._objects = {}
        self._faces = {}
        self._active_behavior = None
        self._active_action = None
        self._init_light_cubes()


    #### Private Methods ####

    def _init_light_cubes(self):
        # XXX assume that the three cubes exist; haven't yet found an example
        # where this isn't hard-coded.  Smells bad.  Don't allocate an object id
        # i've seen them have an id of 0 when observed.
        self.light_cubes = {
            objects.LightCube1Id: self.light_cube_factory(self.conn, self, dispatch_parent=self),
            objects.LightCube2Id: self.light_cube_factory(self.conn, self, dispatch_parent=self),
            objects.LightCube3Id: self.light_cube_factory(self.conn, self, dispatch_parent=self),
        }

    def _allocate_object_from_msg(self, msg):
        if msg.objectFamily == _clad_to_game_cozmo.ObjectFamily.LightCube:
            cube = self.light_cubes.get(msg.objectType)
            if not cube:
                logger.error('Received invalid cube objecttype=%s msg=%s', msg.objectType, msg)
                return
            cube.object_id = msg.objectID
            self._objects[cube.object_id] = cube
            cube._robot = self.robot # XXX this will move if/when we have multi-robot support
            logger.debug('Allocated object_id=%d to light cube %s', msg.objectID, cube)
            return cube

        elif (msg.objectFamily == _clad_to_game_cozmo.ObjectFamily.CustomObject):
            # obj is the base object type for this custom object. We make instances of this for every
            # unique object_id we see of this custom object type.
            obj = self.custom_objects.get(msg.objectType)
            if not obj:
                logger.error('Received a custom object type: %s that has not been defined yet. Msg=%s' %
                                                                                (msg.objectType, msg))
                return
            custom_object = self.custom_object_factory(self.conn, self, obj.object_type,
                                                       obj.x_size_mm, obj.y_size_mm, obj.z_size_mm,
                                                       obj.marker_width_mm, obj.marker_height_mm,
                                                       dispatch_parent=self)
            custom_object.object_id = msg.objectID
            self._objects[custom_object.object_id] = custom_object
            logger.debug('Allocated object_id=%s to CustomObject %s', msg.objectID, custom_object)
            return custom_object

    def _allocate_face_from_msg(self, msg):
        face = self.face_factory(self.conn, self, self.robot, dispatch_parent=self)
        face.face_id = msg.faceID
        self._faces[face.face_id] = face
        logger.debug('Allocated face_id=%s to face=%s', face.face_id, face)
        return face

    #### Properties ####

    @property
    def active_behavior(self):
        '''A bool of whether or not Cozmo is currently executing a behavior.'''
        return self._active_behavior

    @property
    def active_action(self):
        '''A bool of whether or not Cozmo is currently executing an action.'''
        return self._active_action


    #### Private Event Handlers ####

    def _recv_msg_robot_observed_object(self, evt, *, msg):
        #The engine still sends observed messages for fixed custom objects, this is a bug
        if evt.msg.objectType == _clad_to_game_cozmo.ObjectType.Custom_Fixed:
            return
        obj = self._objects.get(msg.objectID)
        if not obj:
            obj = self._allocate_object_from_msg(msg)
        if obj:
            obj.dispatch_event(evt)

    def _recv_msg_robot_observed_face(self, evt, *, msg):
        face = self._faces.get(msg.faceID)
        if not face:
            face = self._allocate_face_from_msg(msg)
        if face:
            face.dispatch_event(evt)

    def _recv_msg_object_tapped(self, evt, *, msg):
        obj = self._objects.get(msg.objectID)
        if not obj:
            logger.warn('Tap event received for unknown object id %s', msg.objectID)
            return
        obj.dispatch_event(evt)



    #### Public Event Handlers ####

    def recv_evt_object_tapped(self, event, *, obj, tap_count, tap_duration, **kw):
        pass

    def recv_evt_behavior_started(self, evt, *, behavior, **kw):
        self._active_behavior = behavior

    def recv_evt_behavior_stopped(self, evt, *, behavior, **kw):
        self._active_behavior = None

    def recv_evt_action_started(self, evt, *, action, **kw):
        self._active_action = action

    def recv_evt_action_completed(self, evt, *, action, **kw):
        self._active_action = None

    def recv_evt_new_raw_camera_image(self, evt, *, image, **kw):
        self._last_image_number += 1
        processed_image = CameraImage(image, self._last_image_number)
        self.latest_image = processed_image
        self.dispatch_event(EvtNewCameraImage, image=processed_image)


    #### Event Wrappers ####

    async def wait_for_observed_light_cube(self, timeout=None):
        '''Waits for one of the light cubes to be observed by the robot.

        Args:
            timeout (float): Number of seconds to wait for a cube to be observed, or None for indefinite
        Returns:
            The :class:`cozmo.objects.LightCube` object that was observed.
        '''
        filter = event.Filter(objects.EvtObjectObserved,
                obj=lambda obj: isinstance(obj, objects.LightCube))
        evt = await self.wait_for(filter, timeout=timeout)
        return evt.obj

    async def wait_for_observed_face(self, timeout=None):
        '''Waits for a face to be observed by the robot.

        Args:
            timeout (float): Number of seconds to wait for a face to be observed, or None for indefinite
        Returns:
            The :class:`cozmo.faces.Face` object that was observed.
        '''
        filter = event.Filter(faces.EvtFaceObserved)
        evt = await self.wait_for(filter, timeout=timeout)
        return evt.face

    # TODO make this so it does not compound timeout time.
    async def wait_until_observe_num_objects(self, num, object_type=None, timeout=None):
        '''Waits for a certain number of objects to be seen.

        If obj_type is provided, the robot will wait to observe specific object types.
        The timeout is for the time in between each object. Theoretically if num=3,
        and timeout=10, the max timeout would be ~30 seconds. TODO fix this.

        Args:
            num (float): The number of unique objects to wait for.
            object_type (class:`cozmo.objects.ObservableObject`): If provided this will filter observed objects.
            timeout (float): Time waited in between each object seen.
        Returns:
            A list of length <= num in order of the unique objects class:`cozmo.objects.ObservableObject`
            observed during this wait.
        '''
        #Filter by object type if provided
        if object_type:
            if not issubclass(object_type, objects.ObservableObject):
                raise TypeError("Expected object_type to be ObservableObject")
            filter = event.Filter(objects.EvtObjectObserved,
                    obj=lambda obj: isinstance(obj, object_type))
        else:
            filter = event.Filter(objects.EvtObjectObserved)
        num_seen = 0
        objs_seen = []
        #Wait until we see a certain number of unique objects
        while num_seen < num:
            #Instead of crashing, if we ever end up not seeing enough objects, just return what we have.
            try:
                evt = await self.wait_for(filter, timeout=timeout)
            except asyncio.TimeoutError:
                return objs_seen
            if evt.obj not in objs_seen:
                objs_seen.append(evt.obj)
                num_seen += 1
        return objs_seen




    #### Commands ####

    def send_available_objects(self):
        # XXX description for this?
        msg = _clad_to_engine_iface.SendAvailableObjects(
                robotID=self.robot.robot_id, enable=True)
        self.conn.send_msg(msg)

    async def _delete_all_objects(self):
        # XXX marked this as private as apparently problematic to call
        # currently as it deletes light cubes too.
        msg = _clad_to_engine_iface.DeleteAllObjects(robotID=self.robot.robot_id)
        self.conn.send_msg(msg)
        await self.wait_for(_clad._MsgRobotDeletedAllObjects)
        # TODO: reset local object state

    async def delete_all_custom_objects(self):
        """Causes the robot to forget about all custom objects it currently knows about."""
        msg = _clad_to_engine_iface.DeleteAllCustomObjects(robotID=self.robot.robot_id)
        self.conn.send_msg(msg)
        # TODO: use a filter to wait only for a message for the active robot
        await self.wait_for(_clad._MsgRobotDeletedAllCustomObjects)
        # TODO: reset local object stte

    async def define_custom_object(self, object_type, x_size_mm, y_size_mm, z_size_mm,
                                   marker_width_mm=25, marker_height_mm=25):
        '''Defines a cuboid of custom size and binds it to a specific custom object type.

        The engine will now detect the markers associated with this object and send an
        object_observed message when they are seen. The markers must be placed in the center
        of their respective sides. The markers must be placed in the same order as given in the diagram.
        TODO MAKE THE DIAGRAM

        Args:
            object_type (:class:`cozmo.objects.CustomObjectTypes`): the object type you are binding this custom object to
            x_size_mm (float): size of the object (in millimeters) in the x axis.
            y_size_mm (float): size of the object (in millimeters) in the y axis.
            z_size_mm (float): size of the object (in millimeters) in the z axis.
            marker_width_mm (float): width of the printed marker (in millimeters).
            maker_height_mm (float): height of the printed marker (in millimeters).

        Returns:
            A :class:`cozmo.object.CustomObject` instance with the specified dimensions.
                                                 This is not included in the world until it has been seen.
        '''
        if not isinstance(object_type, objects._CustomObjectType):
            raise TypeError("Unsupported object_type, requires CustomObjectType")
        custom_object_base = self.custom_object_factory(object_type,
                                                        x_size_mm, y_size_mm, z_size_mm,
                                                        marker_width_mm, marker_height_mm,
                                                        self.conn, self, dispatch_parent=self)
        self.custom_objects[object_type.id] = custom_object_base
        msg = _clad_to_engine_iface.DefineCustomObject(objectType=object_type.id,
                                                       xSize_mm=x_size_mm, ySize_mm=y_size_mm, zSize_mm=z_size_mm,
                                                       markerWidth_mm=marker_width_mm, markerHeight_mm=marker_height_mm)
        self.conn.send_msg(msg)
        await self.wait_for(_clad._MsgDefinedCustomObject)
        return custom_object_base

    async def create_custom_fixed_object(self, pose, x_size_mm, y_size_mm, z_size_mm,
                                         relative_to_robot=False, use_robot_origin=True):
        '''Defines a cuboid of custom size and places it in the world. It cannot be observed.

        Args:
            pose (:class:`cozmo.util.Pose`): The pose of the object we are creating.
            x_size_mm (float): size of the object (in millimeters) in the x axis.
            y_size_mm (float): size of the object (in millimeters) in the y axis.
            z_size_mm (float): size of the object (in millimeters) in the z axis.
            relative_to_robot (bool): whether or not the pose given assumes the robot's pose as its origin.
            use_robot_origin (bool): whether or not to override the origin_id in the given pose to be
                                      the origin_id of Cozmo.

        Returns:
            A :class:`cozmo.object.FixedCustomObject` instance with the specified dimensions and pose.
        '''
        # Override the origin of the pose to be the same as the robot's. This will make sure they are in
        # the same space in the engine every time.
        if use_robot_origin:
            pose.origin_id = self.robot.pose.origin_id
        # In this case define the given pose to be with respect to the robot's pose as its origin.
        if relative_to_robot:
            pose = self.robot.pose.define_pose_relative_this(pose)
        msg = _clad_to_engine_iface.CreateFixedCustomObject(pose=pose.encode_pose(),
                                                            xSize_mm=x_size_mm, ySize_mm=y_size_mm, zSize_mm=z_size_mm)
        self.conn.send_msg(msg)
        response = await self.wait_for(_clad._MsgCreatedFixedCustomObject)
        fixed_custom_object = objects.FixedCustomObject(pose, x_size_mm, y_size_mm, z_size_mm, response.msg.objectID)
        self._objects[fixed_custom_object.object_id] = fixed_custom_object
        return fixed_custom_object


class CameraImage:
    def __init__(self, raw_image, image_number=0):
        self.raw_image = raw_image
        self.image_number = image_number
        self.image_recv_time = time.time()

    @property
    def annotated_image(self):
        return self.raw_image
