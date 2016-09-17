__all__ = ['EvtRobotFound', 'CozmoConnection']


import asyncio
import re

from . import logger
from . import anim
from . import clad_protocol
from . import event
from . import exceptions
from . import robot

from . import _clad
from ._clad import _clad_to_engine_cozmo, _clad_to_engine_iface, _clad_to_game_cozmo, _clad_to_game_iface
from ._internal.clad.externalInterface.messageEngineToGame_hash import messageEngineToGameHash as _messageEngineToGameHash
from ._internal.clad.externalInterface.messageGameToEngine_hash import messageGameToEngineHash as _messageGameToEngineHash
from ._internal.build_version import _build_version
from ._internal.sdk_version import _sdk_version


class EvtConnected(event.Event):
    '''Triggered when the initial connection to the device has been established.

    This connection is setup before contacting the robot - Wait for EvtRobotFound
    or EvtRobotReady for a usefully configured Cozmo instance.
    '''
    conn = 'The connected CozmoConnection object'

class EvtRobotFound(event.Event):
    '''Triggered when a Cozmo robot is detected, but before it's initialized.

    :class:`cozmo.robot.EvtRobotReady` is dispatched when the robot is fully initialized.
    '''
    robot = 'The Cozmo object for the robot'

class EvtConnectionClosed(event.Event):
    '''Triggered when the connection to the controlling device is closed.
    '''
    exc = 'The exception that triggered the closure, or None'


_seen = set() # XXX debug tool
def _log_first_seen(tag_name, msg):
    '''Debug utility to log a message the first time one of its tag is received'''
    if tag_name not in _seen:
        _seen.add(tag_name)
        logger.debug("Connection received message %s" % msg)


