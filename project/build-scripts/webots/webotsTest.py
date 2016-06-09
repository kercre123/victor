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
from datetime import datetime

#set up default logger
UtilLog = logging.getLogger('webots.test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)

runningMultipleTests = False
curTime = datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S')

worldFileTestNamePlaceHolder = '%COZMO_SIM_TEST%'
generatedWorldFileName = '__generated__.wbt'

# The path after project root where the webots world files are located.
WEBOTS_WORLD_SUBPATH = os.path.join('simulator', 'worlds')
BUILD_SUBPATH = os.path.join('build', 'mac')


class WorkContext(object): pass
class TemplateStringNotFoundException(Exception): 
  def __init__(self, template_string, source_data):
    print("Template string was not found in source data!")
    print("Template String: {0}\nSource Data: {1}".format(template_string, source_data))

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
    '-project', os.path.join(options.projectRoot, 'generated/mac/cozmoEngine.xcodeproj'),
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
def runWebots(options, resultQueue, test):
  # prepare run command
  runCommand = [
    '/Applications/Webots/webots', 
    '--stdout',
    '--stderr',
    '--disable-modules-download',
    '--minimize',  # Ability to start without graphics is on the wishlist
    '--mode=fast',
    os.path.join(options.projectRoot, WEBOTS_WORLD_SUBPATH, generatedWorldFileName),
    ]

  if options.showGraphics:
    runCommand.remove('--stdout')
    runCommand.remove('--stderr')
    runCommand.remove('--minimize')
    runCommand = ['--mode=run' if x=='--mode=fast' else x for x in runCommand]

  UtilLog.debug('run command ' + ' '.join(runCommand))

  buildFolder = get_build_folder(options)
  mkdir_p(buildFolder)

  logFileName = get_log_file_path(buildFolder, test, curTime)

  logFile = open(logFileName, 'w')
  startedTimeS = time.time()
  returnCode = subprocess.call(runCommand, stdout = logFile, stderr = logFile, cwd=buildFolder)
  resultQueue.put(returnCode)
  ranForS = time.time() - startedTimeS
  logFile.close()

  UtilLog.info('webots run took %f seconds' % (ranForS))


def WaitUntil(conditionFcn, timeout, period=0.25):
  mustend = time.time() + timeout
  while time.time() < mustend:
    if conditionFcn(): return True
    time.sleep(period)
  return False

def IsWebotsRunning():
  process = subprocess.Popen("ps -ax | grep [/]Applications/Webots/webots.app", shell=True, stdout=subprocess.PIPE)
  result = process.communicate()[0]
  if len(result) > 0:
    return True
  return False
  
def IsWebotsNotRunning():
  return not IsWebotsRunning()

# sleep for some time, then kill webots if needed
def stopWebots(options):
  # kill all webots processes
  ps   = subprocess.Popen(('ps', 'Aux'), stdout=subprocess.PIPE)
  grep = subprocess.Popen(('grep', '[w]ebots'), stdin=ps.stdout, stdout=subprocess.PIPE)
  grepMinusThisProcess = subprocess.Popen(('grep', '-v', os.path.dirname(options.cfg_path)), stdin=grep.stdout, stdout=subprocess.PIPE)
  awk  = subprocess.Popen(('awk', '{print $2}'), stdin=grepMinusThisProcess.stdout, stdout=subprocess.PIPE)
  kill = subprocess.Popen(('xargs', 'kill', '-9'), stdin=awk.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  out,err = kill.communicate()
  if '' != err:
    print err
  print out

  if (WaitUntil(IsWebotsNotRunning, 5, 0.5)):
    cleanWebots(options)
    return True
    
  print 'ERROR: Webots instance was running, but did not die when killed'
  return False
    

# cleans up webots IPC facilities
def cleanWebots(options):
  # Are there other Webots processes running? If so, don't kill anything
  # since they may be in use!
  if IsWebotsRunning():
    print 'INFO: An instance of Webots is running. Will not kill IPC facilities'
    return True


  # kill webots IPC leftovers
  runCommand = [
    os.path.join(options.projectRoot, 'simulator/kill_ipcs.sh'), 
    ]
  # execute
  result = subprocess.call(runCommand)
  if result != 0:
    return False
  return True


def SetTestStatus(testName, status, totalResultFlag, testStatuses):
  testStatuses[testName] = status
  UtilLog.info('Test ' + testName + (' FAILED' if status <> 0 else ' PASSED'))
  if status <> 0 or not totalResultFlag:
    return False
  return True


def generate_file_with_replace(generated_file_path, source_file_path, 
                               template_string, replacement_string):
  """Generates a new file by copying a existing file and replacing a string inside.

  Args:
    generated_file_path (string) --
      Path of the file to be generated

    source_file_path (string) --
      Path of the existing file to be copied with a portion replaced

    template_string (string) --
      The string to look for inside source_file_path to be replaced. TemplateStringNotFoundException
      will be raised if the string cannot be found.

    replacement_string (string) --
      The string to replace template_string with.

  Raises:
    TemplateStringNotFoundException -- 
      if `template_string` does not exist inside `source_file_path`, see 
      generate_file_with_replace_pass_data()
  """

  with open(source_file_path, 'r') as source_file:
    source_data = source_file.read()

  generate_file_with_replace_pass_data(generated_file_path, source_data, 
                                       template_string, replacement_string)

def generate_file_with_replace_pass_data(generated_file_path, source_data, 
                                         template_string, replacement_string):
  """Generates a new file by replacing a string inside given data.

  See generate_file_with_replace(). This function is useful if there are many files to be generated
  from the same source file, albeit with different replacement_string(s). This function allows
  source_data to be passed in directly instead of passing a file path so that the IO operation to
  open the source file only needs to happen once, preceding the call of this funciton. 

  Args:
    generated_file_path (string) --
      Path of the file to be generated

    source_data (string) --
      Data of the source file.

    template_string (string) --
      The string to look for inside source_file_path to be replaced. TemplateStringNotFoundException
      will be raised if the string cannot be found.

    replacement_string (string) --
      The string to replace template_string with.

  Raises:
    TemplateStringNotFoundException -- 
      if `template_string` does not exist inside `source_data`
  """

  # import pdb;pdb.set_trace()
  if template_string not in source_data:
    raise TemplateStringNotFoundException(template_string, source_data)

  # Generate world file with appropriate args passed into test controller
  generated_data = source_data.replace(template_string, replacement_string)

  with open(generated_file_path, 'w+') as generated_file:
    generated_file.write(generated_data)


# runs all threads groups
# returns true if all tests suceeded correctly
def runAll(options):
  # Get list of tests and world files from config
  config = ConfigParser.ConfigParser()
  config.read(options.cfg_path)
  testNames = config.sections()
  testStatuses = {}
  allTestsPassed = True
  totalErrorCount = 0
  totalWarningCount = 0

  for test in testNames:
    if not config.has_option(test, 'world_file'):
      UtilLog.error('ERROR: No world file specified for test ' + test + '.')
      allTestsPassed = SetTestStatus(test, -10, allTestsPassed, testStatuses)
      continue
    
    baseWorldFile = config.get(test, 'world_file')
    UtilLog.info('Running test: ' + test + ' in world ' + baseWorldFile)
    

    try:
      source_file_path = os.path.join(WEBOTS_WORLD_SUBPATH, baseWorldFile)
      generated_file_path = os.path.join(options.projectRoot, WEBOTS_WORLD_SUBPATH, generatedWorldFileName)

      generate_file_with_replace(generated_file_path, source_file_path,
                                 worldFileTestNamePlaceHolder, test)

    except TemplateStringNotFoundException:
      UtilLog.error('ERROR: ' + worldFile + ' is not a valid test world. (No ' + worldFileTestNamePlaceHolder + ' found.)')
      allTestsPassed = SetTestStatus(test, -11, allTestsPassed, testStatuses)


    # Run test in thread
    testResultQueue = Queue.Queue(1)
    runWebotsThread = threading.Thread(target=runWebots, args=[options, testResultQueue, test])
    runWebotsThread.start()
    runWebotsThread.join(120) # with timeout
    
    # Check if timeout exceeded
    if runWebotsThread.isAlive():
      UtilLog.error('ERROR: ' + test + ' exceeded timeout.')
      stopWebots(options)
      allTestsPassed = SetTestStatus(test, -12, allTestsPassed, testStatuses)
      print 'allTestsPassed ' + str(allTestsPassed)
      continue

    # Check log for crashes, errors, and warnings
    # TODO: Crashes affect test result, but errors and warnings do not. Should they?
    buildFolder = get_build_folder(options)
    logFileName = get_log_file_path(buildFolder, test, curTime)

    (crashCount, errorCount, warningCount) = parseOutput(options, logFileName)
    totalErrorCount += errorCount
    totalWarningCount += warningCount

    # Check for crashes
    if crashCount > 0:
      UtilLog.error('ERROR: ' + test + ' had a crashed controller.');
      allTestsPassed = SetTestStatus(test, -13, allTestsPassed, testStatuses)
      continue

    # Get return code from test
    if testResultQueue.empty():
      UtilLog.error('ERROR: No result code received from ' + test)
      allTestsPassed = SetTestStatus(test, -14, allTestsPassed, testStatuses)
      continue
    
    allTestsPassed = SetTestStatus(test, testResultQueue.get(), allTestsPassed, testStatuses)

  return (allTestsPassed, testStatuses, totalErrorCount, totalWarningCount, len (testNames))



# returns true if there were no errors in the log file
def parseOutput(options, logFile):
  # read log file output
  fileHandle = open(logFile, 'r')
  lines = [line.strip() for line in fileHandle]
  fileHandle.close()
  crashCount = 0
  errorCount = 0
  warningCount = 0

  for line in lines:
    if 'The process crashed some time after starting successfully.' in line:
      crashCount = crashCount + 1
    if 'Error' in line or 'ERROR' in line:
      errorCount = errorCount + 1
    if 'Warn' in line:
      warningCount = warningCount + 1

  return (crashCount, errorCount, warningCount)


# tarball valgrind output files together
def tarball(options):
  buildFolder = get_build_folder(options)
  tar = tarfile.open(os.path.join(buildFolder, "webots_out.tar.gz"), "w:gz")
  
  config = ConfigParser.ConfigParser()
  config.read(options.cfg_path)
  testNames = config.sections()
  for test in testNames:
    logFileName = get_log_file_path(buildFolder, test, curTime)
    tar.add(logFileName, arcname=os.path.basename(logFileName))
  
  tar.close()
      
def find_project_root():
  # go to the script dir, so that we can find the project dir
  # in the future replace this with command line param
  selfPath = os.path.dirname(os.path.realpath(__file__))
  os.chdir(selfPath)

  # find project root path
  projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")
  return projectRoot

def get_build_folder(options):
  """Build the build folder path (where the logs are exported)"""
  return os.path.join(options.projectRoot, BUILD_SUBPATH, options.buildType)

def get_log_file_path(log_folder, test_name, timestamp = ""):
  """Returns what the log file names should be.

  log_folder (string) --
    the path where the log files should live

  test_name (string) --
    name of the controller class for the test that is outputting the logs

  timestamp (string, optional)-- 
    timestamp of the test run; needs to be provided if runningMultipleTests is true
  """
  if runningMultipleTests:
    logFileName = os.path.join(log_folder, 'webots_out_' + test_name + '_' + timestamp + '.txt')
  else:
    logFileName = os.path.join(log_folder, 'webots_out_' + test_name + '.txt')

  return logFileName


# executes main script logic
def main(scriptArgs):

  # parse arguments
  version = '1.0'
  parser = argparse.ArgumentParser(
  	# formatter_class=argparse.ArgumentDefaultsHelpFormatter,
  	formatter_class=argparse.RawDescriptionHelpFormatter,
  	description='Runs Webots functional tests', 
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
  parser.add_argument('--numRuns', dest='numRuns', action='store', default=1,
                      help='run the tests this many times also saves logs with timestamps so they arent overwritten')
  parser.add_argument('--configFile', dest='configFile', action='store', default='webotsTests.cfg',
                      help='Name of .cfg file to use, should be located in the same directory as this file.')
  (options, args) = parser.parse_known_args(scriptArgs)

  if options.debug:
    UtilLog.setLevel(logging.DEBUG)
  else:
    UtilLog.setLevel(logging.INFO)



  if options.projectRoot:
    options.projectRoot = os.path.normpath(os.path.join(os.getcwd(), options.projectRoot))
  else:
    options.projectRoot = find_project_root()


  os.chdir(options.projectRoot)

  options.cfg_path = os.path.join('project', 'build-scripts', 'webots', options.configFile)
  assert(os.path.isfile(os.path.join(options.projectRoot, options.cfg_path)))

  UtilLog.debug(options)

  # build the project first
  if not build(options):
    UtilLog.error("ERROR build failed")
    return 1

  # if we are running multiple tests set the flag
  if(int(options.numRuns) > 1):
    global runningMultipleTests
    runningMultipleTests= True

  num_of_failed_tests = 0
  num_of_passed_tests = 0
  num_of_total_tests = 0

  for _ in range(0, int(options.numRuns)):
    # save current time for logs
    global curTime
    curTime = datetime.strftime(datetime.now(), '%Y-%m-%d %H:%M:%S')
  
    # Kill webots in case it's running
    stopWebots(options)
    
    # run the tests
    (testsSucceeded, testResults, totalErrorCount, totalWarningCount, testCount) = runAll(options)
    tarball(options)

    print 'Test results: '
    for key,val in testResults.items():
      print key + ' : ' + str(val)

    returnValue = 0;
    stopResult = stopWebots(options)
    if not stopResult:
      # how do we notify the build system that there is something wrong, but it is not this build specific?
      returnValue = returnValue + 1

    print '##teamcity[buildStatisticValue key=\'WebotsErrorCount\' value=\'%d\']' % (totalErrorCount)
    print '##teamcity[buildStatisticValue key=\'WebotsWarningCount\' value=\'%d\']' % (totalWarningCount)
    print '##teamcity[buildStatisticValue key=\'WebotsTestCount\' value=\'%d\']' % (testCount)

    if not testsSucceeded:
      UtilLog.error("*************************")
      UtilLog.error("SOME TESTS FAILED")
      UtilLog.error("*************************")
      returnValue = returnValue + 1
      num_of_failed_tests += 1
    else:
      UtilLog.info("*************************")
      UtilLog.info("ALL " + str(len(testResults)) + " TESTS PASSED")
      UtilLog.info("*************************")
      num_of_passed_tests += 1

    num_of_total_tests = num_of_failed_tests + num_of_passed_tests
    UtilLog.info("Run #{0}".format(num_of_total_tests))

  UtilLog.info(
    "{0}/{1} ({2:.1f}%) runs failed".format(num_of_failed_tests,
                                            num_of_total_tests,
                                            float(num_of_failed_tests)/num_of_total_tests*100)
  )

  return returnValue



if __name__ == '__main__':
  args = sys.argv
  sys.exit(main(args))
