import time

from anki_vector import annotate
from . import any_robot, make_sync, robot, skip_before, version_lesser_than  # pylint: disable=unused-import


@skip_before("0.5.2")
def test_annotations(robot):
    image_obj = make_sync(robot.camera.capture_single_image())
    raw_image = image_obj.raw_image
    annotated_image = image_obj.annotate_image(scale=1)
    assert (raw_image is not None and annotated_image is not None), "Failed to capture single image(raw and annotated)"


@skip_before("0.5.2")
def test_annotations_on_camera_feed(any_robot):
    try:
        any_robot.camera.init_camera_feed()
        assert any_robot.camera.latest_image.annotate_image() is not None, "The camera feed image could not be annotated"
    finally:
        any_robot.camera.close_camera_feed()
        if version_lesser_than("0.5.2"):
            time.sleep(1)


@skip_before("0.5.2")
def test_custom_annotations(robot):
    robot.camera.image_annotator.add_static_text("text", "Vec-Cam", position=annotate.AnnotationPosition.TOP_RIGHT)
    image_obj = make_sync(robot.camera.capture_single_image())
    annotated_image = image_obj.annotate_image(scale=2)
    assert annotated_image is not None, "Failed to annotate image with custom annotators"