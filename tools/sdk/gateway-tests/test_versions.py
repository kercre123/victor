# This test file runs via pytest, and will test the json endpoints for all of the unary rpcs.
#
# This file reproduces a connection that is similar to the way the app will talk to Vector.
#

import pytest
import time

from interface_version import v1_0_0

from util import vector_connection, json_from_file

def test_protocol_version(vector_connection, protocol):
    vector_connection.send("v1/protocol_version", v1_0_0.ProtocolVersionRequest(), v1_0_0.ProtocolVersionResponse())

def test_battery_state(vector_connection, protocol):
    vector_connection.send("v1/battery_state", v1_0_0.BatteryStateRequest(), v1_0_0.BatteryStateResponse())
