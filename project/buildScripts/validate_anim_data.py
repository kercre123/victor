"""
This script can be used to help validate that all of the
audio events used in animations are in fact available.
"""

import os


# This is the the SoundBanks XML file with audio event info (relative to the EXTERNALS directory)
SOUNDBANKS_XML_FILE = os.path.join("victor-audio-assets", "metadata", "Dev_Mac", "SoundbanksInfo.xml")

# This is the animations directory (relative to the EXTERNALS directory) that
# contains animation TAR files
ANIM_ASSETS_DIR = os.path.join("animation-assets", "animations")

# These are the relevant attribute names/values in the animation JSON files
KEYFRAME_TYPE_ATTR = "Name"
AUDIO_KEYFRAME_TYPE = "RobotAudioKeyFrame"
AUDIO_EVENT_NAMES_ATTR = "audioName"
AUDIO_EVENT_ID_ATTR = "audioEventId"
TRIGGER_TIME_ATTR = "triggerTime_ms"
DURATION_TIME_ATTR = "durationTime_ms"

# These are the relevant attribute names in the SoundBanks info XML file
SOUND_BANKS_XML_ATTR = "SoundBanks"
INCLUDED_EVENTS_XML_ATTR = "IncludedEvents"
AUDIO_EVENT_NAME_XML_ATTR = "Name"
AUDIO_EVENT_ID_XML_ATTR = "Id"


import tarfile
import json
import xml.etree.ElementTree as ET
import tempfile
import zipfile


def get_tar_files(root_dir):
    all_tar_files = []
    for dir_name, subdir_list, file_list in os.walk(root_dir):
        tar_files = [x for x in file_list if x.endswith(".tar")]
        all_tar_files.extend([os.path.join(dir_name, x) for x in tar_files])
    return all_tar_files


def fill_file_dict(file_list):
    file_dict = {}
    for one_file in file_list:
        file_name = os.path.basename(one_file)
        if file_name in file_dict:
            file_dict[file_name].append(one_file)
            print("ALERT: Multiple '%s' files found: %s" % (file_name, file_dict[file_name]))
        else:
            file_dict[file_name] = [one_file]
    return file_dict


def unpack_tarball(tar_file):
    unpacked_files = []
    if not tar_file or not os.path.isfile(tar_file):
        raise ValueError("Invalid TAR file provided: %s" % tar_file)
    dest_dir = tempfile.mkdtemp()
    with tarfile.open(tar_file) as tar:
        tar.extractall(dest_dir)
        for member in tar:
            member = os.path.join(dest_dir, member.name)
            unpacked_files.append(member)
    return unpacked_files


def get_audio_events_in_soundbanks_info_xml_file(xml_file, sound_banks_attr=SOUND_BANKS_XML_ATTR,
                                                 included_events_attr=INCLUDED_EVENTS_XML_ATTR,
                                                 audio_event_name_attr=AUDIO_EVENT_NAME_XML_ATTR,
                                                 audio_event_id_attr=AUDIO_EVENT_ID_XML_ATTR):
    """
    Use this function to parse the SoundbanksInfo.xml file that contains audio event info.
    """
    all_audio_events = []

    if not xml_file or not os.path.isfile(xml_file):
        raise ValueError("Invalid XML file provided: %s" % xml_file)
    with open(xml_file, 'r') as fh:
        xml_data = fh.read()
    xml_data = xml_data.strip()

    root = ET.fromstring(xml_data)
    for sound_banks in root.iter(sound_banks_attr):
        for all_events in sound_banks.iter(included_events_attr):
            for event in all_events:
                # The numerical event ID is what really matters for Wwise audio events,
                # so the event name strings are NOT case sensitive. Therefore, we convert
                # to lowercase here and compare to lowercase event names later.
                event_name = event.get(audio_event_name_attr)
                event_name = event_name.lower()
                event_id = event.get(audio_event_id_attr)
                event_id = long(event_id)
                all_audio_events.append((event_name, event_id))

    return all_audio_events


