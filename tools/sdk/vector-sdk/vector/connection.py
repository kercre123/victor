'''
Management of the connection to and from a Vector Robot

Copyright (c) 2018 Anki, Inc.
'''

from enum import Enum

import inspect
import sys

import aiogrpc

from . import exceptions, sync, util
from .messaging import client, protocol

# TODO Remove this when the proto file is using an enum instead of a number


class SDK_PRIORITY_LEVEL(Enum):
    '''Enum used to specify the priority level that the program requests to run at'''

    # Runs below level "MandatoryPhysicalReactions" and
    # "LowBatteryFindAndGoToHome", and above "TriggerWordDetected".
    SDK_HIGH_PRIORITY = 0


class Connection:
    def __init__(self, robot, name, host, cert_file):
        if cert_file is None:
            raise Exception("Must provide a cert file")
        self._priority = None
        self.name = name
        self.host = host
        self.cert_file = cert_file
        self._robot = robot
        self._interface = None
        self.channel = None
        self.logger = util.get_class_logger(__name__, self)

    @property
    def robot(self):
        return self._robot

    @property
    def interface(self):
        return self._interface

    @property
    def priority(self):
        return self._priority

    def connect(self, timeout=10):
        trusted_certs = None
        with open(self.cert_file, 'rb') as cert:
            trusted_certs = cert.read()
        credentials = aiogrpc.ssl_channel_credentials(root_certificates=trusted_certs)
        self.logger.info(f"Connecting to {self.host} for {self.name} using {self.cert_file}")
        self.channel = aiogrpc.secure_channel(self.host, credentials,
                                              options=(("grpc.ssl_target_name_override", self.name,),))
        self._interface = client.ExternalInterfaceStub(self.channel)

        # # TODO: do this after getting the token (or the token should be available externally already)
        # interface_fns = [x[0] for x in inspect.getmembers(self._interface) if not x[0].startswith("__")]
        # print(interface_fns)

        control = self.request_control(timeout=timeout)
        if isinstance(control, sync.Synchronizer):
            control.wait_for_completed()

    @sync.Synchronizer.wrap
    async def request_control(self, enable=True, timeout=None, priority=SDK_PRIORITY_LEVEL.SDK_HIGH_PRIORITY):
        behavior_control_request = protocol.SDKActivationRequest(slot=priority.value, enable=enable)
        return await self.interface.SDKBehaviorActivation(behavior_control_request, timeout=timeout)

    async def close(self, timeout=10):
        try:
            await self.request_control(enable=False, timeout=timeout)
        except exceptions.VectorException as e:
            raise e
        finally:
            self._priority = None
            if self.channel:
                await self.channel.close()
