#!/usr/bin/env python2
import sys
import os
import subprocess
import logging

#set up default logger
Logger = logging.getLogger('anki_audio_configure')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

def main():
  # Run audio scripts
  # Download wwise sdk files
  fetch_wwise_script = os.path.join(os.path.dirname(sys.argv[0]), 'wwise', 'fetch-wwise.sh')
  if (subprocess.call(fetch_wwise_script) != 0):
    Logger.error('error fetch Wwise SDK')

if __name__ == '__main__':
    main()
