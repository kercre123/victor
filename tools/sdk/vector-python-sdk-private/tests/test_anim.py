import sys
import time

import pytest

import anki_vector
from . import behavior_control_param, make_sync, robot, serial, skip_before  # pylint: disable=unused-import


def test_anim_list(robot):
    anim_list = robot.anim.anim_list
    assert anim_list, "anim_list should not be empty"


def test_play_animation(robot):
    response = robot.anim.play_animation("anim_pounce_success_02")
    response = make_sync(response)


def test_bad_play_animation(robot):
    with pytest.raises(anki_vector.exceptions.VectorException) as excinfo:
        response = robot.anim.play_animation("this is not a valid animation name")
        response = make_sync(response)
    assert 'Unknown animation' in str(excinfo.value)


def test_load_animation_list(robot):
    response = robot.anim.load_animation_list()
    response = make_sync(response)


def test_anim_list_property(robot):
    anim_list = robot.anim.anim_list
    assert anim_list, "robot.anim.anim_list failed to load before access"


def test_anim_list_property_cached():
    #can't use robot from fixture, need to create our own to test anim_list load time
    with anki_vector.Robot(serial=serial, **behavior_control_param) as robot:
        start_time = time.time()
        anim_list = robot.anim.anim_list
        assert len(anim_list) > 1000
        elapsed_time = time.time() - start_time
        assert elapsed_time < 0.5  # This should be nearly instant when cached


def test_anim_list_property_cached_async():
    with anki_vector.AsyncRobot(serial=serial, **behavior_control_param) as robot:
        start_time = time.time()
        anim_list = robot.anim.anim_list
        assert len(anim_list) > 1000
        elapsed_time = time.time() - start_time
        assert elapsed_time < 0.5  # This should be nearly instant when cached


@skip_before("0.5.2")
def test_anim_trigger_list(robot):
    anim_trigger_list = robot.anim.anim_trigger_list
    assert anim_trigger_list, "anim_trigger_list should not be empty"


@skip_before("0.5.2")
def test_play_animation_trigger(robot):
    response = robot.anim.play_animation_trigger("GreetAfterLongTime")
    response = make_sync(response)


@skip_before("0.5.2")
def test_bad_play_animation_trigger(robot):
    with pytest.raises(anki_vector.exceptions.VectorException) as excinfo:
        response = robot.anim.play_animation_trigger("this is not a valid animation trigger name")
        response = make_sync(response)
    assert 'Unknown animation trigger' in str(excinfo.value)


@skip_before("0.5.2")
def test_load_animation_trigger_list(robot):
    response = robot.anim.load_animation_trigger_list()
    response = make_sync(response)


@skip_before("0.5.2")
def test_anim_trigger_list_property(robot):
    anim_trigger_list = robot.anim.anim_trigger_list
    assert anim_trigger_list, "robot.anim.anim_trigger_list failed to load before access"


@skip_before("0.5.2")
def test_anim_trigger_list_property_cached():
    with anki_vector.Robot(serial=serial, behavior_control_level=None) as robot:
        start_time = time.time()
        anim_trigger_list = robot.anim.anim_trigger_list
        assert len(anim_trigger_list) > 300
        elapsed_time = time.time() - start_time
        assert elapsed_time < 0.5  # This should be nearly instant when cached


@skip_before("0.5.2")
def test_anim_trigger_list_property_cached_async():
    with anki_vector.AsyncRobot(serial=serial, behavior_control_level=None) as robot:
        start_time = time.time()
        anim_trigger_list = robot.anim.anim_trigger_list
        assert len(anim_trigger_list) > 300
        elapsed_time = time.time() - start_time
        assert elapsed_time < 0.5  # This should be nearly instant when cached
