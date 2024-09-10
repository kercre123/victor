from . import robot, robot_no_control  # pylint: disable=unused-import


def test_behavior_control_start_on(robot):
    assert robot.conn.requires_behavior_control, "Robot is expected to have behavior control at the start"
    robot.conn.release_control().result()
    assert not robot.conn.requires_behavior_control, "Expected to have released control"
    robot.conn.request_control().result()
    assert robot.conn.requires_behavior_control, "Expected to have acquired control"


def test_behavior_control_start_off(robot_no_control):
    assert not robot_no_control.conn.requires_behavior_control, "Robot is expected to have not behavior control at the start"
    robot_no_control.conn.request_control().result()
    assert robot_no_control.conn.requires_behavior_control, "Expected to have acquired control"
    robot_no_control.conn.release_control().result()
    assert not robot_no_control.conn.requires_behavior_control, "Expected to have released control"
