#!/usr/bin/env python3
# encoding: utf-8

import sys
import os
import easygui as eg
import Squawkbox


programTitle = "MiniSquawks"
endlessPlayMode = "Endless Play"
specificPlayMode = "Specific Number of Plays"

################################################################################
# GUI functions                                                                #
################################################################################
def select_audio_directory():
  script_dir_path = os.path.dirname(os.path.abspath(__file__))
  default_audio_dir = os.path.join(script_dir_path, "audio_files")

  msg = "Please select a directory of audio files to play"
  choices = ["Select Audio Folder","Exit"]
  choice = eg.buttonbox(msg=msg,title=programTitle, choices=choices)
  if choice == "Exit":
    sys.exit()
  else:
    dir_path = eg.diropenbox(msg=msg, title=programTitle, default=default_audio_dir)
  return dir_path

def select_speakers_to_play():
  msg = "Select the number of speakers you'd like to play"
  choices = ["Speaker 1/Left Front",
             "Speaker 2/Right Front",
             "Speaker 3/Left Surround",
             "Speaker 4/Right Surround",
             "Speaker 5/LeftCenterBass",
             "Speaker 6/RightCenterBass",
             "Speaker 7/Left Back",
             "Speaker 8/Right Back" ]

  choice = eg.multchoicebox(msg, programTitle, choices)

  if choice == None:
    sys.exit()
  return choice

def delay_between_audio():
  msg = "Please select a time (in seconds) to delay in between audio"
  value = eg.integerbox(msg=msg, title=programTitle, default=1, lowerbound=0,
                        upperbound=600)
  if value == None:
    sys.exit()
  return value

def repeat_number_of_audio():
  msg = "Please select how many times you'd like each audio to play"
  value = eg.integerbox(msg=msg, title=programTitle, default=1, lowerbound=0,
                        upperbound=600)
  if value == None:
    sys.exit()
  return value

def audio_file_dir_multichoicebox(squawkInstance):
  msg = "Please select audio file(s) you'd like to play"
  title = "MiniSquawks"
  choices = squawkInstance.audio_files_mapping.keys()
  choice = eg.multchoicebox(msg, title, choices)

  if choice == None:
    sys.exit()
  return choice

def play_mode_select():
  msg = "Please select a play mode.\n\n" + \
        "Endless Play: Pick the audio files you want, and then play them non-stop.\n\n" + \
        "Specific Number of Plays: Select how many times you want to play the selected audio\n\n"
  choices = (endlessPlayMode, specificPlayMode)
  choice = eg.choicebox(msg=msg, title=programTitle, choices=choices)

  if choice == None:
    sys.exit()
  return choice

def endless_play_mode(squawkInstance,sound_card_dict):
  sound_card_choice = "usb1"
  audio_playlist = audio_file_dir_multichoicebox(pa)
  speaker_choices = select_speakers_to_play()
  delay = delay_between_audio()
  speaker_file_map = squawkInstance.audio_files_mapping
  speaker_choice_to_index = {v: i + 1 for (i, v) in enumerate(speaker_choices)}

  while True:
    for song_name in audio_playlist:
      for speakers in speaker_choices:
        speaker_num = speaker_choice_to_index[speakers]
        song_path = speaker_file_map[song_name]

        print("Currently playing audio '{}' from {}. Press 'ctrl+c' to exit."
              .format(song_name,speakers))
        squawkInstance.play_single_sound(sound_card_choice,
                                         sound_card_dict,
                                         song_path,
                                         speaker_num)


def specific_play_mode(squawkInstance, sound_card_dict):
  sound_card_choice = "usb1"
  audio_playlist = audio_file_dir_multichoicebox(pa)
  speaker_choices = select_speakers_to_play()
  delay = delay_between_audio()
  repeat = repeat_number_of_audio()
  speaker_file_map = squawkInstance.audio_files_mapping
  speaker_choice_to_index = {v: i + 1 for (i, v) in enumerate(speaker_choices)}

  for i in range(0,repeat):
    for song_name in audio_playlist:
      for speakers in speaker_choices:
        speaker_num = speaker_choice_to_index[speakers]
        song_path = speaker_file_map[song_name]

        print("Currently playing audio '{}' from {}. Press 'ctrl+c' to exit."
              .format(song_name, speakers))
        squawkInstance.play_single_sound(sound_card_choice,
                                         sound_card_dict,
                                         song_path,
                                         speaker_num)

def initialize_squawks():
  dir_path = os.path.dirname(os.path.realpath(__file__))
  speaker_map_file_default = os.path.join(dir_path, "MiniSquawks",
                                          "speaker_map_config_room_24.json")

  pa = Squawkbox.Squawkbox(config_map_json_path=speaker_map_file_default)

  sound_card_dict = pa.are_sound_cards_inserted()

  audio_directory = select_audio_directory()

  pa.audio_files_mapping = pa.gather_and_map_sound_files_from_dir(
    audio_directory)
  return pa, sound_card_dict

if __name__ == '__main__':

  pa, sound_card_dict = initialize_squawks()

  playModeChoice = play_mode_select()

  if playModeChoice == endlessPlayMode:
    endless_play_mode(pa, sound_card_dict)
  elif playModeChoice == specificPlayMode:
    specific_play_mode(pa, sound_card_dict)






