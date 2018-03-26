#!/usr/bin/env python3
# encoding: utf-8

import Squawkbox
import SquawkboxUtil
import sys
import os

if __name__ == '__main__':
  args = sys.argv[1:]
  parser, options = SquawkboxUtil.parse_arguments(args)
  
  speaker_map_file_path = options.speaker_map
  audio_folder_path = options.audio_dir
  test_file = options.auto_test_file
  num_cards = options.num_cards
  
  #Begin test
  pa = Squawkbox.Squawkbox(config_map_json_path=speaker_map_file_path, 
               audio_folder_path=audio_folder_path,
               auto_test_file_path=test_file
               )

  sound_card_result_dict = pa.are_sound_cards_inserted(look_for=num_cards)
  
  pa.automatic_test_single_speaker(sound_card_result_dict)

