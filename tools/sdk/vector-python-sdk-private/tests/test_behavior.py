from concurrent.futures import CancelledError
import time

import pytest

import anki_vector
from anki_vector.messaging import protocol
from . import make_sync, robot, skip_before  # pylint: disable=unused-import


def test_drive_off_charger(robot):
    response = robot.behavior.drive_off_charger()
    # Preventing assert because drive_on_charger tends to be interrupted by losing control
    # which is indicative of a behavior problem, not an SDK problem.
    try:
        response = make_sync(response, assert_not_none=False)
    except CancelledError:
        # This cancelled error is also raised as a part of the loss of control.
        # If we fix the loss of control during docking, we should remove this error handling.
        pass


# Turned off drive_on_charger test because this is too unrelable.
#def test_drive_on_charger(robot):
    #response = robot.behavior.drive_on_charger()
    # Preventing assert because drive_on_charger tends to be interrupted by losing control
    # which is indicative of a behavior problem, not an SDK problem.
    # try:
    #     response = make_sync(response, assert_not_none=False)
    # except CancelledError:
    #     # This cancelled error is also raised as a part of the loss of control.
    #     # If we fix the loss of control during docking, we should remove this error handling.
    #     pass


@skip_before("0.5.2")
def test_say_text(robot):
    response = robot.behavior.say_text("Hello world!")
    response = make_sync(response)


@pytest.mark.parametrize("hue,saturation", [
    pytest.param(0.0, 0.0),
    pytest.param(0.0, 1.0),
    pytest.param(1.0, 0.0),
    pytest.param(1.0, 1.0),
])
def test_set_eye_color(robot, hue, saturation):
    response = robot.behavior.set_eye_color(hue=hue, saturation=saturation)
    response = make_sync(response)
    print(response)
    time.sleep(0.5)


@pytest.mark.parametrize("pose,relative_to_robot,num_retries", [
    pytest.param(anki_vector.util.Pose(x=0,
                                       y=50,
                                       z=0,
                                       angle_z=anki_vector.util.Angle(degrees=0)),
                 True,
                 0),
    pytest.param(anki_vector.util.Pose(x=0,
                                       y=50,
                                       z=0,
                                       angle_z=anki_vector.util.Angle(degrees=0)),
                 False,
                 0),
])
def test_go_to_pose(robot, pose, relative_to_robot, num_retries):
    response = robot.behavior.go_to_pose(pose, relative_to_robot, num_retries)
    response = make_sync(response)


# TODO: test_dock_with_cube
# TODO: test_drive_straight
# TODO: test_turn_in_place
# TODO: test_set_head_angle
# TODO: test_set_lift_height
# TODO: test_turn_towards_face
# TODO: test_go_to_object
# TODO: test_roll_cube
# TODO: test_pop_a_wheelie
# TODO: test_pickup_object
# TODO: test_place_object_on_ground_here

@skip_before("0.5.2")
def test_look_around_in_place(robot):
    response = robot.behavior.look_around_in_place()
    response = make_sync(response)
    assert response.status.code == protocol.ResponseStatus.RESPONSE_RECEIVED

@skip_before("0.5.2")
def test_roll_visible_cube(robot):
    #probably won't ever find a block, but should still execute
    response = robot.behavior.roll_visible_cube()
    response = make_sync(response)
    assert response.status.code == protocol.ResponseStatus.RESPONSE_RECEIVED
