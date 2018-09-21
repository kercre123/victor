# This file runs via pytest, and will test the SDK.
#
# This test provides an extensible way to write complicated SDK tests.
#
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
                 tools_path=str(base_dir / "grpc_tools"),
                 make="make",
                 sdk_path=str(base_dir / "vector-sdk"),
                 pip_install="pip install -e .",
                 configure="python3 configure.py",
             ))

serial = os.environ.get("ANKI_ROBOT_SERIAL")
if serial is None or len(serial) == 0:
    @pytest.fixture(scope="module", params=[
        anki_vector.Robot(serial="Local", port="8443"),
        anki_vector.AsyncRobot(serial="Local", port="8443")
    ])
    def robot(request):
        return request.param
else:
    @pytest.fixture(scope="module", params=[
        anki_vector.Robot(serial=serial),
        anki_vector.AsyncRobot(serial=serial)
    ])
    def robot(request):
        return request.param

@pytest.yield_fixture(autouse=True)
def wrap(robot):
    try:
        robot.connect()
        yield
    finally:
        robot.disconnect()

def synchronize(response):
    return response.wait_for_completed() if isinstance(response, anki_vector.sync.Synchronizer) else response

# Note: sometimes this would hang because of a failure to send from engine to gateway
def test_play_animation(robot):
    response = robot.anim.play_animation("anim_blackjack_victorwin_01")
    response = synchronize(response)

def test_load_animation_list(robot):
    response = robot.anim.load_animation_list()
    response = synchronize(response)