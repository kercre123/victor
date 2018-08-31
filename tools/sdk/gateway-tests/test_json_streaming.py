import sys

import pytest

try:
    import anki_vector.messaging.protocol as p
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")

from util import vector_connection

def test_event_forever(vector_connection):
    if "test_json_streaming.py::test_event_forever" not in sys.argv:
        pytest.skip("test_event_forever may only run when explicitly called")
    vector_connection.send("v1/event_stream", p.EventRequest(), p.EventResponse(), stream=-1)

def test_event(vector_connection):
    vector_connection.send("v1/event_stream", p.EventRequest(), p.EventResponse(), stream=100)

@pytest.mark.parametrize("request_params", [
    {"control_request": p.ControlRequest(priority=p.ControlRequest.UNKNOWN)},
    {"control_request": p.ControlRequest(priority=p.ControlRequest.OVERRIDE_ALL)},
    {"control_request": p.ControlRequest(priority=p.ControlRequest.TOP_PRIORITY_AI)},
    {"control_request": p.ControlRequest()},
    {"control_release": p.ControlRelease()}
])
def test_assume_behavior_control(vector_connection, request_params):
    vector_connection.send("v1/assume_behavior_control", p.BehaviorControlRequest(**request_params), p.BehaviorControlResponse(), stream=10)

def test_assume_behavior_control_nil(vector_connection):
    with pytest.raises(AssertionError):
        vector_connection.send("v1/assume_behavior_control", p.BehaviorControlRequest(), p.BehaviorControlResponse(), stream=10)

def test_camera_feed(vector_connection):
    vector_connection.send("v1/camera_feed", p.CameraFeedRequest(), p.CameraFeedResponse(), stream=100)
