# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['EvtFaceObserved', 'Face']


import asyncio
import collections
import time

from . import logger

from . import action
from . import event
from . import objects
from . import util

from ._clad import _clad_to_engine_iface


# Length of time to go without receiving an observed event before
# assuming that Cozmo can no longer see a face.
FACE_VISIBILITY_TIMEOUT = objects.OBJECT_VISIBILITY_TIMEOUT


class EvtFaceObserved(event.Event):
    '''Triggered whenever an face is identified by the robot'''
    face = 'The face that was observed'
    updated = 'A set of field names that have changed'
    image_box = 'A comzo.util.ImageBox defining where the face is within Cozmo\'s camera view'
    name = 'The name associated with the face that was observed'


class EnrollNamedFace(action.Action):
    '''Represents the enroll named face action in progress.

    Returned by :meth:`cozmo.faces.Face.name_face`
    '''

    def __init__(self, face, name, **kw):
        super().__init__(**kw)
        #: The face (eg. an instance of :class:`cozmo.faces.Face`) that will be named
        self.face = face
        #: The name that is going to be bound to the face
        self.name = name

    def _repr_values(self):
        return "face=%s name=%s" % (self.face, self.name)

    def _encode(self):
        return _clad_to_engine_iface.EnrollNamedFace(faceID=self.face.face_id, name=self.name)


class Face(event.Dispatcher):
    '''The type for faces in Cozmo's world.'''

    enroll_named_face_factory = EnrollNamedFace

    def __init__(self, conn, world, robot, face_id=None, **kw):
        super().__init__(**kw)
        self._face_id = face_id
        self._robot = robot
        self._name = ''
        self._pose = None
        self.conn = conn
        self.world = world

        #: (float) The time the event was received.
        #: ``None`` if no events have yet been received.
        self.last_event_time = None

        #: (float) The time the face was last observed by the robot.
        #: ``None`` if the face has not yet been observed.
        self.last_observed_time = None

        #: (int) The robot's timestamp of the last observed event.
        #: ``None`` if the face has not yet been observed.
        self.last_observed_robot_timestamp = None

        #: (:class:`~cozmo.util.ImageBox`) The ImageBox defining where the
        #: object was last visible within Cozmo's camera view.
        #: ``None`` if the face has not yet been observed.
        self.last_observed_image_box = None

    def __repr__(self):
        return '<%s face_id=%s is_visible=%s name=%s pose=%s>' % (self.__class__.__name__, self.face_id,
                                                      self.is_visible, self.name, self.pose)

    #### Private Methods ####

    def _update_field(self, changed, field_name, new_value):
        # Set only changed fields and update the passed in changed set
        current = getattr(self, field_name)
        if current != new_value:
            setattr(self, field_name, new_value)
            changed.add(field_name)

    #### Properties ####

    @property
    def face_id(self):
        '''(int) The internal id assigned to the face.

        This value can only be assigned once as it is static in the engine.
        '''
        return self._face_id

    @face_id.setter
    def face_id(self, value):
        if self._face_id is not None:
            raise ValueError("Cannot change face id once set (from %s to %s)" % (self._face_id, value))
        logger.debug("Updated face_id for %s from %s to %s", self.__class__, self._face_id, value)
        self._face_id = value

    @property
    def pose(self):
        ''':class:`cozmo.util.Pose` The pose of the face in the world.'''
        return self._pose

    @property
    def name(self):
        '''(string) The name Cozmo has associated with the face in his memory.

            This string will be empty if the face is not recognized or enrolled.
        '''
        return self._name

    #### Private Event Handlers ####

    def _recv_msg_robot_observed_face(self, evt, *, msg):
        changed_fields = {'last_observed_time', 'last_observed_robot_timestamp',
                'last_event_time', 'last_observed_image_box', 'pose'}
        self._pose = util.Pose(msg.pose.x, msg.pose.y, msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3,
                               origin_id=msg.pose.originID)
        self._name = msg.name
        self.last_observed_time = time.time()
        self.last_observed_robot_timestamp = msg.timestamp
        self.last_event_time = time.time()

        image_box = util.ImageBox(msg.img_topLeft_x, msg.img_topLeft_y, msg.img_width, msg.img_height)
        self.last_observed_image_box = image_box

        self.dispatch_event(EvtFaceObserved, face=self, name=self._name,
                updated=changed_fields, image_box=image_box,)

    #### Public Event Handlers ####

    #### Event Wrappers ####

    #### Commands ####

    def name_face(self, name):
        '''Assign a name to this face. Cozmo will remember this name between SDK runs.

        Args:
            name (string): The name that will be assigned to this face

        Returns:
            An instance of :class:`cozmo.faces.EnrollNamedFace` action object
        '''
        logger.info("Sending enroll named face request for face=%s and name=%s", self, name)
        action = self.enroll_named_face_factory(face=self, name=name,
                    conn=self.conn, robot=self._robot, dispatch_parent=self)
        self._robot._action_dispatcher._send_single_action(action)
        return action

    @property
    def is_visible(self):
        '''(bool) True if the face has been observed recently.

        "recently" is defined as FACE_VISIBILITY_TIMEOUT seconds
        '''
        if self.last_observed_time is None:
            return False
        return (time.time() - self.last_observed_time) < FACE_VISIBILITY_TIMEOUT
