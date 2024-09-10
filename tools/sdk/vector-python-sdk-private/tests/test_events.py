import threading

import pytest

import anki_vector
from . import any_robot, make_sync, robot, skip_before  # pylint: disable=unused-import


# TODO: if other events can be simulated, they should be added here

@skip_before("0.5.2")
@pytest.mark.parametrize("event_obj", [
    pytest.param(anki_vector.events.Events.robot_state, id="robot_state"),
])
def test_robot_events(any_robot, event_obj):
    received_event = threading.Event()

    def robot_state_handler(robot, event_type, event_msg):
        setattr(received_event, "robot", robot)
        setattr(received_event, "event_type", event_type)
        setattr(received_event, "event_msg", event_msg)
        received_event.set()
    any_robot.events.subscribe(robot_state_handler, event_obj)
    assert received_event.wait(timeout=5), f"Timeout while waiting for event: {event_obj.value}"
    assert getattr(received_event, "event_type") == event_obj.value
    assert getattr(received_event, "robot") == any_robot


@skip_before("0.5.2")
@pytest.mark.parametrize("event_obj", [
    pytest.param(anki_vector.events.Events.nav_map_update, id="nav_map_update"),
])
def test_robot_nav_map_events(robot, event_obj):
    try:
        received_event = threading.Event()

        def robot_state_handler(robot, event_type, event_msg):
            setattr(received_event, "robot", robot)
            setattr(received_event, "event_type", event_type)
            setattr(received_event, "event_msg", event_msg)
            received_event.set()
        robot.events.subscribe(robot_state_handler, event_obj)
        robot.nav_map.init_nav_map_feed()
        try:
            make_sync(robot.behavior.drive_off_charger())
        except:  # pylint: disable=bare-except
            pass
        try:
            make_sync(robot.motors.set_wheel_motors(-100, 100))  # We gotta make the map
        except:  # pylint: disable=bare-except
            pass
        assert received_event.wait(timeout=5), f"Timeout while waiting for event: {event_obj.value}"
        assert getattr(received_event, "event_type") == event_obj.value
        assert getattr(received_event, "robot") == robot
    finally:
        robot.nav_map.close_nav_map_feed()
