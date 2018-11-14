#!/usr/bin/env python3

"""
Test playing and checking all animations

"""
import os
import sys
import websocket
import argparse
import json
import time
from threading import Thread, Lock
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import anki_vector  # pylint: disable=wrong-import-position

AUX_PORT     = 8889
SOCKET_START = "socket_start"
ANIMATION    = "animation"


locks = dict()
correct_animations = []


def parse_arguments(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-ip', '--vic_ip',
                        action='store',
                        required=False,
                        type=str,
                        help="IP address of Victor")

    options = parser.parse_args(args)
    return (parser, options)

def send_keep_alive(ws):
    subscribe_dict = {"keepalive" : True}
    subscribe_message = json.dumps(subscribe_dict)
    ws.send(subscribe_message)

def subscribe_to_module(ws, module):
    msg_data = {'type': 'subscribe'}
    msg_data['module'] = module
    subscribe_message = json.dumps(msg_data)
    print("Now waiting for a reply")
    ws.send(subscribe_message)

def on_open(ws):
    subscribe_to_module(ws, 'animations')
    locks[SOCKET_START].release()

def on_error(ws, error):
    print("Socket On error : {}".format(error))

def on_close(ws):
    print("Closed connect.")

def on_aux_message(ws, message):
    json_data = json.loads(message)
    check_animation(message)
    send_keep_alive(ws)

def lock_and_acquire():
    retLock = Lock()
    retLock.acquire()
    return retLock

def expect_animation(animation):
    correct_animations.append((animation, 'start'))
    correct_animations.append((animation, 'stop'))
    locks['animation'] = lock_and_acquire()   

def check_lock_and_parse(lock_name, module_type, message):
    if lock_name in locks:
      json_data = json.loads(message)
      if module_type == json_data['module']:
        return json_data
    return None

def wait_for_lock(lock_name):
    retVal = locks[lock_name].acquire(timeout=10)
    del locks[lock_name]
    return retVal

def wait_for_animation():
    return wait_for_lock(ANIMATION)

def check_animation(message):
    anim_data = check_lock_and_parse(ANIMATION, 'animations', message)
    if anim_data:
      #{'data': {'animation': 'anim_neutral_eyes_01', 'type': 'start'}, 'module': 'animations'}
      if correct_animations[0][0] in anim_data["data"][ANIMATION] and \
         correct_animations[0][1] in anim_data["data"]["type"]:

        del correct_animations[0]
        if len(correct_animations) == 0:
          locks[ANIMATION].release()

def init_socket(robot_ip, port):
    websocket_url = "ws://{0}:{1}/socket".format(robot_ip, port)
    print('Creating Web Socket to: {}'.format(websocket_url))

    ws = websocket.WebSocketApp(websocket_url,
                                on_message = on_aux_message,
                                on_error = on_error,
                                on_close = on_close)
    ws.on_open = on_open
    locks[SOCKET_START] = lock_and_acquire()
    #Seperate background thread, to handle incoming messages
    ws_thread = Thread(target=ws.run_forever)
    ws_thread.daemon = True 
    ws_thread.start()
    locks[SOCKET_START].acquire()
    del locks[SOCKET_START]
    return ws

def play_and_check_animation(robot, animation):
    is_good = False
    expect_animation(animation)
    robot.anim.play_animation(animation)
    is_good = wait_for_animation()
    return is_good

def main():
    bad_animations = []
    good_animations = []
    args = anki_vector.util.parse_command_args()
    websocket = init_socket(args.ip, AUX_PORT)
    with anki_vector.Robot(args.serial, default_logging = False) as robot:
        robot.behavior.drive_off_charger()
        anim_names = robot.anim.anim_list
        for idx, animation in enumerate(anim_names):
            print(f"[{idx}] : {animation}")
            is_good = play_and_check_animation(robot, animation)
            if is_good:
                good_animations.append(animation)
            else:
                bad_animations.append(animation)
        if len(bad_animations) > 0:
            print(f"RESULT : {len(bad_animations)}/{len(anim_names)} animations did not work well as below :\n"\
                  f"{str(bad_animations)}")
        else:
            print(f"RESULT : {len(anim_names)} animations worked well.")
    websocket.close()

if __name__ == "__main__":
    main()
