import sys

import pytest

try:
    import anki_vector
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")


@pytest.fixture(scope="module", params=[
    anki_vector.Robot(serial="Local", port="8443"),
    anki_vector.AsyncRobot(serial="Local", port="8443")
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