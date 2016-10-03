# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''
Actions encapsulate specific high-level tasks that the Cozmo robot can perform.
They have a definite beginning and end.

These tasks include picking up an object, rotating in place, saying text, etc.

Actions are usually triggered by a call to a method on the
:class:`cozmo.robot.Robot` class such as :meth:`~cozmo.robot.Robot.turn_in_place`

The call will return an object that subclasses :class:`Action` that can be
used to cancel the action, or be observed to wait or be notified when the
action completes (or fails) by calling its
:meth:`~cozmo.event.Dispatcher.wait_for` or
:meth:`~cozmo.event.Dispatcher.add_event_handler` methods.


Warning:
    Only one action can be active at a time.  Attempting to trigger another
    action while one is already in progress will result in a
    :class:`~cozmo.exceptions.RobotBusy` exception being raised.
'''

__all__ = ['EvtActionCompleted', 'Action']


from . import logger

from . import event
from . import exceptions
from ._clad import _clad_to_engine_iface, _clad_to_game_cozmo


ACTION_IDLE = 'action_idle'
ACTION_RUNNING = 'action_running'
ACTION_SUCCEEDED = 'action_succeeded'
ACTION_FAILED = 'action_failed'

_VALID_STATES = {ACTION_IDLE, ACTION_RUNNING, ACTION_SUCCEEDED, ACTION_FAILED}


class EvtActionStarted(event.Event):
    '''Triggered when a robot starts an action.'''
    action = "The action that started"

class EvtActionCompleted(event.Event):
    '''Triggered when a robot action has completed or failed.'''
    action = "The action that completed"
    state = 'The state of the action; either cozmo.action.ACTION_SUCCEEDED or cozmo.action.ACTION_FAILED'
    failure_code = 'A failure code such as "cancelled"'
    failure_reason = 'A human-readable failure reason'


class Action(event.Dispatcher):
    """An action holds the state of an in-progress robot action
    """

    def __init__(self, *, conn, robot, **kw):
        super().__init__(**kw)
        #self._action_id = action_id
        self.conn = conn
        self.robot = robot
        self._state = ACTION_IDLE
        self._failure_code = None
        self._failure_reason = None

    def __repr__(self):
        extra = self._repr_values()
        if len(extra) > 0:
            extra = ' '+extra
        return '<%s state=%s%s>' % (self.__class__.__name__, self.state, extra)

    def _repr_values(self):
        return ''

    def _encode(self):
        raise NotImplementedError()

    def _start(self):
        self._state = ACTION_RUNNING
        self.dispatch_event(EvtActionStarted, action=self)

    def _set_completed(self, msg):
        self._state = ACTION_SUCCEEDED
        self._dispatch_completed_event(msg)

    def _dispatch_completed_event(self, msg):
        # Override to extra action-specific data from msg and generate
        # an action-specific completion event.  Do not call super if overriden.
        # Must generate a subclass of EvtActionCompleted.
        self.dispatch_event(EvtActionCompleted,
                action=self, state=self._state)

    def _set_failed(self, code, reason):
        self._state = ACTION_FAILED
        self._failure_code = code
        self._failure_reason = reason
        self.dispatch_event(EvtActionCompleted,
            action=self, state=self._state, failure_code=code, failure_reason=reason)

    #### Properties ####

    @property
    def is_running(self):
        return self._state == ACTION_RUNNING

    @property
    def is_completed(self):
        '''True if the action has completed (either succeeded or failed).'''
        return self._state in (ACTION_SUCCEEDED, ACTION_FAILED)

    @property
    def failure_reason(self):
        '''Returns a tuple of (failure_code, failure_reason)'''
        return (self._failure_code, self._failure_reason)

    @property
    def state(self):
        return self._state

    #### Private Event Handlers ####

    def _recv_msg_robot_completed_action(self, evt, *, msg):
        result = msg.result
        types = _clad_to_game_cozmo.ActionResult
        if result == types.SUCCESS:
            # dispatch to the specific type to extract result info
            self._set_completed(msg)

        elif result == types.RUNNING:
            # XXX what does one do with this? it seems to occur after a cancel request!
            logger.warn('Received "running" action notification for action=%s', self)
            self._set_failed('running', 'The action was still running')

        elif result == types.FAILURE_NOT_STARTED:
            # not sure we'll see this?
            self._set_failed('not_started', 'The action was not started')

        elif result == types.FAILURE_TIMEOUT:
            self._set_failed('timeout', 'The action timed out')

        elif result == types.FAILURE_PROCEED:
            self._set_failed('proceed', 'Action completed but failed to find object')

        elif result == types.FAILURE_RETRY:
            self._set_failed('retry', 'Retry the event')

        elif result == types.FAILURE_ABORT:
            self._set_failed('aborted', 'Reached maximum retries for action')

        elif result == types.FAILURE_TRACKS_LOCKED:
            self._set_failed('tracks_locked', 'Action failed due to tracks locked')

        elif result == types.FAILURE_BAD_TAG:
            # guessing this is bad
            self._set_failed('bad_tag', 'Action failed due to bad tag')
            logger.error("Received FAILURE_BAD_TAG for action %s", self)

        elif result == types.CANCELLED:
            self._set_failed('cancelled', 'Action was cancelled')

        elif result == types.INTERRUPTED:
            self._set_failed('interrupted', 'Action was interrupted')

        else:
            self._set_failed('unknown', 'Action failed with unknown reason')
            logger.error('Received unknown action result status %s', msg)


    #### Public Event Handlers ####


    #### Commands ####

    def abort(self):
        if self._state != ACTION_RUNNING:
            raise ValueError("Action isn't currently running")

        logger.info('Sending abort request for action=%s', self)
        msg = _clad_to_engine_iface.CancelAction(
            actionType=self._action_type, robotID=self.robot.robot_id)
        self.conn.send_msg(msg)


    async def wait_for_completed(self, timeout=None):
        """Waits for the action to complete
        """
        return await self.wait_for(EvtActionCompleted, timeout=timeout)

    def on_completed(self, handler):
        """Triggers a handler when the action completes."""
        return self.add_event_handler(EvtActionCompleted, handler)



class _ActionDispatcher(event.Dispatcher):
    _next_action_id = _clad_to_game_cozmo.ActionConstants.FIRST_SDK_TAG

    def __init__(self, robot, **kw):
        super().__init__(**kw)
        self.robot = robot
        self._in_progress = {}

    def _get_next_action_id(self):
        # Post increment _current_action_id (and loop within the SDK_TAG range)
        next_action_id = self.__class__._next_action_id
        if self.__class__._next_action_id == _clad_to_game_cozmo.ActionConstants.LAST_SDK_TAG:
            self.__class__._next_action_id = _clad_to_game_cozmo.ActionConstants.FIRST_SDK_TAG
        else:
            self.__class__._next_action_id += 1
        return next_action_id

    def _send_single_action(self, action, position=0, num_retries=0):
        action_id = self._get_next_action_id()
        action.robot = self.robot

        if len(self._in_progress) > 0:
            action = list(self._in_progress.values())[0]
            raise exceptions.RobotBusy('Robot is already performing action %s' % action)

        if action.is_running:
            raise ValueError('Action is already running')

        if action.is_completed:
            raise ValueError('Action already ran')

        qmsg = _clad_to_engine_iface.QueueSingleAction(
            robotID=self.robot.robot_id, idTag=action_id, numRetries=num_retries,
            position=position, action=_clad_to_engine_iface.RobotActionUnion())
        action_msg = action._encode()
        cls_name = action_msg.__class__.__name__
        # For some reason, the RobotActionUnion type uses properties with a lowercase
        # first character, instead of uppercase like all the other unions
        cls_name = cls_name[0].lower() + cls_name[1:]
        setattr(qmsg.action, cls_name, action_msg)
        self.robot.conn.send_msg(qmsg)
        self._in_progress[action_id] = action
        action._start()

    def _is_sdk_action_id(self, action_id):
        return (action_id >= _clad_to_game_cozmo.ActionConstants.FIRST_SDK_TAG) and (action_id <= _clad_to_game_cozmo.ActionConstants.LAST_SDK_TAG)

    def _is_engine_action_id(self, action_id):
        return (action_id >= _clad_to_game_cozmo.ActionConstants.FIRST_ENGINE_TAG) and (action_id <= _clad_to_game_cozmo.ActionConstants.LAST_ENGINE_TAG)

    def _is_game_action_id(self, action_id):
        return (action_id >= _clad_to_game_cozmo.ActionConstants.FIRST_GAME_TAG) and (action_id <= _clad_to_game_cozmo.ActionConstants.LAST_GAME_TAG)

    def _action_id_type(self, action_id):
        if self._is_sdk_action_id(action_id):
            return "sdk"
        elif self._is_engine_action_id(action_id):
            return "engine"
        elif self._is_game_action_id(action_id):
            return "game"
        else:
            return "unknown"

    def _recv_msg_robot_completed_action(self, evt, *, msg):
        action_id = msg.idTag
        is_sdk_action = self._is_sdk_action_id(action_id)
        action = self._in_progress.get(action_id)
        if action is None:
            if is_sdk_action:
                logger.error('Received completed action message for unknown SDK action_id=%s', action_id)
            return
        else:
            if not is_sdk_action:
                action_id_type = self._action_id_type(action_id)
                logger.error('Received completed action message for sdk-known %s action_id=%s', action_id_type, action_id)
        del self._in_progress[action_id]
        # XXX This should generate a real event, not a msg
        # Should also dispatch to self so the parent can be notified.
        action.dispatch_event(evt)
