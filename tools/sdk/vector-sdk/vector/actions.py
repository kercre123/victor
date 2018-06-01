# Copyright (c) 2018 Anki, Inc.

'''
Actions are used to make functions waitable outside of the
scope of the event loop.
'''

__all__ = ['Action', '_as_actionable']

import logging
import grpc

MODULE_LOGGER = logging.getLogger(__name__)

class Action:
    def __init__(self, loop, remove_pending, func, *args, **kwargs):
        self.remove_pending = remove_pending
        self.loop = loop
        self.task = self.loop.create_task(func(*args, **kwargs))

    def wait_for_completed(self):
        try:
            return self.loop.run_until_complete(self.task)
        except grpc._channel._Rendezvous as e:
            raise e
        finally:
            self.remove_pending(self)
        return None

def _as_actionable(func):
    def waitable(*args, **kwargs):
        that = args[0]
        if that.is_async:
            action = Action(that.loop, that.remove_pending, func, *args, **kwargs)
            that.add_pending(action)
            return action
        return that.loop.run_until_complete(func(*args, **kwargs))
    return waitable
