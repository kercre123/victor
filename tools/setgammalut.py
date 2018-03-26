import argparse
import json
import math
import os
import requests
import time

def main():

  parser = argparse.ArgumentParser(description='tool for uploading and selecting custom color remapping used during face rendering')

  parser.add_argument('--ipaddress', '-i',
                      action='store',
                      default='127.0.0.1',
                      help="IP address of webots/robot")

  parser.add_argument('--file', '-f',
                      action='store',
                      required=True,
                      help="Image file to use a look up table for gamma")

  options = parser.parse_args()

  filename_ext = os.path.split(options.file)[1]

  # upload animation file to resources
  url = 'http://'+options.ipaddress+':8889/cache/'+filename_ext
  r = requests.put(url, data=open(options.file, 'rb').read())

  # register animation with canned animations
  url = 'http://'+options.ipaddress+':8889/consolefunccall?func=LoadFaceGammaLUT&args='+filename_ext
  r = requests.post(url)

if __name__ == "__main__":
  main()
