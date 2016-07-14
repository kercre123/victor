#!/usr/bin/python

import sys
import argparse
import os
import os.path
import tarfile
import shutil

#Assumes that script is being called from project root
sys.path.insert(0, os.getcwd() + '/project/build-scripts/webots')
print sys.path
import webotsTest
from webotsTest import get_build_folder
from webotsTest import find_project_root
from webotsTest import BUILD_SUBPATH


def ZipDirectory(options):
  sourcePath = os.path.join(options.buildFolder, options.path)

  #name tar based on final folder name
  divIndex = options.path.find(os.pathsep)
  divIndex += 1
  tarPath = os.path.join(options.buildFolder, options.path[divIndex:] +".tar.gz")

  with tarfile.open(tarPath, "w:gz") as tar:
    tar.add(sourcePath, arcname=os.path.basename(sourcePath))

  shutil.rmtree(sourcePath)

# executes main script logic
def main(scriptArgs):
  # parse arguments
  version = '1.0'
  parser = argparse.ArgumentParser(
  	formatter_class=argparse.RawDescriptionHelpFormatter,
  	description='Runs Webots functional tests', 
  	version=version
  	)

  parser.add_argument('-z', 
                      dest='zip', 
                      action='store_true',
                      help='Zip up all items as a build artifact')

  parser.add_argument('-p', 
                    dest='path',
                    required=True,
                    help='Path relative to build folder')

  (options, args) = parser.parse_known_args(scriptArgs)

  options.projectRoot = find_project_root()
  options.buildType = "Debug"
  options.buildFolder = get_build_folder(options)

  destFolder = os.path.join(options.buildFolder, options.path)

  if not os.path.exists(destFolder):
    os.makedirs(destFolder)

  if options.zip:
    ZipDirectory(options)

  return 0;

if __name__ == '__main__':
  args = sys.argv
  sys.exit(main(args))