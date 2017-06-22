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

import asyncio
import json
import os
import queue
import sys
from threading import Thread
from time import sleep
import webbrowser

USE_TCP = True
CONNECT_TO_ENGINE = True
USE_VERTICAL_GRAMMAR = True
if USE_VERTICAL_GRAMMAR:
    PAGE_TO_LOAD = "index_vertical.html"
else:
    PAGE_TO_LOAD = "index.html"

#PAGE_TO_LOAD = "extra/tutorial.html"
#PAGE_TO_LOAD = "extra/projects.html"


try:
    from aiohttp import web
except ImportError:
    sys.exit("Cannot import from aiohttp. Do `pip3 install --user aiohttp` to install")


def check_tcp_port(force_tcp_port=True, sdk_port=5106):
    tcp_port = os.environ.get("SDK_TCP_PORT")
    if tcp_port is None:
        if force_tcp_port:
            os.environ["SDK_TCP_PORT"] = str(sdk_port)
            tcp_port = os.environ.get("SDK_TCP_PORT")
            print("Forced SDK_TCP_PORT='%s' - NOTE: Change affects ONLY THIS PROCESS" % tcp_port)
            print('To set permanently in Bash do: export SDK_TCP_PORT="%s"' % sdk_port)
            print("Note: use 'unset SDK_TCP_PORT' to unset, and 'echo $SDK_TCP_PORT' to print current value")
        else:
            print("Note: SDK_TCP_PORT is not set, and force_tcp_port is False, so will not connect to Webots")
    else:
        print("SDK_TCP_PORT = '%s'" % tcp_port)

# Must be done before importing cozmo
if USE_TCP:
    check_tcp_port()

import cozmo
from cozmo.util import degrees, distance_mm, speed_mmps


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


def override_unity_block_handling(robot : cozmo.robot.Robot, msg_payload : str):
    try:
        msg_payload_js = json.loads(msg_payload)
    except json.decoder.JSONDecodeError as e:
        print("Decode error: %s" % e)
        print("msg_payload = [%s]" % msg_payload)
        return False

    command = msg_payload_js["command"]
    try:
        request_id = int(msg_payload_js["requestId"])
    except KeyError:
        # no promise set / required
        request_id = -1

    # You can optionally override any of the C# handling by catching it here, and returning True
    # see the following example that handles the vertical drive block on the SDK side
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


async def handle_sdk_call(request):
    # get the msg from the json request
    json_object = await request.json()
    msg_payload = json_object["msg"]

    robot = app['robot']  # type: cozmo.robot.Robot

    if robot is None:
        pass  # testing without a robot
    elif override_unity_block_handling(robot, msg_payload):
        pass  # handling the block on Python SDK side
    else:
        # Send straight on to C# / Unity
        message_type = "WebViewCallback"

        MAX_PAYLOAD_SIZE = 2024 - len(message_type)  # 2048 is an engine-side limit for clad messages, reserve an extra 24 for rest of the tags and members

        part_count = 1
        if len(msg_payload) > MAX_PAYLOAD_SIZE:
            part_count = (((len(msg_payload) - 1) // MAX_PAYLOAD_SIZE) + 1)

        for i in range(part_count):
            text_to_send = msg_payload
            if len(text_to_send) > MAX_PAYLOAD_SIZE:
                text_to_send = text_to_send[0 : MAX_PAYLOAD_SIZE]
                msg_payload = msg_payload[MAX_PAYLOAD_SIZE:]

            msg = cozmo._clad._clad_to_engine_iface.GameToGame(i, part_count, message_type, text_to_send)
            robot.conn.send_msg(msg)

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
            #cozmo.logger.info("FromUnity:[%s]", str(message_payload))
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


# Handle calls from Javascript as post requests
app.router.add_post('/codelab/sdk_call', handle_sdk_call)
app.router.add_post('/codelab/poll_sdk', handle_poll_sdk)

# Statically serve all of the scratch files
app.router.add_static('/codelab', path='../../unity/Cozmo/Assets/StreamingAssets/Scratch/', name='scratch_dir')


def _delayed_open_web_browser(url, delay, new=0, autoraise=True, specific_browser=None):
    '''
    Spawn a thread and call sleep_and_open_web_browser from within it so that main thread can keep executing at the
    same time. Insert a small sleep before opening a web-browser
    this gives Flask a chance to start running before the browser starts requesting data from Flask.
    '''
    def _sleep_and_open_web_browser(url, delay, new, autoraise, specific_browser):
        sleep(delay)
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


if __name__ == '__main__':
    cozmo.robot.Robot.drive_off_charger_on_connect = False

    try:
        app_loop = asyncio.get_event_loop()
        if CONNECT_TO_ENGINE:
            sdk_conn = cozmo.connect_on_loop(app_loop)
            # Wait for the robot to become available and add it to the app object.
            robot = app_loop.run_until_complete(sdk_conn.wait_for_robot())
            app['robot'] = robot
            robot.conn.add_event_handler(cozmo._clad._MsgGameToGame, handle_game_to_game_msg)

            msg = cozmo._clad._clad_to_engine_iface.GameToGame(0, 1, "EnableGameToGame", "")
            robot.conn.send_msg(msg)
        else:
            app['robot'] = None
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)

    host_port = 9090  # 8080 clashes with the Cozmo USB tunnel
    _delayed_open_web_browser("http://127.0.0.1:" + str(host_port) + "/codelab/" + PAGE_TO_LOAD, delay=1.0)
    web.run_app(app, host="127.0.0.1", port=host_port)
