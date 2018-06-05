# Copyright (c) 2018 Anki, Inc.

'''
Event handler used to make functions subscribe to robot events.
'''

__all__ = ['EventHandler']

import asyncio
from concurrent.futures import CancelledError
from grpc._channel import _Rendezvous
import logging

from . import util
from .messaging import external_interface_pb2 as protocol
from .messaging import external_interface_pb2_grpc as client

MODULE_LOGGER = logging.getLogger(__name__)

class EventHandler:
    def __init__(self, loop):
        self.logger = util.get_class_logger(__name__, self)
        self._loop = loop
        self._connection = None
        self.listening_for_events = False
        self.event_task = None
        self.subscribers = {}

    def start(self, connection):
        self._connection = connection
        self.listening_for_events = True
        self.event_task = self._loop.create_task(self._handle_events())

    def close(self):
        self.listening_for_events = False
        self.event_task.cancel()
        self._loop.run_until_complete(self.event_task)

    async def _handle_events(self):
        try:
            req = protocol.EventRequest()
            async for e in self._connection.EventStream(req):
                if not self.listening_for_events:
                    break
                event_type = e.event.WhichOneof("event_type")
                if event_type in self.subscribers.keys():
                    for func in self.subscribers[event_type]:
                        func(event_type, getattr(e.event, event_type))
        except CancelledError:
            self.logger.debug('Event handler task was cancelled. This is expected during disconnection.')

    def subscribe(self, event_type, func):
        if event_type not in self.subscribers.keys():
            self.subscribers[event_type] = set()
        self.subscribers[event_type].add(func)

    def unsubscribe(self, event_type, func):
        if event_type in self.subscribers.keys():
            event_subscribers = self.subscribers[event_type]
            if func in event_subscribers:
                event_subscribers.remove(func)
                if not event_subscribers:
                    del self.subscribers[event_type]
            else:
                self.logger.error("The function '{}' is not subscribed to '{}'".format(func.__name__, event_type))
        else:
            self.logger.error("Cannot unsubscribe from event_type '{}'. "
                              "It has no subscribers.".format(event_type))
