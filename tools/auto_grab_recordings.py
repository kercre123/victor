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

TEMP_FOLDER_PATH        = "/tmp/temp"
RAW_FOLDER_PATH         = "/tmp/temp/Raw"
ROBOT_FOLDER_PATH       = "/tmp/temp/Robot"
DEBUG_CAPTURE_PATH      = "/data/data/com.anki.victor/cache/micdata/debugCapture"
DROPBOX_RECORDINGS_PATH = "/Victor-Audio-Recordings/Hey-Vector/en_US_takehome"

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

def copy_all_recordings(robot_ip, recordings_path, store_path):
	create_folder(store_path)
	command_text = eval("['scp', '-r', 'root@{}:{}', '{}']".format(robot_ip, recordings_path, store_path))
	process = subprocess.Popen(command_text, stdout = subprocess.PIPE)

def get_command_output(command_text):
	process = subprocess.Popen(command_text, stdout = subprocess.PIPE)
	return process.communicate()[0].decode("utf-8")

def get_list_recordings_folder_name(robot_ip, recordings_path):
	command_text = eval("['ssh', 'root@{}', 'ls {}']".format(robot_ip, recordings_path))
	return get_command_output(command_text)

def get_folder_size(folder_path, robot_ip = ""):
	if robot_ip == "":
		command_text = eval("['du', '-sh', '{}']".format(folder_path))
		folder_size = get_command_output(command_text).split("\t")[0]
	else:
		command_text = eval("['ssh', 'root@{}', 'du -sh {}']".format(robot_ip, folder_path))
		folder_size = get_command_output(command_text).split("\t")[0]
	return folder_size

def create_folder(folder_path):
	if not os.path.exists(folder_path):
		os.makedirs(folder_path)

def wait_for_copy_finish(remote_folder_size, local_folder_path):
	local_folder_size = ""
	while(local_folder_size != remote_folder_size):
		time.sleep(10)
		local_folder_size = get_folder_size(local_folder_path)

def get_channel_number_of_recordings(recordings_path):
	w = wave.open(recordings_path, 'r')
	channel_number = w.getnchannels()
	return channel_number

def get_created_date_of_recordings_folder(robot_ip, recordings_folder_path):
	command_text = eval("['ssh', 'root@{}', 'stat -c \"%y\" {}']".format(robot_ip, recordings_folder_path))
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

def separate_recordings(recordings_folder, raw_folder, robot_folder):
	create_folder(raw_folder)
	create_folder(robot_folder)
	for subdir in glob.glob(os.path.join(recordings_folder, "*", "")):
		for recordings_path in glob.glob(os.path.join(subdir, '*.wav')):
			channel_number = get_channel_number_of_recordings(recordings_path)
			if channel_number == 1:
				shutil.move(recordings_path, robot_folder)
			elif channel_number == 4:
				shutil.move(recordings_path, raw_folder)

def move_recordings_to_dropbox_folder(raw_path, robot_path, dropbox_path):
	dropbox_raw_path = "{}/Raw".format(dropbox_path)
	dropbox_robot_path = "{}/Robot".format(dropbox_path)
	create_folder(dropbox_raw_path)
	create_folder(dropbox_robot_path)
	shutil.move(raw_path, dropbox_raw_path)
	shutil.move(robot_path, dropbox_robot_path)

def delete_old_data(temp_path, robot_ip, recordings_remote_path):
	shutil.rmtree(temp_path)
	command_text = eval("['ssh', 'root@{}', 'rm -rf {}']".format(robot_ip, recordings_remote_path))
	subprocess.Popen(command_text, stdout = subprocess.PIPE)

def grab_recordings(robot_ip, dropbox_path):
	recordings_local_path = "{}/debugCapture".format(TEMP_FOLDER_PATH)
	dropbox_recordings_path = "{}{}".format(dropbox_path, DROPBOX_RECORDINGS_PATH)
	try:
		folder_name = generate_folder_name_by_date(robot_ip, DEBUG_CAPTURE_PATH)
		raw_folder_path = "{}/{}".format(RAW_FOLDER_PATH, folder_name)
		robot_folder_path = "{}/{}".format(ROBOT_FOLDER_PATH, folder_name)
		recordings_remote_size = get_folder_size(DEBUG_CAPTURE_PATH, robot_ip)
		copy_all_recordings(robot_ip, DEBUG_CAPTURE_PATH, TEMP_FOLDER_PATH)
		wait_for_copy_finish(recordings_remote_size, recordings_local_path)
		separate_recordings(recordings_local_path, raw_folder_path, robot_folder_path)
		move_recordings_to_dropbox_folder(raw_folder_path, robot_folder_path, dropbox_recordings_path)
		delete_old_data(TEMP_FOLDER_PATH, robot_ip, DEBUG_CAPTURE_PATH)
		print("All recordings have been separated and moved to Dropbox folder with path: {}".format(dropbox_recordings_path))
	except Exception as e:
		print("An error occurred when connecting with robot: {}".format(e))

if __name__ == '__main__':
	options = parse_arguments(sys.argv[1:])
	grab_recordings(options.robot_ip, options.dropbox_path)
