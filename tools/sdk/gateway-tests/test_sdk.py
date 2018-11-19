# This file runs via pytest, and will test the SDK.
#
# This test provides an extensible way to write complicated SDK tests.
#
import concurrent
import os
from pathlib import Path
import sys

import pytest

try:
    import anki_vector
except ImportError:
    base_dir = Path(os.path.dirname(os.path.realpath(__file__))) / ".."
    base_dir = base_dir.resolve()
    sys.exit("\n\nThis script requires you to install the Vector SDK.\n"
             "To do so, please navigate to '{tools_path}' and run '{make}'\n"
             "Next navigate to '{sdk_path}', run '{pip_install}', and run '{configure}'\n"
             "Then try again".format(
                 tools_path=str(base_dir / "scripts"),
                 make="./update_proto.sh",
                 sdk_path=str(base_dir / "vector-sdk"),
                 pip_install="pip install -e .",
                 configure="python3 configure.py",
             ))


serial = os.environ.get("ANKI_ROBOT_SERIAL", default="Local")

robots_with_control_params = [
    pytest.param(anki_vector.Robot(serial=serial), id="sync"),
    pytest.param(anki_vector.AsyncRobot(serial=serial), id="async")
]
robots_without_control_params = [
    pytest.param(anki_vector.Robot(serial=serial, requires_behavior_control=False), id="sync_no_control"),
    pytest.param(anki_vector.AsyncRobot(serial=serial, requires_behavior_control=False), id="async_no_control")
]
all_robots_params = [
    *robots_with_control_params,
    *robots_without_control_params
]

import uuid


@pytest.fixture(
    params=robots_with_control_params
)
def robot(request):
    id_val = uuid.uuid4().hex
    print(f"\t>{id_val} : Get param")
    robot = request.param
    try:
        print(f"\t>{id_val} : start connect")
        robot.connect()
        print(f"\t>{id_val} : end connect, run test")
        yield robot
        print(f"\t>{id_val} : end test no throw")
    finally:
        print(f"\t>{id_val} : end test, start disconnect")
        robot.disconnect()
        print(f"\t>{id_val} : end disconnect")


@pytest.fixture(
    params=robots_without_control_params
)
def robot_no_control(request):
    robot = request.param
    try:
        robot.connect()
        yield robot
    finally:
        robot.disconnect()


@pytest.fixture(
    params=all_robots_params
)
def any_robot(request):
    robot = request.param
    try:
        robot.connect()
        yield robot
    finally:
        robot.disconnect()


def make_sync(response):
    if isinstance(response, concurrent.futures.Future):
        print("Shawn Test Yes")
    else:
        print("Shawn Test No")
    return response.result() if isinstance(response, concurrent.futures.Future) else response


# # Note: sometimes this would hang because of a failure to send from engine to gateway
def test_play_animation(robot):
    response = robot.anim.play_animation("anim_pounce_success_02")
    response = make_sync(response)


def test_load_animation_list(robot):
    response = robot.anim.load_animation_list()
    response = make_sync(response)


def test_battery(any_robot):
    response = any_robot.get_battery_state()
    response = make_sync(response)


def test_version(any_robot):
    response = any_robot.get_version_state()
    response = make_sync(response)


def test_network(any_robot):
    response = any_robot.get_network_state()
    response = make_sync(response)
