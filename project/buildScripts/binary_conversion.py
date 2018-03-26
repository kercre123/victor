"""
IMPORTANT: This version of binary_conversion.py is now specific to Victor,
so it should NOT be used for the original Cozmo because, for example, this
version strips out the Left and Right backpack light data.
"""

BODY_MOTION_TRACK = "BodyMotionKeyFrame"
ROBOT_AUDIO_TRACK = "RobotAudioKeyFrame"
BACKPACK_LIGHT_TRACK = "BackpackLightsKeyFrame"
PROCEDURAL_FACE_TRACK = "ProceduralFaceKeyFrame"

ALL_TRACKS = ["LiftHeightKeyFrame", "HeadAngleKeyFrame", PROCEDURAL_FACE_TRACK,
              BACKPACK_LIGHT_TRACK, "FaceAnimationKeyFrame", "EventKeyFrame",
              ROBOT_AUDIO_TRACK, BODY_MOTION_TRACK, "RecordHeadingKeyFrame", "TurnToRecordedHeadingKeyFrame"]

BODY_RADIUS_ATTR = "radius_mm"

BODY_SPEED_ATTR = "speed"

LIFT_HEIGHT_ATTR = "height_mm"

HEAD_ANGLE_ATTR = "angle_deg"


# Audio JSON Attributes
AUDIO_EVENT_GROUPS_ATTR = "eventGroups"
AUDIO_EVENT_IDS_ATTR = "eventIds"
AUDIO_VOLUMES_ATTR = "volumes"
AUDIO_PROBABILITIES_ATTR = "probabilities"
# Deprecated keys
AUDIO_DEP_EVENT_ATTR = "audioEventId"
AUDIO_DEP_VOLUME_ATTR = "volume"
AUDIO_DEP_PROBABILITY_ATTR = "probability"


DURATION_TIME_ATTR = "durationTime_ms"

TRIGGER_TIME_ATTR = "triggerTime_ms"

KEYFRAME_TYPE_ATTR = "Name"

ANIM_NAME_ATTR = "Name"

KEYFRAMES_ATTR = "keyframes"

INTEGER_ATTRS = [TRIGGER_TIME_ATTR, DURATION_TIME_ATTR, BODY_SPEED_ATTR, HEAD_ANGLE_ATTR, LIFT_HEIGHT_ATTR]

CLIPS_ATTR = "clips"

BIN_FILE_EXT = ".bin"

OLD_ANIM_TOOL_ATTRS = ["$type", "pathFromRoot"]

OLD_BACKPACK_LIGHT_ATTRS = ["Left", "Right"]


import sys
import os
import tempfile
import json
import pprint
import subprocess
import json
import re
import shutil
import tarfile
import inspect


THIS_DIR = os.path.normpath(os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))

ENGINE_ROOT = os.path.dirname(os.path.dirname(THIS_DIR))

CONFIG_DIR = os.path.join(ENGINE_ROOT, "resources", "config")

SCHEMA_FILE = os.path.join(CONFIG_DIR, "cozmo_anim.fbs")


def read_anim_file(anim_file):
    """
    Given the path to a .json animation file, this function
    will read the contents of that file and return a 2-item
    tuple of (animation name, list of all keyframes)
    """
    fh = open(anim_file, 'r')
    try:
        contents = json.load(fh)
    except StandardError, e:
        print("Failed to read %s file because: %s" % (anim_file, e))
        raise
    finally:
        fh.close()
    anim_clip, keyframes = contents.items()[0]
    #print("The '%s' animation has %s keyframes" % (anim_clip, len(keyframes)))
    return (anim_clip, keyframes)


