import pytest

import anki_vector
from anki_vector import audio
from anki_vector.messaging import protocol
from . import make_sync, robot, robot_no_control, skip_before

#relative to root of repo, as tests are expected to be run from there
TEST_AUDIO_FILEPATH = "tests/test_assets/vector_alert.wav"

@pytest.mark.parametrize("fname,vol", [
    pytest.param(TEST_AUDIO_FILEPATH, 100), #max allowed
    pytest.param(TEST_AUDIO_FILEPATH, 20),
    pytest.param(TEST_AUDIO_FILEPATH, 0),   #min allowed
])
@skip_before("0.5.2")
def test_play_external_audio_file(robot, fname, vol):
    #try different playback volumes
    response = robot.audio.stream_wav_file(fname, vol)
    make_sync(response, False)

@skip_before("0.5.2")
def test_bad_volume_1(robot):
    #out of bounds volume: too high
    with pytest.raises(anki_vector.exceptions.VectorExternalAudioPlaybackException):
        response = robot.audio.stream_wav_file(TEST_AUDIO_FILEPATH, 110)
        make_sync(response, False)

@skip_before("0.5.2")
def test_bad_volume_2(robot):
    with pytest.raises(anki_vector.exceptions.VectorExternalAudioPlaybackException):
        #out of bounds too low
        response = robot.audio.stream_wav_file(TEST_AUDIO_FILEPATH, -10)
        make_sync(response, False)

@skip_before("0.5.2")
@pytest.mark.parametrize("fname", [
    pytest.param("tests/test_assets/8bit.wav"),
    pytest.param("tests/test_assets/22k.wav"),
    pytest.param("tests/test_assets/44k.wav"),
])
def test_bad_file_format(robot, fname):
    #these files are all incompatible with audio playback
    with pytest.raises(anki_vector.exceptions.VectorExternalAudioPlaybackException):
        response = robot.audio.stream_wav_file(fname, 50)
        make_sync(response, False)

@skip_before("0.5.2")
@pytest.mark.parametrize("volume", [
    pytest.param(audio.RobotVolumeLevel.LOW),
    pytest.param(audio.RobotVolumeLevel.MEDIUM_LOW),
    pytest.param(audio.RobotVolumeLevel.MEDIUM),
    pytest.param(audio.RobotVolumeLevel.MEDIUM_HIGH),
    pytest.param(audio.RobotVolumeLevel.HIGH)
])
def test_master_volume(robot_no_control, volume):
    response = robot_no_control.audio.set_master_volume(volume)
    result = make_sync(response, False)
    assert result.status.code == protocol.ResponseStatus.RESPONSE_RECEIVED
