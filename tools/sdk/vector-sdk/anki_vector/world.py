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

"""
Vector's known view of his world.

This view includes objects and faces it knows about and can currently
see with its camera.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['World']

from . import faces
from . import objects
from . import sync
from . import util

from .events import Events
from .messaging import protocol


class World(util.Component):
    """Represents the state of the world, as known to Vector."""

    #: callable: The factory function that returns a
    #: :class:`faces.Face` class or subclass instance
    face_factory = faces.Face

    #: callable: The factory function that returns an
    #: :class:`objects.LightCube` class or subclass instance.
    light_cube_factory = objects.LightCube

    #: callable: The factory function that returns an
    #: :class:`objects.CustomObject` class or subclass instance.
    custom_object_factory = objects.CustomObject

    def __init__(self, robot):
        super().__init__(robot)

        self._custom_object_definitions = {}

        self._faces = {}
        self._light_cube = {objects.LIGHT_CUBE_1_TYPE: self.light_cube_factory(robot=robot)}
        self._custom_objects = {}

        # Subscribe to callbacks that updates the world view
        robot.events.subscribe(
            self._on_face_observed,
            Events.robot_observed_face)

        robot.events.subscribe(
            self._on_object_observed,
            Events.robot_observed_object)

    def close(self):
        """The world will tear down all its faces and objects."""
        for face in self._faces.values():
            face.teardown()

        for cube in self._light_cube.values():
            cube.teardown()

        for custom_object in self._custom_objects.values():
            custom_object.teardown()

        robot.events.unsubscribe(
            self._on_face_observed,
            Events.robot_observed_face)

        robot.events.unsubscribe(
            self._on_object_observed,
            Events.robot_observed_object)

    @property
    def visible_faces(self):
        """generator: yields each face that Vector can currently see.

        .. code-block:: python

            # Print the visible face's attributes
            for face in robot.world.visible_faces:
                print("Face attributes:")
                print(f"Face id: {face.face_id}")
                print(f"Updated face id: {face.updated_face_id}")
                print(f"Name: {face.name}")
                print(f"Expression: {face.expression}")
                print(f"Timestamp: {face.timestamp}")
                print(f"Pose: {face.pose}")
                print(f"Image Rect: {face.face_rect}")
                print(f"Expression score: {face.expression_score}")
                print(f"Left eye: {face.left_eye}")
                print(f"Right eye: {face.right_eye}")
                print(f"Nose: {face.nose}")
                print(f"Mouth: {face.mouth}")

        Returns:
            A generator yielding :class:`anki_vector.faces.Face` instances
        """
        for face in self._faces.values():
            yield face

    def get_face(self, face_id: int) -> faces.Face:
        """Fetches a Face instance with the given id."""
        return self._faces.get(face_id)

    def get_light_cube(self) -> objects.LightCube:
        """Returns the vector light cube object, regardless of its connection status.

        .. code-block:: python

            cube = robot.world.get_light_cube()
            print('LightCube {0} connected.'.format("is" if cube.is_connected else "isn't"))

        Raises:
            :class:`ValueError` if the cube_id is invalid.
        """
        cube = self._light_cube.get(objects.LIGHT_CUBE_1_TYPE)
        # Only return the cube if it has an object_id
        if cube.object_id is not None:
            return cube
        return None

    @property
    def connected_light_cube(self) -> objects.LightCube:
        """A light cube connected to Vector, if any.

        .. code-block:: python

            robot.world.connect_cube()
            if robot.world.connected_light_cube:
                dock_response = robot.behavior.dock_with_cube(robot.world.connected_light_cube)
        """
        result = None
        cube = self._light_cube.get(objects.LIGHT_CUBE_1_TYPE)
        if cube and cube.is_connected:
            result = cube

        return result

    @sync.Synchronizer.wrap
    async def connect_cube(self) -> protocol.ConnectCubeResponse:
        """Attempt to connect to a cube.

        If a cube is currently connected, this will do nothing.

        .. code-block:: python

            robot.world.connect_cube()
        """
        req = protocol.ConnectCubeRequest()
        result = await self.grpc_interface.ConnectCube(req)

        # dispatch cube connected message
        event = protocol.ObjectConnectionState(
            object_id=result.object_id,
            factory_id=result.factory_id,
            connected=result.success,
            object_type=objects.LIGHT_CUBE_1_TYPE)

        self._robot.events.dispatch_event(event, Events.object_connection_state)

        return result

    @sync.Synchronizer.wrap
    async def disconnect_cube(self) -> protocol.DisconnectCubeResponse:
        """Requests a disconnection from the currently connected cube.

        .. code-block:: python

            robot.world.disconnect_cube()
        """
        req = protocol.DisconnectCubeRequest()
        return await self.grpc_interface.DisconnectCube(req)

    @sync.Synchronizer.wrap
    async def flash_cube_lights(self) -> protocol.FlashCubeLightsResponse:
        """Flash cube lights

        Plays the default cube connection animation on the currently
        connected cube's lights.
        """
        req = protocol.FlashCubeLightsRequest()
        return await self.grpc_interface.FlashCubeLights(req)

    @sync.Synchronizer.wrap
    async def forget_preferred_cube(self) -> protocol.ForgetPreferredCubeResponse:
        """Forget preferred cube.

        'Forget' the robot's preferred cube. This will cause the robot to
        connect to the cube with the highest RSSI (signal strength) next
        time a connection is requested.

        .. code-block:: python

            robot.world.forget_preferred_cube()
        """
        req = protocol.ForgetPreferredCubeRequest()
        return await self.grpc_interface.ForgetPreferredCube(req)

    @sync.Synchronizer.wrap
    async def set_preferred_cube(self, factory_id: str) -> protocol.SetPreferredCubeResponse:
        """Set preferred cube.

        Set the robot's preferred cube and save it to disk. The robot
        will always attempt to connect to this cube if it is available.
        This is only used in simulation (for now).

        .. code-block:: python

            connected_cube = robot.world.connected_light_cube
            if connected_cube:
                robot.world.set_preferred_cube(connected_cube.factory_id)

        :param factory_id: The unique hardware id of the physical cube.
        """
        req = protocol.SetPreferredCubeRequest(factory_id=factory_id)
        return await self.grpc_interface.SetPreferredCube(req)

    @sync.Synchronizer.wrap
    async def delete_all_custom_objects(self):
        """Causes the robot to forget about all custom (fixed + marker) objects it currently knows about.

        Note: This includes all fixed custom objects, and all custom marker object instances,
        BUT this does NOT remove the custom marker object definitions, so Cozmo
        will continue to add new objects if he sees the markers again. To remove
        the definitions for those objects use: :meth:`undefine_all_custom_marker_objects`
        """
        req = protocol.DeleteAllCustomObjectsRequest()
        result = await self.grpc_interface.DeleteAllCustomObjects(req)

        self._remove_custom_marker_object_instances()
        self._remove_fixed_custom_object_instances()

        return result

    @sync.Synchronizer.wrap
    async def delete_custom_marker_objects(self):
        """Causes the robot to forget about all custom marker objects it currently knows about.

        Note: This removes custom marker object instances only, it does NOT remove
        fixed custom objects, nor does it remove the custom marker object definitions, so Cozmo
        will continue to add new objects if he sees the markers again. To remove
        the definitions for those objects use: :meth:`undefine_all_custom_marker_objects`
        """
        req = protocol.DeleteCustomMarkerObjectsRequest()
        result = await self.grpc_interface.DeleteCustomMarkerObjects(req)

        self._remove_custom_marker_object_instances()

        return result

    @sync.Synchronizer.wrap
    async def delete_fixed_custom_objects(self):
        """Causes the robot to forget about all fixed custom objects it currently knows about.

        Note: This removes fixed custom objects only, it does NOT remove
        the custom marker object instances or definitions.
        """
        req = protocol.DeleteFixedCustomObjectsRequest()
        result = await self.grpc_interface.DeleteFixedCustomObjects(req)

        self._remove_fixed_custom_object_instances()

        return result

    @sync.Synchronizer.wrap
    async def undefine_all_custom_marker_objects(self):
        """Remove all custom marker object definitions, and any instances of them in the world."""
        req = protocol.UndefineAllCustomMarkerObjectsRequest()
        result = await self.grpc_interface.UndefineAllCustomMarkerObjects(req)

        self._remove_custom_marker_object_instances()
        # Remove all custom object definitions / archetypes
        self._custom_object_definitions.clear()

        return result

    async def define_custom_box(self, custom_object_type,
                                marker_front, marker_back,
                                marker_top, marker_bottom,
                                marker_left, marker_right,
                                depth_mm, width_mm, height_mm,
                                marker_width_mm, marker_height_mm,
                                is_unique=True):
        '''Defines a cuboid of custom size and binds it to a specific custom object type.

        The engine will now detect the markers associated with this object and send an
        object_observed message when they are seen. The markers must be placed in the center
        of their respective sides. All 6 markers must be unique.

        Args:
            custom_object_type (:class:`cozmo.objects.CustomObjectTypes`): the
                object type you are binding this custom object to
            marker_front (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the front of the object
            marker_back (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the back of the object
            marker_top (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the top of the object
            marker_bottom (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the bottom of the object
            marker_left (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the left of the object
            marker_right (:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the right of the object
            depth_mm (float): depth of the object (in millimeters) (X axis)
            width_mm (float): width of the object (in millimeters) (Y axis)
            height_mm (float): height of the object (in millimeters) (Z axis)
                (the height of the object)
            marker_width_mm (float): width of the printed marker (in millimeters).
            maker_height_mm (float): height of the printed marker (in millimeters).
            is_unique (bool): If True, the engine will assume there is only 1 of this object
                (and therefore only 1 of each of any of these markers) in the world.

        Returns:
            A :class:`cozmo.object.CustomObject` instance with the specified dimensions.
                This is None if the definition failed internally.
                Note: No instances of this object are added to the world until they have been seen.

        Raises:
            TypeError if the custom_object_type is of the wrong type.
            ValueError if the 6 markers aren't unique.
        '''
        if not isinstance(custom_object_type, objects._CustomObjectType):
            raise TypeError("Unsupported object_type, requires CustomObjectType")

        # verify all 6 markers are unique
        markers = {marker_front, marker_back, marker_top, marker_bottom, marker_left, marker_right}
        if len(markers) != 6:
            raise ValueError("all markers must be unique for a custom box")

        custom_object_archetype = self.custom_object_factory(self.conn, self, custom_object_type,
                                                             depth_mm, width_mm, height_mm,
                                                             marker_width_mm, marker_height_mm,
                                                             is_unique, dispatch_parent=self)

        msg = _clad_to_engine_iface.DefineCustomBox(customType=custom_object_type.id,
                                                    markerFront=marker_front.id,
                                                    markerBack=marker_back.id,
                                                    markerTop=marker_top.id,
                                                    markerBottom=marker_bottom.id,
                                                    markerLeft=marker_left.id,
                                                    markerRight=marker_right.id,
                                                    xSize_mm=depth_mm,
                                                    ySize_mm=width_mm,
                                                    zSize_mm=height_mm,
                                                    markerWidth_mm=marker_width_mm,
                                                    markerHeight_mm=marker_height_mm,
                                                    isUnique=is_unique)

        self.conn.send_msg(msg)

        return await self._wait_for_defined_custom_object(custom_object_archetype)

    async def define_custom_cube(self, custom_object_type,
                                 marker,
                                 size_mm,
                                 marker_width_mm, marker_height_mm,
                                 is_unique=True):
        """Defines a cube of custom size and binds it to a specific custom object type.

        The engine will now detect the markers associated with this object and send an
        object_observed message when they are seen. The markers must be placed in the center
        of their respective sides.

        Args:
            custom_object_type (:class:`cozmo.objects.CustomObjectTypes`): the
                object type you are binding this custom object to.
            marker:(:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to every side of the cube.
            size_mm: size of each side of the cube (in millimeters).
            marker_width_mm (float): width of the printed marker (in millimeters).
            maker_height_mm (float): height of the printed marker (in millimeters).
            is_unique (bool): If True, the engine will assume there is only 1 of this object
                (and therefore only 1 of each of any of these markers) in the world.

        Returns:
            A :class:`cozmo.object.CustomObject` instance with the specified dimensions.
                This is None if the definition failed internally.
                Note: No instances of this object are added to the world until they have been seen.

        Raises:
            TypeError if the custom_object_type is of the wrong type.
        """

        if not isinstance(custom_object_type, objects._CustomObjectType):
            raise TypeError("Unsupported object_type, requires CustomObjectType")

        custom_object_archetype = self.custom_object_factory(self.conn, self, custom_object_type,
                                                             size_mm, size_mm, size_mm,
                                                             marker_width_mm, marker_height_mm,
                                                             is_unique, dispatch_parent=self)

        msg = _clad_to_engine_iface.DefineCustomCube(customType=custom_object_type.id,
                                                     marker=marker.id,
                                                     size_mm=size_mm,
                                                     markerWidth_mm=marker_width_mm,
                                                     markerHeight_mm=marker_height_mm,
                                                     isUnique=is_unique)

        self.conn.send_msg(msg)

        return await self._wait_for_defined_custom_object(custom_object_archetype)

    async def define_custom_wall(self, custom_object_type,
                                 marker,
                                 width_mm, height_mm,
                                 marker_width_mm, marker_height_mm,
                                 is_unique=True):
        """Defines a wall of custom width and height, with a fixed depth of 10mm, and binds it to a specific custom object type.

        The engine will now detect the markers associated with this object and send an
        object_observed message when they are seen. The markers must be placed in the center
        of their respective sides.

        Args:
            custom_object_type (:class:`cozmo.objects.CustomObjectTypes`): the
                object type you are binding this custom object to.
            marker:(:class:`cozmo.objects.CustomObjectMarkers`): the marker
                affixed to the front and back of the wall
            width_mm (float): width of the object (in millimeters). (Y axis).
            height_mm (float): height of the object (in millimeters). (Z axis).
            width_mm: width of the wall (along Y axis) (in millimeters).
            height_mm: height of the wall (along Z axis) (in millimeters).
            marker_width_mm (float): width of the printed marker (in millimeters).
            maker_height_mm (float): height of the printed marker (in millimeters).
            is_unique (bool): If True, the engine will assume there is only 1 of this object
                (and therefore only 1 of each of any of these markers) in the world.

        Returns:
            A :class:`cozmo.object.CustomObject` instance with the specified dimensions.
                This is None if the definition failed internally.
                Note: No instances of this object are added to the world until they have been seen.

        Raises:
            TypeError if the custom_object_type is of the wrong type.
        """

        if not isinstance(custom_object_type, objects._CustomObjectType):
            raise TypeError("Unsupported object_type, requires CustomObjectType")

        # TODO: share this hardcoded constant from engine
        WALL_THICKNESS_MM = 10.0

        custom_object_archetype = self.custom_object_factory(self.conn, self, custom_object_type,
                                                             WALL_THICKNESS_MM, width_mm, height_mm,
                                                             marker_width_mm, marker_height_mm,
                                                             is_unique, dispatch_parent=self)

        msg = _clad_to_engine_iface.DefineCustomWall(customType=custom_object_type.id,
                                                     marker=marker.id,
                                                     width_mm=width_mm,
                                                     height_mm=height_mm,
                                                     markerWidth_mm=marker_width_mm,
                                                     markerHeight_mm=marker_height_mm,
                                                     isUnique=is_unique)

        self.conn.send_msg(msg)

        return await self._wait_for_defined_custom_object(custom_object_archetype)

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
            A :class:`cozmo.objects.FixedCustomObject` instance with the specified dimensions and pose.
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
        #pylint: disable=no-member
        response = await self.wait_for(_clad._MsgCreatedFixedCustomObject)
        fixed_custom_object = objects.FixedCustomObject(pose, x_size_mm, y_size_mm, z_size_mm, response.msg.objectID)
        self._objects[fixed_custom_object.object_id] = fixed_custom_object
        return fixed_custom_object

    #### Private Methods ####

    def _allocate_custom_marker_object(self, msg)
        # obj is the base object type for this custom object. We make instances of this for every
        # unique object_id we see of this custom object type.
        obj = self._custom_objects.get(msg.objectType)
        if not obj:
            logger.error('Received a custom object type: %s that has not been defined yet. Msg=%s' %
                                                                            (msg.objectType, msg))
            return None
        custom_object = self.custom_object_factory(self.conn, self, obj.object_type,
                                                   obj.x_size_mm, obj.y_size_mm, obj.z_size_mm,
                                                   obj.marker_width_mm, obj.marker_height_mm,
                                                   obj.is_unique, dispatch_parent=self)
        custom_object.object_id = msg.objectID
        self._objects[custom_object.object_id] = custom_object
        logger.debug('Allocated object_id=%s to CustomObject %s', msg.objectID, custom_object)
        return custom_object

    def _remove_all_custom_marker_object_instances(self):
        for id, obj in list(self._custom_objects.items()):
            if isinstance(obj, objects.CustomObject):
                logger.info("Removing CustomObject instance: id %s = obj '%s'", id, obj)
                self._custom_objects.pop(id, None)

    def _remove_all_fixed_custom_object_instances(self):
        for id, obj in list(self._custom_objects.items()):
            if isinstance(obj, objects.FixedCustomObject):
                logger.info("Removing FixedCustomObject instance: id %s = obj '%s'", id, obj)
                self._custom_objects.pop(id, None)

    #### Private Event Handlers ####

    def _on_face_observed(self, _, msg):
        """Adds a newly observed face to the world view."""
        if msg.face_id not in self._faces:
            pose = util.Pose(x=msg.pose.x, y=msg.pose.y, z=msg.pose.z,
                             q0=msg.pose.q0, q1=msg.pose.q1,
                             q2=msg.pose.q2, q3=msg.pose.q3,
                             origin_id=msg.pose.origin_id)
            image_rect = util.ImageRect(msg.img_rect.x_top_left,
                                        msg.img_rect.y_top_left,
                                        msg.img_rect.width,
                                        msg.img_rect.height)
            face = self.face_factory(self.robot,
                                     pose, image_rect, msg.face_id, msg.name, msg.expression, msg.expression_values,
                                     msg.left_eye, msg.right_eye, msg.nose, msg.mouth, msg.timestamp)
            self._faces[face.face_id] = face

    def _on_object_observed(self, _, msg):
        """Adds a newly observed custom object to the world view."""
        if msg.object_family == protocol.ObjectFamily.Value("CUSTOM_OBJECT"):
            if msg.object_id not in self._custom_objects:
                custom_object = self._allocate_custom_object(msg)
                if custom_object:
                    self._custom_objects[msg.object_id] = custom_object
