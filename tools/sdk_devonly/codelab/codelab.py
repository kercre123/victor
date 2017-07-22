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
import json
import os
import queue
import sys
from threading import Thread
import time
import urllib
import webbrowser

try:
    from aiohttp import web, web_request
except ImportError:
    sys.exit("Cannot import from aiohttp. Do `pip3 install --user aiohttp` to install")


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
    arg_parser.add_argument('-vert', '--show_vertical',
                        dest='show_vertical',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Open the vertical grammar page')

    options = arg_parser.parse_args()
    return options


command_args = parse_command_args()


pending_log_text = ""
def log_text(text_to_log):
    global pending_log_text
    pending_log_text += text_to_log + "\n"
    # limit pending_log_text in event of it not being polled?
    print(text_to_log)


if command_args.locale_to_use not in LOCALE_OPTIONS:
    sys.exit("Unexpected locale %s" % command_args.locale_to_use)

locale_suffix = "?locale=" + command_args.locale_to_use


if command_args.page_to_load is not None:
    PAGE_TO_LOAD = command_args.page_to_load
elif command_args.show_horizontal:
    PAGE_TO_LOAD = "index.html" + locale_suffix
elif command_args.show_vertical:
    PAGE_TO_LOAD = "index_vertical.html" + locale_suffix
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


_commands_for_web = queue.Queue()
_coz_state_commands_for_web = queue.Queue()


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
            global _commands_for_web
            _commands_for_web.put("window.resolveCommands[" + str(self._RequestId) + "]();")
            self._RequestId = -1

    def AdvanceToNextBlock(self, success):
        self.ResolveRequestPromise()
        self.ReleaseFromPool()

    def RemoveAllCallbacks(self):
        # TODO (not really added any yet)
        pass


def override_unity_block_handling(msg_payload : str):
    try:
        msg_payload_js = json.loads(msg_payload)
    except json.decoder.JSONDecodeError as e:
        log_text("override_unity_block_handling: Decode error: %s" % e)
        log_text("msg_payload = [%s]" % msg_payload)
        return False

    command = msg_payload_js["command"]
    try:
        request_id = int(msg_payload_js["requestId"])
    except KeyError:
        # no promise set / required
        request_id = -1

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


def send_game_to_game(message_type, msg_payload):
    log_text("SendToUnity: %s('%s')" % (message_type, msg_payload))
    conn = app['sdk_conn']  # type: cozmo.conn.CozmoConnection

    if conn is None:
        pass  # testing without an SDK connection (or robot)
    else:
        # Send straight on to C# / Unity
        MAX_PAYLOAD_SIZE = 2024 - len(message_type)  # 2048 is an engine-side limit for clad messages, reserve an extra 24 for rest of the tags and members

        part_count = 1
        if len(msg_payload) > MAX_PAYLOAD_SIZE:
            part_count = (((len(msg_payload) - 1) // MAX_PAYLOAD_SIZE) + 1)

        for i in range(part_count):
            text_to_send = msg_payload
            if len(text_to_send) > MAX_PAYLOAD_SIZE:
                text_to_send = text_to_send[0 : MAX_PAYLOAD_SIZE]
                msg_payload = msg_payload[MAX_PAYLOAD_SIZE:]

            msg = _clad_to_engine_iface.GameToGame(i, part_count, message_type, text_to_send)
            #log_text("sending msg: '%s'" % msg)
            conn.send_msg(msg)


async def handle_sdk_call(request: web_request.Request):
    #log_text("handle_sdk_call")
    # get the msg from the json request
    text_object = await request.text()
    msg_payload = urllib.parse.unquote(text_object)

    if not override_unity_block_handling(msg_payload):
        # Send straight on to C# / Unity
        message_type = "WebViewCallback"
        send_game_to_game(message_type, msg_payload)

    return web.Response(text="OK")


async def handle_poll_sdk(request):
    global _commands_for_web

    # For now just send one command at a time
    # May need to extend this to send multiple with one response
    try:
        command = _commands_for_web.get(block=False)
        return web.Response(text=command)
    except queue.Empty:
        pass

    try:
        command = _coz_state_commands_for_web.get(block=False)
        return web.Response(text=command)
    except queue.Empty:
        pass

    return web.Response(text="OK")


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


_game_to_game_conn = GameToGameConn()


def handle_game_to_game_contents(message_type, message_payload):
    if message_type == "EvaluateJS":
        if message_payload.startswith("window.setCozmoState"):
            # special-case, we only ever need to cache the latest state and send that on when polled
            # in practice we should be polled frequently enough that it rarely happens
            if not _coz_state_commands_for_web.empty():
                # discard the one item, so we never grow this queue here
                old_command = _coz_state_commands_for_web.get(block=False)
                #cozmo.logger.info("discarding old_command len %s", len(old_command))
            _coz_state_commands_for_web.put(message_payload)
            pass
        else:
            message_payload_str = str(message_payload)
            if "window.Scratch.vm.runtime.startHats" not in message_payload_str:
                log_text("FromUnity:[%s]" % str(message_payload_str))
            global _commands_for_web
            _commands_for_web.put(message_payload)
    else:
        cozmo.logger.error("Unexpected GameToGame message type '%s'", message_type)


def handle_game_to_game_msg(evt, *, msg):
    # Handles any size of message, including large multi-part messages
    # Only fully assembled messages are passed on to handle_game_to_game_contents
    global _game_to_game_conn
    if _game_to_game_conn.append_message(msg):
        handle_game_to_game_contents(_game_to_game_conn.pending_message_type,
                                     _game_to_game_conn.pending_message)
        _game_to_game_conn.reset_pending_message()


async def handle_anki_comms_settings(request):
    return web.Response(text="gEnableSdkConnection = true;");


async def upload_file(conn : cozmo.conn.CozmoConnection, file_path, file_name):
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
        log_text("File not found: '%s'" % e)
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


# Handle calls from Javascript as post requests
app.router.add_post('/codelab/sdk_call', handle_sdk_call)
app.router.add_post('/codelab/poll_sdk', handle_poll_sdk)
app.router.add_get('/codelab/js/anki_comms_settings.js', handle_anki_comms_settings)


valid_extensions = [".css", ".cur", ".gif", ".html", ".js", ".jpg", ".jpeg", ".json", ".otf", ".png", ".svg"]

def is_valid_file(filename):
    for extension in valid_extensions:
        if filename.endswith(extension):
            return True
    return False


base_src_scratch_path = "../../../unity/Cozmo/Assets/StreamingAssets/Scratch"
cache_write_time_path = "./.codelab_cache_write"
async def upload_all_files(conn: cozmo.conn.CozmoConnection):
    start_time = time.time()
    # min_file_mod_time is the minimum/earliest file modification time that we
    # should upload, because we've already uploaded all files older than that
    # (it's read from the cache_write_time_path (if present) which is set
    # each time you upload the files) - it defaults to -1 which will all file
    # times are automatically greater than, so if no cache file is present
    # then all files will be uploaded.
    min_file_mod_time = -1.0
    try:
        with open(cache_write_time_path, "r") as file_object:
            contents = file_object.read()
            min_file_mod_time = float(contents)

    except FileNotFoundError as e:
        # No cache time stored - write all files
        # Might as well clear the cache first
        log_text("No '%s' file - deleting cache and uploading all files" % cache_write_time_path)
        delete_cache(conn)
        pass

    num_files = 0
    total_bytes = 0
    for root, dirs, files in os.walk(base_src_scratch_path):
        for file in files:
            full_path = os.path.join(root, file)
            last_mod_time = os.path.getmtime(full_path)
            if is_valid_file(file) and (last_mod_time > min_file_mod_time):
                offset_path = "CodeLabDev" + full_path[len(base_src_scratch_path):]
                num_bytes = await upload_file(conn, full_path, offset_path)
                total_bytes += num_bytes
                num_files += 1

    # update the cache write file with the time just before we uploaded anything
    with open(cache_write_time_path, "w") as file_object:
        file_object.write(str(start_time))

    log_text("Uploaded %s files (%s bytes) in %s seconds" % (num_files, "{:,}".format(total_bytes), time.time()-start_time))


async def delete_cache(conn: cozmo.conn.CozmoConnection):
    log_text("Deleting cache")
    try:
        os.remove(cache_write_time_path)
    except FileNotFoundError:
        pass

    # Special case CodeLab TransferFile message with 0 bytes and 0/0 parts triggers
    # a clear/delete of that cache directory
    msg = _clad_to_engine_iface.TransferFile(fileBytes=[],
                                             filePart=0,
                                             numFileParts=0,
                                             filename=base_src_scratch_path,
                                             fileType=_clad_to_engine_iface.FileType.CodeLab)
    if conn is not None:
        conn.send_msg(msg)


async def handle_DeleteCache(request):
    await delete_cache(app['sdk_conn'])
    return web.Response(text="OK")
app.router.add_post('/devconn/DeleteCache', handle_DeleteCache)


async def handle_UploadToCache(request):
    await upload_all_files(app['sdk_conn'])
    return web.Response(text="OK")
app.router.add_post('/devconn/UploadToCache', handle_UploadToCache)


async def handle_GetDebugInfo(request: web.Request):
    global pending_log_text
    if len(pending_log_text) > 0:
        return_text = pending_log_text.replace("\n", "<br>")
        pending_log_text = ""
        return web.Response(text=return_text)
    return web.Response(text="")
app.router.add_post('/devconn/GetDebugInfo', handle_GetDebugInfo)


async def handle_CallGameToGame(request: web.Request):
    json_object = await request.json()
    method_name = json_object["method_name"]
    param_text = json_object["param_text"]
    send_game_to_game(method_name, param_text)
    return web.Response(text="OK")
app.router.add_post('/devconn/CallGameToGame', handle_CallGameToGame)


# Statically serve all of the scratch files
app.router.add_static('/codelab', path=base_src_scratch_path+'/', name='scratch_dir')
app.router.add_static('/devconn', path='./static/', name='dev_dir')


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
                          "BlockFilter.UpdateConnecting",
                          "CozmoAPI.CozmoInstanceRunner.overtime",
                          "cozmo_engine.update.run.slow",
                          "NavMeshQuadTreeProcessor.FillBorder",
                          "robot.play_area_size",
                          "robot.vision.dropped_frame_overall_count",
                          "robot.vision.dropped_frame_recent_count",
                          "robot.vision.profiler.VisionSystem.Profiler",
                          "RobotImplMessaging.HandleImageChunk",
                          "VisionComponent.SetCameraSettings",
                          "[@VisionComponent] DroppedFrameStats",
                          "VisionSystem.Profiler"]

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


if __name__ == '__main__':
    cozmo.robot.Robot.drive_off_charger_on_connect = False
    try:
        app_loop = asyncio.get_event_loop()
        if command_args.connect_to_sdk:
            sdk_conn = cozmo.connect_on_loop(app_loop)
            app['sdk_conn'] = sdk_conn

            if command_args.connect_to_robot:
                # Wait for the robot to become available and add it to the app object.
                app['robot'] = app_loop.run_until_complete(sdk_conn.wait_for_robot())
            else:
                app['robot'] = None

            sdk_conn.add_event_handler(cozmo._clad._MsgGameToGame, handle_game_to_game_msg)

            if command_args.log_from_engine:
                sdk_conn.add_event_handler(cozmo._clad._MsgDebugAppendConsoleLogLine, handle_log_line)
                enable_all_log_channels(sdk_conn)
                set_console_var(sdk_conn, "EnableCladLogger", "1")

            send_game_to_game("EnableGameToGame", "")
        else:
            app['sdk_conn'] = None
            app['robot'] = None
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)

    host_port = 9090  # 8080 clashes with the Cozmo USB tunnel
    if PAGE_TO_LOAD is not None:
        _delayed_open_web_browser("http://127.0.0.1:" + str(host_port) + "/codelab/" + PAGE_TO_LOAD, delay=1.0)
    if command_args.open_dev_tab:
        _delayed_open_web_browser("http://127.0.0.1:" + str(host_port) + "/devconn/index.html", delay=1.0)
    web.run_app(app, host="127.0.0.1", port=host_port)

