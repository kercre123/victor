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
import ConfigParser
import Queue

#set up default logger
UtilLog = logging.getLogger('webots.test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)


worldFileTestNamePlaceHolder = '%COZMO_SIM_TEST%'
generatedWorldFileName = '__generated__.wbt'
testStatuses = {}
allTestsPassed = True


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
def runWebots(options, resultQueue):
  # prepare run command
  runCommand = [
    '/Applications/Webots/webots', 
    '--stdout',
    '--stderr',
    '--minimize',  # Ability to start without graphics is on the wishlist
    '--mode=fast',
    os.path.join(options.projectRoot, 'simulator/worlds/' +  generatedWorldFileName),
    ]

  if options.showGraphics:
    runCommand.remove('--minimize')
    runCommand = ['--mode=run' if x=='--mode=fast' else x for x in runCommand]

  UtilLog.debug('run command ' + ' '.join(runCommand))

  buildFolder = os.path.join(options.projectRoot, 'build/mac/', options.buildType)
  mkdir_p(buildFolder)
  logFileName = os.path.join(buildFolder, 'webots_out.txt')
  logFile = open(logFileName, 'w')
  startedTimeS = time.time()
  returnCode = subprocess.call(runCommand, stdout = logFile, stderr = logFile, cwd=buildFolder)
  resultQueue.put(returnCode)
  ranForS = time.time() - startedTimeS
  logFile.close()

  UtilLog.info('webots run took %f seconds' % (ranForS))



# sleep for some time, then kill webots if needed
def stopWebots(options):
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


def SetTestStatus(testName, status):
  testStatuses[testName] = status
  UtilLog.info('Test ' + testName + (' FAILED' if status < 0 else ' PASSED'))
  if status < 0:
    testsSucceeded = False


# runs all threads groups
# returns true if all tests suceeded correctly
def runAll(options):
  
  # Get list of tests and world files from config
  config = ConfigParser.ConfigParser()
  webotsTestCfgPath = 'project/buildServer/steps/webotsTests.cfg'
  config.read(webotsTestCfgPath)
  testNames = config.sections()

  testStatuses = {}

  for test in testNames:
    if not config.has_option(test, 'world_file'):
      UtilLog.error('ERROR: No world file specified for test ' + test + '. Aborting.')
      SetTestStatus(test, -10)
      continue
    
    baseWorldFile = config.get(test, 'world_file')
    UtilLog.info('Running test: ' + test + ' in world ' + baseWorldFile)
    
    # Check if world file contains valid test name place holder
    baseWorldFile = open('simulator/worlds/' + baseWorldFile, 'r')
    baseWorldData = baseWorldFile.read()
    baseWorldFile.close()
    if worldFileTestNamePlaceHolder not in baseWorldData:
      UtilLog.error('ERROR: ' + worldFile + ' is not a valid test world. (No ' + worldFileTestNamePlaceHolder + ' found.)')
      SetTestStatus(test, -11)
      continue

    # Generate world file with appropriate args passed into test controller
    generatedWorldData = baseWorldData.replace(worldFileTestNamePlaceHolder, test)
    generatedWorldFile = open(os.path.join(options.projectRoot, 'simulator/worlds/' + generatedWorldFileName), 'w+')
    generatedWorldFile.write(generatedWorldData)
    generatedWorldFile.close()

    # Run test in thread
    testResultQueue = Queue.Queue(1)
    runWebotsThread = threading.Thread(target=runWebots, args=[options, testResultQueue])
    runWebotsThread.start()
    runWebotsThread.join(60) # with timeout
    
    # Check if timeout exceeded
    if runWebotsThread.isAlive():
      UtilLog.error('ERROR: ' + test + ' exceeded timeout. Aborting')
      stopWebots(options)
      SetTestStatus(test, -12)
      continue

    # Check log for errors and warnings
    # TODO: This doesn't effect test outcome at the moment. Should it?
    buildFolder = os.path.join(options.projectRoot, 'build/mac/', options.buildType)
    logFileName = os.path.join(buildFolder, 'webots_out.txt')
    (errorCount, warningCount) = parseOutput(options, logFileName)
    # UtilLog.info("webot error count %d warning count %d" % (errorCount, warningCount))
    print '##teamcity[buildStatisticValue key=\'WebotsErrorCount\' value=\'%d\']' % (errorCount)
    print '##teamcity[buildStatisticValue key=\'WebotsWarningCount\' value=\'%d\']' % (warningCount)


    # Get return code from test
    if testResultQueue.empty():
      UtilLog.error('ERROR: No result code received from ' + test)
      SetTestStatus(test, -13)
    
    SetTestStatus(test, testResultQueue.get())
      
  return (allTestsPassed, testStatuses)



# returns true if there were no errors in the log file
def parseOutput(options, logFile):
  # read log file output
  fileHandle = open(logFile, 'r')
  lines = [line.strip() for line in fileHandle]
  fileHandle.close()
  errorCount = 0
  warningCount = 0

  for line in lines:
    if 'Error' in line or 'ERROR' in line:
      errorCount = errorCount + 1
    if 'Warn' in line:
      warningCount = warningCount + 1

  return (errorCount, warningCount)


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
  parser.add_argument('--showGraphics', dest='showGraphics', action='store_true',
                      help='display Webots window')
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
  (testsSucceeded, testResults) = runAll(options)
  tarball(options)

  returnValue = 0;
  cleanResult = cleanWebots(options)
  if not cleanResult:
    # how do we notify the build system tha there is something wrong, but it is not this build specific?
    returnValue = returnValue + 1


  if not testsSucceeded:
    UtilLog.error("*************************")
    UtilLog.error("SOME TESTS FAILED")
    UtilLog.error("*************************")
    returnValue = returnValue + 1
  else:
    UtilLog.info("*************************")
    UtilLog.info("ALL " + str(len(testStatuses)) + " TESTS PASSED")
    UtilLog.info("*************************")

  return returnValue



if __name__ == '__main__':
  args = sys.argv
  sys.exit(main(args))
