#!/usr/bin/env python3

import argparse
import logging
import os
import sys
import textwrap

#set up default logger
Logger = logging.getLogger('smartling.strip-special-characters')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)


def strip_special_characters(args):
  for root, dirs, files in os.walk(args.localized_strings_dir):
    if 'ja-JP' in root:
      for file in files:
        if file.endswith('.json'):
          with open(os.path.join(root, os.path.basename(file)), "r+", encoding='utf8') as inoutfile:
            lines = [line.replace(u'\u200b', '') for line in inoutfile]   # ZERO-WIDTH SPACE
            lines = [line.replace(u'\u00A0', '') for line in lines]       # NO-BREAK SPACE
            lines = [line.replace(u'\u23CE', '') for line in lines]       # RETURN SYMBOL
            inoutfile.seek(0)
            inoutfile.truncate()
            inoutfile.writelines(lines)

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
  return strip_special_characters(args)


if __name__ == '__main__':
  args = parse_args(sys.argv[1:])
  sys.exit(run(args))
