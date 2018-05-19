#!/usr/bin/env python3
# encoding: utf-8

import json
import argparse
import sys
import logging
import os
from datetime import datetime
################################################################################
# Global variables (Keys for log file dicts)
################################################################################
log_file_speaker_played         = "speaker_played"
log_file_expected_direction     = "expected_audio_direction"
log_file_direction_confidence   = "audio_direction_confidence"
log_file_triggerword_played     = "triggerword_played"
log_file_triggerword_detected   = "triggerword_detected"
log_file_intent_played          = "intent_played"
log_file_cloudIntent_recognized = "cloudIntent_recognized"


################################################################################
# Helpers
################################################################################
def init_logging():
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    console = logging.StreamHandler() #for now, print to just console
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s -" + \
                                   " %(message)s")
    console.setFormatter(formatter)
    logger.addHandler(console)
    return logger

# Argument Parser
def parse_arguments(args):
  parser = argparse.ArgumentParser()

  parser.add_argument("-f", "--log_file",
                      action="store",
                      required=True,
                      help="Path to log file")

  parser.add_argument("-d", "--direction",
                      action="store",
                      required=False,
                      help="The speaker that Victor was facing")

  options = parser.parse_args(args)
  return (parser, options)

################################################################################
# Calculating functions
################################################################################
def log_file_to_sample_list(log_file):
  list_result = []
  try:
    log_file = [list_result.append(json.loads(readline))
                for readline in log_file]
  except json.decoder.JSONDecodeError:
    logger.error("JSONDecodeError when parsing log file")
    sys.exit()
  return list_result

def compute_individual_trigger_detected(list_of_samples):
  '''
  For multiple triggerword audio played, it will be the format
  {
    Individual Trigger Results: {
                                Trigger 1: { Results},
                                Trigger 2: { Results},
                                }
  }
  '''
  trigger_dict = {}

  played_string        = "Individual Trigger Audio Played"
  detected_string      = "Individual Triggers Detected"
  success_rate_string  = "Individual Triggers Detected Success"
  results_string       = "Individual Trigger Results"

  #Find all unique triggerwords played, to construct result dict
  trigger_unique_names = set(sample[log_file_triggerword_played]
                             for sample in list_of_samples)

  #Construct return dict
  for trigger_file_name in trigger_unique_names:
    trigger_dict[trigger_file_name] = {
                                        played_string : 0,
                                        detected_string : 0,
                                        success_rate_string: None,
                                      }

  #Tally individual triggers
  for sample in list_of_samples:
    trigger_played_name = sample[log_file_triggerword_played]
    trigger_detected_result = sample[log_file_triggerword_detected]

    trigger_dict[trigger_played_name][played_string] += 1

    if trigger_detected_result == True:
      trigger_dict[trigger_played_name][detected_string] += 1

  # Compute success rate for individual triggers
  for trigger_name in trigger_unique_names:
    total_played = trigger_dict[trigger_name][played_string]
    total_true = trigger_dict[trigger_name][detected_string]
    try:
      success_rate = float(total_true/total_played)
    except ZeroDivisionError:
      success_rate = 0

    trigger_dict[trigger_name][success_rate_string] = success_rate

  result_dict = {results_string : trigger_dict}

  return result_dict

