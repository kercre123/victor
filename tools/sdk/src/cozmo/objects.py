# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

__all__ = ['EvtObjectTapped', 'EvtObjectObserved', 'EvtObjectConnectChanged',
    'ObservableObject', 'LightCube', 'CustomObject', 'FixedCustomObject']


import collections
import math
import time

from . import logger

from . import action
from . import event
from . import lights
from . import util

from ._clad import _clad_to_engine_iface, _clad_to_game_cozmo, _clad_to_engine_cozmo


# Length of time to go without receiving an observed event before
# assuming that Cozmo can no longer see an object.
OBJECT_VISIBILITY_TIMEOUT = 0.2


class EvtObjectObserved(event.Event):
    '''Triggered whenever an object is identified by the robot'''
    obj = 'The object that was observed'
    updated = 'A set of field names that have changed'
    image_box = 'A comzo.util.ImageBox defining where the object is within Cozmo\'s camera view'

class EvtObjectTapped(event.Event):
    'Triggered when an active object is tapped.'
    obj = 'The object that was tapped'
    tap_count = 'Number of taps detected'
    tap_duration = 'The duration of the tap in ms'

class EvtObjectConnectChanged(event.Event):
    'Triggered when an active object has connected or disconnected from the robot.'
    obj = 'The object that connected or disconnected'
    connected = 'True if the object connected, False if it disconnected'



class ObservableObject(event.Dispatcher):
    '''The base type for objects in Cozmo's world.'''

    #: (bool) Whether this type of object can be picked up by Cozmo
    pickupable = False
    place_objects_on_this = False

    def __init__(self, conn, world, object_id=None, **kw):
        super().__init__(**kw)
        self._object_id = object_id
        self._pose = None
        self.conn = conn
        self.world = world
        self._robot = None # the robot controlling this object (if an active object)

        #: (float) The time the last event was received.
        #: ``None`` if no events have yet been received.
        self.last_event_time = None

        #: (float) The time the object was last observed by the robot.
        #: ``None`` if the object has not yet been observed.
        self.last_observed_time = None

        #: (int) The robot's timestamp of the last observed event.
        #: ``None`` if the object has not yet been observed.
        self.last_observed_robot_timestamp = None

        #: (:class:`~cozmo.util.ImageBox`) The ImageBox defining where the
        #: object was last visible within Cozmo's camera view.
        #: ``None`` if the object has not yet been observed.
        self.last_observed_image_box = None


    def __repr__(self):
        extra = self._repr_values()
        if len(extra) > 0:
            extra = ' '+extra

        return '<%s is_visible=%s pose=%s%s>' % (self.__class__.__name__,
                self.is_visible, self.pose, extra)

    def _repr_values(self):
        return ''

    #### Private Methods ####

    def _update_field(self, changed, field_name, new_value):
        # Set only changed fields and update the passed in changed set
        current = getattr(self, field_name)
        if current != new_value:
            setattr(self, field_name, new_value)
            changed.add(field_name)


    #### Properties ####

    @property
    def object_id(self):
        '''(int) The internal id assigned to the object.

        This value can only be assigned once as it is static in the engine.
        '''
        return self._object_id

    @object_id.setter
    def object_id(self, value):
        if self._object_id is not None:
            raise ValueError("Cannot change object id once set (from %s to %s)" % (self._object_id, value))
        logger.debug("Updated object_id for %s from %s to %s", self.__class__, self._object_id, value)
        self._object_id = value

    @property
    def pose(self):
        '''(:class:`cozmo.util.Pose`) The pose of this object in the world.'''
        return self._pose

    @property
    def time_since_last_seen(self):
        '''float: time since this face was last seen (math.inf if never)'''
        if self.last_observed_time is None:
            return float(math.inf)
        return time.time() - self.last_observed_time

    @property
    def is_visible(self):
        '''(bool) True if the object has been observed recently.

        "recently" is defined as OBJECT_VISIBILITY_TIMEOUT seconds.
        '''
        if self.last_observed_time is None:
            return False
        return (time.time() - self.last_observed_time) < OBJECT_VISIBILITY_TIMEOUT


    #### Private Event Handlers ####

    def _recv_msg_object_connection_state(self, _, *, msg):
        if self.connected != msg.connected:
            self.connected = msg.connected
            self.dispatch_event(EvtObjectConnectChanged, obj=self, connected=self.connected)

    def _recv_msg_robot_observed_object(self, evt, *, msg):
        changed_fields = {'last_observed_time', 'last_observed_robot_timestamp',
                'last_observed_image_box', 'last_event_time', 'pose'}
        self._pose = util.Pose(msg.pose.x, msg.pose.y, msg.pose.z,
                               q0=msg.pose.q0, q1=msg.pose.q1,
                               q2=msg.pose.q2, q3=msg.pose.q3,
                               origin_id=msg.pose.originID)
        self.last_observed_time = time.time()
        self.last_observed_robot_timestamp = msg.timestamp
        self.last_event_time = time.time()

        image_box = util.ImageBox(msg.img_topLeft_x, msg.img_topLeft_y, msg.img_width, msg.img_height)
        self.last_observed_image_box = image_box

        self.dispatch_event(EvtObjectObserved, obj=self,
                updated=changed_fields, image_box=image_box)

    #### Public Event Handlers ####

    #### Event Wrappers ####

    #### Commands ####