def prep_json_for_binary_conversion(anim_name, keyframes):
    """
    Given the name of the animation and a list of all keyframes for that
    animation, this function will separate those keyframes by each track
    to build and return a dictionary that looks like:

        {'Name'     : 'anim_blah',
         'keyframes': {'BackpackLightsKeyFrame': [<list of dictionaries>],
                       'BodyMotionKeyFrame': [<list of dictionaries>],
                       'EventKeyFrame': [<list of dictionaries>],
                       'FaceAnimationKeyFrame': [<list of dictionaries>],
                       'HeadAngleKeyFrame': [<list of dictionaries>],
                       'LiftHeightKeyFrame': [<list of dictionaries>],
                       'ProceduralFaceKeyFrame': [<list of dictionaries>],
                       'RobotAudioKeyFrame': [<list of dictionaries>],
                       'RecordHeadingKeyFrame': [<list of dictionaries>],
                       'TurnToRecordedHeadingKeyFrame': [<list of dictionaries>],
                      }
        }
    """
    anim_dict = {}
    anim_dict[ANIM_NAME_ATTR] = anim_name
    anim_dict[KEYFRAMES_ATTR] = {}

    for track in ALL_TRACKS:
        # When converting to binary, we need ALL animation tracks to exist, even if
        # some of those don't have any keyframes, so we create those empty lists here.
        anim_dict[KEYFRAMES_ATTR][track] = []

    for keyframe in keyframes:
        track = keyframe[KEYFRAME_TYPE_ATTR]
        if track not in anim_dict[KEYFRAMES_ATTR]:
            anim_dict[KEYFRAMES_ATTR][track] = []
        keyframe.pop(KEYFRAME_TYPE_ATTR)

        # All keyframes are required to have a trigger time.
        try:
            trigger_time = keyframe[TRIGGER_TIME_ATTR]
        except KeyError, e:
            error_msg = "At least one '%s' in '%s' is missing '%s'" % (track, anim_name, TRIGGER_TIME_ATTR)
            raise KeyError(error_msg)

        # Remove old attributes that are no longer used but potentially lingering in old data.
        for old_attr in OLD_ANIM_TOOL_ATTRS:
            try:
                keyframe.pop(old_attr)
            except KeyError:
                pass

        if track == BACKPACK_LIGHT_TRACK:
            # Many old anim files will have "Left" and "Right" backpack lights,
            # but we need to strip those out for Victor
            for old_attr in OLD_BACKPACK_LIGHT_ATTRS:
                try:
                    keyframe.pop(old_attr)
                except KeyError:
                    pass

        if track == ROBOT_AUDIO_TRACK:
            # There are so many migration changes audio gets its own method =)
            keyframe = prep_audio_key_frame_json(keyframe, anim_name)

        if track == PROCEDURAL_FACE_TRACK:
            # The engine doesn't use "durationTime_ms" for ProceduralFaceKeyFrame
            try:
                keyframe.pop(DURATION_TIME_ATTR)
            except KeyError:
                pass

        # Since the 'radius_mm' attribute of 'BodyMotionKeyFrame' can be set to "TURN_IN_PLACE"
        # or "STRAIGHT", that attribute is always stored as a string for FlatBuffers. When the
        # engine is then loading that data, it will convert numerical values back to float
        if track == BODY_MOTION_TRACK:
            if not isinstance(keyframe[BODY_RADIUS_ATTR], basestring):
                keyframe[BODY_RADIUS_ATTR] = str(keyframe[BODY_RADIUS_ATTR])

        # Some attributes (including times in ms, speed in mm/s or deg/s, head angle in deg
        # and lift height in mm) are expected to be integers in the engine. However, until
        # December 2017, the animation exporter was not doing a good job of forcing those
        # values to be integers, so we explicitly convert those values to integers here.
        for int_attr in INTEGER_ATTRS:
            try:
                orig_val = keyframe[int_attr]
            except KeyError:
                pass
            else:
                int_val = int(round(orig_val))
                keyframe[int_attr] = int_val

        anim_dict[KEYFRAMES_ATTR][track].append(keyframe)

    return anim_dict


def prep_audio_key_frame_json(keyframe, anim_name):
    """
    1. Check audio key frame json format
    2. Migrate old version to new format
    3. Handle exporter edge cases
    Current Audio Key Frame format
    {
        "triggerTime_ms": uint
        "eventGroups": [
          {
            "eventIds": [uint],
            "volumes": [float],
            "probabilities": [float]
          }
        ],
        "states": [
          {
            stateGroupId: uint,
            stateId: uint
          }
        ]
        "switches": [
          {
            switchGroupId: uint,
            stateId: uint
          }
        ]
        "parameters": [
          {
            parameterId: uint,
            value: float,
            time_ms: uint,
            curve: byte
          }
        ]
    }
    """

    # Check audio key frame format
    if not AUDIO_DEP_EVENT_ATTR in keyframe:
        # Key frame is in correct format
        return keyframe

    # Migrate to new audio key frame format
    audioFrame = {}
    audioFrame[TRIGGER_TIME_ATTR] = keyframe[TRIGGER_TIME_ATTR]
    eventGroup = {}

    # Update Audio Event Id values
    if isinstance(keyframe[AUDIO_DEP_EVENT_ATTR], list):
        eventGroup[AUDIO_EVENT_IDS_ATTR] = keyframe[AUDIO_DEP_EVENT_ATTR]
    else:
        eventGroup[AUDIO_EVENT_IDS_ATTR] = [keyframe[AUDIO_DEP_EVENT_ATTR]]

    eventCount = 0
    if AUDIO_EVENT_IDS_ATTR in eventGroup:
        eventCount = len(eventGroup[AUDIO_EVENT_IDS_ATTR])

    # Return empty key frame, this will signal an error when loaded
    if eventCount == 0:
        audioFrame[AUDIO_EVENT_GROUPS_ATTR] = [eventGroup]
        return audioFrame

    # Update probabiltiy value & handle migration edge cases
    probCount = 0
    if AUDIO_DEP_PROBABILITY_ATTR in keyframe:
        # Get probability values
        if isinstance(keyframe[AUDIO_DEP_PROBABILITY_ATTR], list):
            # Expect a value for every event
            dep_probability = keyframe[AUDIO_DEP_PROBABILITY_ATTR]
            probCount = len(dep_probability)
            eventGroup[AUDIO_PROBABILITIES_ATTR] = dep_probability            
        else:
            # Expect a single event
            eventGroup[AUDIO_PROBABILITIES_ATTR] = [keyframe[AUDIO_DEP_PROBABILITY_ATTR]]
            probCount = 1
    
    if probCount != eventCount:
        if probCount != 0:
            # Event count miss match
            msg = "Failed to migrate '%s' because the event and probability count do not match" % (anim_name)
            print(msg)
        # Equal chance for each event
        val = 1.0 / eventCount
        probabilities = [val] * eventCount
        eventGroup[AUDIO_PROBABILITIES_ATTR] = probabilities

    # Update Volume value
    defaultVol = 1.0
    if AUDIO_DEP_VOLUME_ATTR in keyframe:
        # Old versions only have 1 volume
        defaultVol = keyframe[AUDIO_DEP_VOLUME_ATTR]
    # Set same volume for all events
    eventGroup[AUDIO_VOLUMES_ATTR] = [defaultVol] * eventCount

    # Add single event group to audio frame
    audioFrame[AUDIO_EVENT_GROUPS_ATTR] = [eventGroup]

    return audioFrame


