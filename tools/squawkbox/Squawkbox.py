from time import sleep
import pyaudio
import logging
import glob
import wave
import sys
import json
import os

class Squawkbox:
  'Squawkbox class'
  def __init__(self,config_map_json_path, audio_folder_path='', auto_test_file_path=''):
    self.logger = self.logging()
    
    if audio_folder_path != '':
      self.audio_files_mapping = self.gather_and_map_sound_files_from_dir(audio_folder_path)
    else:
      #Will set this using '.set_audio_dir'
      self.audio_files_mapping = False
    
    if auto_test_file_path != '':
      self.auto_test_list = self.json_test_file_to_test_list(auto_test_file_path)
    else:
      #Will have to set this using '.set_auto_test'
      self.auto_test_list = False

    self.usb_name_list,\
    self.speaker_number_to_output_name, \
    self.channel_options = self.parse_speaker_config_json(config_map_json_path)
    
    self.pa = pyaudio.PyAudio()

  def logging(self):
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    console = logging.StreamHandler() #for now, print to just console
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    console.setFormatter(formatter)
    logger.addHandler(console)
    return logger
   
   #PLAY MODES
  def automatic_test_single_speaker(self, sound_card_dict, websocket = None):

    #Make sure audio dir set
    if self.audio_files_mapping == False:
      self.logger.warn("Couldnt find an audio mapping, make sure you used '.set_audio_dir'.")
      return
    
    #Make sure auto test list set
    if self.auto_test_list == False: 
      self.logger.warn("Couldn't find an auto test file. Make sure you used '.set_auto_test'")
      return

    #play test
    for index in self.auto_test_list:
      sound_name = index['Sound']
      sound_delay = index['Delay']
      sound_repeat = index['Play_Amount']
      sound_speaker = index['Speaker']

      usb_card_choice = self.map_speaker_to_usb_number(sound_speaker)
      
      if usb_card_choice == False:
        continue

      sound_path = self.audio_files_mapping[sound_name]
      sound_card_index = sound_card_dict[usb_card_choice]
      if sound_repeat > 0:
        self.logger.info("will play the sound '{0}' {1} times, with a {2} second delay"
                        .format(sound_name, sound_repeat, sound_delay)
                        )
        for i in range(0,sound_repeat):
          self.play_single_sound(usb_card_choice, sound_card_dict, sound_path, sound_speaker, websocket)
          self.logger.info("Pausing for {} seconds.".format(sound_delay))
          sleep(sound_delay)
      else:
        self.play_single_sound(usb_card_choice, sound_card_dict, sound_path, sound_speaker, websocket)
        self.logger.info("Pausing for {} seconds.".format(sound_delay))
        sleep(sound_delay)

  def map_speaker_to_usb_number(self, speaker_number):
    for usb_number, usb_speakers_dict in self.channel_options.items():
      if speaker_number in usb_speakers_dict.keys():
        return usb_number 
    return False
  
  def gather_and_map_sound_files_from_dir(self, dir_path):
    wav_glob_pattern = "*.wav"
    file_paths_of_sounds = glob.glob(os.path.join(dir_path,wav_glob_pattern))
    
    if file_paths_of_sounds == []:
      self.logger.error("There doesn't appear to appear to be any wav files in this directory")
      sys.exit()

    filename_to_path_mapping = {}

    # construct the mapping of sound file name to path
    for path in file_paths_of_sounds:
      base = os.path.basename(path)
      file_name = os.path.splitext(base)[0]
      filename_to_path_mapping[file_name] = path
    return filename_to_path_mapping

  def set_audio_dir(self, dir_path):
    self.audio_files_mapping = self.gather_and_map_sound_files_from_dir(dir_path)
    return

  def json_test_file_to_test_list(self, json_file_path):
    json_file = open(json_file_path,'r')
    data = json.load(json_file)
    test_sounds_list = [x for x in data['play']]
    return test_sounds_list

  def set_auto_test_path(self, json_file_path):
    self.auto_test_file_path = self.json_test_file_to_test_list(json_file_path)
    return

  def play_single_sound(self,
                        sound_card_choice, 
                        sound_card_dict, 
                        sound_file_path, 
                        speaker_choice, 
                        websocket=None):
    chunk = 1024
    wf = wave.open(sound_file_path, 'rb')
    
    speaker_choice_map = self.channel_options[sound_card_choice][speaker_choice]
    usb_card_index = sound_card_dict[sound_card_choice]

    #TODO: add ability to play on Linux
    try:
      stream_info = pyaudio.PaMacCoreStreamInfo(
        flags=pyaudio.PaMacCoreStreamInfo.paMacCorePlayNice, # default
        channel_map=speaker_choice_map
        )
    except AttributeError:
      self.logger.error("Couldn't find PaMacCoreStreamInfo. Make sure that "
          "you're running on Mac OS X.")
      sys.exit(-1)

    def audio_callback(in_data, frame_count, time_info, status):
      data = wf.readframes(frame_count)
      return (data, pyaudio.paContinue)

    # open stream
    stream = self.pa.open(
      format=self.pa.get_format_from_width(wf.getsampwidth()),
      channels=wf.getnchannels(),
      rate=wf.getframerate(),
      output=True,
      output_device_index=usb_card_index, #Important, points to our usb sound card
      output_host_api_specific_stream_info=stream_info,
      stream_callback=audio_callback)

    stream.start_stream()

    #If websocket provided, Flags will begin and end gathering the micdata
    #while audio stream is active/playing
    if websocket != None:
      websocket.mic_stream_flag = True
      
      while stream.is_active():
        sleep(0.1)      
      
      websocket.mic_stream_flag = False
    else:
      while stream.is_active():
        sleep(0.1)

    stream.stop_stream()
    stream.close()

  def parse_speaker_config_json(self, file_path):
    map_config = open(file_path,'r')
    json_data = json.load(map_config)
    
    usb_name_list = []
    speaker_number_to_channel_map = {}
    speaker_number_to_output_name = {}
    
    for usb in list(json_data.keys()):
      usb_name_list.append(usb)
      speaker_number_to_channel_map[usb] = {}

      for sound_out_name, sound_out_name_values in json_data[usb].items():
        if sound_out_name_values == False:
          continue
        speaker_number = sound_out_name_values['Speaker_Number']
        speaker_channel_mapping = tuple(sound_out_name_values['Speaker_Channel_Map'])
        speaker_number_to_channel_map[usb].update({speaker_number:speaker_channel_mapping})
        speaker_number_to_output_name[speaker_number] = sound_out_name
    return usb_name_list, speaker_number_to_output_name, speaker_number_to_channel_map

  #Upon finding USB sound cards, returns the Index that USB card(s) is addressed too, as a dict.
  def are_sound_cards_inserted(self,look_for=None):
    # Iniitalize things to check for
    if look_for == None: look_for = len(self.usb_name_list) 
    cards_inserted = {}    
    counter = look_for
    usb_name_found = 1
    startech_usb_sound_card = "USB Sound Device" 
    
    self.logger.info("Auto-detecting that {} sound cards are connected".format(look_for))
    while counter > 0:
      info = self.pa.get_host_api_info_by_index(0)
      numDevices = info.get('deviceCount')

      for index in range(0,numDevices):
        device_info = self.pa.get_device_info_by_host_api_device_index(0,index)
        device_info_name = device_info.get("name")
        device_info_output_value = device_info.get("maxOutputChannels")
        min_num_output_channels = 2

        if device_info_output_value >= min_num_output_channels and \
            startech_usb_sound_card in device_info_name:
          
          cards_inserted["usb"+str(usb_name_found)] = index
          usb_name_found += 1
          counter -= 1
          if counter == 0: 
            return cards_inserted
      
      if counter > 0: #Re-initialize for next checks
        usb_name_found = 1
        counter = look_for
        self.pa.terminate()
        self.pa = pyaudio.PyAudio()
        sleep(2) 


  #VICTOR HELPER FUNCTIONS
  def connect_to_victor(self, victor_ip_address):
    pass

  def begin_victor_logging(self):
    pass

  def get_victor_event_played(self, name_of_event):
    pass