LightCube1Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE1
LightCube2Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE2
LightCube3Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE3


class LightCube(ObservableObject):
    '''A light cube object has four LEDs that Cozmo can actively manipulate and communicate with.'''
    #TODO investigate why the top marker orientation of a cube is a bit strange

    pickupable = True
    place_objects_on_this = True

    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)

        #: (float) The time the object was last tapped
        self.last_tapped_time = None

        #: (int) The robot's timestamp of the last tapped event
        self.last_tapped_robot_timestamp = None


    #### Private Methods ####

    def _set_light(self, msg, idx, light):
        if not isinstance(light, lights.Light):
            raise TypeError("Expected a lights.Light")
        msg.onColor[idx] = light.on_color.int_color
        msg.offColor[idx] = light.off_color.int_color
        msg.onPeriod_ms[idx] = light.on_period_ms
        msg.offPeriod_ms[idx] = light.off_period_ms
        msg.transitionOnPeriod_ms[idx] = light.transition_on_period_ms
        msg.transitionOffPeriod_ms[idx] = light.transition_off_period_ms


    #### Event Wrappers ####

    async def wait_for_tap(self, timeout=None):
        '''Wait for the object to receive a tap event.

        Args:
            timeout (float): Maximum time to wait for a tap, in seconds.  None for indefinite
        Returns:
            A :class:`EvtObjectTapped` object if a tap was received.
        '''
        return await self.wait_for(EvtObjectTapped, timeout=timeout)


    #### Properties ####

    #### Private Event Handlers ####
    def _recv_msg_object_tapped(self, evt, *, msg):
        changed_fields = {'last_event_time', 'last_tapped_time',
            'last_tapped_robot_timestamp'}
        self.last_event_time = time.time()
        self.last_tapped_time = time.time()
        self.last_tapped_robot_timestamp = msg.timestamp
        self.dispatch_event(EvtObjectTapped, obj=self,
            tap_count=msg.numTaps, tap_duration=msg.tapTime)


    #### Public Event Handlers ####

    def recv_evt_object_tapped(self, evt, **kw):
        pass

    #### Commands ####

    # TODO: make this explicit as to which light goes to which corner.
    def set_light_corners(self, light1, light2, light3, light4):
        """Set the light for each corner"""
        msg = _clad_to_engine_iface.SetAllActiveObjectLEDs(
                objectID=self.object_id, robotID=self._robot.robot_id)
        for i, light in enumerate( (light1, light2, light3, light4) ):
            if light is not None:
                lights._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_lights(self, light):
        '''Set all lights on the cube

        Args:
            light (`class:`cozmo.lights.Light`): The settings for the lights.
        '''
        msg = _clad_to_engine_iface.SetAllActiveObjectLEDs(
                objectID=self.object_id, robotID=self._robot.robot_id)
        for i in range(4):
            lights._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_lights_off(self):
        '''Turn off all the lights on the cube.'''
        self.set_lights(lights.off_light)



