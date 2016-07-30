__all__ = ['World']


from . import logger

from . import event
from . import objects

from . import _clad
from ._clad import _clad_to_engine_iface, _clad_to_game_cozmo


class World(event.Dispatcher):
    '''Represents the state of the world, as known to a Cozmo robot.'''

    #: Factory to generate LightCube objects.
    light_cube_factory = objects.LightCube

    def __init__(self, conn, robot, **kw):
        super().__init__(**kw)
        self.conn = conn
        self.robot = robot
        self.light_cubes = {}
        self._objects = {}
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


    #### Properties ####

    @property
    def active_behavior(self):
        return self._active_behavior

    @property
    def active_action(self):
        return self._active_action


    #### Private Event Handlers ####

    def _recv_msg_robot_observed_object(self, evt, *, msg):
        obj = self._objects.get(msg.objectID)
        if not obj:
            obj = self._allocate_object_from_msg(msg)
        if obj:
            obj.dispatch_event(evt)

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


    #### Event Wrappers ####

    async def wait_for_observed_light_cube(self, timeout=None, visible_markers=False):
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


    #### Commands ####

    def send_available_objects(self):
        # XXX description for this?
        msg = _clad_to_engine_iface.SendAvailableObjects(
                robotID=self.robot.robot_id, enable=True)
        self.robot.conn.send_msg(msg)

    async def _delete_all_objects(self):
        # XXX marked this as private as apparently problematic to call
        # currently as it deletes light cubes too.
        msg = _clad_to_engine_iface.DeleteAllObjects(robotID=self.robot.robot_id)
        self.robot.conn.send_msg(msg)
        await self.wait_for(_clad._MsgRobotDeletedAllObjects)
        # TODO: reset local object state

    async def delete_all_custom_objects(self):
        """Causes the robot to forget about all custom objects it currently knows about."""
        msg = _clad_to_engine_iface.DeleteAllCustomObjects(robotID=self.robot.robot_id)
        self.robot.conn.send_msg(msg)
        # TODO: use a filter to wait only for a message for the active robot
        await self.wait_for(_clad._MsgRobotDeletedAllCustomObjects)
        # TODO: reset local object stte
