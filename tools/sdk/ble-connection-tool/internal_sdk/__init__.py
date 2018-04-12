#!/usr/bin/env python3

import asyncio
import configparser
import logging
import os
import re
import socket
import subprocess
import sys
import time
from enum import Enum

from . import messages, state, util

try:
    from termcolor import colored
except ImportError:
    def colored(msg, color):
        return msg

try:
    import nacl.bindings.crypto_aead as crypto_aead
    import nacl.bindings.crypto_generichash as crypto_generichash
    import nacl.bindings.crypto_kx as crypto_kx
    import nacl.bindings.utils as utils
except ImportError:
    sys.exit(f"{colored('Error', 'red')} : Unable to import crypto_aead.\n"
             "You must install PyNaCl version 1.30+.\n"
             "Use a custom build from github if not released yet.")

from victorclad.externalInterface import messageExternalComms
ext_comms = messageExternalComms.Anki.Victor.ExternalComms
ConnType = ext_comms.RtsConnType

logger = logging.getLogger('internal_sdk')
logger.setLevel(logging.DEBUG)
# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
# create formatter and add it to the handlers
formatter = logging.Formatter('%(message)s')
ch.setFormatter(formatter)
# add the handlers to the logger
logger.addHandler(ch)


class Mode(Enum):
    AP = 1
    NETWORK = 2
    SKIP = 3


class Keys:
    def __init__(self,
                 client_pk,
                 client_sk,
                 server_pk,
                 pin,
                 encryption_key=None,
                 decryption_key=None):
        if client_pk is None:
            raise Exception("Cannot create key. Missing client_pk")
        if client_sk is None:
            raise Exception("Cannot create key. Missing client_sk")
        if server_pk is None:
            raise Exception("Cannot create key. Missing server_pk")
        self.client_pk = client_pk
        self.client_sk = client_sk
        self.server_pk = server_pk
        if encryption_key is None and decryption_key is None:
            if pin is None:
                raise Exception("Cannot create key. Missing pin")
            keys = crypto_kx.crypto_kx_client_session_keys(client_pk,
                                                        client_sk,
                                                        server_pk)

            salt = crypto_generichash.generichash_blake2b_salt_personal
            decryption_key = salt(keys[0], key=pin)
            encryption_key = salt(keys[1], key=pin)
        elif encryption_key is None or decryption_key is None:
            raise Exception("Invalid Keys. One of decryption or encryption is None.")
        self.ekey = encryption_key
        self.dkey = decryption_key
        self._enc_f = crypto_aead.crypto_aead_xchacha20poly1305_ietf_encrypt
        self._dec_f = crypto_aead.crypto_aead_xchacha20poly1305_ietf_decrypt
        self.enonce = None
        self.dnonce = None

    def set_nonces(self, encryption_nonce, decryption_nonce):
        self.enonce = encryption_nonce
        self.dnonce = decryption_nonce

    def encrypt(self, data):
        ctext = self._enc_f(data, aad=None, nonce=self.enonce, key=self.ekey)
        self.enonce = utils.sodium_increment(self.enonce)
        return ctext

    def decrypt(self, data):
        msg = self._dec_f(data, aad=None, nonce=self.dnonce, key=self.dkey)
        self.dnonce = utils.sodium_increment(self.dnonce)
        return msg

    def to_file(self, key, filename='/tmp/pairing_keys.ini'):
        config = configparser.ConfigParser()
        config.read(filename)
        if key not in config.sections():
            config[key] = {}
        config[key]['encrypt'] = ' '.join([str(int(b)) for b in self.ekey])
        config[key]['decrypt'] = ' '.join([str(int(b)) for b in self.dkey])
        config[key]['pk'] = ' '.join([str(int(b)) for b in self.client_pk])
        config[key]['sk'] = ' '.join([str(int(b)) for b in self.client_sk])
        config[key]['server_pk'] = ' '.join([str(int(b)) for b in self.server_pk])
        with open(filename, 'w') as configfile:
            config.write(configfile)

    @classmethod
    def _read_bytes(cls, s):
        return bytes([int(num) for num in s.split(' ')])

    @classmethod
    def from_file(cls, key, filename='/tmp/pairing_keys.ini'):#, encryption_nonce, decryption_nonce):
        config = configparser.ConfigParser()
        config.read(filename)
        if key in config.sections():
            enc = cls._read_bytes(config[key]['encrypt'])
            dec = cls._read_bytes(config[key]['decrypt'])
            pk = cls._read_bytes(config[key]['pk'])
            sk = cls._read_bytes(config[key]['sk'])
            server_pk = cls._read_bytes(config[key]['server_pk'])
            return Keys(pk, sk, server_pk, None, encryption_key=enc, 
                       decryption_key=dec)
        return None


