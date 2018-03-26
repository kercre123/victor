import argparse
import os

def parse_arguments(args):
  #Default values 
  dir_path = os.path.dirname(os.path.realpath(__file__))
  num_cards_default = 2
  audio_dir_default = os.path.join(dir_path, "audio_files")
  speaker_map_file_default = os.path.join(dir_path, "speaker_map_config.json")
  auto_test_file_default = os.path.join(dir_path, "basic_automated_test.json")

  parser = argparse.ArgumentParser()
  
  parser.add_argument('-n', '--num_cards',
                      action='store',
                      required=False,
                      type=int,
                      default=num_cards_default,
                      help="# of sound cards trying to use.")

  parser.add_argument('-ad', '--audio_dir',
                      action='store',
                      required=False,
                      default=audio_dir_default,
                      help="Directory path of audio files to play")

  parser.add_argument('-af', '--auto_test_file',
                      action='store',
                      required=False,
                      default=auto_test_file_default,
                      help="Path to JSON test file")
  
  parser.add_argument('-sm', '--speaker_map',
                      action='store', 
                      required=False,
                      default=speaker_map_file_default, 
                      help="File path of json speaker mapping.")

  parser.add_argument('-cs', '--config_speaker',
                    action='store_true', 
                    required=False, 
                    help="Play all speakers, to confirm they work.")

  parser.add_argument('-cc', '--config_usb_order',
                    action='store_true', 
                    required=False, 
                    help="Confirm correct USB sound card order")

  options = parser.parse_args(args)
  return (parser, options)
