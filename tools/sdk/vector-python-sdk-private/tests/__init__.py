# This file runs via pytest, and will test the SDK.
#
# This test provides an extensible way to write complicated SDK tests.
#
import concurrent
from distutils.version import LooseVersion
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
                 tools_path=str(base_dir),
                 make="./update.py",
                 sdk_path=str(base_dir / "sdk"),
                 pip_install="pip install -e .",
                 configure="python3 -m anki_vector.configure",
             ))


serial = os.environ.get("ANKI_ROBOT_SERIAL", default="Local")

def version_check(greater, lesser):
    return LooseVersion(greater) > LooseVersion(lesser)


def version_lesser_than(version):
    return version_check(version, anki_vector.version.__version__)


cache_animation_lists_name = "cache_animation_lists"
if version_lesser_than("0.5.2"):
    cache_animation_lists_name = "cache_animation_list"
cache_animation_lists = {cache_animation_lists_name: False}

if version_lesser_than("0.5.2"):
    behavior_control_param = {"requires_behavior_control": False}
else:
    behavior_control_param = {"behavior_control_level": None}

robots_with_control_params = [
    pytest.param(anki_vector.Robot(serial=serial, **cache_animation_lists), id="sync"),
    pytest.param(anki_vector.AsyncRobot(serial=serial, **cache_animation_lists), id="async")
]
robots_without_control_params = [
    pytest.param(anki_vector.Robot(serial=serial, **cache_animation_lists, **behavior_control_param), id="sync_no_control"),
    pytest.param(anki_vector.AsyncRobot(serial=serial, **cache_animation_lists, **behavior_control_param), id="async_no_control")
]
all_robots_params = [
    *robots_with_control_params,
    *robots_without_control_params,
]


def general_robot(request):
    robot = request.param
    try:
        robot.connect()
        yield robot
    finally:
        robot.disconnect()


@pytest.fixture(
    params=robots_with_control_params
)
def robot(request):
    for r in general_robot(request):
        yield r


@pytest.fixture(
    params=robots_without_control_params
)
def robot_no_control(request):
    for r in general_robot(request):
        yield r


@pytest.fixture(
    params=all_robots_params
)
def any_robot(request):
    for r in general_robot(request):
        yield r


def make_sync(response, assert_not_none=True):
    response = response.result() if isinstance(response, concurrent.futures.Future) else response
    if assert_not_none:
        assert response is not None, "Received no response"
    return response


def version_greater_than(version):
    return version_check(anki_vector.version.__version__, version)


def skip_before(version):
    def func(fn):
        return pytest.mark.skipif(version_lesser_than(version),
                                  reason=f"Requires SDK {version} or higher")(fn)
    return func
