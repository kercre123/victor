# Copyright (c) 2018 Anki, Inc.

'''Face recognition and enrollment.

Vector is capable of recognizing human faces, tracking their position and rotation
("pose") and assigning names to them via an enrollment process.

The :class:`anki_vector.world.World` object keeps track of faces the robot currently
knows about, along with those that are currently visible to the camera.

Each face is assigned a :class:`Face` object, which generates a number of
observable events whenever the face is observed, has its ID updated.
'''

# __all__ should order by constants, event classes, other classes, functions.
__all__ = ['Face', 'Expression', 'FaceComponent']

from enum import Enum
import math
import time

from . import sync, util
from .messaging import protocol

# TODO: Add event classes in to this moodule once event subscription logic changes
# TODO: Move MeetVictor codebase(like erase_all_enrolled_faces,
# erase_enrolled_face_by_id and update_enrolled_face_by_id) in to this
# module


class Expression(Enum):
    '''Facial expressions that Vector can distinguish

    Facial expression not recognized.
    Call :func:`anki_vector.robot.Robot.enable_vision_mode` to enable recognition.
    '''
    UNKNOWN = protocol.FacialExpression.Value("EXPRESSION_UNKNOWN")
    #: Facial expression neutral
    NEUTRAL = protocol.FacialExpression.Value("EXPRESSION_NEUTRAL")
    #: Facial expression happiness
    HAPPINESS = protocol.FacialExpression.Value("EXPRESSION_HAPPINESS")
    #: Facial expression surprise
    SURPRISE = protocol.FacialExpression.Value("EXPRESSION_SURPRISE")
    #: Facial expression anger
    ANGER = protocol.FacialExpression.Value("EXPRESSION_ANGER")
    #: Facial expression sadness
    SADNESS = protocol.FacialExpression.Value("EXPRESSION_SADNESS")


class Face:
    '''A single face that Vector has detected.

    May represent a face that has previously been enrolled, in which case
    :attr:`name` will hold the name that it was enrolled with.

    Each Face instance has a :attr:`face_id` integer - This may change if
    Vector later gets an improved view and makes a different prediction about
    which face it is looking at.
    '''

    def __init__(self, face_id=None):
        self._face_id = face_id
        self._updated_face_id = None
        self._name = ''
        self._expression = None
        self._last_observed_time: int = None
        self._last_observed_robot_timestamp: int = None
        self._pose = None
        self._img_rect = None

        # Individual expression values histogram, sums to 100 (Exception: all
        # zero if expression=Unknown)
        self._expression_score = None

        # Face landmarks
        self._left_eye = None
        self._right_eye = None
        self._nose = None
        self._mouth = None

    def __repr__(self):
        return (f"<{self.__class__.__name__} Face id: {self.face_id} "
                f"Updated face id: {self.updated_face_id} Name: {self.name} "
                f"Expression: {protocol.FacialExpression.Name(self.expression)}>")

    @property
    def face_id(self):
        ''':return: The internal ID assigned to the face.'''
        return self._face_id if self._updated_face_id is None else self._updated_face_id

    @face_id.setter
    def face_id(self, face_id):
        # True if this face been updated / superseded by a face with a new ID
        if self._face_id is not None:
            raise ValueError(f"Cannot change face ID once set (from {self._face_id} to {face_id})")
        self._face_id = face_id

    @property
    def has_updated_face_id(self):
        '''bool: True if this face been updated / superseded by a face with a new ID

        .. code-block:: python

            my_was_face_originally_unrecognized_but_is_now_recognized = face.has_updated_face_id
        '''
        return self._updated_face_id is not None

    @property
    def updated_face_id(self):
        '''int: The ID for the face that superseded this one (if any, otherwise :meth:`face_id`)'''
        if self._updated_face_id:
            return self._updated_face_id
        return self._face_id

    @updated_face_id.setter
    def updated_face_id(self, face_id):
        self._updated_face_id = face_id

    @property
    def name(self):
        '''string: The name Vector has associated with the face in his memory.

        This string will be empty if the face is not recognized or enrolled.
        '''
        return self._name

    @property
    def last_observed_time(self):
        """ float: The time the element was last observed by the robot.
        ``None`` if the element has not yet been observed.

        .. code-block:: python

            my_was_face_originally_unrecognized_but_is_now_recognized = face.has_updated_face_id
        """
        return self._last_observed_time

    @property
    def time_since_last_seen(self):
        """float: time since this element was last seen (math.inf if never)

        .. code-block:: python

            my_last_seen_time = cube.time_since_last_seen
        """
        if self._last_observed_time is None:
            return math.inf
        return time.time() - self._last_observed_time

    @property
    def timestamp(self):
        '''int: Timestamp of event'''
        return self._last_observed_robot_timestamp

    @property
    def pose(self):
        ''':class:`anki_vector.util.Pose`: Position and rotation of the face observed'''
        return self._pose

    @property
    def img_rect(self):
        ''':class:`anki_vector.util.ImageRect`: Position in image coords'''
        return self._img_rect

    @property
    def expression(self):
        '''string: The facial expression Vector has recognized on the face.

        Will be :attr:`Expression.UNKNOWN` by default if you haven't called
        :meth:`anki_vector.robot.Robot.enable_vision_mode` to enable
        the facial expression estimation. Otherwise it will be equal to one of:
        :attr:`Expression.NEUTRAL`, :attr:`Expression.HAPPINESS`,
        :attr:`Expression.SURPRISE`, :attr:`Expression.ANGER`,
        or :attr:`Expression.SADNESS`.
        '''
        return self._expression

    @property
    def expression_score(self):
        '''int: The score/confidence that :attr:`expression` was correct.

        Will be 0 if expression is :attr:`Expression.UNKNOWN` (e.g. if
        :meth:`anki_vector.robot.Robot.enable_vision_mode` wasn't
        called yet). The maximum possible score is 100.
        '''
        return self._expression_score

    @property
    def left_eye(self):
        '''sequence of tuples of float (x,y): points representing the outline of the left eye'''
        return self._left_eye

    @property
    def right_eye(self):
        '''sequence of tuples of float (x,y): points representing the outline of the right eye'''
        return self._right_eye

    @property
    def nose(self):
        '''sequence of tuples of float (x,y): points representing the outline of the nose'''
        return self._nose

    @property
    def mouth(self):
        '''sequence of tuples of float (x,y): points representing the outline of the mouth'''
        return self._mouth

    def unpack_face_stream_data(self, msg):
        '''Unpacks the face observed stream data in to a Face instance'''
        self._face_id = msg.face_id
        self._name = msg.name
        self._last_observed_time = time.time()
        self._last_observed_robot_timestamp = msg.timestamp
        self._pose = util.Pose(x=msg.pose.x, y=msg.pose.y, z=msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3,
                               origin_id=msg.pose.origin_id)
        self._img_rect = util.ImageRect(msg.img_rect.x_top_left,
                                        msg.img_rect.y_top_left,
                                        msg.img_rect.width,
                                        msg.img_rect.height)
        self._expression = msg.expression
        self._expression_score = msg.expression_values
        self._left_eye = msg.left_eye
        self._right_eye = msg.right_eye
        self._nose = msg.nose
        self._mouth = msg.mouth


