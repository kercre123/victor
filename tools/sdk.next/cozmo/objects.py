__all__ = ['EvtObjectTapped', 'EvtObjectObserved', 'EvtObjectConnectChanged',
    'BaseObject', 'LightCube']


import asyncio

from . import logger

from . import action
from . import event
from . import lights
from . import util

from ._clad import _clad_to_engine_iface, _clad_to_game_cozmo


class EvtObjectObserved(event.Event):
    '''Triggered whenever an object is identified by the robot'''
    obj = 'The object that was observed'
    updated = 'A set of field names that have changed'
    image_box = 'A comzo.util.ImageBox defining where the object is within Cozmo\'s camera view'
    markers_visible = 'True if the robot can actually see the markers on the object right now.'

class EvtObjectTapped(event.Event):
    'Triggered when an active object is tapped.'
    obj = 'The object that was tapped'
    tap_count = 'Number of taps detected'
    tap_duration = 'The duration of the tap in ms'

class EvtObjectConnectChanged(event.Event):
    'Triggered when an active object has connected or disconnected from the robot.'
    obj = 'The object that connected or disconnected'
    connected = 'True if the object connected, False if it disconnected'




class BaseObject(event.Dispatcher):
    '''The base type for objects in Cozmo's world.'''

    #: (bool) Whether this type of object can be picked up by Cozmo
    pickupable = False

    def __init__(self, conn, world, object_id=None, **kw):
        super().__init__(**kw)
        self._object_id = object_id
        self.conn = conn
        self.world = world
        self._robot = None # the robot controlling this object (if an active object)

        #: (float) The time the event was received
        self.last_event_time = None
        #: (float) The time the object was last observed by the robot
        self.last_observed_time = None
        #: (float) The time the object was last tapped
        self.last_tapped_time = None

    def __repr__(self):
        return '<%s object_id=%s>' % (self.__class__.__name__, self.object_id)


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
        '''The internal id assigned to the object.'''
        return self._object_id

    @object_id.setter
    def object_id(self, value):
        if self._object_id is not None:
            raise ValueError("Cannot change object id once set (from %s to %s)" % (self._object_id, value))
        logger.debug("Updated object_id for %s from %s to %s", self.__class__, self._object_id, value)
        self._object_id = value


    #### Private Event Handlers ####

    def _recv_msg_object_connection_state(self, _, *, msg):
        if self.connected != msg.connected:
            self.connected = msg.connected
            self.dispatch_event(EvtObjectConnectChanged, obj=self, connected=self.connected)

    def _recv_msg_robot_observed_object(self, evt, *, msg):
        if not msg.markersVisible:
            # Don't trigger a user-visible update if the markers haven't been
            # confirmed visible to avoid false positives
            return
        changed_fields = {'last_observed_time', 'last_event_time'}
        self.last_observed_time = msg.timestamp
        self.last_event_time = msg.timestamp
        image_box = util.ImageBox(msg.img_topLeft_x, msg.img_topLeft_y, msg.img_width, msg.img_height)
        self.dispatch_event(EvtObjectObserved, obj=self,
                markers_visible=msg.markersVisible,
                updated=changed_fields, image_box=image_box)

    def _recv_msg_object_tapped(self, evt, *, msg):
        changed_fields = {'last_event_time', 'last_tapped_time'}
        self.last_event_time = msg.timestamp
        self.last_tapped_time = msg.timestamp
        self.dispatch_event(EvtObjectTapped, obj=self, tap_count=msg.numTaps, tap_duration=msg.tapTime)


    #### Public Event Handlers ####

    def recv_evt_object_tapped(self, evt, **kw):
        pass


    #### Event Wrappers ####

    #### Commands ####


LightCube1Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE1
LightCube2Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE2
LightCube3Id = _clad_to_game_cozmo.ObjectType.Block_LIGHTCUBE3


class LightCube(BaseObject):
    '''A light cube object has four LEDs that Cozmo can actively manipulate and communicate with.'''

    pickupable = True

    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)

    def __repr__(self):
        return '<%s object_id=%s>' % (self.__class__.__name__, self._object_id)

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
    #### Public Event Handlers ####

    #### Commands ####

    # TODO: make this explicit as to which light goes to which corner.
    def set_light_corners(self, light1, light2, light3, light4):
        """Set the light for each corner"""
        msg = _clad_to_engine_iface.SetAllActiveObjectLEDs(
                objectID=self.object_id, robotID=self._robot.robot_id)
        for i, light in enumerate( (light1, light2, light3, light4) ):
            if light is not None:
                self._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_lights(self, light):
        '''Set all lights on the cube

        Args:
            light (`class:`cozmo.lights.Light`): The settings for the lights.
        '''
        msg = _clad_to_engine_iface.SetAllActiveObjectLEDs(
                objectID=self.object_id, robotID=self._robot.robot_id)
        for i in range(4):
            self._set_light(msg, i, light)

        self.conn.send_msg(msg)

    def set_lights_off(self):
        '''Turn off all the lights on the cube.'''
        self.set_lights(lights.off_light)
