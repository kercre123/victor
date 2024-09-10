import time

import pytest

import anki_vector
from . import robot  # pylint: disable=unused-import


@pytest.mark.parametrize("color,duration", [
    pytest.param([255, 128, 0], 1.0, id="yellow"),
    pytest.param([128, 0, 255], 1.0, id="purple"),
    pytest.param([0, 0, 0], 1.0, id="black"),
    pytest.param([255, 255, 255], 1.0, id="white"),
])
def test_set_screen_to_color(robot, color, duration):
    robot.screen.set_screen_to_color(anki_vector.color.Color(rgb=color), duration_sec=duration)
    time.sleep(duration)
