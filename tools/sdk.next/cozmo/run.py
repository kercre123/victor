__all__ = ['connect', 'connect_async', 'setup_basic_logging']

import threading

import asyncio
import concurrent.futures
import functools
import logging
import os
import queue
import sys

from . import logger, logger_protocol

from . import clad_protocol
from . import base
from . import conn

def exception_handler(loop, context):
    loop.default_exception_handler(context)
    loop.stop()

def connect_async(f=None, loop=None, block=True, conn_factory=conn.CozmoConnection,
        stop_on_exception=True, q=None):
    """Conects to the Cozmo Engine on the mobile device and executes an asynchronous function.

    Accepts a function, f, that is given a :class:`cozmo.CozmoConnection` object as
    a parameter.  This function is expected to operate asynchronously.  That is,
    it may register event handles and use Python's await keyword to wait for
    events to complete.

    For simple programs that don't need to handle events happening in the background,
    try the connect() call instead.

    Args:
        f (callable): The coroutine function to execute
        loop (:class:`asyncio.BaseEventLoop`): An explicit event loop to use;
            None causes a default loop to be used.
        block (bool): If True then this function will not return until the callable, f, completes.
        conn_factory (callable): Override the factory function to generate a
            :class:`cozmo.conn.CozmoConnection` (or subclass) instance.
        stop_on_exception (bool): If True then the event loop will exit if an
            unhandled exception is thrown.  If false then the exception will just
            be logged.
        q: Internal helper parameter.
    """

    try:
        if loop is None:
            loop = asyncio.get_event_loop()

        coz_conn = conn_factory(loop=loop)

        async def connect():
            await loop.create_connection(lambda: coz_conn, '127.0.0.1', 5106)
            # wait until the connected CLAD message is received from the engine.
            await coz_conn.wait_for(conn.EvtConnected)

        # will throw an exception on connection failure
        loop.run_until_complete(connect())
    except Exception as e:
        if q:
            q.put(e)
            return
        raise

    if q:
        q.put(coz_conn)

    # connected
    if stop_on_exception:
        loop.set_exception_handler(exception_handler)

    if block:
        if f:
            loop.run_until_complete(f(coz_conn))
        else:
            loop.run_forever()
    else:
        loop.ensure_future(f(coz_conn))


class SyncConn(conn.CozmoConnection):
    """A subclass of CozmoConnection that can be used synchronously from another thread.
    """
    _is_sync = True


def _sync_exception_handler(abort_future, loop, context):
    loop.default_exception_handler(context)
    exception = context.get('exception')
    if exception is not None:
        abort_future.set_exception(context['exception'])
    else:
        abort_future.set_exception(RuntimeError(context['message']))


def connect(f, stop_on_exception=True):
    """Connects to the Cozmo Engine on the mobile device.

    Accepts a function, f, that is given a cozmo.CozmoConnection object as
    a parameter.  This function is expected to operate synchronously.  That is,
    it may not use Python's "await" or coroutine syntax to interact with
    the SDK, nor may it register asynchronous handlers.

    For more complex asynchronous code, use connect_async instead.

    Args:
        f (callable): A callable (typically a function or method) to execute
            when a connection to the device is ready.  The callable must
            accept a :class:`cozmo.conn.CozmoConnection` argument
        stop_on_exception (bool): If True then any exception generated in the
            underlying system will be propogated to f to cause the program to
            exit.
    """
    # Create a new event loop and start it running on its own thread
    # Set its conn_factory to produce SyncConn objects, which will automatically
    # be wrapped into base._SyncProxy objects by metaclass voodoo.

    loop = asyncio.new_event_loop()
    abort_future = concurrent.futures.Future()
    loop.set_exception_handler(functools.partial(_sync_exception_handler, abort_future))
    def loop_watcher():
        try:
            connect_async(loop=loop, stop_on_exception=False, conn_factory=functools.partial(SyncConn, _sync_abort_future=abort_future), q=q)
        except Exception as e:
            abort_future.set_exception(e)

    q = queue.Queue()
    thread = threading.Thread(target=loop_watcher)
    thread.start()

    coz_conn = q.get(timeout=15)
    if isinstance(coz_conn, Exception):
        raise(coz_conn)

    try:
        print("START FROM THREAD", threading.get_ident())
        f(base._SyncProxy(coz_conn))
    finally:
        loop.stop()
        thread.join()


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
            value will be read from the COZMO_PROTOCOL_LOG_LEVEL enviroment
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

