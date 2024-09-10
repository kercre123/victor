from . import any_robot, make_sync  # pylint: disable=unused-import


def test_battery(any_robot):
    response = any_robot.get_battery_state()
    response = make_sync(response)


def test_version(any_robot):
    response = any_robot.get_version_state()
    response = make_sync(response)


def test_status(any_robot):
    assert any_robot.status is not None, "The robot status was not ready at startup"


def test_last_image_time_stamp(any_robot):
    assert any_robot.last_image_time_stamp is not None, "The last_image_time_stamp was not ready at startup"


def test_localized_to_object_id(any_robot):
    assert any_robot.localized_to_object_id is not None, "The localized_to_object_id was not ready at startup"


def test_head_tracking_object_id(any_robot):
    assert any_robot.head_tracking_object_id is not None, "The head_tracking_object_id was not ready at startup"


def test_carrying_object_id(any_robot):
    assert any_robot.carrying_object_id is not None, "The carrying_object_id was not ready at startup"


def test_gyro(any_robot):
    assert any_robot.gyro is not None, "The gyro data was not ready at startup"


def test_accel(any_robot):
    assert any_robot.accel is not None, "The accel value was not ready at startup"


def test_lift_height_mm(any_robot):
    assert any_robot.lift_height_mm is not None, "The lift_height_mm was not ready at startup"


def test_head_angle_rad(any_robot):
    assert any_robot.head_angle_rad is not None, "The head_angle_rad value was not ready at startup"


def test_right_wheel_speed_mmps(any_robot):
    assert any_robot.right_wheel_speed_mmps is not None, "The right_wheel_speed_mmps value was not ready at startup"


def test_left_wheel_speed_mmps(any_robot):
    assert any_robot.left_wheel_speed_mmps is not None, "The left_wheel_speed_mmps value was not ready at startup"


def test_pose_pitch_rad(any_robot):
    assert any_robot.pose_pitch_rad is not None, "The pose_pitch_rad value was not ready at startup"


def test_pose_angle_rad(any_robot):
    assert any_robot.pose_angle_rad is not None, "The pose_angle_rad value was not ready at startup"


def test_pose(any_robot):
    assert any_robot.pose is not None, "The pose value was not ready at startup"
