
BODY_MOTION_TRACK = "BodyMotionKeyFrame"

ROBOT_AUDIO_TRACK = "RobotAudioKeyFrame"

ALL_TRACKS = ["LiftHeightKeyFrame", "HeadAngleKeyFrame", "ProceduralFaceKeyFrame",
              "BackpackLightsKeyFrame", "FaceAnimationKeyFrame", "EventKeyFrame",
              ROBOT_AUDIO_TRACK, BODY_MOTION_TRACK, "RecordHeadingKeyFrame", "TurnToRecordedHeadingKeyFrame"]

BODY_RADIUS_ATTR = "radius_mm"

DURATION_TIME_ATTR = "durationTime_ms"

TRIGGER_TIME_ATTR = "triggerTime_ms"

KEYFRAME_TYPE_ATTR = "Name"

ANIM_NAME_ATTR = "Name"

KEYFRAMES_ATTR = "keyframes"

CLIPS_ATTR = "clips"

BIN_FILE_EXT = ".bin"

OLD_ANIM_TOOL_ATTRS = ["$type", "pathFromRoot"]


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

        # Some old anim files have an "durationTime_ms" attribute for audio keyframes, but those
        # aren't used anymore and thus not defined in the schema, so they should be removed here.
        if track == ROBOT_AUDIO_TRACK:
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

        anim_dict[KEYFRAMES_ATTR][track].append(keyframe)

    return anim_dict


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


