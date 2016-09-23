# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''
Behaviors represent a task that the Cozmo robot may perform for an
indefinite amount of time.

For example, the "LookAround" behavior causes Cozmo to start exploring
the space around him, which will cause events such as
:class:`cozmo.objects.EvtObjectObserved` to be generated as he comes across
objects.

Behaviors must be explicitly stopped before having the robot do something else
(eg. to pickup the object he just observed).

Behaviors are started by a call to :meth:`cozmo.robot.Cozmo.start_behavior`,
which returns a :class:`Behavior` object.  Calling the :meth:`~Behavior.stop`
method on that object terminate the behavior.

The :class:`BehaviorTypes` class in this module holds a list of all available
behaviors.
'''

__all__ = ['EvtBehaviorStarted', 'EvtBehaviorStopped',
           'Behavior', 'BehaviorTypes']

import asyncio
import collections

from . import logger

from . import event

from ._clad import _clad_to_engine_iface, _clad_to_engine_cozmo


class EvtBehaviorStarted(event.Event):
    '''Triggered when a behavior starts'''
    behavior = 'The Behavior object'
    behavior_type_name = 'The behavior type name - equivalent to behavior.type.name'

class EvtBehaviorStopped(event.Event):
    '''Triggered when a behavior stops'''
    behavior = 'The behavior type object'
    behavior_type_name = 'The behavior type name - equivalent to behavior.type.name'



class Behavior(event.Dispatcher):
    '''A Behavior describes a behavior the robot is currently performing.

    Returned by :meth:`cozmo.robot.Robot.start_behavior`.
    '''

    def __init__(self, robot, behavior_type, is_active=False, **kw):
        super().__init__(**kw)
        self.robot = robot
        self.type = behavior_type
        self._is_active = is_active
        if is_active:
            self.dispatch_event(EvtBehaviorStarted, behavior=self, behavior_type_name=self.type.name)

    def __repr__(self):
        return '<%s type="%s">' % (self.__class__.__name__, self.type.name)

    def stop(self):
        if not self._is_active:
            return
        msg = _clad_to_engine_iface.ExecuteBehavior(
                behaviorType=_clad_to_engine_cozmo.BehaviorType.NoneBehavior)
        self.robot.conn.send_msg(msg)
        self._is_active = False
        self.dispatch_event(EvtBehaviorStopped, behavior=self, behavior_type_name=self.type.name)

    @property
    def is_active(self):
        '''Return True if the behavior is currently active on the robot.'''
        return self._is_active



class BehaviorTypes:
    '''Defines all available robot behaviors.

    For use with :meth:`cozmo.robot.Robot.start_behavior`.
    '''


_BehaviorType = collections.namedtuple('_BehaviorType', 'name id')

for (_name, _id) in _clad_to_engine_cozmo.BehaviorType.__dict__.items():
    if not _name.startswith('_') and _id > 0:
        # don't index NoneBehavior
        setattr(BehaviorTypes, _name, _BehaviorType(_name, _id))
