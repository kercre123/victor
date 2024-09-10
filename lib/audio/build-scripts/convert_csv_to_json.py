#!/usr/bin/python

"""
Author: Jordan Rivas
Date:   05/29/18

Description: Convert a CSV file into JSON. Each CSV row is an dictionary entry into JSON list. Cells and rows that are
             empty are not added to the JSON file.
"""

import argparse
import csv
import json
import logging
import os
import sys


# Set up default logger
Logger = logging.getLogger('ConvertCsvToJson')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

# Parse Script Arguments
def __parse_input_args(args, print_usage=False):

  parser = argparse.ArgumentParser(description='Convert a CSV file into a JSON file.')

  parser.add_argument('csv_file_path', action='store', type=str,
                      help='Location of the csv file')

  parser.add_argument('json_file_path', action='store', type=str,
                      help='Location to store the generated JSON file')

  parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                      help='Print debug output')

  if print_usage:
    parser.print_help()
    sys.exit(os.EX_NOINPUT)

  args = parser.parse_args(args)

  if args.debug:
    Logger.setLevel(logging.DEBUG)
    Logger.debug(args)
  else:
    Logger.setLevel(logging.INFO)

  # Check for valid file paths
  if not os.path.isfile(args.csv_file_path):
    Logger.error("File Can't be found \'{}\'".format(args.csv_file_path))
    sys.exit(os.EX_NOINPUT)

  return args


# Parse csv file
def __parse_cvs_file(file_path):
  Logger.debug("Open & parse csv file: \'{}\'".format(file_path))
  csv_data = []
  with open(file_path, 'r') as csv_file:
    reader = csv.DictReader(csv_file)
    for aRow in reader:
      csv_data.append(aRow)
  return csv_data



# Not much to do but clean up data
# Note: This is not optimal but this is Python so who cares
def __convert_csv_to_json(csv_data):
  Logger.debug("Convert csv data into json format")
  json_data = []
  for aRow in csv_data:
    remove_keys = []
    for key, val in aRow.items():
      if len(aRow[key]) == 0:
        remove_keys.append(key)

    # Remove only keys with no value
    for key in remove_keys:
      del aRow[key]

    # Copy valid rows into json data
    if len(aRow.items()) > 0:
      json_data.append(aRow)

  return json_data


# Do all the work!
def main(sys_args):

  args = __parse_input_args(sys_args)

  csv_data = __parse_cvs_file(args.csv_file_path)
  json_data = __convert_csv_to_json(csv_data)

  # Write json file
  with open(args.json_file_path, 'w') as file:
    json.dump(json_data, file, sort_keys=True, ensure_ascii=False, indent=2)

  Logger.info("Successfully converted CSV to JSON file \'{}\'".format(args.json_file_path))
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
