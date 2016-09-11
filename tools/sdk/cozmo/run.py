__all__ = ['connect',  'connect_on_loop', 'setup_basic_logging']

import threading

import asyncio
import concurrent.futures
import functools
import logging
import os
import queue
import signal
import sys

from . import logger, logger_protocol

from . import clad_protocol
from . import base
from . import conn



def _sync_exception_handler(abort_future, loop, context):
    loop.default_exception_handler(context)
    exception = context.get('exception')
    if exception is not None:
        abort_future.set_exception(context['exception'])
    else:
        abort_future.set_exception(RuntimeError(context['message']))


class _LoopThread:
    def __init__(self, loop, f=None, conn_factory=conn.CozmoConnection, abort_future=None):
        self.loop = loop
        self.f = f
        if not abort_future:
            abort_future = concurrent.futures.Future()
        self.abort_future = abort_future
        self.conn_factory = conn_factory
        self.thread = None

    def start(self):
        q = queue.Queue()
        abort_future = concurrent.futures.Future()
        def run_loop():
            asyncio.set_event_loop(self.loop)
            try:
                coz_conn = connect_on_loop(self.loop, self.conn_factory)
                q.put(coz_conn)
            except Exception as e:
                self.abort_future.set_exception(e)
                q.put(e)
                return

            if self.f:
                self.loop.run_until_complete(self.f(coz_conn))
            else:
                self.loop.run_forever()

        self.thread = threading.Thread(target=run_loop)
        self.thread.start()

        coz_conn = q.get(10)
        if coz_conn is None:
            raise TimeoutError("Timed out waiting for connection to device")
        if isinstance(coz_conn, Exception):
            raise coz_conn
        return coz_conn

    async def _stop(self):
        self.loop.call_soon(lambda: self.loop.stop())

    def stop(self):
        asyncio.run_coroutine_threadsafe(self._stop(), self.loop).result()
        self.thread.join()


def _connect_async(f, conn_factory=conn.CozmoConnection):
    loop = asyncio.new_event_loop()
    coz_conn = connect_on_loop(loop, conn_factory)
    try:
        loop.run_until_complete(f(coz_conn))
    finally:
        loop.run_until_complete(coz_conn.shutdown())
        loop.stop()
        loop.run_forever()


def _connect_sync(f, conn_factory=conn.CozmoConnection):
    loop = asyncio.new_event_loop()
    abort_future = concurrent.futures.Future()
    conn_factory = functools.partial(conn_factory, _sync_abort_future=abort_future)
    lt = _LoopThread(loop, conn_factory=conn_factory, abort_future=abort_future)
    loop.set_exception_handler(functools.partial(_sync_exception_handler, abort_future))

    coz_conn = lt.start()

    def _sighandler(signum, frame):
        abort_future.set_exception(Exception("SIGINT"))

    async def _abort(exc):
        coz_conn.abort(exc)

    orgsig = signal.signal(signal.SIGINT, _sighandler)

    try:
        f(base._SyncProxy(coz_conn))
    except Exception as exc:
        asyncio.run_coroutine_threadsafe(_abort(exc), loop).result()
        lt.stop()
        raise
    else:
        asyncio.run_coroutine_threadsafe(coz_conn.shutdown(), loop).result()
        lt.stop()
    finally:
        signal.signal(signal.SIGINT, orgsig)


def connect_on_loop(loop, conn_factory=conn.CozmoConnection):
    '''Uses the supplied event loop to connect to a device.

    Will run the event loop in the current thread until the
    connection succeeds or fails.

    If you do not want/need to manage your own loop, then use the
    :func:`connect` function to handle setup/teardown and execute
    a user supplied function.

    Args:
        loop (:class:`asyncio.BaseEventLoop`): The event loop to use to
            connect to Cozmo.
        conn_factory (callable): Override the factory function to generate a
            :class:`cozmo.conn.CozmoConnection` (or subclass) instance.

    Returns:
        A :class:`cozmo.conn.CozmoConnection` instance.
    '''
    async def connect():
        await loop.create_connection(lambda: coz_conn, '127.0.0.1', 5106)
        # wait until the connected CLAD message is received from the engine.
        await coz_conn.wait_for(conn.EvtConnected)

    coz_conn = conn_factory(loop=loop)
    loop.run_until_complete(connect())
    return coz_conn


def connect(f, conn_factory=conn.CozmoConnection):
    '''Connects to the Cozmo Engine on the mobile device and executes an asynchronous function.

    Accepts a function, f, that is given a :class:`cozmo.CozmoConnection` object as
    a parameter.

    The supplied function may be either an asynchronous coroutine function
    (normally defined using "async def") or a regular synchronous function.

    If an asynchronous function is supplied it will be run on the same thread
    as the Cozmo event loop and must use the "await" keyword to yield control
    back to the loop.

    If a synchronous function is supplied then it will run on the main thread
    and Cozmo's event loop will run on a separate thread.  Calls to
    asynchronous methods returned from CozmoConnection will automatically
    be translated to synchronous ones.

    The connect function will return once the supplied function has completed,
    as which time it will terminate the connection to the robot.

    Args:
        f (callable): The function to execute
        conn_factory (callable): Override the factory function to generate a
            :class:`cozmo.conn.CozmoConnection` (or subclass) instance.
    '''
    if asyncio.iscoroutinefunction(f):
        return _connect_async(f, conn_factory)
    return _connect_sync(f, conn_factory)


def setup_basic_logging(general_log_level=None, protocol_log_level=None,
        protocol_log_messages=clad_protocol.LOG_ALL, target=sys.stderr):
    '''Helper to perform basic setup of the Python logging machinery.

    The SDK defines two loggers:

    * :data:`logger` ("cozmo.general") - For general purpose information
      about events within the SDK
    * :data:`logger_protocol` ("cozmo.protocol") - For low level
      communication messages between the device and the SDK.

    Generally only :data:`logger` is interesting.

    Args:
        general_log_level (str): 'DEBUG', 'INFO', 'WARN', 'ERROR' or an equivalent
            constant from the :mod:`logging` module.  If None then a
            value will be read from the COZMO_LOG_LEVEL environment variable.
        protocol_log_level (str): as general_log_level.  If None then a
            value will be read from the COZMO_PROTOCOL_LOG_LEVEL environment
            variable.
        protocol_log_messages (list): The low level messages that should be
            logged to the protocol log.  Defaults to all.  Will read from
            the COMZO_PROTOCOL_LOG_MESSAGES if available which should be
            a comma separated list of message names (case sensitive).
        target (object): The stream to send the log data to; defaults to stderr
    '''
    if general_log_level is None:
        general_log_level = os.environ.get('COZMO_LOG_LEVEL', logging.INFO)
    if protocol_log_level is None:
        protocol_log_level = os.environ.get('COZMO_PROTOCOL_LOG_LEVEL', logging.INFO)
    if protocol_log_level:
        if 'COMZO_PROTOCOL_LOG_MESSAGES' in os.environ:
            lm = os.environ['COMZO_PROTOCOL_LOG_MESSAGES']
            if lm.lower() == 'all':
                clad_protocol.CLADProtocol._clad_log_which = clad_protocol.LOG_ALL
            else:
                clad_protocol.CLADProtocol._clad_log_which = set(lm.split(','))
        else:
            clad_protocol.CLADProtocol._clad_log_which = protocol_log_messages

    h = logging.StreamHandler(stream=target)
    f = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
    h.setFormatter(f)
    logger.addHandler(h)
    logger. setLevel(general_log_level)
    if protocol_log_level is not None:
        logger_protocol.addHandler(h)
        logger_protocol.setLevel(protocol_log_level)

