#!/usr/bin/env python3

# Copyright (c) 2017 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Connect CodeLab running in a local web-browser (via a local aiohttp server)

Opens the CodeLab web assets from the Unity location.
Passes all Unity<->Javascript calls over GameToGame messages via the SDK
'''

import argparse
import asyncio
import calendar
import datetime
from io import BytesIO
import json
import os
import queue
import re
import sys
from threading import Thread
import time
import urllib
import uuid
import webbrowser

try:
    from aiohttp import web, web_request
except ImportError:
    sys.exit("Cannot import from aiohttp. Do `pip3 install --user aiohttp` to install")

try:
    from PIL import Image, ImageDraw
except ImportError:
    sys.exit("Cannot import from PIL. Do `pip3 install --user Pillow` to install")


LOCALE_OPTIONS = ["de-DE", "en-US", "fr-FR", "ja-JP"]


def parse_command_args():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-dev', '--open_dev_tab',
                            dest='open_dev_tab',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Open the devconn page')
    arg_parser.add_argument('-horiz', '--show_horizontal',
                        dest='show_horizontal',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Open the horizontal grammar page')
    arg_parser.add_argument('--use_horizontal',
                            dest='use_horizontal',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Start in horizontal grammar')
    arg_parser.add_argument('-l', '--log_from_engine',
                        dest='log_from_engine',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Log Engine Output')
    arg_parser.add_argument('-nr', '--no_robot',
                        dest='connect_to_robot',
                        default=True,
                        action='store_const',
                        const=False,
                        help='Skip Robot Connection')
    arg_parser.add_argument('-ns', '--no_sdk',
                        dest='connect_to_sdk',
                        default=True,
                        action='store_const',
                        const=False,
                        help='Skip SDK/Engine (and therefore Robot) Connection')
    arg_parser.add_argument('-p', '--page_to_load',
                        dest='page_to_load',
                        default=None,
                        action='store',
                        help='The CodeLab HTML page to open (if set)')
    arg_parser.add_argument('-import', '--import_codelab',
                        dest='import_codelab_file',
                        default=None,
                        action='store',
                        help='The .codelab file to import')
    arg_parser.add_argument('-export', '--export_codelab',
                        dest='export_codelab_file',
                        default=None,
                        action='store',
                        help='The project (UUID or name) to export')
    arg_parser.add_argument('-clone_src', '--clone_source',
                        dest='source_clone_uuid',
                        default=None,
                        action='store',
                        help='The UUID of the project to clone')
    arg_parser.add_argument('-clone_dest', '--clone_destination',
                        dest='destination_clone_uuid',
                        default=None,
                        action='store',
                        help='The UUID of the project to clone into')
    arg_parser.add_argument('-csp', '--clone-sample-projects',
                        dest='clone_samples',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Clone samples to user projects on startup')
    arg_parser.add_argument('-loc', '--locale',
                        dest='locale_to_use',
                        default="en-US",
                        action='store',
                        help='The language locale to use (one of ' + str(LOCALE_OPTIONS) +')')
    arg_parser.add_argument('--projects',
                        dest='show_projects',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Open the projects page')
    arg_parser.add_argument('-t', '--tcp',
                        dest='use_tcp',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Connect over local TCP socket (for Webots)')
    arg_parser.add_argument('--tutorial',
                        dest='show_tutorial',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Open the tutorial page')
    arg_parser.add_argument('-nu', '--not_unique',
                            dest='unique_url',
                            default=True,
                            action='store_const',
                            const=False,
                            help='Dont use a unique URL - files may be cached by browser')
    arg_parser.add_argument('-upp', '--use_python_projects',
                            dest='use_python_projects',
                            default=False,
                            action='store_const',
                            const=True,
                            help="Use CodeLab.py's project store for listing and serving projects (instead of Unity's")
    arg_parser.add_argument('-ltp', '--load_test_projects',
                            dest='load_test_projects',
                            default=False,
                            action='store_const',
                            const=True,
                            help="Load test projects (they add to the list of sample projects)")
    arg_parser.add_argument('-vert', '--show_vertical',
                        dest='show_vertical',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Open the vertical grammar page')
    arg_parser.add_argument('-v', '--verbose',
                            dest='verbose',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Verbose logging of everything (very spammy)')
    arg_parser.add_argument('-dpjc', '--debug_project_json_contents',
                            dest='debug_project_json_contents',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Print out the unescaped ProjectJson contents on every save')


    options = arg_parser.parse_args()
    return options


command_args = parse_command_args()


pending_log_text = ""
def log_text(text_to_log):
    global pending_log_text
    pending_log_text += text_to_log + "\n"
    # limit pending_log_text in event of it not being polled?
    print(text_to_log)


if (command_args.source_clone_uuid is not None) != (command_args.destination_clone_uuid is not None):
    sys.exit("source_clone_uuid and destination_clone_uuid must both be provided")


if command_args.locale_to_use not in LOCALE_OPTIONS:
    sys.exit("Unexpected locale %s" % command_args.locale_to_use)

locale_suffix = "?locale=" + command_args.locale_to_use


def get_workspace_url(is_vertical):
    if is_vertical:
        return "index_vertical.html"
    else:
        return "index.html"

if command_args.page_to_load is not None:
    PAGE_TO_LOAD = command_args.page_to_load
elif command_args.show_horizontal:
    PAGE_TO_LOAD = get_workspace_url(is_vertical=False) + locale_suffix
elif command_args.show_vertical:
    PAGE_TO_LOAD = get_workspace_url(is_vertical=True) + locale_suffix
elif command_args.show_tutorial:
    PAGE_TO_LOAD = "extra/tutorial.html" + locale_suffix
elif command_args.show_projects:
    PAGE_TO_LOAD = "extra/projects.html" + locale_suffix
else:
    PAGE_TO_LOAD = None
log_text("PAGE_TO_LOAD = %s" % PAGE_TO_LOAD)


def check_tcp_port(force_tcp_port=True, sdk_port=5106):
    tcp_port = os.environ.get("SDK_TCP_PORT")
    if tcp_port is None:
        if force_tcp_port:
            os.environ["SDK_TCP_PORT"] = str(sdk_port)
            tcp_port = os.environ.get("SDK_TCP_PORT")
            log_text("Forced SDK_TCP_PORT='%s' - NOTE: Change affects ONLY THIS PROCESS" % tcp_port)
            log_text('To set permanently in Bash do: export SDK_TCP_PORT="%s"' % sdk_port)
            log_text("Note: use 'unset SDK_TCP_PORT' to unset, and 'echo $SDK_TCP_PORT' to print current value")
        else:
            log_text("Note: SDK_TCP_PORT is not set, and force_tcp_port is False, so will not connect to Webots")
    else:
        log_text("SDK_TCP_PORT = '%s'" % tcp_port)

# Must be done before importing cozmo
if command_args.use_tcp:
    check_tcp_port()

import cozmo
from cozmo.util import degrees, distance_mm, speed_mmps

from cozmo._clad import _clad_to_engine_iface  # for transferring files


cozmo.setup_basic_logging()
app = web.Application()

if command_args.unique_url:
    UNIQUE_RUN_ID = "/" + str(uuid.uuid4())
    log_text("[unique_url] This session will serve pages at %s/" % UNIQUE_RUN_ID)
else:
    UNIQUE_RUN_ID = ""

BASE_SRC_SCRATCH_PATH = "../../../unity/Cozmo/Assets/StreamingAssets/Scratch"
CACHE_WRITE_TIME_PATH = "./.codelab_cache_write"
PROJECTS_DIR= "projects/"
CONTENTS_FILENAME = "contents.json"
PROJECTJSON_CONTENTS_FILENAME = "ProjectJSON.json"
CODELAB_BASE_URL = UNIQUE_RUN_ID + "/codelab/"
DEVCONN_BASE_URL = UNIQUE_RUN_ID + "/devconn/"

VALID_SCRATCH_EXTENSIONS = [".css", ".cur", ".gif", ".html", ".js", ".jpg", ".jpeg", ".json", ".otf", ".png", ".svg"]

CODELAB_FILE_HEADER = "CODELAB:"

WEB_VIEW_CALLBACK = "WebViewCallback"


kCurrentVersionNum = 3


def is_unity_timestamp(in_date_time):
    return str(in_date_time).startswith("/Date(")


def datetime_to_unity_timestamp(in_datetime):
    if isinstance(in_datetime, str):
        in_datetime = datetime.datetime.strptime(in_datetime, "%Y-%m-%d %H:%M:%S.%f")
    timestamp = calendar.timegm(in_datetime.timetuple()) * 1000 + (in_datetime.microsecond / 1000)
    return "/Date(%s)/" % timestamp


def extract_timestamp_number(in_timestamp):
    if not is_unity_timestamp(in_timestamp):
        in_timestamp = datetime_to_unity_timestamp(in_timestamp)
    numbers = re.findall(r'\d+', str(in_timestamp))
    return int(numbers[0])


def python_bool_to_js_bool_str(python_bool):
    if python_bool:
        return "true"
    else:
        return "false"


def js_bool_to_python_bool(js_bool):
    # Handle varios string representations
    js_bool_str = str(js_bool).lower()
    if js_bool_str == "true":
        return True
    elif js_bool_str == "false":
        return False
    else:
        log_text("WARNING unexpected js_bool value == '%s'" % repr(js_bool))
        return False


def _is_escaped(in_string, char_index):
    # Are there an odd number of slashes, because even numbers are just unescaping the slash...

    num_slashes = 0
    i = char_index - 1  # start looking prior to the character
    while i >= 0:
        if in_string[i] == "\\":
            num_slashes += 1
            i -= 1
        else:
            # reached the end of prior slashes
            break

    return (num_slashes % 2) == 1


def split_method_args(args_string):
    # Split the arguments in the string into individual arguments based on quotes, e.g:
    # "arg-1", 'arg-2', "'arg'3", 'a"rg"4'   becomes:
    # [arg-1, arg-2, 'arg'3, a"rg"4]
    # Note: This assumes that all arugments are quoted, e.g. "a", 4, "b", would only find [a, b]
    args = []

    # Handle both single and double quoutes, whichever quote starts the quoted argument text must end it.
    QUOTE_1 = "'"
    QUOTE_2 = '"'
    i = 0
    while i < len(args_string):
        # Find the earliest quoute from the current position
        # the type will dictate the closing quoute

        start_quote_1 = args_string.find(QUOTE_1, i)
        start_quote_2 = args_string.find(QUOTE_2, i)
        quote_delimiter = None

        if start_quote_1 >= 0:
            if start_quote_2 >= 0:
                if start_quote_1 < start_quote_2:
                    quote_delimiter = QUOTE_1
                else:
                    quote_delimiter = QUOTE_2
            else:
                quote_delimiter = QUOTE_1
        elif start_quote_2 >= 0:
            quote_delimiter = QUOTE_2

        # Find the end of this quoted string

        if quote_delimiter is not None:
            start_quote = start_quote_1 if (quote_delimiter == QUOTE_1) else start_quote_2
            end_quote = -1  #start_quote_1
            end_quote_start_index = start_quote + 1

            # Search for the end quoute, skipping over escaped quoutes until a real one is found
            while end_quote < 0:
                end_quote = args_string.find(quote_delimiter, end_quote_start_index)
                # Special case for inside strings - need to ignore espaced terminaters
                if end_quote > 0:
                    # found closing quote - is it real, or escaped
                    if _is_escaped(args_string, end_quote):
                        # escaped - ignore it
                        end_quote_start_index = end_quote + 1
                        end_quote = -1
                else:
                    # no more quotes - end of string
                    break

            if end_quote >= 0:
                args.append(args_string[(start_quote+1):end_quote])
                i = end_quote + 1
            else:
                log_text("Error: split_method_args() - string with no end at %s char in [%s]\n" %
                         (start_quote, args_string))
        else:
            # no more quoutes - stop searching in string
            i = len(args_string)
    # end of string reached - return all args found
    return args


def make_json_vector3(x, y, z):
    return {'x': x, 'y': y, 'z': z}


def make_json_vector2(x, y):
    return {'x': x, 'y': y}


def load_and_verify_json(json_string, debug_name):
    # deserialize string to json object, log more helpful error message on failure
    try:
        loaded_json = json.loads(json_string)
        return loaded_json
    except json.decoder.JSONDecodeError as e:
        error_str = str(e)
        log_text("Error: Failed to decode %s: %s" % (debug_name, error_str))
        if error_str.startswith("Invalid control character at"):
            numbers = re.findall(r'\d+', error_str)
            bad_char_index = int(numbers[len(numbers) - 1])
            log_text("Invalid control char at %s ='%s' ord=%s" %
                     (bad_char_index, json_string[bad_char_index], ord(json_string[bad_char_index])))
        return None


# As close to C# version as possible for ease of conversion
class InProgressScratchBlock:
    def __init__(self, requestId = -1):
        self._RequestId = requestId

    def action_completed(self, evt,  **kw):
        self.AdvanceToNextBlock(True)

    def Init(self, requestId = -1):
        self._RequestId = requestId

    def OnReleased(self):
        self.ResolveRequestPromise()
        self.RemoveAllCallbacks()
        self.Init()

    def ReleaseFromPool(self):
        # TODO - C# maintains a pool list, and while we don't need the performance
        # benefits of that here, that list is also used to cancel in-progress blocks later
        self.OnReleased()

    def ResolveRequestPromise(self):
        if self._RequestId >= 0:
            # Calls the JavaScript function resolving the Promise on the block
            self.send_to_webpage("window.resolveCommands[" + str(self._RequestId) + "]();")
            self._RequestId = -1

    def AdvanceToNextBlock(self, success):
        self.ResolveRequestPromise()
        self.ReleaseFromPool()

    def RemoveAllCallbacks(self):
        # TODO (not really added any yet)
        pass


class GameToGameConn:
    def __init__(self):
        self.pending_message_type = None
        self.pending_message = None
        self._pending_message_next_index = -1
        self._pending_message_count = -1
        self.has_received_any_msg_start = False

    def has_pending_message(self):
        return self._pending_message_next_index > 0

    def reset_pending_message(self):
        self.pending_message_type = None
        self.pending_message = None
        self._pending_message_next_index = -1
        self._pending_message_count = -1

    def is_expected_part(self, msg):
        if self.has_pending_message():
            return ((self._pending_message_next_index == msg.messagePartIndex) and
                    (self._pending_message_count == msg.messagePartCount) and
                    (self.pending_message_type == msg.messageType))
        return msg.messagePartIndex == 0

    def append_message(self, msg):
        if not self.is_expected_part(msg):
            # can connect mid part stream, so ignore until we receive the start of any message
            if self.has_received_any_msg_start:
                cozmo.logger.error("AppendMessage.UnexpectedPart - Expected %s/%s '%s', got %s/%s '%s'",
                                   self._pending_message_next_index, self._pending_message_count, self.pending_message_type,
                                   msg.messagePartIndex, msg.messagePartCount, msg.messageType)
            self.reset_pending_message()
            return False
        else:
            if self.has_pending_message():
                self.pending_message += msg.payload
            else:
                self.has_received_any_msg_start = True
                self.pending_message_type = msg.messageType
                self.pending_message = msg.payload
                self._pending_message_count = msg.messagePartCount
            self._pending_message_next_index = msg.messagePartIndex + 1
            return self._pending_message_next_index == self._pending_message_count


class CodeLabInterface():
    def __init__(self, is_vertical_grammar):
        self._is_vertical_grammar = is_vertical_grammar
        self._current_url = None

        # Database of projects stored in a dictionary, UUID as key
        self._user_projects = {}
        self._sample_projects = {}
        self._featured_projects = {}

        self._game_to_game_conn = GameToGameConn()

        self._commands_for_web = queue.Queue()
        self._delayed_commands_for_web = queue.Queue()  # commands delayed until the next workspace page load completes
        self._coz_state_commands_for_web = queue.Queue()

        self._sent_dummy_state = False

        self.load_anki_projects()
        self.load_user_projects()

    def on_connect_to_unity(self):
        self.send_game_to_game("EnableGameToGame", "")
        # Ensure that unity is in the same grammar mode as us
        if self._is_vertical_grammar:
            self.send_game_to_game("UseVerticalGrammar", "")
        else:
            self.send_game_to_game("UseHorizontalGrammar", "")

        if command_args.destination_clone_uuid is not None:
            # save the cloned project into Unity
            try:
                project_data = self._user_projects[command_args.destination_clone_uuid]
            except KeyError:
                project_data = None
                log_text("Error: Skipping save to Unity no project UID %s found" % command_args.destination_clone_uuid)

            if project_data is not None:
                payload = {"requestId":-1,
                           "command":"cozmoSaveUserProject",
                           "argString": project_data["ProjectJSON"],
                           "argUUID": project_data["ProjectUUID"]}
                self.send_game_to_game(WEB_VIEW_CALLBACK, [payload])

    def send_to_webpage(self, message, wait_for_page_load=False):
        if command_args.verbose:
            command = "LoadThenSendToWeb" if wait_for_page_load else "SendToWeb"
            if (message.startswith("window.Scratch.vm.runtime.startHats") or
                message.startswith("window.setPerformanceData")):
                # very spammy/verbose
                pass
            else:
                log_text("%s: %s" % (command, message))
        if wait_for_page_load:
            self._delayed_commands_for_web.put(message)
        else:
            self._commands_for_web.put(message)

    def override_unity_block_handling(self, msg_payload_js):
        command = msg_payload_js["command"]
        try:
            request_id = int(msg_payload_js["requestId"])
        except KeyError:
            # no promise set / required
            request_id = -1

        if not (command_args.connect_to_sdk and command_args.connect_to_robot):
            # There is nothing to resolve the blocks..
            if request_id > 0:
                def _delayed_resolve(delay, request_id):
                    def _sleep_and_do(delay, request_id):
                        time.sleep(delay)
                        self.send_to_webpage("window.resolveCommands[" + str(request_id) + "]();")

                    thread = Thread(target=_sleep_and_do,
                                    kwargs=dict(delay=delay, request_id=request_id))
                    thread.daemon = True  # Force to quit on main quitting
                    thread.start()

                _delayed_resolve(1.0, request_id)

        # You can optionally override any of the C# handling by catching it here, and returning True
        # see the following example that handles the vertical drive block on the SDK side
        # robot = app['robot']  # type: cozmo.robot.Robot
        # if command == "cozVertDrive":
        #     dist_mm = float(msg_payload_js["argFloat"])
        #     speed = float(msg_payload_js["argFloat2"])
        #
        #     action = robot.drive_straight(distance_mm(-dist_mm), speed_mmps(speed), in_parallel=True)
        #     new_block = InProgressScratchBlock(request_id)
        #     action.on_completed(new_block.action_completed)
        #
        #     return True

        return False

    def make_json_cube_state(self, cube_id):
        return {'pos': make_json_vector3(0.0, 0.0, 0.0),
                'camPos': make_json_vector2(0,0),
                'isValid': False,
                'isVisible': False,
                'pitch_d': 0.0,
                'roll_d': 0.0,
                'yaw_d': 0.0 }

    def make_json_face_state(self):
        return {'pos': make_json_vector3(0.0, 0.0, 0.0),
                'camPos': make_json_vector2(0,0),
                'name': '',
                'isVisible': False,
                'expression': ''
                }

    def make_json_cozmo_state(self):
        cozmo_state_dict = {'pos': make_json_vector3(0.0, 0.0, 0.0),
                            'poseYaw_d': 0.0,
                            'posePitch_d': 0.0,
                            'poseRoll_d': 0.0,
                            'liftHeightPercentage': 0.0,
                            'headAngle_d': 0.0,
                            'cube1': self.make_json_cube_state(1),
                            'cube2': self.make_json_cube_state(2),
                            'cube3': self.make_json_cube_state(3),
                            'face': self.make_json_face_state(),
                            'device': {'pitch_d': 0.1,
                                       'roll_d': '2.3',
                                       'yaw_d': 4.5}
                            }

        return "window.setCozmoState('" + json.dumps(cozmo_state_dict) + "');"

    def load_project_dict(self, dest_dict, path_name):
        if command_args.verbose:
            log_text("load_project_dict: path_name = %s" % path_name)
        with open(path_name) as data_file:
            data = json.load(data_file)
            for project_json in data:
                project_uuid = project_json["ProjectUUID"]
                try:
                    project_language = project_json["Language"]
                except KeyError:
                    project_language = None

                try:
                    project_json_file = project_json["ProjectJSONFile"]
                except KeyError:
                    project_json_file = None

                if not project_language or project_language == command_args.locale_to_use:

                    if project_json_file:
                        # We need to load the actual project data from a separate file
                        # directory is hardcoded as featuredProjects for now (no other project type uses this)
                        contents_filename = project_json_file + "_" + command_args.locale_to_use + ".json"
                        contents_pathname = os.path.join(BASE_SRC_SCRATCH_PATH, "featuredProjects", contents_filename)

                        with open(contents_pathname) as contents_data_file:
                            contents_data = json.load(contents_data_file)
                            project_json['ProjectJSON'] = contents_data['ProjectJSON']

                    if command_args.verbose:
                        log_text("load_project_dict Adding %s '%s'" % (project_uuid, project_json["ProjectName"]))
                    dest_dict[project_uuid] = project_json

                    if command_args.clone_samples:
                        # Clone the sample to a new user project on load
                        user_project_json = dict(project_json)
                        user_project_uuid = str(uuid.uuid4())  # Create a new UUID
                        user_project_name = "Clone" + str(len(dest_dict)) + "_of_" + project_json["ProjectName"]
                        user_project_json["ProjectUUID"] = user_project_uuid
                        user_project_json["ProjectName"] = user_project_name
                        log_text("Cloning sample %s '%s' to %s '%s'" % (project_uuid, project_json["ProjectName"],
                                                                        user_project_uuid, user_project_name))
                        self._user_projects[user_project_uuid] = user_project_json
                else:
                    if command_args.verbose:
                        log_text("load_project_dict Skipping %s '%s' (incorrect locale %s)" %
                                 (project_uuid, project_json["ProjectName"], project_language))


    def load_anki_projects(self):
        self.load_project_dict(self._sample_projects,
                               os.path.join(BASE_SRC_SCRATCH_PATH, "sample-projects.json"))
        if command_args.load_test_projects:
            self.load_project_dict(self._sample_projects,
                                   os.path.join(BASE_SRC_SCRATCH_PATH, "test-projects.json"))
        self.load_project_dict(self._featured_projects,
                               os.path.join(BASE_SRC_SCRATCH_PATH, "featured-projects.json"))


    def load_user_projects(self):
        try:
            list_of_directories = [dir for dir in os.listdir(PROJECTS_DIR) if os.path.isdir(os.path.join(PROJECTS_DIR, dir))]
        except FileNotFoundError:
            log_text("load_user_projects - no projects directory")
            return
        for directory in list_of_directories:
            path_name = os.path.join(PROJECTS_DIR, directory, CONTENTS_FILENAME)
            try:
                with open(path_name) as data_file:
                    project_json = json.load(data_file)
                    project_uuid = project_json["ProjectUUID"]
                    if command_args.verbose:
                        log_text("load_user_projects Adding %s '%s'" % (project_uuid, project_json["ProjectName"]))
                    self._user_projects[project_uuid] = project_json
            except FileNotFoundError:
                log_text("load_user_projects - no contents (just history) for deleted file %s" % path_name)

    def save_user_project(self, msg_payload_js, is_connected_to_unity):
        # Save a local (in-memory and on-disk) copy of this data.
        # If we're not connected to Unity then also send additional create / complete
        # messages to keep Javascript side working correctly.
        project_uuid = msg_payload_js["argUUID"]
        project_xml = None  # urllib.parse.unquote(msg_payload_js["argString"])

        project_json = msg_payload_js["argString"]
        if command_args.verbose:
            # verify that this data can be reloaded
            load_and_verify_json(project_json, "verify-on-save")

        utcnow = datetime.datetime.utcnow()
        version_num = kCurrentVersionNum
        is_vertical_str = python_bool_to_js_bool_str(self._is_vertical_grammar)
        project_name = "Unknown"

        if len(project_uuid) == 0:
            if command_args.verbose:
                log_text("save_user_project() - No UUID - new project:")

            if is_connected_to_unity:
                # Let unity assign the UUID, ignore this save
                return
            else:
                # Assign a UUID ourselves (there's no Unity connection)
                project_uuid = str(uuid.uuid4())  # Create a new UUID
                project_name = "Offline Project " + str((len(self._user_projects) + 1))
                self.send_to_webpage("window.newProjectCreated('%s','%s');" % (project_uuid, project_name))

        try:
            project_data = self._user_projects[project_uuid]
            if command_args.verbose:
                log_text("save_user_project() - updating project: '%s'" % project_data)
            project_data['ProjectXML'] = project_xml
            project_data['ProjectJSON'] = project_json
            project_data['DateTimeLastModifiedUTC'] = str(utcnow)
            project_data['VersionNum'] = version_num
            project_data['IsNew'] = False
        except KeyError:
            project_data = {'VersionNum': version_num,
                            'ProjectUUID': project_uuid,
                            'IsVertical': is_vertical_str,
                            'IsNew': False,
                            'ProjectXML': project_xml,
                            'ProjectJSON': project_json,
                            'ProjectName': project_name,
                            'DateTimeCreatedUTC': str(utcnow),
                            'DateTimeLastModifiedUTC': str(utcnow)
                            }
            if command_args.verbose:
                log_text("save_user_project() unknown/new project: '%s'" % project_data)

        # Save out the file

        self._user_projects[project_uuid] = project_data
        self._save_project(project_uuid, project_data, utcnow=utcnow)

        # Let Unity confirm save is complete if it's connected, otherwise do it ourselves
        if not is_connected_to_unity:
            self.send_to_webpage("window.saveProjectCompleted();")

    def request_page_load(self, page_url):
        self._current_url = page_url
        self.send_to_webpage('window.location.href = "' + CODELAB_BASE_URL + page_url + locale_suffix + '";')

    def escape_project_name(self, project_name):
        escaped_name = project_name.replace("\"", "\\\"")
        return escaped_name.replace("'", "\\'")

    def unescape_project_name(self, project_name):
        unescaped_name = project_name.replace("\\'", "'")
        return unescaped_name.replace("\\\"", "\"")

    def escape_project_xml(self, project_xml):
        return project_xml.replace("\"", "\\\"")

    def unescape_project_xml(self, project_xml):
        return project_xml.replace("\\\"", "\"")

    def send_game_to_game_web_view_callback(self, list_of_messages):
        conn = app['sdk_conn']  # type: cozmo.conn.CozmoConnection
        is_connected_to_unity = (conn is not None)

        list_of_messages_to_forward = []
        for sub_message in list_of_messages:
            if command_args.verbose:
                log_text("[HandleGameToGame] sub_message = '%s'" % sub_message)

            forward_message = is_connected_to_unity
            command = ""

            try:
                command = sub_message["command"]
            except json.decoder.JSONDecodeError as e:
                log_text("send_game_to_game: Decode error: %s" % e)
                log_text("sub_message = [%s]" % sub_message)

            if self.override_unity_block_handling(sub_message):
                forward_message = False

            # Handle some commands here, and optionally don't forward them to Unity
            if command == "cozmoSaveUserProject":
                self.save_user_project(sub_message, is_connected_to_unity)
            elif command == "cozmoLoadProjectPage":
                if command_args.verbose:
                    log_text("%s - load projects" % command)
                self.request_page_load("extra/projects.html")
            elif ((command == "cozmoRequestToOpenSampleProject") or
                      (command == "cozmoRequestToOpenUserProject") or
                      (command == "cozmoRequestToOpenFeaturedProject")):
                is_sample = command == "cozmoRequestToOpenSampleProject"
                is_featured = command == "cozmoRequestToOpenFeaturedProject"
                project_uuid = sub_message["argString"]
                try:
                    if is_sample:
                        project = self._sample_projects[project_uuid]
                    elif is_featured:
                        project = self._featured_projects[project_uuid]
                    else:
                        project = self._user_projects[project_uuid]
                except KeyError:
                    log_text("ERROR: Ignoring command %s - no project %s" % (command, project_uuid))
                    project = None

                if is_connected_to_unity and not command_args.use_python_projects:
                    # just trigger the correct workspace load for the external browser
                    if command_args.verbose:
                        log_text("%s - load workspace" % command)
                    url = get_workspace_url(self._is_vertical_grammar)
                    self.request_page_load(url)
                elif project is not None:
                    try:
                        project_xml = project["ProjectXML"]
                    except KeyError:
                        project_xml = None
                    try:
                        project_json = project["ProjectJSON"]
                    except KeyError:
                        project_json = None

                    if project_xml is None and project_json is None:
                        log_text("Error: Ignoring command %s for project %s - ProjectXML + ProjectJSON are None" % (
                        command, project_uuid))
                    else:
                        is_vertical = js_bool_to_python_bool(project["IsVertical"])
                        url = get_workspace_url(is_vertical)
                        self.request_page_load(url)

                        project_name = project["ProjectName"]
                        # TODO - sample/featured project names are localization keys
                        project_name_escaped = self.escape_project_name(project_name)

                        is_sample_str = python_bool_to_js_bool_str(is_sample or is_featured)

                        # If loading python's project versions, but connected to Unity, then ensure that Unity
                        # has an entry for this project (at least for the UUID so that it will see saves)
                        if command_args.use_python_projects and is_connected_to_unity:
                            self.send_game_to_game("EnsureProjectExists",
                                                   project_uuid + "," + project_name + "," + str(is_vertical))

                        load_json = project_json is not None
                        if command_args.verbose:
                            log_text(
                                "%s - load and open %s workspace UUID %s '%s'" % (command, ("JSON" if load_json else "XML"),
                                                                                  project_uuid, project_name))

                        if load_json:
                            project_json_loaded = load_and_verify_json(project_json, "open_project")

                            project_data = {'projectName': project_name_escaped,
                                            'projectJSON': project_json_loaded,
                                            'projectUUID': project_uuid,
                                            'isSampleStr': is_sample_str
                                            }
                            project_data_str = json.dumps(project_data)

                            self.send_to_webpage(
                                "window.openCozmoProjectJSON(" + project_data_str + ");", wait_for_page_load=True)
                        else:
                            # XML
                            project_xml_escaped = self.escape_project_xml(project_xml)

                            self.send_to_webpage(
                                "window.openCozmoProjectXML('" + project_uuid + "','" + project_name_escaped + "',\"" + project_xml_escaped + "\",'" + is_sample_str + "');",
                                wait_for_page_load=True)
            elif command == "cozmoRequestToCreateProject":
                if command_args.verbose:
                    log_text("%s - load workspace" % command)
                self.request_page_load(get_workspace_url(self._is_vertical_grammar))
                if not is_connected_to_unity:
                    self.send_to_webpage("window.ensureGreenFlagIsOnWorkspace();", wait_for_page_load=True)
            elif command == "cozmoDeleteUserProject":
                project_uuid = sub_message["argString"]
                # delete from in-memory dict
                try:
                    del self._user_projects[project_uuid]
                    log_text("%s - removed in-memory entry for uuid %s" % (command, project_uuid))
                except KeyError:
                    log_text("%s - no record in memory for uuid %s" % (command, project_uuid))

                # delete the latest contents (but not the history)
                try:
                    contents_filename = os.path.join(PROJECTS_DIR, project_uuid, CONTENTS_FILENAME)
                    os.remove(contents_filename)
                    log_text("%s - removed file %s" % (command, contents_filename))
                except FileNotFoundError:
                    log_text("%s - no file for %s" % (command, contents_filename))
            elif command == "cozmoStopAll":
                # This also signifies that a workspace page load is complete
                try:
                    # move all delayed commands onto queue
                    while True:
                        delayed_command = self._delayed_commands_for_web.get(block=False)
                        if command_args.verbose:
                            method_name = delayed_command.split("(")[0]
                            log_text("Page Loaded: Moving delayed command %s(...) to command queue" % method_name)
                        self._commands_for_web.put(delayed_command)
                except queue.Empty:
                    pass
            elif command == "cozmoDASLog":
                log_text("Das.Log: %s %s" % (sub_message["argString"], sub_message["argString2"]))
            elif command == "cozmoDASError":
                log_text("Das.Error: %s %s" % (sub_message["argString"], sub_message["argString2"]))
            elif command == "cozmoSwitchProjectTab":
                grammar_type = sub_message["argString"]
                self._is_vertical_grammar = grammar_type != "horizontal"
            elif command == "getCozmoFeaturedProjectList":
                if command_args.use_python_projects or not is_connected_to_unity:
                    # Handle the message ourselves
                    forward_message = False
                    jsCallback = sub_message["argString"]

                    featuredProjectsAsJSON = []
                    for key, value in self._featured_projects.items():
                        is_vertical = js_bool_to_python_bool(value["IsVertical"])
                        if is_vertical == self._is_vertical_grammar:
                            project_copy = dict(value)
                            project_copy["ProjectXML"] = None
                            project_copy["ProjectJSON"] = None
                            featuredProjectsAsJSON.append(project_copy)

                    featuredProjectsAsJSON = sorted(featuredProjectsAsJSON, key=lambda project: int(project["DisplayOrder"]))
                    featuredProjectsAsEncodedJSON = json.dumps(featuredProjectsAsJSON, ensure_ascii=False)

                    self.send_to_webpage(
                        jsCallback + "('" + featuredProjectsAsEncodedJSON + "');")
            elif command == "getCozmoUserAndSampleProjectLists":
                if command_args.use_python_projects or not is_connected_to_unity:
                    # Handle the message ourselves
                    forward_message = False

                    if command_args.verbose:
                        log_text("%s - sending from Python" % command)
                    # return list of projects to the requested method
                    # We copy the projects, and set XML to none, to avoid sending the project contents through
                    # (the contents have to be specially encoded to avoid raising an exception in JS)
                    jsCallback = sub_message["argString"]  # e.g. "window.renderProjects"

                    userProjectsAsJSON = []
                    for key, value in self._user_projects.items():
                        is_vertical = js_bool_to_python_bool(value["IsVertical"])
                        if is_vertical == self._is_vertical_grammar:
                            project_copy = dict(value)
                            project_copy["ProjectXML"] = None
                            project_copy["ProjectJSON"] = None
                            userProjectsAsJSON.append(project_copy)

                    def get_last_modified_time(project_data):
                        try:
                            last_modified_time = project_data['DateTimeLastModifiedUTC']
                            if last_modified_time is not None:
                                timestamp_number = extract_timestamp_number(last_modified_time)
                            else:
                                timestamp_number = 0
                        except KeyError:
                            last_modified_time = None
                            timestamp_number = 0
                        return timestamp_number

                    userProjectsAsJSON = sorted(userProjectsAsJSON,
                                                key=lambda project: -get_last_modified_time(project))
                    sampleProjectsAsJSON = []
                    for key, value in self._sample_projects.items():
                        is_vertical = js_bool_to_python_bool(value["IsVertical"])
                        if is_vertical == self._is_vertical_grammar:
                            project_copy = dict(value)
                            project_copy["ProjectXML"] = None
                            project_copy["ProjectJSON"] = None
                            sampleProjectsAsJSON.append(project_copy)

                    sampleProjectsAsJSON = sorted(sampleProjectsAsJSON, key=lambda project: int(project["DisplayOrder"]))

                    userProjectsAsEncodedJSON = json.dumps(userProjectsAsJSON, ensure_ascii=False)
                    sampleProjectsAsEncodedJSON = json.dumps(sampleProjectsAsJSON, ensure_ascii=False)

                    self.send_to_webpage(
                        jsCallback + "('" + userProjectsAsEncodedJSON + "','" + sampleProjectsAsEncodedJSON + "');")

            if forward_message:
                list_of_messages_to_forward.append(sub_message)

        if is_connected_to_unity and (len(list_of_messages_to_forward) > 0):
            message_wrapper = {'messages': list_of_messages_to_forward}
            self.send_message_payload_to_game(WEB_VIEW_CALLBACK, json.dumps(message_wrapper))

    def send_game_to_game(self, message_type, msg_payload):
        if command_args.verbose:
            log_text("SendToUnity: %s('%s')" % (message_type, msg_payload))

        if message_type == WEB_VIEW_CALLBACK:
            # special case for this type (list of callback messages)
            self.send_game_to_game_web_view_callback(msg_payload)
        else:
            if message_type == "UseVerticalGrammar":
                self._is_vertical_grammar = True
            elif message_type == "UseHorizontalGrammar":
                self._is_vertical_grammar = False
                
            self.send_message_payload_to_game(message_type, msg_payload)

    def send_message_payload_to_game(self, message_type, msg_payload):
        conn = app['sdk_conn']  # type: cozmo.conn.CozmoConnection
        if conn is not None:
            # Send straight on to C# / Unity
            MAX_PAYLOAD_SIZE = 2024 - len(message_type)  # 2048 is an engine-side limit for clad messages, reserve an extra 24 for rest of the tags and members

            part_count = 1
            if len(msg_payload) > MAX_PAYLOAD_SIZE:
                part_count = (((len(msg_payload) - 1) // MAX_PAYLOAD_SIZE) + 1)

            for i in range(part_count):
                text_to_send = msg_payload
                if len(text_to_send) > MAX_PAYLOAD_SIZE:
                    text_to_send = text_to_send[0: MAX_PAYLOAD_SIZE]
                    msg_payload = msg_payload[MAX_PAYLOAD_SIZE:]

                msg = _clad_to_engine_iface.GameToGame(i, part_count, message_type, text_to_send)
                # log_text("sending msg: '%s'" % msg)
                conn.send_msg(msg)

    async def hande_sdk_page_loaded(self, request: web_request.Request):
        if command_args.verbose:
            ("[HandleRequest] sdk_page_loaded")
        return web.Response(text="OK")

    async def handle_sdk_call(self, request: web_request.Request):
        # get the msg from the json request
        text_object = await request.text()
        msg_payload = urllib.parse.unquote(text_object)

        if command_args.verbose:
            log_text("[HandleRequest] sdk_call(%s)" % msg_payload)

        try:
            msg_payload_js = json.loads(msg_payload)
        except json.decoder.JSONDecodeError as e:
            log_text("handle_sdk_call: Decode error: %s" % e)
            log_text("msg_payload = [%s]" % msg_payload)
            msg_payload_js = None

        if msg_payload_js is not None:
            messages = msg_payload_js["messages"]

            # Forward messages on to C# / Unity
            if len(messages) > 0:
                self.send_game_to_game(WEB_VIEW_CALLBACK, messages)

        return web.Response(text="OK")

    async def handle_poll_sdk(self, request):
        # For now just send one command at a time
        # May need to extend this to send multiple with one response (to increase throughput)
        try:
            command = self._commands_for_web.get(block=False)
            return web.Response(text=command)
        except queue.Empty:
            pass

        if not command_args.connect_to_sdk:
            # Testing without an an app/engine/robot connection, so generate
            # a dummy status messages to ensure something is available in JS
            if self._coz_state_commands_for_web.empty():
                if not self._sent_dummy_state:
                    self._coz_state_commands_for_web.put(self.make_json_cozmo_state())
                    self._sent_dummy_state = True

        try:
            command = self._coz_state_commands_for_web.get(block=False)
            return web.Response(text=command)
        except queue.Empty:
            pass

        return web.Response(text="OK")

    def _save_project(self, project_uuid, project_data, save_to_contents=True, save_to_history=True, utcnow=None):
        # Create a projects subdirectory if one didn't already exist
        project_directory = PROJECTS_DIR + project_uuid
        history_directory = project_directory + "/history"
        try:
            os.makedirs(history_directory)
        except FileExistsError:
            pass  # directory already exists - this is fine

        project_json_data = project_data["ProjectJSON"]

        if project_json_data is not None:
            project_json_data = load_and_verify_json(project_json_data, "save_contents")

        if save_to_contents:
            # Overwrite contents file (this is always the latest save)
            path_name = os.path.join(project_directory, CONTENTS_FILENAME)
            with open(path_name, 'w') as out_file:
                json.dump(project_data, out_file, indent=4)

            if project_json_data is not None:
                path_name = os.path.join(project_directory, PROJECTJSON_CONTENTS_FILENAME)
                with open(path_name, 'w') as out_file:
                    json.dump(project_json_data, out_file, indent=4)

                if command_args.debug_project_json_contents:
                    log_text("ProjectJSON =\n" + json.dumps(project_json_data, indent=4) + "\n")

        if save_to_history:
            # Save a new file to history (so we have a full history of every edit)
            if utcnow is None:
                utcnow = datetime.datetime.utcnow()
            base_filename = "_".join(str(utcnow).split())
            file_name = base_filename + '.json'
            path_name = os.path.join(history_directory, file_name)
            with open(path_name, 'w') as out_file:
                json.dump(project_data, out_file, indent=4)

            if project_json_data is not None:
                path_name = os.path.join(history_directory, base_filename + "_" + PROJECTJSON_CONTENTS_FILENAME)
                with open(path_name, 'w') as out_file:
                    json.dump(project_json_data, out_file, indent=4)

    def _update_project(self, user_project):
        project_uuid = user_project["ProjectUUID"]
        try:
            project_data = self._user_projects[project_uuid]
        except KeyError:
            project_data = None

        if project_data is not None:
            updated_project = False
            for key, value in user_project.items():
                if value is not None:
                    # Some fields From C# needs unescaping before we store them, otherwise
                    # they'll end up double-escaped when we send them on to JS
                    # Note ProjectJSON doesn't currently need unescaping
                    if key == "ProjectXML":
                        value = self.unescape_project_xml(value)
                    elif key == "ProjectName":
                        value = self.unescape_project_name(value)

                    try:
                        field_matches = project_data[key] == value
                    except KeyError:
                        project_data[key] = None
                        field_matches = False
                    if not field_matches:
                        if command_args.verbose:
                            log_text("_update_project() - updating field %s from %s to %s" % (key, project_data[key], value))
                        project_data[key] = value
                        updated_project = True

            if updated_project:
                if command_args.verbose:
                    log_text("_update_project() - Updating/saving project %s" % project_uuid)
                # Don't save this to history, it's not an active save from the workspace
                self._save_project(project_uuid, project_data, save_to_history=False)
            else:
                if command_args.verbose:
                    log_text("_update_project() - NOT Updating/saving project %s" % project_uuid)
        else:
            # No existing entry - add it
            self._user_projects[project_uuid] = user_project
            if command_args.verbose:
                log_text("_update_project() - Adding project %s" % project_uuid)
            self._save_project(project_uuid, user_project)

    def on_openCozmoProject_helper(self, project_uuid, project_name, project_xml, project_json, is_sample):
        is_connected_to_unity = (app['sdk_conn'] is not None)
        use_python_projects_with_unity = is_connected_to_unity and not command_args.use_python_projects
        if use_python_projects_with_unity:
            # Forward the message on to the computer's web browser too

            if project_xml is None and project_json is None:
                log_text("Error: on_openCozmoProject_helper project '%s' UUID='%s' - ProjectXML + ProjectJSON are None" % (project_name, project_uuid))
            else:
                wait_for_page_load = False
                url = get_workspace_url(self._is_vertical_grammar)

                if url != self._current_url:
                    wait_for_page_load = True
                    self.request_page_load(url)

                load_json = project_json is not None
                if command_args.verbose:
                    log_text("on_openCozmoProject_helper - load and open %s workspace UUID %s '%s'" %
                             (("JSON" if load_json else "XML"), project_uuid, project_name))

                if load_json:
                    project_json_loaded = load_and_verify_json(project_json, "open_project")

                    project_data = {'projectName': project_name,
                                    'projectJSON': project_json_loaded,
                                    'projectUUID': project_uuid,
                                    'isSampleStr': is_sample
                                    }
                    project_data_str = json.dumps(project_data)

                    self.send_to_webpage(
                        "window.openCozmoProjectJSON(" + project_data_str + ");", wait_for_page_load=wait_for_page_load)
                else:
                    # XML
                    self.send_to_webpage(
                        "window.openCozmoProjectXML('" + project_uuid + "','" + project_name + "',\"" + project_xml + "\",'" + is_sample + "');",
                        wait_for_page_load=wait_for_page_load)

        is_sample = js_bool_to_python_bool(is_sample)
        if not is_sample and not use_python_projects_with_unity:
            is_vertical_str = python_bool_to_js_bool_str(self._is_vertical_grammar)
            project_data = {'VersionNum': None,
                            'ProjectUUID': project_uuid,
                            'IsVertical': is_vertical_str,
                            'IsNew': None,
                            'ProjectXML': project_xml,
                            'ProjectJSON': project_json,
                            'ProjectName': project_name,
                            'DateTimeCreatedUTC': None,
                            'DateTimeLastModifiedUTC': None
                            }
            if command_args.verbose:
                log_text("on_openCozmoProject_helper() - project_data = %s" % str(project_data))
            self._update_project(project_data)

    def on_recv_window_setPerformanceData(self, loaded_json):
        pass

    def on_recv_window_openCozmoProjectJSON(self, loaded_json):
        project_xml = None
        project_name = loaded_json["projectName"]
        project_json = json.dumps(loaded_json["projectJSON"])
        project_uuid = loaded_json["projectUUID"]
        is_sample = loaded_json["isSampleStr"]
        self.on_openCozmoProject_helper(project_uuid, project_name, project_xml, project_json, is_sample)

    def on_recv_window_openCozmoProjectXML(self, project_uuid, project_name, project_xml, is_sample):
        project_json = None
        self.on_openCozmoProject_helper(project_uuid, project_name, project_xml, project_json, is_sample)

    def on_recv_window_renderProjects(self, user_projects, sample_projects):
        is_connected_to_unity = (app['sdk_conn'] is not None)
        use_python_projects_with_unity = is_connected_to_unity and not command_args.use_python_projects
        if use_python_projects_with_unity:
            # ignore
            return

        # Update our data on user projects
        user_projects = user_projects.replace('\\"', '"')
        try:
            user_projects_json = json.loads(user_projects)
        except json.decoder.JSONDecodeError as e:
            log_text("on_recv_window_renderProjects: Decode error: %s" % e)
            log_text("user_projects =\n" + str(user_projects) + "\n")
            return

        for user_project in user_projects_json:
            self._update_project(user_project)

    def on_recv_window_newProjectCreated(self, project_uuid, project_name):
        is_vertical_str = python_bool_to_js_bool_str(self._is_vertical_grammar)
        project_data = {'VersionNum': None,
                        'ProjectUUID': project_uuid,
                        'IsVertical': is_vertical_str,
                        'IsNew': None,
                        'ProjectXML': None,
                        'ProjectJSON': None,
                        'ProjectName': project_name,
                        'DateTimeCreatedUTC': None,
                        'DateTimeLastModifiedUTC': None
                        }
        if command_args.verbose:
            log_text("on_recv_window_newProjectCreated() - project_data = %s" % str(project_data))
        self._update_project(project_data)

    def handle_method_from_unity(self, message_payload_str):
        # parse the method call from unity to Javascript so we can intercept it
        meth_args = message_payload_str.split("(")
        if len(meth_args) == 0:
            return
        method_name = meth_args[0]

        if method_name:
            IGNORE_FROM_UNITY = {"window.saveProjectCompleted",
                                 "window.Scratch.vm.runtime.startHats",
                                 "window.setPerformanceData"
                                 }
            if method_name in IGNORE_FROM_UNITY or method_name.startswith("window.resolveCommands"):
                # if command_args.verbose:
                #     log_text("handle_method_from_unity() - ignoring %s" % method_name)
                return

            payload_str = message_payload_str[len(method_name)+1:-2]  # assumes "method_name(payload);"
            args = split_method_args(payload_str)

            handler_name = "on_recv_" + method_name.replace(".", "_")

            if (len(args) > 1) or ((len(args) > 0) and (len(args[0]) > 0)):
                if command_args.verbose:
                    log_text("handle_method_from_unity() - %s(...):" % method_name)
                    log_text("args:")
                    arg_i = 0
                    for arg in args:
                        log_text("    %s: '%s'\n" % (arg_i, str(arg)))
                        arg_i += 1
            else:
                # This is expected for methods that take no args - e.g. saveProjectCompleted
                if command_args.verbose:
                    log_text("Warning: handle_method_from_unity() - no args for '%s'" % method_name)

            handler = getattr(self, handler_name, None)

            if handler is not None:
                if ((handler_name == "on_recv_window_openCozmoProjectJSON") or
                        (handler_name == "on_recv_window_setPerformanceData")):
                    # special case - we need to convert the string into a json object
                    loaded_json = load_and_verify_json(payload_str, handler_name)
                    handler(loaded_json)
                else:
                    handler(*args)
            else:
                log_text("Warning: handle_method_from_unity() - no handler for '%s'" % handler_name)
        else:
            log_text("Error: handle_method_from_unity() with no method_name!")

    def handle_game_to_game_contents(self, message_type, message_payload):
        if message_type == "EvaluateJS":
            if message_payload.startswith("window.setCozmoState"):
                # special-case, we only ever need to cache the latest state and send that on when polled
                # in practice we should be polled frequently enough that it rarely happens
                if not self._coz_state_commands_for_web.empty():
                    # discard the one item, so we never grow this queue here
                    old_command = self._coz_state_commands_for_web.get(block=False)
                    #log_text("discarding old_command len %s", len(old_command))
                self._coz_state_commands_for_web.put(message_payload)
                pass
            else:
                message_payload_str = str(message_payload)
                if (("window.Scratch.vm.runtime.startHats" in message_payload_str) or
                        ("window.setPerformanceData" in message_payload_str)):
                    # Too verbose / spammy - ignore
                    pass
                else:
                    if command_args.verbose:
                        log_text("\nFromUnity:[%s]" % str(message_payload_str))

                self.handle_method_from_unity(message_payload_str)

                self.send_to_webpage(message_payload)
        else:
            log_text("handle_game_to_game_contents() - Unexpected GameToGame message type '%s'", message_type)


    def handle_game_to_game_msg(self, evt, *, msg):
        # Handles any size of message, including large multi-part messages
        # Only fully assembled messages are passed on to handle_game_to_game_contents
        if self._game_to_game_conn.append_message(msg):
            self.handle_game_to_game_contents(self._game_to_game_conn.pending_message_type,
                                         self._game_to_game_conn.pending_message)
            self._game_to_game_conn.reset_pending_message()

    async def handle_anki_comms_settings(self, request):
        return web.Response(text="gEnableSdkConnection = true;");

    async def upload_file(self, conn : cozmo.conn.CozmoConnection, file_path, file_name):
        # the theoretical max is 2K for entire packet
        # = u16 byte count, 2xu16 part x/x, filename+u8len, u8 filetype
        # = 2u8 + 4u8 + u8 + u8 + strlen = 8u8 + strlen
        extra_message_size = 16  # 8 bytes for this message, and 8 for rest of the messaging system
        chunk_size = 2048 - (extra_message_size + len(file_name))
        chunks = []
        num_file_bytes = 0
        try:
            with open(file_path, "rb") as file_object:
                chunk = file_object.read(chunk_size)
                while chunk:
                    num_file_bytes += len(chunk)
                    chunks.append(chunk)
                    chunk = file_object.read(chunk_size)
        except FileNotFoundError as e:
            log_text("upload_file() - File not found: '%s'" % e)
            return

        num_chunks = len(chunks)

        # upload our max clad size
        for i in range(num_chunks):
            msg = _clad_to_engine_iface.TransferFile(fileBytes=chunks[i],
                                                     filePart=i,
                                                     numFileParts=num_chunks,
                                                     filename=file_name,
                                                     fileType=_clad_to_engine_iface.FileType.CodeLab)
            if conn is not None:
                conn.send_msg(msg)
            await asyncio.sleep(0.001)  # 0.001 = 1ms

        log_text("Uploaded '%s' in %s chunks (%s bytes)" % (file_name, num_chunks, "{:,}".format(num_file_bytes)))
        return num_file_bytes

    def is_valid_scratch_file(self, filename):
        for extension in VALID_SCRATCH_EXTENSIONS:
            if filename.endswith(extension):
                return True
        return False

    async def upload_all_files(self, conn: cozmo.conn.CozmoConnection):
        start_time = time.time()
        # min_file_mod_time is the minimum/earliest file modification time that we
        # should upload, because we've already uploaded all files older than that
        # (it's read from the CACHE_WRITE_TIME_PATH (if present) which is set
        # each time you upload the files) - it defaults to -1 which will all file
        # times are automatically greater than, so if no cache file is present
        # then all files will be uploaded.
        min_file_mod_time = -1.0
        try:
            with open(CACHE_WRITE_TIME_PATH, "r") as file_object:
                contents = file_object.read()
                min_file_mod_time = float(contents)

        except FileNotFoundError as e:
            # No cache time stored - write all files
            # Might as well clear the cache first
            log_text("No '%s' file - deleting cache and uploading all files" % CACHE_WRITE_TIME_PATH)
            await self.delete_cache(conn)
            pass

        num_files = 0
        total_bytes = 0
        for root, dirs, files in os.walk(BASE_SRC_SCRATCH_PATH):
            for file in files:
                full_path = os.path.join(root, file)
                last_mod_time = os.path.getmtime(full_path)
                if self.is_valid_scratch_file(file) and (last_mod_time > min_file_mod_time):
                    offset_path = "CodeLabDev" + full_path[len(BASE_SRC_SCRATCH_PATH):]
                    num_bytes = await self.upload_file(conn, full_path, offset_path)
                    total_bytes += num_bytes
                    num_files += 1

        # update the cache write file with the time just before we uploaded anything
        with open(CACHE_WRITE_TIME_PATH, "w") as file_object:
            file_object.write(str(start_time))

        log_text("Uploaded %s files (%s bytes) in %s seconds" % (num_files, "{:,}".format(total_bytes), time.time()-start_time))

    async def delete_cache(self, conn: cozmo.conn.CozmoConnection):
        log_text("Deleting cache")
        try:
            os.remove(CACHE_WRITE_TIME_PATH)
        except FileNotFoundError:
            pass

        # Special case CodeLab TransferFile message with 0 bytes and 0/0 parts triggers
        # a clear/delete of that cache directory
        msg = _clad_to_engine_iface.TransferFile(fileBytes=[],
                                                 filePart=0,
                                                 numFileParts=0,
                                                 filename=BASE_SRC_SCRATCH_PATH,
                                                 fileType=_clad_to_engine_iface.FileType.CodeLab)
        if conn is not None:
            conn.send_msg(msg)


_default_to_vertical_grammar = not (command_args.show_horizontal or command_args.use_horizontal)
_code_lab = CodeLabInterface(_default_to_vertical_grammar)


def load_codelab_file(file_contents):
    unencoded_contents = urllib.parse.unquote_plus(file_contents)

    if not unencoded_contents.startswith(CODELAB_FILE_HEADER):
        log_text("Error: load_codelab_file - file doesn't start with %s" % CODELAB_FILE_HEADER)
        return None

    unencoded_contents = unencoded_contents[len(CODELAB_FILE_HEADER):]

    try:
        json_contents = json.loads(unencoded_contents)
    except json.decoder.JSONDecodeError as e:
        log_text("load_codelab_file: Decode error: %s" % e)
        log_text("unencoded_contents = [%s]" % unencoded_contents)
        return None

    return json_contents


def save_codelab_file(project_data):
    project_name = project_data["ProjectName"]
    project_uuid = project_data["ProjectUUID"]

    project_filename = project_name.replace(" ", "_")
    project_filename = project_filename.replace("/", "_")
    project_extension = ".codelab"
    filename = project_filename + project_extension
    suffix = 1
    while (os.path.exists(filename)):
        filename = project_filename + "(" + str(suffix) + ")" + project_extension
        suffix += 1

    log_text("Exporting project '%s' UUID %s to %s" % (project_name, project_uuid, filename))

    encoded_contents = urllib.parse.quote_plus(json.dumps(project_data))
    with open(filename, 'w') as out_file:
        out_file.write(CODELAB_FILE_HEADER)
        out_file.write(encoded_contents)


# Handle calls from Javascript as post requests
app.router.add_post(CODELAB_BASE_URL + 'sdk_call', _code_lab.handle_sdk_call)
app.router.add_post(CODELAB_BASE_URL + 'poll_sdk', _code_lab.handle_poll_sdk)
app.router.add_post(CODELAB_BASE_URL + 'sdk_page_loaded', _code_lab.hande_sdk_page_loaded)
# HTML pages in extra (e.g. projects) all direct to their subdirectory, so redirect them
app.router.add_post(CODELAB_BASE_URL + 'extra/sdk_call', _code_lab.handle_sdk_call)
app.router.add_post(CODELAB_BASE_URL + 'extra/poll_sdk', _code_lab.handle_poll_sdk)
app.router.add_post(CODELAB_BASE_URL + 'extra/sdk_page_loaded', _code_lab.hande_sdk_page_loaded)
# We override anki_comms_settings.js to automatically enable the SDK connection
app.router.add_get(CODELAB_BASE_URL + 'js/anki_comms_settings.js', _code_lab.handle_anki_comms_settings)


async def handle_DeleteCache(request):
    await _code_lab.delete_cache(app['sdk_conn'])
    return web.Response(text="OK")
app.router.add_post(DEVCONN_BASE_URL + 'DeleteCache', handle_DeleteCache)


async def handle_UploadToCache(request):
    await _code_lab.upload_all_files(app['sdk_conn'])
    return web.Response(text="OK")
app.router.add_post(DEVCONN_BASE_URL + 'UploadToCache', handle_UploadToCache)


async def handle_GetDebugInfo(request: web.Request):
    global pending_log_text
    if len(pending_log_text) > 0:
        return_text = pending_log_text.replace("\n", "<br>")
        pending_log_text = ""
        return web.Response(text=return_text)
    return web.Response(text="")
app.router.add_post(DEVCONN_BASE_URL + 'GetDebugInfo', handle_GetDebugInfo)


async def handle_CallGameToGame(request: web.Request):
    json_object = await request.json()
    method_name = json_object["method_name"]
    param_text = json_object["param_text"]
    _code_lab.send_game_to_game(method_name, param_text)
    return web.Response(text="OK")
app.router.add_post(DEVCONN_BASE_URL + 'CallGameToGame', handle_CallGameToGame)


def _create_default_image(image_width, image_height):
    '''Create a place-holder PIL image to use until we have a live feed from Cozmo'''
    image_bytes = bytearray([0x70, 0x70, 0x70]) * image_width * image_height
    image = Image.frombytes('RGB', (image_width, image_height), bytes(image_bytes))

    # get a drawing context to draw Text onto default image
    dc = ImageDraw.Draw(image)
    dc.text((10, 10), "Cozmo Camera Feed", fill=(255, 255, 255, 255), font=None)

    return image


_default_camera_image = _create_default_image(320, 240)


def _make_uncached_headers():
    headers = {'Pragma-Directive': 'no-cache',
               'Cache-Directive': 'no-cache',
               'Cache-Control':  'no-cache, no-store, must-revalidate',
               'Pragma':  'no-cache',
               'Expires': '0'}
    return headers


def _serve_pil_image(pil_img, serve_as_jpeg=False, jpeg_quality=70):
    '''Convert PIL image to relevant image file and send it'''
    img_io = BytesIO()

    if serve_as_jpeg:
        pil_img.save(img_io, 'JPEG', quality=jpeg_quality)
    else:
        pil_img.save(img_io, 'PNG')
    img_io.seek(0)

    return web.Response(body=img_io, headers=_make_uncached_headers())


async def handle_EnableCameraFeed(request):
    json_object = await request.json()
    enabled = json_object["enabled"]
    robot = app['robot']
    if robot is not None:
        robot.camera.image_stream_enabled = enabled
    return web.Response(text="OK")
app.router.add_post(DEVCONN_BASE_URL + 'EnableCameraFeed', handle_EnableCameraFeed)


async def handle_get_image(request):
    # Called very frequently from Javascript to request the latest camera image
    robot = app['robot']  # type: cozmo.robot.Robot
    if robot is not None:
        image = robot.world.latest_image
        if image:
            image = image.annotate_image(scale=1.5)
            return _serve_pil_image(image)
    # Fallback to default image (for first frame, or when not connected)
    return _serve_pil_image(_default_camera_image)
app.router.add_get(DEVCONN_BASE_URL + 'cozmoImage', handle_get_image)


# Statically serve all of the scratch files
app.router.add_static(CODELAB_BASE_URL, path=BASE_SRC_SCRATCH_PATH+'/', name='scratch_dir')
app.router.add_static(DEVCONN_BASE_URL, path='./static/', name='dev_dir')


def _delayed_open_web_browser(url, delay, new=0, autoraise=True, specific_browser=None):
    '''
    Spawn a thread and call sleep_and_open_web_browser from within it so that main thread can keep executing at the
    same time. Insert a small sleep before opening a web-browser
    this gives Flask a chance to start running before the browser starts requesting data from Flask.
    '''
    def _sleep_and_open_web_browser(url, delay, new, autoraise, specific_browser):
        time.sleep(delay)
        browser = webbrowser

        # E.g. On macOS the following would use the Chrome browser app from that location
        # specific_browser = 'open -a /Applications/Google\ Chrome.app %s'
        if specific_browser:
            browser = webbrowser.get(specific_browser)

        browser.open(url, new=new, autoraise=autoraise)

    thread = Thread(target=_sleep_and_open_web_browser,
                    kwargs=dict(url=url, new=new, autoraise=autoraise, delay=delay, specific_browser=specific_browser))
    thread.daemon = True  # Force to quit on main quitting
    thread.start()


def handle_log_line(evt, *, msg):
    ignore_event_names = ["AnimationStreamer.Update.SendBufferNotEmpty",
                          "BehaviorManager.DisableReactionsWithLock.DisablingWithLock",
                          "BlockFilter.UpdateConnecting",
                          "CozmoAPI.CozmoInstanceRunner.overtime",
                          "cozmo_engine.robot_msg_queue.recent_incoming_size",
                          "cozmo_engine.update.run.slow",
                          "CubeLightComponent.OnlyGameLayerEnabled",
                          "[@CubeLightComponent] CubeLightComponent.PlayLightAnim.CantBeOverridden",
                          "GetResponseForAnimationTrigger.Found",
                          "NavMeshQuadTreeProcessor.FillBorder",
                          "robot.play_area_size",
                          "robot.accessory_connection",
                          "robot.vision.dropped_frame_overall_count",
                          "robot.vision.dropped_frame_recent_count",
                          "robot.vision.profiler.VisionSystem.Profiler",
                          "Robot.HandleActiveObjectConnectionState",
                          "Robot.HandleConnectedToObject",
                          "Robot.HandleDisconnectedFromObject",
                          "RobotImplMessaging.HandleImageChunk",
                          "VisionComponent.SetCameraSettings",
                          "[@VisionComponent] DroppedFrameStats",
                          "[@VisionComponent] VisionComponent.VisionComponent",
                          "[@VisionSystem] MotionDetector.DetectMotion.FoundCentroid",
                          "VisionSystem.Profiler",
                          "Animation.Init.SendBufferNotEmpty"]

    msg_line = msg.line.strip()

    for event_name in ignore_event_names:
        if event_name in msg_line:
            # ignore this msg
            return

    log_level = msg.logLevel
    log_text("[Engine][%s] %s" % (log_level, msg_line))


def set_console_var(conn, var_name, new_value):
    # Set a Console Variable in the Engine (Only works in non-SHIPPING
    # builds of the App, otherwise the variables are const and the
    # message is ignored).
    msg = _clad_to_engine_iface.SetDebugConsoleVarMessage(varName=var_name,
                                                          tryValue=new_value)
    conn.send_msg(msg)


def enable_log_channel(conn, channel_name, enable=True):
    new_value = "1" if enable else "0"
    set_console_var(conn, channel_name, new_value)


def enable_all_log_channels(conn, enable=True):
    channel_names = ['actions',
                     'aiinfoanalysis',
                     'aiwhiteboard',
                     'animations',
                     'audio',
                     'behaviors',
                     'blockpool',
                     'blockworld',
                     'bodylightcomponent',
                     'buildpyramid',
                     'cubelightcomponent',
                     'cpuprofiler',
                     'facerecognizer',
                     'mood',
                     'network',
                     'nvstorage',
                     'poseconfirmer',
                     'profiler',
                     'reactiontriggers',
                     'uicomms',
                     'unfiltered',
                     'unity',
                     'unlockcomponent',
                     'unnamed',
                     'visioncomponent',
                     'visionsystem']

    for channel_name in channel_names:
        enable_log_channel(conn, channel_name, enable)


def export_codelab_files(file_id):
    if file_id == "all":
        log_text("Exporting all user projects:")
        for key, project_data in _code_lab._user_projects.items():
            save_codelab_file(project_data)

        log_text("Exporting all sample projects:")
        for key, project_data in _code_lab._sample_projects.items():
            is_vertical = js_bool_to_python_bool(project_data["IsVertical"])
            if is_vertical:
                save_codelab_file(project_data)

        log_text("Exporting all featured projects:")
        for key, project_data in _code_lab._featured_projects.items():
            save_codelab_file(project_data)

        return

    project_data = None
    if len(file_id) == 36 and file_id.count("-") == 4:
        # file_id looks like a valid UUID - try looking up by it directly
        try:
            project_data = _code_lab._user_projects[file_id]
        except KeyError:
            log_text("Error: Export: no project with UUID %s" % file_id)

    if project_data is None:
        file_id_lower = file_id.lower()
        # Fall back to finding by project name
        for key, value in _code_lab._user_projects.items():
            if value["ProjectName"].lower() == file_id_lower:
                #project_uuid = key
                project_data = value
                break

    if project_data:
        save_codelab_file(project_data)
    else:
        sys.exit("Export failed to find project matching UUID or name to '%s'" % file_id)


if __name__ == '__main__':
    cozmo.robot.Robot.drive_off_charger_on_connect = False

    if command_args.import_codelab_file is not None:
        filename = command_args.import_codelab_file
        try:
            with open(filename) as data_file:
                file_contents = data_file.read()
                project_data = load_codelab_file(file_contents)
                project_uuid = project_data["ProjectUUID"]
                project_name = project_data["ProjectName"]
                _code_lab._save_project(project_uuid, project_data)
                log_text("File '%s' imported to '%s' UUID %s" % (filename, project_name, project_uuid))
        except FileNotFoundError:
            log_text("Error file '%s' not found" % filename)

    if command_args.export_codelab_file is not None:
        export_codelab_files(command_args.export_codelab_file)

    if (command_args.source_clone_uuid is not None) and (command_args.destination_clone_uuid is not None):
        try:
            source_project = _code_lab._user_projects[command_args.source_clone_uuid]
        except KeyError:
            log_text("Error: Clone: no project with UUID %s" % command_args.source_clone_uuid)
            source_project = None

        if source_project is not None:
            dest_project = dict(source_project)
            dest_project["ProjectUUID"] = command_args.destination_clone_uuid
            _code_lab._save_project(command_args.destination_clone_uuid, dest_project)

    try:
        app_loop = asyncio.get_event_loop()
        if command_args.connect_to_sdk:
            sdk_conn = cozmo.connect_on_loop(app_loop)
            app['sdk_conn'] = sdk_conn

            if command_args.connect_to_robot:
                # Wait for the robot to become available and add it to the app object.
                robot = app_loop.run_until_complete(sdk_conn.wait_for_robot())  # type: cozmo.robot.Robot
                app['robot'] = robot
            else:
                app['robot'] = None

            sdk_conn.add_event_handler(cozmo._clad._MsgGameToGame, _code_lab.handle_game_to_game_msg)

            if command_args.log_from_engine:
                sdk_conn.add_event_handler(cozmo._clad._MsgDebugAppendConsoleLogLine, handle_log_line)
                enable_all_log_channels(sdk_conn, False)
                enable_log_channel(sdk_conn, "unity", True)
                set_console_var(sdk_conn, "EnableCladLogger", "1")

            _code_lab.on_connect_to_unity()
        else:
            app['sdk_conn'] = None
            app['robot'] = None
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)

    host_port = 9090  # 8080 clashes with the Cozmo USB tunnel
    if PAGE_TO_LOAD is not None:
        _delayed_open_web_browser("http://127.0.0.1:" + str(host_port) + CODELAB_BASE_URL + PAGE_TO_LOAD, delay=1.0)
    if command_args.open_dev_tab:
        _delayed_open_web_browser("http://127.0.0.1:" + str(host_port) + DEVCONN_BASE_URL + "index.html", delay=1.0)
    web.run_app(app, host="127.0.0.1", port=host_port)

