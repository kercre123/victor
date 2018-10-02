# This test file runs via pytest, and will test the json endpoints for all of the streaming rpcs.
#
# This file reproduces a connection that is similar to the way the app will talk to Vector.
#
import json
import random
import string
import sys

import pytest

try:
    from google.protobuf.json_format import MessageToJson, Parse
    import anki_vector.messaging.protocol as p
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")

from util import vector_connection

try:
    # Non-critical import to add color output
    from termcolor import colored
except ImportError:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

def test_event_forever(vector_connection):
    if "test_json_streaming.py::test_event_forever" not in sys.argv:
        pytest.skip("test_event_forever will only run when explicitly called")
    vector_connection.send("v1/event_stream", p.EventRequest(), p.EventResponse(), stream=-1)

def random_id_generator(count, length):
    for _ in range(count):
        yield ''.join(random.choice(string.ascii_lowercase + string.digits) for _ in range(random.randint(1, length)))

def test_event_repeated(vector_connection):
    def callback(response, response_type, iterations=10):
        i = 0
        for i, r in enumerate(response.iter_lines()):
            data = json.loads(r.decode('utf8'))
            print("Stream response: {}".format(colored(json.dumps(data, indent=4, sort_keys=True), "cyan")))
            assert "result" in data
            Parse(json.dumps(data["result"]), response_type, ignore_unknown_fields=True)
            print("Converted Protobuf: {}".format(colored(response_type, "cyan")))
            assert response_type.event.connection_response.is_primary
            break

    for id in random_id_generator(10, 50):
        vector_connection.send("v1/event_stream", p.EventRequest(connection_id=id), p.EventResponse(), stream=1, callback=callback)

@pytest.mark.parametrize("request_params", [
    {},
    {"white_list": p.FilterList(list=["wake_word"])},
    {"black_list": p.FilterList(list=["robot_state", "robot_observed_face"])},
])
def test_event(vector_connection, request_params):
    vector_connection.send("v1/event_stream", p.EventRequest(**request_params), p.EventResponse(), stream=25)

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
