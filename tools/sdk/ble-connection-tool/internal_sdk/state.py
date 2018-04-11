#!/usr/bin/env python3

import logging
import sys

from . import messages, util

try:
    from termcolor import colored
except ImportError:
    def colored(msg, color):
        return msg

from victorclad.externalInterface import messageExternalComms
ext_comms = messageExternalComms.Anki.Victor.ExternalComms
ConnType = ext_comms.RtsConnType

logger = logging.getLogger('internal_sdk.state')


class State:
    def __init__(self, conn):
        self.conn = conn
        logger.debug(f"state: {colored(self.__class__.__name__, 'magenta')}")
        self.on_enter()

    def handshake(self, handshake):
        self.conn.clear(handshake.version)
        return Discovered(self.conn)

    def on_enter(self):
        raise NotImplementedError()

    def on_event(self, event):
        raise NotImplementedError()


class InitialState(State):
    def on_enter(self):
        pass

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        return self


class Discovered(State):
    def on_enter(self):
        self.conn.send_handshake()

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if util.is_rts_of(event, ext_comms.RtsConnRequest):
            logger.debug(f"Found: {event}")
            self.conn.set_server_pk(bytes(event.RtsConnRequest.publicKey))
            return AwaitingPairingMode(self.conn)
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class AwaitingPairingMode(State):
    def on_enter(self):
        self.first_time = True
        self.conn.check_double_click()

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if isinstance(event, messages.DoubleClick):
            if event.clicked or self.conn.connection_type == ConnType.Reconnection:
                return StartPairing(self.conn)
            else:
                if self.first_time:
                    logger.info("Please double click the robot...")
                    self.first_time = False
                self.conn.check_double_click()
                return self
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class StartPairing(State):
    def on_enter(self):
        self.conn.send_public_key()

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if isinstance(event, messages.Pin):
            logger.debug(f"Found: {event}")
            self.conn.set_keys(bytes(event.pin, 'ascii'))
            return SecurePairing(self.conn)
        if util.is_rts_of(event, ext_comms.RtsNonceMessage):
            logger.debug(f"Found: {event}")
            self.conn.store_nonces(bytes(event.RtsNonceMessage.toRobotNonce),
                                 bytes(event.RtsNonceMessage.toDeviceNonce))
            if self.conn.connection_type == ConnType.FirstTimePair:
                logger.debug("Requesting pin entry")
                print("Please enter the pin: ", end='')
                sys.stdout.flush()
                return self
            return SecurePairing(self.conn)
            
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class SecurePairing(State):
    def on_enter(self):
        self.conn.send_ack()

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if util.is_rts_of(event, ext_comms.RtsChallengeMessage):
            logger.debug(f"Found: {event}")
            self.conn.challenge_response(event.RtsChallengeMessage.number)
            return self
        if util.is_rts_of(event, ext_comms.RtsChallengeSuccessMessage):
            self.conn.save_keys()
            logger.debug(f"Found: {event}")
            return Paired(self.conn)
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class Paired(State):
    def on_enter(self):
        self.conn.on_paired()

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if isinstance(event, messages.NetworkPassword):
            self.conn.set_network_password(event.password)
            self.conn.connect_to_network()
            return self
        if isinstance(event, messages.IPAddress):
            self.conn.connect_to_access_point(event.ip)
            file = colored('ota-test.tar', 'cyan')
            port = colored('9090', 'cyan')
            logger.info(f"Please server a file named '{file}' "
                        f"on your machine using a server on port {port}.")
            return Connected(self.conn)
        if isinstance(event, messages.StartOTA):
            self.conn.ota_address = event.url
            return Connected(self.conn)
        if util.is_rts_of(event, ext_comms.RtsWifiScanResponse):
            r = event.RtsWifiScanResponse.scanResult
            results = self.conn.scan_results(r)
            if results:
                if self.conn.network_name is None:
                    logger.info("Select a network:")
                    for key, _ in results.items():
                        logger.info(f"> {key}")
                elif self.conn.network_name in results:
                    self.conn.select_network(self.conn.network_name)
                else:
                    status = "Still scanning... "\
                             f"Current results: {results.keys()}"
                    logger.debug(status)
            return self
        if util.is_rts_of(event, ext_comms.RtsWifiConnectResponse):
            response = event.RtsWifiConnectResponse
            if response.wifiState == 1:
                name = util.from_hex(response.wifiSsidHex)
                logger.debug(f"Connected to {colored(name, 'green')}!")
                return Connected(self.conn)
            warning = colored('Warning', 'yellow')
            logger.warning(f"{warning} : failed to connect to network")
            return self
        if util.is_rts_of(event, ext_comms.RtsWifiAccessPointResponse):
            r = event.RtsWifiAccessPointResponse
            if r.enabled:
                ssid = r.ssid
                password = r.password
                logger.info(f"Connect to {ssid} with password '{password}'")
                print("Enter ip: ", end="")
            else:
                logger.error("Failed to enable access point")
            return self
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class Connected(State):
    def on_enter(self):
        self.conn.on_connected()

    def show_progress(self, status, current, total):
        s = colored(status, 'green')
        percent = int(10000 * current / total) / 100.0
        logger.debug(f"status: {status}, current: {current}, total: {total}")
        print(f"{s}: {percent:.2}%   {current:10} / {total:10}    ", end='\r')

    def on_event(self, event):
        if isinstance(event, messages.Handshake):
            return self.handshake(event)
        if isinstance(event, messages.StartOTA):
            self.conn.ota_address = event.url
            return Connected(self.conn)
        if util.is_rts_of(event, ext_comms.RtsOtaUpdateResponse):
            response = event.RtsOtaUpdateResponse
            status = self.conn.get_ota_status(response.status)
            if status == "UNKNOWN":
                logger.warning("OTA update unknown or pending...")
            elif status == "UNDEFINED" or status == "ERROR":
                err = colored('ERROR', 'red')
                logger.error(f"{err}: there was an problem with the update")
            else:
                if self.conn.progress_fn is not None:
                    self.conn.progress_fn(status,
                                          response.current,
                                          response.expected)
                self.show_progress(status,
                                   response.current,
                                   response.expected)
            return self
        logger.error(f"Unhandled: {self.__class__.__name__} {event}")
        return self


class StateManager:
    def __init__(self, conn):
        self.state = InitialState(conn)

    def on_event(self, event):
        event_log = f"Event received: {colored(event, 'magenta')}"
        # Hide these ultra spammy logs
        if "DoubleClick(clicked=bool(False))" not in event_log:
            logger.debug(event_log)
        self.state = self.state.on_event(event)
