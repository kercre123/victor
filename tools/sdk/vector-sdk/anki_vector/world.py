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
The "world" represents the robot's known view of its environment.
It keeps track of all the faces Vector has observed.
"""

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['World']

from . import faces
from . import objects
from . import sync
from . import util

from .messaging import protocol


class World(util.Component):
    """Represents the state of the world, as known to Vector."""

    #: callable: The factory function that returns a
    #: :class:`faces.Face` class or subclass instance
    face_factory = faces.Face

    #: callable: The factory function that returns an
    #: :class:`objects.LightCube` class or subclass instance.
    light_cube_factory = objects.LightCube

    def __init__(self, robot):
        super().__init__(robot)

        self._faces = {}

        self.light_cube = {objects.LightCube1Type: self.light_cube_factory(robot=robot, world=self)}
        self._objects = {}

        # Subscribe to a callback that updates the world view
        robot.events.subscribe("robot_observed_face",
                               self.add_update_face_to_world_view)
        # Subscribe to a callback that updates a face's id
        robot.events.subscribe("robot_changed_observed_face_id",
                               self.update_face_id)

        # Subscribe to callbacks related to objects
        robot.events.subscribe("object_event",
                               self.on_object_event)

    @property
    def visible_faces(self):
        """generator: yields each face that Vector can currently see.

        Returns:
            A generator yielding :class:`anki_vector.faces.Face` instances
        """
        for face in self._faces.values():
            yield face

    def get_face(self, face_id):
        """anki_vector.faces.Face: Fetch a Face instance with the given id"""
        return self._faces.get(face_id)

    def add_update_face_to_world_view(self, _, msg):
        """Adds/Updates the world view when a face is observed"""
        face = self.face_factory()
        face.unpack_face_stream_data(msg)
        self._faces[face.face_id] = face

    def update_face_id(self, _, msg):
        """Updates the face id when a tracked face (negative ID) is recognized and
        receives a positive ID or when face records get merged"""
        face = self.get_face(msg.old_id)
        if face:
            face.updated_face_id = msg.new_id

    def _allocate_light_cube(self, object_type, object_id, factory_id):
        cube = self.light_cube.get(object_type)
        if not cube:
            self.robot.logger.error('Received invalid cube object_type=%s', object_type)
            return None
        cube.object_id = object_id
        self._objects[cube.object_id] = cube
        cube.factory_id = factory_id
        self.robot.logger.debug('Allocated object_id=%d to light cube %s', object_id, cube)
        return cube

    def get_light_cube(self):
        """Returns the connected light cube

        Returns:
            :class:`anki_vector.objects.LightCube`: The LightCube object with that cube_id

        Raises:
            :class:`ValueError` if the cube_id is invalid.
        """
        cube = self.light_cube.get(objects.LightCube1Type)
        # Only return the cube if it has an object_id
        if cube.object_id is not None:
            return cube
        return None

    def get_object_by_id(self, object_id):
        if object_id not in self._objects:
            raise ValueError("Invalid object_id %s" % object_id)

        return self._objects[object_id]

    @property
    def connected_light_cubes(self):
        """Returns all light cube attached to anki_vector

        Returns:
            A list of :class:`anki_vector.objects.LightCube` instances
        """
        result = []
        cube = self.light_cube.get(objects.LightCube1Type)
        if cube and cube.is_connected:
            result.append(cube)

        return result

    @sync.Synchronizer.wrap
    async def connect_cube(self):
        req = protocol.ConnectCubeRequest()
        result = await self.interface.ConnectCube(req)

        if not result.object_id in self._objects:
            self.light_cube[objects.LightCube1Type] = self._allocate_light_cube(objects.LightCube1Type, result.object_id, result.factory_id)
        self._objects[result.object_id].on_connection_state_changed(result.success, result.factory_id)

        return result

    @sync.Synchronizer.wrap
    async def disconnect_cube(self):
        req = protocol.DisconnectCubeRequest()
        return await self.interface.DisconnectCube(req)

    @sync.Synchronizer.wrap
    async def flash_cube_lights(self):
        req = protocol.FlashCubeLightsRequest()
        return await self.interface.FlashCubeLights(req)

    @sync.Synchronizer.wrap
    async def forget_preferred_cube(self):
        req = protocol.ForgetPreferredCubeRequest()
        return await self.interface.ForgetPreferredCube(req)

    @sync.Synchronizer.wrap
    async def set_preferred_cube(self, factory_id):
        req = protocol.SetPreferredCubeRequest(factory_id=factory_id)
        return await self.interface.SetPreferredCube(req)

    def on_object_event(self, _, msg):
        object_event_type = msg.WhichOneof("object_event_type")

        object_event_handlers = {
            "object_connection_state": self._on_object_connection_state,
            "object_moved": self._on_object_moved,
            "object_stopped_moving": self._on_object_stopped_moving,
            "object_up_axis_changed": self._on_object_up_axis_changed,
            "object_tapped": self._on_object_tapped,
            "robot_observed_object": self._on_robot_observed_object}

        if object_event_type in object_event_handlers:
            handler = object_event_handlers[object_event_type]
            handler(object_event_type, getattr(msg, object_event_type))
        else:
            self.logger.warning('An object_event was recieved with unknown type:{0}'.format(object_event_type))

    def _on_object_connection_state(self, _, msg):
        self.logger.debug('Got Object Connection State Message ( object_id: {0}, factory_id: {1}, object_type: {2}, connected: {3} )'.format(msg.object_id, msg.factory_id, msg.object_type, msg.connected))
        # Currently only one lightcube id is supported
        if msg.object_type == objects.LightCube1Type:
            if not msg.object_id in self._objects:
                self.light_cube[objects.LightCube1Type] = self._allocate_light_cube(msg.object_type, msg.object_id, msg.factory_id)

            self._objects[msg.object_id].on_connection_state_changed(msg.connected, msg.factory_id)
        else:
            self.logger.warning('An object without the expected LightCube type is sending a connection state update, object_id:{0}, factory_id:{1}'.format(msg.object_id, msg.factory_id))

    def _on_object_moved(self, _, msg):
        self.logger.debug('Got Object Moved Message ( timestamp: {0}, object_id: {1} )'.format(msg.timestamp, msg.object_id))

        if msg.object_id in self._objects:
            self._objects[msg.object_id].on_moved(msg)
        else:
            self.logger.warning('An object not currently tracked by the world moved with id {0}'.format(msg.object_id))

    def _on_object_stopped_moving(self, _, msg):
        self.logger.debug('Got Object Stopped Moving Message ( timestamp: {0}, object_id: {1} )'.format(msg.timestamp, msg.object_id))

        if msg.object_id in self._objects:
            self._objects[msg.object_id].on_stopped_moving(msg)
        else:
            self.logger.warning('An object not currently tracked by the world stopped moving with id {0}'.format(msg.object_id))

    def _on_object_up_axis_changed(self, _, msg):
        self.logger.debug('Got Object Up Axis Changed Message ( timestamp: {0}, object_id: {1}, up_axis: {2} )'.format(msg.timestamp, msg.object_id, msg.up_axis))

        if msg.object_id in self._objects:
            self._objects[msg.object_id].on_up_axis_changed(msg)
        else:
            self.logger.warning('Up Axis changed on an object not currently tracked by the world with id {0}'.format(msg.object_id))

    def _on_object_tapped(self, _, msg):
        self.logger.debug('Got Object Tapped Message ( timestamp: {0}, object_id: {1} )'.format(msg.timestamp, msg.object_id))

        if msg.object_id in self._objects:
            self._objects[msg.object_id].on_tapped(msg)
        else:
            self.logger.warning('Tapped an object not currently tracked by the world with id {0}'.format(msg.object_id))

    def _on_robot_observed_object(self, _, msg):
        self.logger.debug('Got Robot Observed Object Message ( timestamp: {0}, object_family: {1}, object_type: {2}, object_id: {3}, img_rect: {4}, pose: {5}, top_face_orientation_rad: {6}, is_active: {7} )'.format(msg.timestamp, msg.object_family, msg.object_type, msg.object_id, msg.img_rect, msg.pose, msg.top_face_orientation_rad, msg.is_active))

        # is_active refers to whether an object has a battery, which is a given for the cube
        if msg.object_id in self._objects:
            self._objects[msg.object_id].on_observed(msg)
        else:
            self.logger.warning('Observed an object not currently tracked by the world with id {0}'.format(msg.object_id))
