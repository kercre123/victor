#!/usr/bin/env python2

""" Utility that verifies clad enums that identify external assets/files map to actual data
    Map/Clad/Resource paths are loaded in from VerifyExternalsCladMapsConfig.json
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

# Gather all resources recursively from the basePath
# resourcesAreFiles will gather files and not directories - if false will gather directories not files
def getExternalsResources(basePath, resourcesAreFiles=True):
  resources = []
  for root, dirs, fnames in os.walk(basePath):
    if resourcesAreFiles:
      for fname in fnames:
        resources.append(fname.split(".")[0])
    else:
      for dirName in dirs:
        resources.append(dirName)
          
  return resources

'''
Read in all of the clad files that map CLAD definitions to resource paths
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


  scriptConfigPath = projectRoot + "/project/build-scripts/VerifyExternalsCladMapsConfig.json"
  scriptConfigData = open(scriptConfigPath)
  scriptConfigJSON = json.load(scriptConfigData)
  scriptConfigData.close()

  for externalsEntry in scriptConfigJSON:
    mapPath  = projectRoot + externalsEntry["mapPath"]
    cladPath = projectRoot + externalsEntry["cladPath"]

    externalResources = []
    # gather resources
    for resourceEntry in externalsEntry["resources"]:
      resoucePath = externalsPath + resourceEntry["basePath"]
      resourcesAreFiles   = resourceEntry["resourcesAreFiles"]

      # gather new resources and verify that all file names are unique
      newResources = getExternalsResources(resoucePath, resourcesAreFiles)
      resourceDiff = [x for x in newResources if x in externalResources]
      if len(resourceDiff) != 0:
        print("Error: the following file names occur more than once in resource paths")
        for name in resourceDiff:
          print("  - " + name)
        
        exit_code = 1

      externalResources += newResources

    ####
    ## Verify all file paths are valid
    ####
    shouldSkipEntry = False
    if not os.path.isfile(mapPath):
      print("Error: Map Path " + mapPath + " is invalid")
      shouldSkipEntry = True
    if not os.path.isfile(cladPath):
      print("Error: Clad Path " + cladPath + " is invalid")
      shouldSkipEntry = True

    if shouldSkipEntry:
      exit_code = 1
      continue

    #########
    # Put all lines into the array
    cladNamesArray = []
    with open(cladPath, 'r') as cladNamesFile:
      cladNamesArray = cladNamesFile.readlines()

    # Iterate over array to find indexes of opening/closing bracket
    startIdx = 0
    endIdx = 0
    for i in range(len(cladNamesArray)):
      strippedLine = cladNamesArray[i].strip()
      if strippedLine == "{":
        startIdx = i
      if strippedLine == "}":
        endIdx = i

    cladNamesArray = cladNamesArray[startIdx + 1:endIdx - 1]
    # strip out commas, empty lines and comments
    cladNamesArray = [entry.strip().split(",")[0] for entry in cladNamesArray]
    cladNamesArray = [entry for entry in cladNamesArray if len(entry) > 0]
    cladNamesArray = [entry for entry in cladNamesArray if entry[0] != "/"]

    #################

    mapData = open(mapPath)
    mapJson = json.load(mapData)
    mapData.close()

    # Engine expects all clad maps to use the same key
    cladKey = "CladEvent"
    resourceKey = externalsEntry["resourceKey"]

    # subtract one for Count enum
    #if len(externalResources) != len(cladNamesArray):
    #  print("Warning: Missing resources or clad defs for " + mapPath + ": resources " + str(len(externalResources)) + " but " + str((len(cladNamesArray) - 1)) + " clad values")

    for pair in mapJson:
      if( pair[resourceKey] not in externalResources ):
        print("Invalid resource in " + mapPath + " " + str(pair[resourceKey]))
        exit_code = 1
      elif( pair[cladKey] not in cladNamesArray ):
        print("Invalid Cladname in " + cladPath + " " + str(pair[cladKey]))
        exit_code = 1

  return exit_code


if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    result = run(args)
    exit(result)