class FaceComponent(util.Component):
    '''Manage the state of all the photos on the robot'''
    @sync.Synchronizer.wrap
    async def meet_victor(self, name):
        req = protocol.AppIntentRequest(intent='intent_meet_victor',
                                        param=name)
        return await self.interface.AppIntent(req)

    @sync.Synchronizer.wrap
    async def cancel_face_enrollment(self):
        req = protocol.CancelFaceEnrollmentRequest()
        return await self.interface.CancelFaceEnrollment(req)

    @sync.Synchronizer.wrap
    async def request_enrolled_names(self):
        req = protocol.RequestEnrolledNamesRequest()
        return await self.interface.RequestEnrolledNames(req)

    @sync.Synchronizer.wrap
    async def update_enrolled_face_by_id(self, face_id, old_name, new_name):
        '''Update the name enrolled for a given face.

        Args:
            face_id (int): The ID of the face to rename.
            old_name (string): The old name of the face (must be correct, otherwise message is ignored).
            new_name (string): The new name for the face.
        '''
        req = protocol.UpdateEnrolledFaceByIDRequest(faceID=face_id,
                                                     oldName=old_name, newName=new_name)
        return await self.interface.UpdateEnrolledFaceByID(req)

    @sync.Synchronizer.wrap
    async def erase_enrolled_face_by_id(self, face_id):
        '''Erase the enrollment (name) record for the face with this ID.

        Args:
            face_id (int): The ID of the face to erase.
        '''
        req = protocol.EraseEnrolledFaceByIDRequest(faceID=face_id)
        return await self.interface.EraseEnrolledFaceByID(req)

    @sync.Synchronizer.wrap
    async def erase_all_enrolled_faces(self):
        '''Erase the enrollment (name) records for all faces.'''
        req = protocol.EraseAllEnrolledFacesRequest()
        return await self.interface.EraseAllEnrolledFaces(req)

    @sync.Synchronizer.wrap
    async def set_face_to_enroll(self, name, observedID=0, saveID=0, saveToRobot=True, sayName=False, useMusic=False):
        req = protocol.SetFaceToEnrollRequest(name=name,
                                              observedID=observedID,
                                              saveID=saveID,
                                              saveToRobot=saveToRobot,
                                              sayName=sayName,
                                              useMusic=useMusic)
        return await self.interface.SetFaceToEnroll(req)

    @sync.Synchronizer.wrap
    async def enable_vision_mode(self, enable, mode=protocol.VisionMode.Value("VISION_MODE_DETECTING_FACES")):
        '''Edit the vision mode

        Args:
            enable (bool): Enable/Disable the mode specified
            mode (messaging.protocol.VisionMode): Specifies the vision mode to edit
        '''
        enable_vision_mode_request = protocol.EnableVisionModeRequest(mode=mode, enable=enable)
        return await self.interface.EnableVisionMode(enable_vision_mode_request)
