#!/usr/bin/env python

""" Utility to Verify AnimaitonGroups are valid...
"""

from __future__ import print_function

import argparse
import os
import pprint
import re
import shutil
import subprocess
import sys
import time
import json

def parse_args(argv=[], print_usage=False):

    class DefaultHelpParser(argparse.ArgumentParser):
        def error(self, message):
            sys.stderr.write('error: %s\n' % message)
            self.print_help()
            sys.exit(2)

    parser = DefaultHelpParser(description='Generate version number of the form <major>.<minor>.<patch>.<build_number>.<YYMMDD>.<HHMM>.<build_type>.<git_hash>')
    parser.add_argument('-v', '--verbose', action='store_true', 
                        help='print verbose output')

    parser.add_argument('--projectRoot', dest='projectRoot', action='store', default=None,
                      help='project location, assumed to be same as git repo root')

    parser.add_argument('--externals', metavar='EXTERNALS_DIR', dest='externalsPath', action='store', default=None,
                      help='Use repos checked out at EXTERNALS_DIR')


    if print_usage:
        parser.print_help()
        sys.exit(2)

    args = parser.parse_args(argv)
    return args

#Animation Groups support being in nested folders
def getAllAnimGroups(animaitonGroupPath):
  animGroupNames = []
  for root, dirs, fnames in os.walk(animaitonGroupPath):
      for fname in fnames:
        animGroupNames.append(fname.replace(".json",""))
          
  return animGroupNames
'''
Read in AnimationTriggerMap.json (path passed in )
Verify that AnimName matches files in AnimationGroupsPath.
'''
def run(args):
  if args.verbose:
      pprint.pprint(vars(args))
  #No problems
  exit_code = 0

  externalsPath = args.externalsPath;
  projectRoot = args.projectRoot;
  if( externalsPath == None ):
    externalsPath = os.path.abspath(os.path.join(os.path.dirname( __file__ ), "../../EXTERNALS"))

  if( projectRoot == None ):
    projectRoot = os.path.abspath(os.path.join(os.path.dirname( __file__ ), "../.."))

  animGroupPath = externalsPath + "/cozmo-assets/animationGroups"

  animGroupArray = getAllAnimGroups(animGroupPath)

  animationTriggerMapFilename = projectRoot + "/lib/anki/products-cozmo-assets/animationGroupMaps/AnimationTriggerMap.json"

  animationTriggerCladFilename = projectRoot + "/clad/src/clad/types/animationTrigger.clad"

  # All we need are names, not real python so just strip comments and names
  cladNamesArray = []
  with open(animationTriggerCladFilename, 'r') as cladNamesFile:
    cladNamesArray = cladNamesFile.readlines()
  for i in range(len(cladNamesArray)):
    cladNamesArray[i] = cladNamesArray[i].strip().split(",")[0]

  animationTriggerMapData = open(animationTriggerMapFilename)
  animationTriggerMapJson = json.load(animationTriggerMapData)
  animationTriggerMapData.close()

  for animTriggerVal in animationTriggerMapJson['Pairs']:
    if( animTriggerVal['AnimName'] not in animGroupArray ):
      print("Invalid AnimName in AnimationTriggerMap.json: " + str(animTriggerVal['AnimName']))
      exit_code = 1
    elif( animTriggerVal['CladEvent'] not in cladNamesArray ):
      print("Invalid Cladname in clad: " + str(animTriggerVal['CladEvent']))
      exit_code = 1

  return exit_code


if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    result = run(args)
    exit(result)