class CozmoConnection(event.Dispatcher, clad_protocol.CLADProtocol):
    '''Manages the connection to the Cozmo app to communicate with the core engine.'''

    cozmo_factory = robot.Cozmo
    anim_names_factory = anim.AnimationNames

    # overrides for CLADProtocol
    clad_decode_union = _clad_to_game_iface.MessageEngineToGame
    clad_encode_union = _clad_to_engine_iface.MessageGameToEngine


    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        self._is_connected = False
        self._is_ui_connected = False
        self._running = True
        self._robots = {}
        self._primary_robot = None

        #: An anim.AnimationNames object that references all available animation names
        self.anim_names = self.anim_names_factory(self)

    def connection_made(self, transport):
        super().connection_made(transport)
        self._is_connected = True

    def connection_lost(self, exc):
        super().connection_lost(exc)
        self._is_connected = False
        if self._running:
            logger.error("Lost connection to the device: %s" % exc)
            self.dispatch_event(EvtConnectionClosed, exc=exc)
            event._abort_futures(exceptions.ConnectionAborted("Lost connection to the device"))
            self._stop_dispatcher()
            self._running = False

    async def shutdown(self):
        '''Close the connection to the device'''
        if self._running and self._is_connected:
            logger.info("Shutting down connection")
            self._running = False
            event._abort_futures(exceptions.SDKShutdown())
            self._stop_dispatcher()
            self.transport.close()

    def msg_received(self, msg):
        '''Receives low level communication messages from the engine.'''
        tag_name = msg.tag_name

        if tag_name == 'Ping':
            # short circuit to avoid unnecessary event overhead
            return self._handle_ping(msg._data)

        elif tag_name == 'UiDeviceConnected':
            # handle outside of event dispatch for quick abort in case
            # of a version mismatch problem.
            return self._handle_ui_device_connected(msg._data)

        msg = msg._data
        robot_id = getattr(msg, 'robotID', None)
        event_name = '_Msg' +  tag_name

        evttype = getattr(_clad, event_name, None)
        if evttype is None:
            logger.error('Received unknown CLAD message %s', event_name)
            return

        _log_first_seen(tag_name, msg)

        if robot_id:
            # dispatch robot-specific messages to Cozmo robot instances
            return self._process_robot_msg(robot_id, evttype, msg)

        self.dispatch_event(evttype, msg=msg)

    def _process_robot_msg(self, robot_id, evttype, msg):
        if robot_id > 1:
            # One day we might support multiple robots.. if we see a robot_id != 1
            # currently though, it's an error.
            logger.error('INVALID ROBOT_ID SEEN robot_id=%s event=%s msg=%s', robot_id, evttype, msg.__str__())
            robot_id = 1 # XXX remove when errant messages have been fixed

        robot = self._robots.get(robot_id)
        if not robot:
            logger.info('Found robot id=%s', robot_id)
            robot = self.cozmo_factory(self, robot_id, is_primary=self._primary_robot is None)
            self._robots[robot_id] = robot
            if not self._primary_robot:
                self._primary_robot = robot
            # Dispatch an event notifying that a new robot has been found
            # the robot itself will send EvtRobotReady after initialization
            self.dispatch_event(EvtRobotFound, robot=robot)

            # _initialize will set the robot to a known good state in the
            # background and dispatch a EvtRobotReady event when completed.
            robot._initialize()

        robot.dispatch_event(evttype, msg=msg)



    #### Properties ####

    @property
    def is_connected(self):
        '''True if currently connected to a controlling device.'''
        return self._is_connected


    #### Private Event handlers ####

    def _handle_ping(self, msg):
        '''Response to a ping event'''
        if msg.isResponse:
            # To avoid duplication, pings originate from engine, and engine accumulates the latency info from the responses
            logger.error("Only engine should receive responses")
        else:
            resp = _clad_to_engine_iface.Ping(
                counter=msg.counter,
                timeSent_ms=msg.timeSent_ms,
                isResponse=True)
            self.send_msg(resp)

    def _recv_default_handler(self, event, **kw):
        '''Default event handler'''
        if event.event_name.startswith('msg_animation'):
            return self.anim.dispatch_event(event)

        logger.debug('Engine received unhandled event_name=%s  kw=%s', event, kw)

    def _recv_msg_animation_available(self, evt, msg):
        self.anim_names.dispatch_event(evt)

    def _recv_msg_end_of_message(self, evt, *a, **kw):
        self.anim_names.dispatch_event(evt)

    def _compare_clad_hashes(self, msgHash, pyHash, name):
        "Compares the from-engine msgHash with the python side pyHash to verify that CLAD is binary compatible"

        pyHashBytes = pyHash.to_bytes(16, byteorder='little')  # hash is generated on a little endian machine

        if len(pyHashBytes) != len(msgHash):
            logger.error("%s: CLAD Hash length mismatch (%s != %s))", name, len(pyHashBytes), len(msgHash))
            return False

        for i in range(len(pyHashBytes)):
            if (pyHashBytes[i] != msgHash[i]):
                logger.error("%s: CLAD Hash mismatch at %s (%s != %s))",  name, i, pyHashBytes[i], msgHash[i])
                return False

        #logger.info("%s: CLAD Hashes Match (%s == %s))", name, pyHash, msgHash)
        return True

    def _handle_ui_device_connected(self, msg):
        if msg.connectionType != _clad_to_engine_cozmo.UiConnectionType.SdkOverTcp:
            # This isn't for us
            return

        if msg.deviceID != 1:
            logger.error('Unexpected Device Id %s' % msg.deviceID)
            return

        # Verify that engine and SDK are compatible

        clad_hashes_match = self._compare_clad_hashes(msg.toGameCLADHash, _messageEngineToGameHash, "EngineToGame") and\
                            self._compare_clad_hashes(msg.toEngineCLADHash, _messageGameToEngineHash, "GameToEngine")
        build_versions_match = _build_version == msg.buildVersion

        if clad_hashes_match:
            if not build_versions_match:
                logger.warning("Build versions don't match (%s != %s) but CLAD is still compatible" % (_build_version, msg.buildVersion))

            connection_success_msg = _clad_to_engine_iface.UiDeviceConnectionSuccess(
                connectionType=msg.connectionType,
                deviceID=msg.deviceID,
                buildVersion = _build_version + "_" + str(_sdk_version))
            self.send_msg(connection_success_msg)
        else:
            logger.error('Your Python and C++ CLAD versions do not match - connection refused')

            try:
                wrong_version_msg = _clad_to_engine_iface.UiDeviceConnectionWrongVersion(
                    reserved=0,
                    connectionType=msg.connectionType,
                    deviceID = msg.deviceID,
                    buildVersion = _build_version + "_" + str(_sdk_version))

                self.send_msg(wrong_version_msg)
            except AttributeError:
                pass
            exc = exceptions.SDKVersionMismatch("SDK library does not match software running on device")
            self.shutdown()
            raise exc

        self._is_ui_connected = True
        self.dispatch_event(EvtConnected, conn=self)
        self.anim_names.refresh()
        logger.info("UI device connected")

    def _recv_msg_image_chunk(self, evt, *, msg):
        if self._primary_robot:
            self._primary_robot.dispatch_event(evt)


    #### Public Event Handlers ####

    #### Commands ####

    async def wait_for_robot(self, timeout=None):
        '''Wait for a Cozmo robot to connect and complete initialization.

        Args:
            timeout (float): Maximum length of time to wait for a robot to be ready
        Returns:
            A :class:`cozmo.robot.Cozmo` instance that's ready to use.
        '''
        if not self._primary_robot:
            await self.wait_for(EvtRobotFound, timeout=timeout)
        if self._primary_robot.is_ready:
            return self._primary_robot
        await self._primary_robot.wait_for(robot.EvtRobotReady, timeout=timeout)
        return self._primary_robot