def get_audio_event_usage_in_anim(json_file, all_available_events):
    """
    Given an animation JSON file and a list of all available audio
    events, this function will parse the animation file to
    determine what audio events it uses.  It will then return a
    tuple of two lists:

    (available_events, unavailable_events)

    The first list is valid audio events in the animation. The second
    list is the audio events in the animation that are unavailable.
    """
    available_events = []
    unavailable_events = []
    if not json_file or not os.path.isfile(json_file):
        raise ValueError("Invalid JSON file provided: %s" % json_file)
    with open(json_file, 'r') as fh:
        contents = json.load(fh)
    for anim_clip, keyframes in contents.items():
        # Loop over all keyframes in this animation and look for audio keyframes...
        anim_clip = str(anim_clip)
        for keyframe in keyframes:
            try:
                keyframe_type = str(keyframe[KEYFRAME_TYPE_ATTR])
            except KeyError:
                # This loop is looking for audio keyframes and checking what audio event they use.
                # If we get KeyError here, then we don't know what type of keyframe this is, i.e.
                # HeadAngleKeyFrame, LiftHeightKeyFrame, RobotAudioKeyFrame, etc. That would be odd,
                # but in this context we just assume that it's NOT an audio keyframe and move on.
                continue
            if keyframe_type == AUDIO_KEYFRAME_TYPE:
                audio_events = keyframe[AUDIO_EVENT_NAMES_ATTR]
                audio_ids = keyframe[AUDIO_EVENT_ID_ATTR]
                if len(audio_events) != len(audio_ids):
                    raise ValueError("Bad audio keyframe in %s has mismatched number of audio "
                                     "event IDs and names: %s" % (json_file, keyframe))
                for idx in range(len(audio_events)):
                    # The numerical event ID is what really matters for Wwise audio events,
                    # so the event name strings are NOT case sensitive. Therefore, we converted
                    # to lowercase earlier and compare to lowercase event names here.
                    audio_event = str(audio_events[idx]).lower()
                    audio_id = long(audio_ids[idx])
                    if (audio_event, audio_id) in all_available_events:
                        available_events.append((audio_event, audio_id))
                    else:
                        unavailable_events.append((audio_event, audio_id))
    return (available_events, unavailable_events)


def check_audio_events_all_anims(externals_dir, anim_assets_dir=ANIM_ASSETS_DIR,
                                 soundbanks_xml_file=SOUNDBANKS_XML_FILE):
    """
    This function will raise ValueError with relevant info if any
    animations use any audio events that are unavailable.
    """
    problem_msg = "Unable to validate audio events used in animations because: %s"

    # Get a list of all available audio events
    soundbanks_xml_file = os.path.join(externals_dir, soundbanks_xml_file)
    #print("Soundbanks XML file = %s" % soundbanks_xml_file)
    try:
        all_audio_events = get_audio_events_in_soundbanks_info_xml_file(soundbanks_xml_file)
    except (IOError, OSError, ValueError, ET.ParseError), e:
        raise ValueError(problem_msg % e)

    # Check all audio events in all animations and keep track of what unavailable
    # audio events are currently being used
    problems = {}
    tar_files_dir = os.path.join(externals_dir, anim_assets_dir)
    tar_files = get_tar_files(tar_files_dir)
    if not tar_files:
        this_prob = "No tar files available in %s" % tar_files_dir
        raise ValueError(problem_msg % this_prob)
    tar_file_dict = fill_file_dict(tar_files)
    for file_name, file_paths in tar_file_dict.items():
        file_path = file_paths[0]
        unpacked_files = unpack_tarball(file_path)
        for json_file in unpacked_files:
            available_events, unavailable_events = get_audio_event_usage_in_anim(json_file, all_audio_events)
            if unavailable_events:
                anim_name = os.path.basename(json_file)
                problems[anim_name] = unavailable_events

    if problems:
        msgs = []
        for anim_name, unavailable_events in problems.items():
            formatted_events = []
            for event in unavailable_events:
                event_name = event[0]
                event_id = event[1]
                formatted_events.append("%s (%s)" % (event_name, event_id))
            if formatted_events:
                msg = "%s uses: " % anim_name
                msg += ", ".join(formatted_events)
                msgs.append(msg)
        msgs.sort()
        msg = os.linesep * 2
        msg += "Found unavailable audio events used in the following animations:"
        msg += os.linesep
        msg += os.linesep.join(msgs)
        msg += os.linesep
        raise ValueError(msg)


def get_anim_length(keyframe_list):
    anim_length = 0
    for keyframe in keyframe_list:
        try:
            trigger_time_ms = keyframe[TRIGGER_TIME_ATTR]
        except KeyError:
            continue
        try:
            duration_time_ms = keyframe[DURATION_TIME_ATTR]
        except KeyError:
            duration_time_ms = 0
        keyframe_length_ms = trigger_time_ms + duration_time_ms
        anim_length = max(anim_length, keyframe_length_ms)
    return anim_length


def get_anim_name_and_length(json_file):
    anim_name_length_mapping = {}
    if not json_file or not os.path.isfile(json_file):
        raise ValueError("Invalid JSON file provided: %s" % json_file)
    with open(json_file, 'r') as fh:
        contents = json.load(fh)
    for anim_name, keyframes in contents.items():
        anim_name = str(anim_name)
        anim_length = get_anim_length(keyframes)
        if not isinstance(anim_length, int):
            if anim_length == int(anim_length):
                anim_length = int(anim_length)
            else:
                print("WARNING: The length of '%s' is not an integer (length = %s)"
                      % (anim_name, anim_length))
        anim_name_length_mapping[anim_name] = anim_length
    return anim_name_length_mapping