class Connection:
    def __init__(self, ble=None, log_file=None, mode=None,
                 network_name=None, network_password=None,
                 connection_type=ConnType.FirstTimePair,
                 ota_address=None, progress_fn=None, debug=False):
        if debug:
            ch.setLevel(logging.DEBUG)
        if log_file is not None:
            print(f"Writing logs to file: {log_file}")
            fh = logging.FileHandler(log_file, mode='w')
            fh.setLevel(logging.DEBUG)
            f = '(%(levelname)s | %(name)s): %(message)s'
            formatter = util.ColorlessFormatter(f)
            fh.setFormatter(formatter)
            logger.addHandler(fh)
        self.progress_fn=progress_fn
        self.ota_address = ota_address
        if self.ota_address is None:
            url = 'sai-general.s3.amazonaws.com/build-assets'
            file = 'ota-test.tar'
            self.ota_address = f'http://{url}/{file}'
        self.mode = mode
        self.q = asyncio.Queue()
        self.command_handler_map = \
            {
                'exit': self.finish,
                'pin': self.set_pin,
                'ip': self.set_access_point_ip,
                'help': self.command_line_help,
                'ota': self.start_ota
            }
        self.message_handler_map = \
            {
                b'check_for_double_click0': self.require_double_click,
                b'check_for_double_click1': self.recv_double_click
            }
        self.finished = False
        self.state_manager = state.StateManager(self)
        self.process = None
        self.ble_address = self.get_ble() if ble is None else ble.upper()
        self.server_address = "node-ble/uds_socket"
        self.end_token = b'<UNENCRYPTED_END>'
        self.connection_type = connection_type
        self.network_name = network_name
        self.network_password = network_password
        self.clear()

    def get_ble(self):
        return input("Please enter ble name: ").upper()

    def node_log(self):
        msg = self.process.stdout.readline().decode().rstrip()
        logger.debug(colored(msg, 'green'))

    def __enter__(self):
        logger.info("Starting pairing process")
        self.loop = asyncio.get_event_loop()
        self.loop.add_reader(sys.stdin, self.got_stdin_data)
        try:
            os.unlink(self.server_address)
        except OSError:
            if os.path.exists(self.server_address):
                raise
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        command = f'node node-ble/ble-server.js {self.ble_address}'
        self.process = subprocess.Popen(command,
                                        shell=True,
                                        stdout=subprocess.PIPE,
                                        bufsize=0)
        self.loop.add_reader(self.process.stdout, self.node_log)

        time.sleep(1)
        logger.debug(f'Connecting to {self.server_address}')

        self.socket.connect(self.server_address)
        self.loop.add_reader(self.socket, self.recv_message)
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        try:
            self.loop.run_until_complete(self._main())
        except KeyboardInterrupt:
            self.finished = True
        finally:
            self.close()
            if self.process is not None:
                self.process.kill()
            if self.socket is not None:
                self.socket.close()

    async def _main(self):
        try:
            while not self.finished:
                line = await self.q.get()
                if self.expecting_password:
                    self.state_manager.on_event(messages.NetworkPassword(line))
                else:
                    segments = line.split(" ")
                    key = segments[0]
                    remainder = ' '.join(segments[1:])
                    if self.scanned_results is not None and \
                       key in self.scanned_results:
                        self.select_network(key)
                        continue
                    ip_regex = r"^((25[0-5]|2[0-4][0-9]|[01]?" \
                               r"[0-9][0-9]?)\.){3}(25[0-5]|" \
                               r"2[0-4][0-9]|[01]?[0-9][0-9]?)$"
                    if re.match(ip_regex, key):
                        remainder = key
                        key = 'ip'
                    elif re.match(r"[0-9]{6}(?![0-9])", key):
                        remainder = key
                        key = 'pin'

                    if key in self.command_handler_map:
                        self.command_handler_map[key](remainder)
                    else:
                        logger.warning(f"Unknown command '{key}'")
        except RuntimeError as e:
            if str(e) == "Event loop is closed":
                return
            raise e

    def command_line_help(self, *args):
        logger.warning("Help not implemented yet")

    def finish(self, *args):
        self.finished = True

    def set_pin(self, pin, *args):
        self.state_manager.on_event(messages.Pin(pin))

    def set_access_point_ip(self, ip, *args):
        self.state_manager.on_event(messages.IPAddress(ip))

    def start_ota(self, url, *args):
        self.state_manager.on_event(messages.StartOTA(url))

    def recv_handshake(self, *args):
        print(args)
        self.state_manager.on_event(args[0])

    def recv_double_click(self):
        self.state_manager.on_event(messages.DoubleClick(True))

    def require_double_click(self):
        self.state_manager.on_event(messages.DoubleClick(False))

    def clear(self, version=2):
        self.expecting_password = False
        self.is_scanning = False
        self.to_robot_nonce = None
        self.to_device_nonce = None
        self.scanned_results = None
        self.version = version
        self.keys = Keys.from_file(self.ble_address)
        self.server_pk = None
        self.pk, self.sk = None, None
        self.connection_type = ConnType.FirstTimePair
        if self.keys is not None:
            self.server_pk = self.keys.server_pk
            self.pk, self.sk = self.keys.client_pk, self.keys.client_sk
            self.connection_type = ConnType.Reconnection

    def send_handshake(self):
        handshake = messages.Handshake(self.version)
        self.send_message(handshake.as_message())

    def check_double_click(self):
        self.send_message(b'check_for_double_click')

    def send_public_key(self):
        if not self.pk:
            self.pk, self.sk = crypto_kx.crypto_kx_keypair()
        msg = ext_comms.RtsConnResponse(self.connection_type, self.pk)
        msg = util.tagAndPackMessage(msg, self.version)
        self.send_message(msg)

    def got_stdin_data(self):
        asyncio.ensure_future(self.q.put(sys.stdin.readline().rstrip()))

    def send_message(self, message):
        if message != b'check_for_double_click':
            util.log_byte_message('Sending', message, log=logger.debug)
        if self.keys is not None and self.keys.enonce is not None:
            message = self.keys.encrypt(message)
            util.log_byte_message('Encrypt', message, log=logger.debug)
        self.socket.sendall(message + self.end_token)

    def set_server_pk(self, pk):
        self.server_pk = pk

    def store_nonces(self, to_robot_nonce, to_device_nonce):
        self.to_robot_nonce = to_robot_nonce
        self.to_device_nonce = to_device_nonce

    # TODO: handle v2 vs v1
    def set_keys(self, pin):
        self.keys = Keys(self.pk, self.sk,
                         self.server_pk, pin)
        
    def save_keys(self):
        self.keys.to_file(self.ble_address)

    def send_ack(self):
        if getattr(ext_comms, "RtsConnection", None): # TEMP: until factory-0.9 gets merged
            ack_value = ext_comms.RtsConnection.Tag.RtsNonceMessage
        else:
            ack_value = ext_comms.RtsConnection_1.Tag.RtsNonceMessage
        msg = ext_comms.RtsAck(ack_value)
        msg = util.tagAndPackMessage(msg, self.version)
        self.send_message(msg)
        self.keys.set_nonces(self.to_robot_nonce,
                             self.to_device_nonce)

    def challenge_response(self, number):
        msg = ext_comms.RtsChallengeMessage(number + 1)
        msg = util.tagAndPackMessage(msg, self.version)
        self.send_message(msg)

    def scan_wifi(self, repeat=True):
        if self.is_scanning:
            msg = util.tagAndPackMessage(ext_comms.RtsWifiScanRequest(), self.version)
            self.send_message(msg)
            self.loop.call_later(10, self.scan_wifi)

    def scan_results(self, results):
        if self.scanned_results is None:
            self.scanned_results = {}
        for result in results:
            key = ("NULL" if result.wifiSsidHex == "!"
                   else util.from_hex(result.wifiSsidHex)) \
                   if result.wifiSsidHex and result.wifiSsidHex != "hidden" \
                   else "hidden"
            self.scanned_results[key] = result
        return self.scanned_results

    def on_paired(self):
        if self.mode is Mode.NETWORK:
            self.is_scanning = True
            self.scan_wifi()
        elif self.mode is Mode.AP:
            msg = ext_comms.RtsWifiAccessPointRequest(True)
            msg = util.tagAndPackMessage(msg, self.version)
            self.send_message(msg)
        elif self.mode is Mode.SKIP:
            def out_of_body():
                self.state_manager.on_event(messages.StartOTA(self.ota_address))
            self.loop.call_soon(out_of_body)

    def select_network(self, network_name):
        self.is_scanning = False
        self.network_name = network_name

        if self.network_password is None:
            self.expecting_password = True
            logger.debug("Prompting for password")
            print("Enter password: ", end="")
            sys.stdout.flush()
        else:
            self.connect_to_network()

    def set_network_password(self, password):
        self.expecting_password = False
        self.network_password = password

    def connect_to_network(self):
        results = self.scanned_results[self.network_name]
        # App tells the robot to connect to WiFi
        is_hidden = self.network_name == "hidden"
        msg = ext_comms.RtsWifiConnectRequest(results.wifiSsidHex,
                                              self.network_password,
                                              30,  # timeout
                                              results.authType,
                                              is_hidden)
        msg = util.tagAndPackMessage(msg, self.version)
        self.send_message(msg)

    def connect_to_access_point(self, ip):
        url = f'{ip}:9090'
        file = 'ota-test.tar'
        self.ota_address = f'http://{url}/{file}'

    def on_connected(self):
        msg = ext_comms.RtsOtaUpdateRequest(self.ota_address)
        msg = util.tagAndPackMessage(msg, self.version)
        self.send_message(msg)

    def recv_message(self):
        buf_size = 256
        result = None
        result = b''
        while not result.endswith(self.end_token):
            result += self.socket.recv(buf_size)
        result = result[:-len(self.end_token)]
        handshake = messages.Handshake.from_buffer(result)
        if handshake is not None:
            self.recv_handshake(handshake)
        elif result in self.message_handler_map:
            self.message_handler_map[result]()
        else:
            if self.keys is not None and self.keys.dnonce is not None:
                util.log_byte_message('Encrypted', result, log=logger.debug)
                result = self.keys.decrypt(result)
                util.log_byte_message('Decrypted', result, log=logger.debug)
            unpacked = ext_comms.ExternalComms.unpack(result)
            
            if getattr(ext_comms, "RtsConnection", None): # TEMP: until factory-0.9 gets merged
                rts_conn = unpacked.RtsConnection
            else:
                try:
                    rts_conn = unpacked.RtsConnection_1
                except TypeError:
                    rts_conn = unpacked.RtsConnection.RtsConnection_2
            self.state_manager.on_event(rts_conn)

    def get_ota_status(self, value):
        values_dict = {
            1: "UNKNOWN",
            2: "IN_PROGRESS",
            3: "COMPLETED",
            4: "REBOOTING",
            5: "ERROR"
        }
        if value in values_dict:
            return values_dict[value]
        return "UNDEFINED"

    def close(self):
        self.finished = True
        self.loop.close()
