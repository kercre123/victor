#!/usr/bin/python

import os
import os.path
import shutil
import errno
import subprocess
import textwrap
import sys
import argparse
import time
import glob
import xml.etree.ElementTree as ET
from decimal import Decimal
import threading
import tarfile
import logging

#set up default logger
UtilLog = logging.getLogger('webots.test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)


class WorkContext(object): pass


def mkdir_p(path):
  try:
    os.makedirs(path)
  except OSError as exc: # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else: 
      raise



# build unittest executable
def build(options):
  # prepare paths
  project = os.path.join(options.projectRoot, 'generated/mac')
  derivedData = os.path.join(options.projectRoot, 'generated/mac/DerivedData')


  # prepare build command
  buildCommand = [
    'xcodebuild', 
    '-project', os.path.join(options.projectRoot, 'generated/mac/cozmoGame.xcodeproj'),
    '-target', 'webotsControllers',
    '-sdk', 'macosx',
    '-configuration', options.buildType,
    'SYMROOT=' + derivedData,
    'OBJROOT=' + derivedData,
    'build'
    ]

  UtilLog.debug('build command ' + ' '.join(buildCommand))
  # return true if build is good
  return subprocess.call(buildCommand) == 0


# runs webots test
def runWebots(options):
  # prepare run command
  runCommand = [
    '/Applications/Webots/webots', 
    '--stdout', 
    '--stderr',
    # '--minimize',
    '--mode=realtime',
    os.path.join(options.projectRoot, 'simulator/worlds/buildServer.wbt'),
    ]

  UtilLog.debug('run command ' + ' '.join(runCommand))

  buildFolder = os.path.join(options.projectRoot, 'build/mac/', options.buildType)
  mkdir_p(buildFolder)
  logFileName = os.path.join(buildFolder, 'webots_out.txt')
  logFile = open(logFileName, 'w')
  startedTimeS = time.time()
  subprocess.call(runCommand, stdout = logFile, stderr = logFile, cwd=buildFolder)
  ranForS = time.time() - startedTimeS
  logFile.close()

  UtilLog.info('webots run took %f seconds' % (ranForS))



# sleep for some time, then kill webots if needed
def stopWebots(options):
  time.sleep(35)
  # kill webots
  # prepare run command
  runCommand = [
    'pkill', 
    'webots', 
    ]
  # execute
  subprocess.call(runCommand)

# cleans up webots IPC facilities
def cleanWebots(options):
  # kill webots IPC leftovers
  runCommand = [
    os.path.join(options.projectRoot, 'lib/anki/cozmo-engine/simulator/kill_ipcs.sh'), 
    ]
  # execute
  result = subprocess.call(runCommand)
  if result != 0:
    return False
  return True


# runs all threads groups
# returns true if all tests suceeded correctly
def runAll(options):

  runWebotsThread = threading.Thread(target=runWebots, args=[options])
  stopWebotsThread = threading.Thread(target=stopWebots, args=[options])
  runWebotsThread.start()
  stopWebotsThread.start()
  runWebotsThread.join()
  stopWebotsThread.join()

  buildFolder = os.path.join(options.projectRoot, 'build/mac/', options.buildType)
  logFileName = os.path.join(buildFolder, 'webots_out.txt')
  return parseOutput(options, logFileName)



# returns true if there were no errors in the log file
def parseOutput(options, logFile):
  # read log file output
  fileHandle = open(logFile, 'r')
  lines = [line.strip() for line in fileHandle]
  fileHandle.close()
  testCompleted = False
  errorCount = 0
  warningCount = 0

  for line in lines:
    if 'TestController.Update : all tests completed' in line:
      testCompleted = True
    if 'Error' in line or 'ERROR' in line:
      errorCount = errorCount + 1
    if 'Warn' in line:
      warningCount = warningCount + 1

  return (testCompleted, errorCount, warningCount)


# tarball valgrind output files together
def tarball(options):
  buildFolder = os.path.join(options.projectRoot, 'build/mac/', options.buildType)
  logFileName = os.path.join(buildFolder, 'webots_out.txt')
  tar = tarfile.open(os.path.join(buildFolder, "webots_out.tar.gz"), "w:gz")
  tar.add(logFileName, arcname=os.path.basename(logFileName))
  tar.close()
      

# executes main script logic
def main(scriptArgs):

  # parse arguments
  version = '1.0'
  parser = argparse.ArgumentParser(
  	# formatter_class=argparse.ArgumentDefaultsHelpFormatter,
  	formatter_class=argparse.RawDescriptionHelpFormatter,
  	description='runs valgrind on set of unittest', 
  	epilog=textwrap.dedent('''
      available modes description:
      	short - runs all unittests that run under 100 milliseconds
      	all - runs all unittests
	'''),
  	version=version
  	)
  parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                      help='prints debug output')
  parser.add_argument('--buildType', '-b', dest='buildType', action='store', default='Debug',
                      help='build types [ Debug, Release ]. (default: Debug)')
  parser.add_argument('--projectRoot', dest='projectRoot', action='store',
                      help='location of the project root')
  (options, args) = parser.parse_known_args(scriptArgs)

  if options.debug:
    UtilLog.setLevel(logging.DEBUG)
  else:
    UtilLog.setLevel(logging.INFO)

  # if no project root fund - make one up
  if not options.projectRoot:
    # go to the script dir, so that we can find the project dir
  	# in the future replace this with command line param
  	selfPath = os.path.dirname(os.path.realpath(__file__))
  	os.chdir(selfPath)

  	# find project root path
  	projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")
  	options.projectRoot = projectRoot
  else:
    options.projectRoot = os.path.normpath(os.path.join(os.getcwd(), options.projectRoot))
  os.chdir(options.projectRoot)

  UtilLog.debug(options)

  # build the project first
  if not build(options):
    UtilLog.error("ERROR build failed")
    return 1

  # run the tests
  (testCompleted, errorCount, warningCount) = runAll(options)
  tarball(options)
  # UtilLog.info("webot error count %d warning count %d" % (errorCount, warningCount))
  print '##teamcity[buildStatisticValue key=\'WebotsErrorCount\' value=\'%d\']' % (errorCount)
  print '##teamcity[buildStatisticValue key=\'WebotsWarningCount\' value=\'%d\']' % (warningCount)

  returnValue = 0;
  cleanResult = cleanWebots(options)
  if not cleanResult:
    # how do we notify the build system tha there is something wrong, but it is not this build specific?
    returnValue = returnValue + 1


  if not testCompleted:
    UtilLog.error("webot test ERROR")
    returnValue = returnValue + 1

  return returnValue



if __name__ == '__main__':
  args = sys.argv
  sys.exit(main(args))
