import time

from . import robot, skip_before  # pylint: disable=unused-import


def test_set_wheel_motors(robot):
    robot.motors.set_wheel_motors(100, -100)
    time.sleep(5)
    robot.motors.set_wheel_motors(-100, 100)
    time.sleep(1)


def test_set_head_motor(robot):
    robot.motors.set_head_motor(1)
    time.sleep(5)
    robot.motors.set_head_motor(-1)
    time.sleep(1)


def test_set_lift_motor(robot):
    robot.motors.set_lift_motor(1)
    time.sleep(5)
    robot.motors.set_lift_motor(-1)
    time.sleep(1)

@skip_before("0.5.2")
def test_stop_all_motors(robot):
    robot.motors.set_wheel_motors(25, 50)
    time.sleep(0.5)
    robot.motors.stop_all_motors()