def compute_individual_intent_detected(list_of_samples):
  '''
  For multiple intent audio played, it will be the format
  {
    Individual Intents Results: {
                                Intent 1: { Results},
                                Intent 2: { Results},
                                }
  }
  '''
  intents_played_string   = "Total Intents Audio Played"
  intents_detected_string = "Overall Intents Detected"
  intents_success_rate    = "Intents Detected Success Rate"

  # Figure out all unique intents played
  intents_played_names = set(sample["intent_played"]
                             for sample in list_of_samples)

  # Construct return dict
  intents_dict = {}
  for intent_name in intents_played_names:
    intents_dict[intent_name] = {
                                  intents_played_string : 0,
                                  intents_detected_string : 0,
                                  intents_success_rate : 0
                                }

  # Tally up individual intents played/detected
  for sample in list_of_samples:
    intent_played = sample["intent_played"]
    if sample["triggerword_detected"] == True:
      intents_dict[intent_played][intents_played_string] += 1
    if sample["cloudIntent_recognized"] == True:
      intents_dict[intent_played][intents_detected_string] += 1

  # compute intent success
  # for intent_name, intent_results in intents_dict.items(): 
  for intent_name in intents_played_names:
    intent_key = intents_dict[intent_name]
    total_intents_detected = intent_key[intents_detected_string]
    total_intents_played = intent_key[intents_played_string]

    try:
      intents_detected_success_rate = float(total_intents_detected/total_intents_played)
    except ZeroDivisionError:
      intents_detected_success_rate = 0

    intent_key[intents_success_rate] = intents_detected_success_rate
  results_dict = {"Individual Intents Results" : intents_dict}
  return results_dict

def compute_overall_trigger_stats(list_of_samples):
  # calculate triggerwords detected
  played = len(list_of_samples)
  detected = 0

  for sample in list_of_samples:
    if sample["triggerword_detected"] == True:
      detected += 1
  try:
    success_rate = float(detected/played)
  except ZeroDivisionError:
    success_rate = 0

  result_dict = {
    "Total Triggerword Audio Played": played,
    "Overall Triggers Detected Success": success_rate,
  }

  return result_dict

def compute_overall_intent_stats(list_of_samples):
  triggerword_detected_string  = "triggerword_detected"
  intent_detected_string       = "cloudIntent_recognized"

  # calculate intents
  total_intents_played   = 0.0
  total_intents_detected = 0.0
  intents_success_rate   = 0.0

  # Tally up intents played/detected
  for sample in list_of_samples:
    if sample[triggerword_detected_string] is True:
      total_intents_played += 1
    if sample[intent_detected_string] is None:
      continue
    elif sample[intent_detected_string] is True:
      total_intents_detected += 1

  # compute intent success
  try:
    intents_success_rate = float(total_intents_detected/total_intents_played)
  except ZeroDivisionError:
    intents_success_rate = 0

  result_dict = {
                  "Total Intent Audio Played": total_intents_played,
                  "Total Intent Detected Success": total_intents_detected,
                  "Total Intent Detected Success Rate": intents_success_rate
                }
  return result_dict

def print_all(log_file):
  '''
  Will attempt to calculate every stat (individual, overall) and compile into
  one json object to print to screen, and return object
  '''
  log_file_list = log_file_to_sample_list(log_file)
  result_dict = {}

  try:
    result_dict.update(compute_individual_trigger_detected(log_file_list))
  except:
    logger.error("Unable to compute individual triggerword stats")

  try:
    result_dict.update(compute_individual_intent_detected(log_file_list))
  except:
    logger.error("Unable to compute individual intent stats")

  try:
    result_dict.update(compute_overall_trigger_stats(log_file_list))
  except:
    logger.error("Unable to compute overall trigger stats")

  try:
    result_dict.update(compute_overall_intent_stats(log_file_list))
  except:
    logger.error("Unable to compute overall intent stats")

  print_to_txt_file(result_dict,log_file)
  print(json.dumps(result_dict, indent=4))
  return result_dict

def print_to_txt_file(result_dict, log_file):
  dir_path = os.path.dirname(os.path.realpath(__file__))
  new_file_name = "processed_" + log_file.name

  audio_log_file_path = os.path.join(dir_path, new_file_name)
  output_file = open(audio_log_file_path, 'a+')
  output_file.write(json.dumps(result_dict, indent=4))

# Plot
def plot_basic_bar(overall_stats_result_dict, individual_stats_result_dict):
  pass

if __name__ == "__main__":
  args = sys.argv[1:]
  parser, options = parse_arguments(args)
  log_file = options.log_file
  logger = init_logging()
  
  #check that log file path is valid 
  try:
    log_file = open(log_file, "r")
  except:
    logger.error("File path is invalid")
    sys.exit()

  result = print_all(log_file)






