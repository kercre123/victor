#!/usr/bin/env python3
# encoding: utf-8

import os
import sys
import argparse
import os.path, time
import wave
import glob
import shutil
import datetime
import subprocess
from datetime import datetime

TEMP_FOLDER_PATH          = "/tmp/temp"
RAW_FOLDER_PATH           = "{}/Raw".format(TEMP_FOLDER_PATH)
ROBOT_FOLDER_PATH         = "{}/Robot".format(TEMP_FOLDER_PATH)
LOCAL_DEBUG_CAPTURE_PATH  = "{}/debugCapture".format(TEMP_FOLDER_PATH) 
REMOTE_DEBUG_CAPTURE_PATH = "/data/data/com.anki.victor/cache/micdata/debugCapture"
ROBOT_USERNAME            = "root"
TAB_CHARACTER             = "\t"
ONE_CHANNEL               = 1
FOUR_CHANNEL              = 4

def parse_arguments(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('-ip', '--robot_ip',
                        action = 'store', required = True,
                        type = str, help = "The ip of robot")
    parser.add_argument('-path', '--dropbox_path',
                        action = 'store', required = True,
                        type = str, help = "Path of dropbox folder")
    options = parser.parse_args(args)
    return (options)

def execute_command_line(command_text):
    process = subprocess.Popen(command_text, stdout = subprocess.PIPE)
    return process

def copy_all_recordings(robot_ip, recordings_path, store_path):
    remote_recordings_total = get_total_wav_file(REMOTE_DEBUG_CAPTURE_PATH, robot_ip)
    create_folder(store_path)
    command_text = eval("['scp', '-r', '{}@{}:{}', '{}']"
                   .format(ROBOT_USERNAME, robot_ip, recordings_path, store_path))
    execute_command_line(command_text)
    wait_for_copy_finish(remote_recordings_total, LOCAL_DEBUG_CAPTURE_PATH)

def get_command_output(command_text):
    process = execute_command_line(command_text)
    return process.communicate()[0].decode("utf-8")

def get_list_recordings_folder_name(robot_ip, recordings_path):
    command_text = eval("['ssh', '{}@{}', 'ls {}']".format(ROBOT_USERNAME, robot_ip, recordings_path))
    return get_command_output(command_text)

def get_total_wav_file(folder_path, robot_ip = ""):
    total_file = 0;
    if robot_ip == "":
        for subdir in glob.glob(os.path.join(folder_path, "*", "")):
            for recordings_path in glob.glob(os.path.join(subdir, '*.wav')):
                total_file += 1;
    else:
        command_text = eval("['ssh', '{}@{}', 'ls -R {} | grep .wav | wc -l']" \
                       .format(ROBOT_USERNAME, robot_ip, folder_path))
        total_file = int(get_command_output(command_text)[:-1])
    return total_file

def create_folder(folder_path):
    if not os.path.exists(folder_path):
        os.makedirs(folder_path)

def wait_for_copy_finish(remote_recordings_total, local_folder_path):
    local_recordings_total = 0
    while(local_recordings_total != remote_recordings_total):
        time.sleep(10)
        local_recordings_total = get_total_wav_file(local_folder_path)

def wait_for_delete_folder_finish(folder_path, robot_ip = ""):
    folder_exist = True
    while (folder_exist == True):
        time.sleep(10)
        if robot_ip == "":
            folder_exist = os.path.isdir(folder_path)
        else:
            command_text = eval("['ssh', '{}@{}', 'test -d {} && echo True || echo False']" \
                           .format(ROBOT_USERNAME, robot_ip, folder_path))
            folder_exist = eval(get_command_output(command_text).split(TAB_CHARACTER)[0])

def get_channel_number_of_recordings(recordings_path):
    w = wave.open(recordings_path, 'r')
    channel_number = w.getnchannels()
    w.close()
    return channel_number

def get_created_date_of_recordings_folder(robot_ip, recordings_folder_path):
    command_text = eval("['ssh', '{}@{}', 'stat -c \"%y\" {}']" \
                   .format(ROBOT_USERNAME, robot_ip, recordings_folder_path))
    created_date = get_command_output(command_text).split(" ")[0]
    created_date = datetime.strptime(created_date, '%Y-%m-%d').strftime('%m%d%Y')
    return created_date

def generate_folder_name_by_date(robot_ip, recordings_folder_path):
    list_folder_name = get_list_recordings_folder_name(robot_ip, recordings_folder_path)[:-1].split("\n")
    first_folder_path = "{}/{}".format(recordings_folder_path, list_folder_name[0])
    last_folder_path = "{}/{}".format(recordings_folder_path, list_folder_name[-1])
    first_created_date = get_created_date_of_recordings_folder(robot_ip, first_folder_path)
    last_created_date = get_created_date_of_recordings_folder(robot_ip, last_folder_path)
    final_folder_name = "{}-{}".format(first_created_date, last_created_date)
    return final_folder_name

def filter_recordings_by_channel(robot_ip, recordings_folder, raw_folder, robot_folder):
    copy_all_recordings(robot_ip, REMOTE_DEBUG_CAPTURE_PATH, TEMP_FOLDER_PATH)
    create_folder(raw_folder)
    create_folder(robot_folder)
    for subdir in glob.glob(os.path.join(recordings_folder, "*", "")):
        for recordings_path in glob.glob(os.path.join(subdir, '*.wav')):
            channel_number = get_channel_number_of_recordings(recordings_path)
            if channel_number == ONE_CHANNEL:
                shutil.move(recordings_path, robot_folder)
            elif channel_number == FOUR_CHANNEL:
                shutil.move(recordings_path, raw_folder)

def process_recordings(raw_path, robot_path, dropbox_path):
    dropbox_raw_path = "{}/Raw".format(dropbox_path)
    dropbox_robot_path = "{}/Robot".format(dropbox_path)
    create_folder(dropbox_raw_path)
    create_folder(dropbox_robot_path)
    shutil.move(raw_path, dropbox_raw_path)
    shutil.move(robot_path, dropbox_robot_path)

def clean_up_recordings(temp_path, robot_ip, recordings_remote_path):
    shutil.rmtree(temp_path)
    wait_for_delete_folder_finish(TEMP_FOLDER_PATH)
    command_text = eval("['ssh', '{}@{}', 'rm -rf {}']".format(ROBOT_USERNAME, robot_ip, recordings_remote_path))
    execute_command_line(command_text)
    wait_for_delete_folder_finish(REMOTE_DEBUG_CAPTURE_PATH, robot_ip)

def grab_recordings(robot_ip, dropbox_recordings_path):
    try:
        folder_name = generate_folder_name_by_date(robot_ip, REMOTE_DEBUG_CAPTURE_PATH)
        raw_folder_path = "{}/{}".format(RAW_FOLDER_PATH, folder_name)
        robot_folder_path = "{}/{}".format(ROBOT_FOLDER_PATH, folder_name)
        filter_recordings_by_channel(robot_ip, LOCAL_DEBUG_CAPTURE_PATH, raw_folder_path, robot_folder_path)
        process_recordings(raw_folder_path, robot_folder_path, dropbox_recordings_path)
        clean_up_recordings(TEMP_FOLDER_PATH, robot_ip, REMOTE_DEBUG_CAPTURE_PATH)
        print("All recordings have been separated and moved to Dropbox folder with path: {}" \
        .format(dropbox_recordings_path))
    except Exception as e:
        print("An error occurred when connecting with robot: {}".format(e))

if __name__ == '__main__':
    options = parse_arguments(sys.argv[1:])
    grab_recordings(options.robot_ip, options.dropbox_path)
