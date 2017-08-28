#!/usr/bin/env python3

import argparse
import fnmatch
import json
import logging
import os
import sys
import textwrap

#set up default logger
Logger = logging.getLogger('smartling.check-json-keys')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

ENGLISH_LOCALE = 'en-US'

def check_json_keys(localized_strings_dir):
  error = False
  smartling_files_en_US = {}
  smartling_files_localized = {}
  for root, dirs, files in os.walk(localized_strings_dir):
    for file in files:
      if file.endswith(".json"):
        locale = os.path.basename(os.path.dirname(os.path.join(root, file)))

        if has_locale_dir(locale) and locale == ENGLISH_LOCALE:
          smartling_files_en_US["%s_%s" % (locale,file)] = os.path.join(root, file)

        if has_locale_dir(locale) and locale != ENGLISH_LOCALE:
          smartling_files_localized["%s_%s" % (locale,file)] = os.path.join(root, file)

  for locale_filename_en_US, smartling_filepath_en_US in smartling_files_en_US.items():
    file_en_US = locale_filename_en_US.split("_")[1]

    with open(smartling_filepath_en_US, encoding='utf8') as smart_data_file_en_US:
      smart_data_en_US = json.load(smart_data_file_en_US)

    for locale_filename, smartling_filepath in smartling_files_localized.items():
      locale = locale_filename.split("_")[0]
      file_localized = locale_filename.split("_")[1]

      if file_en_US == file_localized:
        with open(smartling_filepath, encoding='utf8') as smart_data_file:
          smart_data_localized = json.load(smart_data_file)

        for key in smart_data_en_US.keys():
          if not key in smart_data_localized:
            Logger.error('{} is missing key: {}'.format(smartling_filepath, key))
            print('{}-{} is missing key: {}'.format(locale,file_localized, key))
            error = True

        for key in smart_data_localized.keys():
          if not key in smart_data_en_US:
            Logger.error('{} has key not in en_US: {}'.format(smartling_filepath, key))
            print('{}-{} has key not in en_US: {}'.format(locale, file_localized, key))
            error = True

  if error:
    raise KeyError('Bad smartling key(s).')

####################################
############# HELPERS ##############
####################################
def has_locale_dir(directory):
  pattern = '[a-z][a-z]-[A-Z][A-Z]'
  if fnmatch.fnmatch(directory, pattern):
    return True
  else:
    return False

####################################
############### MAIN ###############
####################################

def parse_args(argv=[], print_usage=False):

  parser = argparse.ArgumentParser(
  formatter_class=argparse.RawDescriptionHelpFormatter,
  description='Strips special characters from Smartling files.',
  epilog=textwrap.dedent('''
    options description:
      [localized strings directory]
    ''')
  )

  parser.add_argument('--localized-strings-dir', action='store',
                      default="../../unity/Cozmo/Assets/StreamingAssets/LocalizedStrings/",
                      help='Localized strings dir')

  if print_usage:
    parser.print_help()
    sys.exit(2)

  args = parser.parse_args(argv)

  return args


def run(args):
  return check_json_keys(args.localized_strings_dir)


if __name__ == '__main__':
  args = parse_args(sys.argv[1:])
  run(args)
