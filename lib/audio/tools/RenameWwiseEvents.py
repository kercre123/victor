#!/usr/bin/env python2

# This script will rename events in WWise project using .CSV file
# Input Wwise project event folder and .CSV file where the first column is 'Old Event Name' and the second is
#  'New Event Name'

import csv
import argparse
import logging
import os
import glob
import sys
import errno
import xml.etree.ElementTree as ET


#set up default logger
Logger = logging.getLogger('Rename-Wwise-Events')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)


def CreateMapOfEventNames(rename_csv_file_path):
    event_rename_map = {}
    current_name_key = 'Old Event Name'
    new_name_key = 'New Event Name'
    with open(rename_csv_file_path, 'r') as csv_file:
        reader = csv.DictReader(csv_file)
        for aRow in reader:
            event_rename_map[aRow[current_name_key].lower()] = aRow[new_name_key].title()

    return event_rename_map


def UpdateWwiseEventNames(wwise_event_file_path, rename_event_map):
    # Read in the file
    root = ET.parse(wwise_event_file_path)
    # Rename Events
    for event in root.iter('Event'):
        logging.debug('Old Attribute: {0}'.format(event.attrib))
        name = event.attrib['Name'].lower()
        if name in rename_event_map:
            event.attrib['Name'] = rename_event_map[name]
        else:
            logging.error('Missing Event name \'{0}\''.format(name))

        logging.debug('New Attribute: {0}'.format(event.attrib))

    # Write file w/ new event names
    root.write(wwise_event_file_path, encoding='utf-8', xml_declaration=True)


def parse_args(argv=[], print_usage=False):

  parser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description='Rename Wwise Events')

  parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                      help='prints debug output')
  parser.add_argument('event_dir', action='store', help='Path to wwise event directory')
  parser.add_argument('rename_table_csv', action='store', help='Path to renameTable.csv')

  if print_usage:
    parser.print_help()
    sys.exit(0)

  args = parser.parse_args(argv)

  if (args.debug):
    logging.debug(args)
    Logger.setLevel(logging.DEBUG)
  else:
    Logger.setLevel(logging.INFO)

  return args


def run(args):

  if not os.path.isdir(args.event_dir):
    logging.error('Event Directory \'{0}\' does NOT exist'.format(args.event_dir))
    return errno.ENOENT

  if not os.path.exists(args.rename_table_csv):
    logging.error('Rename .CSV file does NOT exist'.format(args.rename_table_csv))
    return errno.ENOENT

  # Create Event map
  event_map = CreateMapOfEventNames(args.rename_table_csv)
  logging.debug('Event map:\n{0}'.format(event_map))

  # Loop through all files in directory
  os.chdir(args.event_dir)
  for file in glob.glob('*.wwu'):
      logging.info('Update event names in \'{0}\''.format(file))
      UpdateWwiseEventNames(file, event_map)

  return 0


if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    sys.exit(run(args))