class CustomObject(ObservableObject):
    '''An object defined by the SDK. It is bound to a specific objectType i.e Custom_STAR5_Box.

    This defined object is given a size in the x,y and z axis. The dimensions of the markers on the
    object are also defined. We get an :class:`cozmo.objects.EvtObjectObserved` message when the robot sees these markers.
    '''

    def __init__(self, conn, world, object_type,
                 x_size_mm, y_size_mm, z_size_mm,
                 marker_width_mm, marker_height_mm, **kw):
        super().__init__(conn, world, **kw)

        self.object_type = object_type
        self._x_size_mm = x_size_mm
        self._y_size_mm = y_size_mm
        self._z_size_mm = z_size_mm
        self._marker_width_mm = marker_width_mm
        self._marker_height_mm = marker_height_mm


    def _repr_values(self):
        return ('object_type={self.object_type} '
                'x_size_mm={self.x_size_mm:.1f} '
                'y_size_mm={self.y_size_mm:.1f} '
                'z_size_mm={self.z_size_mm:.1f} '.format(self=self))

    #### Private Methods ####

    #### Event Wrappers ####
    #### Properties ####
    @property
    def x_size_mm(self):
        '''Size of this object in its X axis, in millimeters.'''
        return self._x_size_mm

    @property
    def y_size_mm(self):
        '''Size of this object in its Y axis, in millimeters.'''
        return self._y_size_mm

    @property
    def z_size_mm(self):
        '''Size of this object in its Z axis, in millimeters.'''
        return self._z_size_mm

    @property
    def marker_width_mm(self):
        '''Width in millimeters of the marker on this object.'''
        return self._marker_width_mm

    @property
    def marker_height_mm(self):
        '''Height in millimeters of the marker on this object.'''
        return self._marker_height_mm


    #### Private Event Handlers ####

    #### Public Event Handlers ####

    #### Commands ####


class CustomObjectTypes:
    '''Defines all available object types.

    For use with :meth:`cozmo.world.World.define_custom_object`.
    '''

_CustomObjectType = collections.namedtuple('_CustomObjectType', 'name id')

for (_name, _id) in _clad_to_engine_cozmo.ObjectType.__dict__.items():
    if not _name.startswith('_') and _name.startswith('Custom_') and _id > 0:
        # only index CustomObjects
        setattr(CustomObjectTypes, _name, _CustomObjectType(_name, _id))


class FixedCustomObject():
    '''A fixed object defined by the SDK. It is given a pose and x,y,z sizes.

    This object cannot be observed by the robot so its pose never changes.
    Their position is static in Cozmo's world view, once instantiated, these objects
    never move. This could be used to make Cozmo aware of objects and know to path around them
    even when they don't have any markers.
    '''

    is_visible = False

    def __init__(self, pose, x_size_mm, y_size_mm, z_size_mm, object_id, *a, **kw):
        super().__init__(*a, **kw)
        self._pose = pose
        self._object_id = object_id
        self._x_size_mm = x_size_mm
        self._y_size_mm = y_size_mm
        self._z_size_mm = z_size_mm

    def __repr__(self):
        return ('<%s pose=%s object_id=%d x_size_mm=%.1f y_size_mm=%.1f z_size_mm=%.1f=>' %
                                        (self.__class__.__name__, self.pose, self.object_id,
                                         self.x_size_mm, self.y_size_mm, self.z_size_mm))

    #### Private Methods ####
    #### Event Wrappers ####
    #### Properties ####
    @property
    def object_id(self):
        '''(int) The internal id assigned to the object.

        This value can only be assigned once as it is static in the engine.
        '''
        return self._object_id

    @object_id.setter
    def object_id(self, value):
        if self._object_id is not None:
            raise ValueError("Cannot change object id once set (from %s to %s)" % (self._object_id, value))
        logger.debug("Updated object_id for %s from %s to %s", self.__class__, self._object_id, value)
        self._object_id = value

    @property
    def pose(self):
        '''(:class:`cozmo.util.Pose`) The pose of the object in the world.'''
        return self._pose

    @property
    def x_size_mm(self):
        '''The length of the object in its X axis, in millimeters.'''
        return self._x_size_mm

    @property
    def y_size_mm(self):
        '''The length of the object in its Y axis, in millimeters.'''
        return self._y_size_mm

    @property
    def z_size_mm(self):
        '''The length of the object in its Z axis, in millimeters.'''
        return self._z_size_mm


    #### Private Event Handlers ####
    #### Public Event Handlers ####
    #### Commands ####
