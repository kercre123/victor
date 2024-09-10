import time

import pytest

from anki_vector import (Robot, AsyncRobot)
from . import serial, skip_before  # pylint: disable=unused-import


@skip_before("0.5.2")
@pytest.mark.parametrize("viewer_robot", [
    pytest.param(Robot(serial), id="sync"),
    pytest.param(AsyncRobot(serial), id="async"),
])
def test_camera_viewer(viewer_robot):
    try:
        viewer_robot.connect()
        try:
            viewer_robot.viewer.show()
            time.sleep(5)
        finally:
            viewer_robot.viewer.close()
    finally:
        viewer_robot.disconnect()


@pytest.mark.parametrize("viewer_robot", [
    pytest.param(Robot(serial, enable_nav_map_feed=True), id="sync"),
    pytest.param(AsyncRobot(serial, enable_nav_map_feed=True), id="async"),
])
def test_opengl_viewer(viewer_robot):
    try:
        viewer_robot.connect()
        try:
            viewer_robot.viewer_3d.show()
            time.sleep(5)
        finally:
            viewer_robot.viewer_3d.close()
    finally:
        viewer_robot.disconnect()


@pytest.mark.parametrize("viewer_robot", [
    pytest.param(Robot(serial, show_viewer=True), id="sync-camera"),
    pytest.param(AsyncRobot(serial, show_viewer=True), id="async-camera"),
    pytest.param(Robot(serial, show_3d_viewer=True), id="sync-3d"),
    pytest.param(AsyncRobot(serial, show_3d_viewer=True), id="async-3d"),
    pytest.param(Robot(serial, show_viewer=True, show_3d_viewer=True), id="sync-camera+3d"),
    pytest.param(AsyncRobot(serial, show_viewer=True, show_3d_viewer=True), id="async-camera+3d"),
])
def test_show_viewers(viewer_robot):
    try:
        viewer_robot.connect()
        time.sleep(5)
    finally:
        viewer_robot.disconnect()
