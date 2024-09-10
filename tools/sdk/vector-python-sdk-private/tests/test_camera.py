import time

from . import any_robot, robot, skip_before, version_lesser_than  # pylint: disable=unused-import


def test_latest_image(any_robot):
    try:
        any_robot.camera.init_camera_feed()
        assert any_robot.camera.latest_image is not None, "The camera image was not ready at startup"
    finally:
        any_robot.camera.close_camera_feed()
        if version_lesser_than("0.5.2"):
            time.sleep(1)


def test_latest_image_id(any_robot):
    try:
        any_robot.camera.init_camera_feed()
        assert any_robot.camera.latest_image_id is not None, "The camera image id was not ready at startup"
    finally:
        any_robot.camera.close_camera_feed()
        if version_lesser_than("0.5.2"):
            time.sleep(1)


@skip_before("0.5.2")
def test_capture_single_image(robot):
    assert robot.camera.capture_single_image() is not None, "Failed to capture single image"