def write_json_file(json_file, data):
    """
    Given the path to a .json file and a dictionary of animation
    data, this function will write out the animation file
    (overwriting any existing file at that path).
    """
    try:
        with open(json_file, 'w') as fh:
            json.dump(data, fh, indent=2, separators=(',', ': '))
    except (OSError, IOError), e:
        error_msg = "Failed to write '%s' file because: %s" % (json_file, e)
        print(error_msg)


def convert_json_to_binary(file_path, flatc_dir, schema_file, bin_file_ext):
    """
    Given:
        1: the path to a .json animation file
        2: the path to a directory that contains the "flatc" binary
        3: the path to an .fbs FlatBuffers schema file, eg. "cozmo_anim.fbs"
        4: the desired file extension for the resulting binary file, eg. ".bin"
    this function will use "flatc" to generate a binary animation
    file and return the path to that file.

    See https://google.github.io/flatbuffers/flatbuffers_guide_using_schema_compiler.html
    for additional info about the "flatc" schema compiler.
    """
    output_dir = os.path.dirname(file_path)
    flatc = os.path.join(flatc_dir, "flatc")
    if not os.path.isfile(flatc):
        raise EnvironmentError("%s is not a file so JSON data cannot be converted to binary" % flatc)
    args = [flatc, "-o", output_dir, "-b", schema_file, file_path]
    #print("Running: %s" % " ".join(args))
    p = subprocess.Popen(args)
    stdout, stderr = p.communicate()
    exit_status = p.poll()
    output_file = os.path.splitext(file_path)[0] + bin_file_ext
    if not os.path.isfile(output_file) or exit_status != 0:
        raise ValueError("Unable to successfully generate binary file %s (exit status "
                         "of external process = %s)" % (output_file, exit_status))
    #print("Converted %s to %s" % (file_path, output_file))
    return output_file


def main(json_files, bin_name, flatc_dir, schema_file=SCHEMA_FILE, bin_file_ext=BIN_FILE_EXT):
    """
    Given:
        1: a list of .json animation files
        2: the desired name for the resulting binary file
        3: the path to a directory that contains the "flatc" binary
        4: the path to an .fbs FlatBuffers schema file (optional, default = "cozmo_anim.fbs")
        5: the desired file extension for the resulting binary file (optional, default = ".bin")
    this function will use "flatc" to generate a binary animation
    file and return the path to that file.

    See https://google.github.io/flatbuffers/flatbuffers_guide_using_schema_compiler.html
    for additional info about the "flatc" schema compiler.
    """
    anim_clips = []
    for json_file in json_files:
        #print("Preparing for binary conversion: %s" % json_file)
        anim_clip, keyframes = read_anim_file(json_file)
        anim_dict = prep_json_for_binary_conversion(anim_clip, keyframes)
        anim_clips.append(anim_dict)
    fd, tmp_json_file = tempfile.mkstemp(suffix=".json")
    write_json_file(tmp_json_file, {CLIPS_ATTR:anim_clips})
    bin_file = convert_json_to_binary(tmp_json_file, flatc_dir, schema_file, bin_file_ext)
    os.close(fd)
    renamed_bin_file = os.path.join(os.path.dirname(bin_file), bin_name)
    os.rename(bin_file, renamed_bin_file)
    if not os.path.isfile(renamed_bin_file):
        raise ValueError("Binary file missing: %s" % renamed_bin_file)
    return renamed_bin_file

