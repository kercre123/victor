#!/usr/bin/env python3

import os
import errno
import glob
import inspect
import subprocess
import sys
import argparse
import signal
import time
import threading
import tarfile
import logging
import configparser
from datetime import datetime
import json
from enum import Enum, unique
from functools import lru_cache
import shutil
import getpass

# Root folder path of cozmo-one repo
COZMO_ENGINE_ROOT = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip(b"\r\n").decode("utf-8")

BUILD_TOOLS_ROOT = os.path.join(COZMO_ENGINE_ROOT, 'tools', 'build', 'tools')
sys.path.insert(0, BUILD_TOOLS_ROOT)
from ankibuild import util

#set up default logger
UtilLog = logging.getLogger('webots.test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('{name} {levelname} - {message}', style='{')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)

CONTROLLER_ARGS_PLACEHOLDER = r'%COZMO_SIM_TEST%'
COZMOBOT_PLACEHOLDER = r'%COZMOBOT%'
GENERATED_WORLD_FILE_NAME = '__generated__.wbt'

# Path to firewall control app
SOCKETFILTERFW = os.path.join(os.sep + 'usr', 'libexec', 'ApplicationFirewall', 'socketfilterfw')

# Name of user created certificate for signing webots executables to be
# excepted by the firewall
CERTIFICATE_NAME = "WebotsFirewall"

class ThreadOutput(object):
  test_return_code = None

class TemplateStringNotFoundException(Exception):
  def __init__(self, template_string, source_data):
    UtilLog.error("Template string was not found in source data!")
    UtilLog.error("Template String: {0}\nSource Data: {1}".format(template_string, source_data))


class ResultCode(Enum):
  succeeded = 0
  failed = 1


@unique
class BuildType(Enum):
  Debug = 0
  Release = 1

GeneratorType = Enum('GeneratorType', names=['Ninja', 'Xcode', 'Unix Makefiles'])

@unique
class ForwardWebotsLogLevel(Enum):
  no_forwarding = 0
  only_errors = 1
  only_teamcity_stats_and_errors = 2
  full_forwarding = 3


@lru_cache()
def get_subpath(partial_path, *args):
  """Joins `partial_path` with project root folder and returns the full path to the subfolder.

  This exists instead of just using os.path.join(COZMO_ENGINE_ROOT, 'path', 'to', 'folder')
  everywhere so that it is much easier to find and replace a particular path if the folder moves in
  the future. Having paths in a path/to/folder as opposed to os.path.join(...) also makes the folder
  paths easier to read.


  Arguments:
    partial_path (string) - a hardcoded unix style path

    *args -
      Extend the join command. Useful for things in the path that are not static/constant so that
      you don't have code like os.path.join(get_subpath("generated/mac"), build_type) and have
      additional os.path.join littered everywhere.

  """
  forward_args = partial_path.split(os.sep) + list(args)
  if forward_args:
    return os.path.join(COZMO_ENGINE_ROOT, *(forward_args))
  else:
    return COZMO_ENGINE_ROOT

def mkdir_p(path):
  try:
    os.makedirs(path)
  except OSError as exc: # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise

def sudo_this(command, password):
  """
  Wrapper to call subprocess commands with sudo.

  command (list) --
    Similar to the `command` parameter in os.Popen(), this list
    consists of space separated items that make up a regular bash command. ie.
    `curl -O http://website.com/file` would be ["curl", "-O",
    "http://website.com/file"]

  password (string) --
    The password you would normally enter for a sudo command.
  """

  if password is None:
    raise AttributeError("""Password needs to be passed as an argument to this script in order to
    call commands with sudo. Ex. if your firewall is on, sudo is needed to add executables to the
    exception list.""")

  password_pipe = ["echo", password]
  sudo_command = ["sudo"] + ["-S"] + command
  output = util.File.evaluate(password_pipe, sudo_command)
  return output

def is_firewall_enabled(password):
  output = firewall_cli(["--getglobalstate"], password, sudo=False)
  return b"Firewall is enabled. (State = 1)" in output

def firewall_cli(flags, password, sudo=True, executable_path=""):
  """
  Wrapper for /usr/libexec/ApplicationFirewall/socketfilterfw calls. Requires
  sudo or socketfilterfw will silently fail. Also checks that socketfilterfw
  exists before calling it.

  flags (list) --
    Flags to be joined in the command. ex. ["--add"] or ["--setblockall", "on"].
    Consult `/usr/libexec/ApplicationFirewall/socketfilterfw -h` for list of
    avaliable options.

  executable_path (optional) --
    The path of executable for argument of `--add`, `--unblock`, etc.
  """

  # Check that socketfilterfw exists and is executable
  assert os.path.isfile(SOCKETFILTERFW)
  assert os.access(SOCKETFILTERFW, os.X_OK)

  command = [SOCKETFILTERFW]

  # Tack on arguments
  # Difference in extend and append because one is a list and the other is a string
  command.extend(flags)

  if executable_path != "":
    command.append(executable_path)

  if sudo:
    # Pipes password into sudo
    output = sudo_this(command, password)
  else:
    output = util.File.evaluate(command)

  UtilLog.info(output.decode("utf-8").rstrip("\n"))

  return output

def is_certificate_installed():
  """
  Installs a custom certificate named CERTIFICATE_NAME (declared at the top of
  this file), is installed already in the user's keychain.
  """
  command = [
    "security",
    "find-certificate",
    "-c",
    CERTIFICATE_NAME
  ]

  try:
    subprocess.check_output(command)
  except subprocess.CalledProcessError as e:
    # The `security find-certificate` command will return an error code of 44 if the given
    # certificate name isn't found in the keychain
    if e.returncode == 44:
      return False
    else:
      raise e

  return True


def install_certificate(password):
  """
  Installs a custom certificate named CERTIFICATE_NAME (declared at the top of
  this file), which exists in the same directory as this python file, to the
  user's keychain to allow for codesigning to bypass the firewall.
  """

  certificate_path = get_subpath(os.path.join("project","build-scripts","webots",""), "{0}.p12".format(CERTIFICATE_NAME))

  command = [
    "security",
    "import",
    certificate_path,
    "-k",
    "login.keychain",
    "-t",
    "priv",
    "-f",
    "pkcs12",
    "-A",
    "-P",
    ""
  ]

  output = sudo_this(command, password)


  if "1 identity imported." not in output:
    raise RuntimeError("""The WebotsFirewall certificate could not be imported.
                          Output from `security import`: {0}""".format(output))

def get_build_name(generator, build_type):
  if generator.name == 'Ninja':
    return build_type.name
  if generator.name == 'Xcode':
    return build_type.name + '-Xcode'
  if generator.name == 'Unix Makefiles':
    return build_type.name + '-Makefiles'
  raise 'Unexpected generator ' + generator.name

def sign_webot_executables(generator, build_type, password):
  """
  Sign webot executables generated by xcodebuild such that machines with
  firewall enabled will not have a popup asking user to allow or deny the
  connection. Will also install the certificate required for codesign if
  it is not installed.
  """

  if not is_firewall_enabled(password):
    # No need to sign executables if firewall is not enabled.
    UtilLog.info("Firewall not detected as enabled.")
    return

  if not is_certificate_installed():
    install_certificate(password)

  assert is_certificate_installed()

  UtilLog.info("Your password may be needed in order to add the webots executables to the firewall exception list.")

  build_name = get_build_name(generator, build_type)
  executables_folder = get_subpath(os.path.join("_build","mac"), build_name, "bin")
  executables = glob.glob(os.path.join(executables_folder, 'webotsCtrl*'))

  codesign_command = [
    'codesign',
    '-f',
    '-s',
    CERTIFICATE_NAME
  ]

  # Add the executables to the firewall list and explicitly allow incoming connections
  for exe_path in executables:
    if os.path.isfile(exe_path):
      firewall_cli(["--add"], password, executable_path=exe_path)
      firewall_cli(["--unblock"], password, executable_path=exe_path)
      sudo_this(codesign_command + [exe_path], password)

  # There is a strange issue where select tests will fail for no reason if
  # firewall is not reset like below (CST_RobotKidnapping and
  # CST_VariableHeightMedium) are the two known ones. The problem can also
  # seemingly be solved by waiting around 3 minutes after the firewall signing
  # operations. Not entirely sure either method is actually the solution as the
  # tests fail nondeterministically, and they don't necessarily fail together at
  # the same time. ie. The first might fail and the other might pass on the
  # first run, but then on the second run the first one might run and then the
  # other might fail.

  firewall_cli(["--setglobalstate", "off"], password)
  firewall_cli(["--setglobalstate", "on"], password)

  UtilLog.info("sign_webot_executables() finished")


# build unittest executable
def build(generator, build_type):
  victor_build_script_path = get_subpath(os.path.join("project","victor","build-victor.sh"))
  build_command = [
          victor_build_script_path,
          '-p', 'mac',              # build the mac project (with webots sim controllers)
          '-g', generator.name,
          '-c', build_type.name
          ]

  UtilLog.debug('build command {command}'.format(command=' '.join(build_command)))

  return subprocess.call(build_command) == 0

def exec_this(command):
  """
  Run command as subprocess.
  Takes list of ["name", "arg", "arg"...]
  Returns output of executing "name arg arg ...".
  """
  print(' '.join(command))
  output = util.File.evaluate(command)
  output = output.decode('utf-8').rstrip('\n')
  print(output)
  return output

def setup_exe(generator, build_type, exe):
  """
  Execute commands to sign executable, then add it as a firewall exception
  """
  exec_this(['codesign', '-f', '-s', CERTIFICATE_NAME, exe])
  output = exec_this([SOCKETFILTERFW, '--add', exe])
  if 'already a part of the firewall' not in output:
    print("You must run webotsTest.py -g '{}' -b {} --setupFirewall to add {}".format(generator.name, build_type.name, exe))
  output = exec_this([SOCKETFILTERFW, '--unblock', exe])
  if 'is permitted' not in output:
    print("You must run webotsTest.py -g '{}' -b {} --setupFirewall to unblock {}".format(generator.name, build_type.name, exe))

def setup_exes(generator, build_type, exes):
  if not is_firewall_enabled(''):
    print('Firewall is not enabled. No need to sign executables.')
    return
  if not is_certificate_installed():
    print('Firewall certificate is not installed. Unable to sign executables.')
    return
  for exe in exes:
    setup_exe(generator, build_type, exe)

def run_webots(output, wbt_file_path, show_graphics, log_file_name):
  """Run an individual webots simulation and return the return code of the process through `output`.

  Args:
    output (ThreadOutput object) --
      attributes:
      test_return_code - the return code of the webots process

    wbt_file_path (string) --
      The path to the webots file to run.

    show_graphics (boolean) --
      Shows the webots gui and runs simulation at regular speed if true; hides gui and runs in fast
      mode if false

    log_file_name (string) --
      Path to pipe the webots logs to.
  """
  stop_webots()

  run_command = [
    os.path.join(os.sep + 'Applications','Webots.app','webots'),
    '--stdout',
    '--stderr',
    '--minimize',  # Ability to start without graphics is on the wishlist
    '--mode=fast',
    get_subpath(os.path.join("simulator","worlds"), GENERATED_WORLD_FILE_NAME)
    ]

  if show_graphics:
    run_command.remove('--stdout')
    run_command.remove('--stderr')
    run_command.remove('--minimize')
    run_command = ['--mode=run' if x == '--mode=fast' else x for x in run_command]

  UtilLog.debug('run command {command}'.format(command=' '.join(run_command)))

  log_folder = os.path.dirname(log_file_name)
  mkdir_p(log_folder)

  with open(log_file_name, 'w') as log_file:
    start_time = time.time()
    return_code = subprocess.call(run_command, stdout=log_file, stderr=log_file, cwd=log_folder)
    output.test_return_code = return_code
    run_duration = time.time() - start_time

  UtilLog.info("webots run took {x:.1f} seconds".format(x=run_duration))


def wait_until(condition_fn, timeout, period=0.25):
  mustend = time.time() + timeout
  while time.time() < mustend:
    if condition_fn(): return True
    time.sleep(period)
  return False

def is_webots_running():
  process = subprocess.Popen("ps -ax | grep [" + os.sep + "]" + os.path.join("Applications","Webots.app","webots"),
                              shell=True, stdout=subprocess.PIPE)
  result = process.communicate()[0]
  if len(result) > 0:
    return True
  return False

def is_webots_not_running():
  return not is_webots_running()

# sleep for some time, then kill webots if needed
def stop_webots():
  currFile = inspect.getfile(inspect.currentframe())

  # kill all webots processes
  ps   = subprocess.Popen(('ps', 'Auxc'), stdout=subprocess.PIPE)
  grep = subprocess.Popen(('grep', '[w]ebots'), stdin=ps.stdout, stdout=subprocess.PIPE)
  grep_minus_this_process = subprocess.Popen(('grep', '-v', currFile), stdin=grep.stdout, stdout=subprocess.PIPE)
  awk  = subprocess.Popen(('awk', '{print $2}'), stdin=grep_minus_this_process.stdout, stdout=subprocess.PIPE)
  kill = subprocess.Popen(('xargs', 'kill', '-9'), stdin=awk.stdout, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  out, err = kill.communicate()
  if err:
    UtilLog.error(err)
  if out:
    UtilLog.info(out)

  if wait_until(is_webots_not_running, 5, 0.5):
    clean_webots()
    return True

  UtilLog.error('Webots instance was running, but did not die when killed')
  return False


# cleans up webots IPC facilities
def clean_webots():
  # Are there other Webots processes running? If so, don't kill anything
  # since they may be in use!
  if is_webots_running():
    UtilLog.info('An instance of Webots is running. Will not kill IPC facilities')
    return True

  # kill webots IPC leftovers
  run_command = [get_subpath(os.path.join("simulator","kill_ipcs.sh"))]
  result = subprocess.call(run_command)
  if result != 0:
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
      if `template_string` does not exist inside the file at `source_file_path`
  """

  with open(source_file_path, 'r') as source_file:
    source_data = source_file.read()

  if template_string not in source_data:
    raise TemplateStringNotFoundException(template_string, source_data)

  # Generate world file with appropriate args passed into test controller
  generated_data = source_data.replace(template_string, replacement_string)

  with open(generated_file_path, 'w+') as generated_file:
    generated_file.write(generated_data)

def get_tests(config_file_path):
  config = configparser.ConfigParser()
  config.read(config_file_path)
  test_controllers = config.sections()
  tests = {}

  # Check that the config file is valid and has worlds for each test controller
  for test_controller in test_controllers:
    if not config.has_option(test_controller, 'world_file'):
      UtilLog.error('No world file(s) specified for test {test_controller}.'.format(
                      test_controller=test_controller))
      assert config.has_option(test_controller, 'world_file')

    tests[test_controller] = {}
    tests[test_controller]['world_file'] = json.JSONDecoder().decode(config.get(test_controller, 'world_file'))
    if config.has_option(test_controller, 'timeout'):
      tests[test_controller]['timeout'] = config.getfloat(test_controller, 'timeout')

  return tests


def run_tests(tests, log_folder, show_graphics, default_timeout, forward_webots_log_level, num_retries = 0,
              fail_on_error=False, quit_webots_after_test=True):
  """Run webots tests and store the logs.

  Args:
    tests (dict) --
      dictionary of tests to run and parameters for running them. See get_tests() return value.
      test controller (key) -> 'world_file' (key) -> list of worlds to run test controller in
                               'timeout' (key)    -> timeout to use for the test (default is used if this key doesn't exist)

    log_folder (string) --
      Path of directory to keep the webots logs

    show_graphics (boolean) --
      Shows the webots gui and runs simulation at regular speed if true; hides gui and runs in fast
      mode if false

    default_timeout (int) --
      Default max time to allow each test case to run before force quitting it and declaring
      test case as failure (this is overridden if a timeout is specified in the config file)

    forward_webots_log_level (ForwardWebotsLogLevel enum) --
      How much of the webots logs to forward to this script's stdout.

    num_retries (int) --
      When a test fails, the number of times to retry the test before declaring it as failed. Be
      aware of off-by-one gotcha here, if num_retries == 1, that means each test will be run for a
      maximum of two times, if the first run of each test fails.

  Returns:
    test_statuses (dict) --
      Dictionary of test controller name (key) -> test_controllers (value, dict) where:
        test_controllers is a dictionary of webots worlds (key) -> test result (value, ResultCode enum)

    total_error_count (int) --
      the total number of errors that were in the webots logs, summed over every run

    total_warning_count (int) --
      the total number of warnings that were in the webots logs, summed over every run
  """
  test_statuses = {}
  log_file_paths = []
  total_error_count = 0
  total_warning_count = 0

  # Keep track of how many times we get a seg fault from webots binary.
  # Allow this and retry the affected test up to the global max number of times.
  total_sigsegv_count = 0
  max_sigsegv_count = 5

  cur_time = datetime.strftime(datetime.now(), '%Y-%m-%d_%H-%M-%S')
  GENERATED_FILE_PATH = get_subpath(os.path.join("simulator","worlds"), GENERATED_WORLD_FILE_NAME)

  for test_controller, params in tests.items():
    worlds = params['world_file']
    timeout = params.get('timeout', default_timeout)
    test_statuses[test_controller] = {}
    # Loop through all the webots worlds this test controller should run in.
    for world_file in worlds:
      test_result = ResultCode.failed
      UtilLog.info('Running test: {test_controller} in world {world_file}'.format(
                      test_controller=test_controller, world_file=world_file))

      source_file_path = get_subpath(os.path.join("simulator","worlds"), world_file)
      controller_args = test_controller
      if quit_webots_after_test :
        controller_args += ' --quitWebotsAfterTest'
      generate_file_with_replace(GENERATED_FILE_PATH, source_file_path,
                                 CONTROLLER_ARGS_PLACEHOLDER, controller_args)

      # cozmo proto for Victor is CozmoBot2
      generate_file_with_replace(GENERATED_FILE_PATH,
                                 GENERATED_FILE_PATH,
                                 COZMOBOT_PLACEHOLDER,
                                 "DEF cozmoBot CozmoBot2")

      num_retries_counting_up = -1 # only needed for teamcity variable logging

      # need to keep num_retries from changing so the num_retries_mutable can reset to num_retries
      # after the loop
      num_retries_mutable = num_retries

      error_count = 0
      warning_count = 0

      # num_retries_mutable > -1 because even if num_retries_mutable is 0, we want to run the test at least once.
      while num_retries_mutable > -1 and test_result is ResultCode.failed:
        num_retries_mutable -= 1
        num_retries_counting_up += 1
        if num_retries_counting_up > 0:
          UtilLog.info("Retry #{retry_number}".format(retry_number=num_retries_counting_up))

        # Setup log file
        log_file_name = get_log_file_path(log_folder, test_controller, world_file, cur_time, num_retries_counting_up)
        UtilLog.info('results will be logged to {file}'.format(file=log_file_name))
        log_file_paths.append(log_file_name)

        # Clear out files that shouldn't persist between test runs
        clear_out_unwanted_persistent_files()

        # Setup the webots thread to run the test - thread should exit before the join timeout
        output = ThreadOutput()
        run_webots_thread = threading.Thread(target=run_webots, args=[output, GENERATED_FILE_PATH,
                                                                      show_graphics, log_file_name])
        run_webots_thread.start()
        run_webots_thread.join(timeout)

        # Check log for crashes, errors, and warnings
        (crash_count, error_count, warning_count) = parse_output(forward_webots_log_level, log_file_name)

        # Combines the webots test output file and the devLog directory into a human readable format
        didFail = any([(fail_on_error and (error_count > 0)), (crash_count > 0),
                          run_webots_thread.isAlive(), (output.test_return_code is None), (output.test_return_code != 0)])

        generate_combined_webots_devLog(log_folder, log_file_name,
                                        didFail, test_controller, world_file,
                                        cur_time)

        # Check if timeout exceeded
        if run_webots_thread.isAlive():
          UtilLog.error('{test_controller} exceeded timeout.'.format(test_controller=test_controller))
          stop_webots()
          continue

        # Check for crashes
        if crash_count > 0:
          UtilLog.error('{test_controller} had a crashed controller.'.format(test_controller=test_controller))
          continue

        # Get return code from test
        if output.test_return_code is None:
          UtilLog.error('No result code received from {test_controller}'.format(test_controller=test_controller))
          continue

        # check for SIGSEGV (seg fault) return code from Webots. Sometimes Webots itself just crashes
        # before the tests even run, but this shouldn't be considered a test failure, so re-run the test.
        if os.WIFSIGNALED(output.test_return_code) and os.WTERMSIG(output.test_return_code) == signal.SIGSEGV and total_sigsegv_count < max_sigsegv_count:
          UtilLog.info('Received SIGSEGV from webots. Retrying test.')
          total_sigsegv_count += 1
          num_retries_mutable += 1
          num_retries_counting_up -= 1
          continue

        if output.test_return_code != 0:
          UtilLog.error('Received non-zero return code from webots. Received {return_code}'.format(
                          return_code=output.test_return_code))
          continue

        if fail_on_error and error_count > 0:
          UtilLog.error('There was an error in the webots log.')
          continue

        test_result = ResultCode.succeeded

      total_error_count += error_count
      total_warning_count += warning_count

      UtilLog.info("##teamcity[buildStatisticValue key='WebotsNumRetries_{test_controller}_{world_file}' value='{num_of_retries}']".format(
                      test_controller=test_controller, world_file=world_file,
                      num_of_retries=num_retries_counting_up))

      UtilLog.info("Test {test_controller} {test_result}".format(test_controller=test_controller,
                                                                 test_result=test_result.name))
      test_statuses[test_controller][world_file] = test_result

  tarzip_logs(log_file_paths, cur_time)

  return (test_statuses, total_error_count, total_warning_count)


def generate_combined_webots_devLog(log_folder, log_file_name, didFail, test_controller, world_file, cur_time):
  """ Combines the webots test output and the devLog into a single human readable foldername

      This function relies on the fact that we zip up previous devLogs so only the devLog
      which was generated in association with the log file is still a folder
      and that the dev_log_folder path is stable.
      The two asserts in this function ensure that those assumptions are true.
  """

  dev_log_folder = os.path.join(log_folder, "playbackLogs", "webotsCtrlGameEngine2", "gameLogs","devLogger")
  print("Path to DevLogger folder: " + dev_log_folder)
  assert os.path.isdir(dev_log_folder)
  dirs = [entry.path for entry in os.scandir(dev_log_folder) if entry.is_dir()]
  assert len(dirs) == 1

  shutil.copy(log_file_name, dirs[0])
  didFailPrefix = ""
  if(didFail):
    didFailPrefix = "_FAILURE_"

  human_readable_output = os.path.join(dev_log_folder, didFailPrefix + test_controller + "_" + world_file + "_webotsTestTime_" + str(cur_time))
  shutil.move(dirs[0], human_readable_output)


def clear_out_unwanted_persistent_files():
  # clear out the persistant needsState file
  fullPath = get_subpath(os.path.join("simulator","controllers"), "webotsCtrlGameEngine2", "persistent", "nurture", "needsState.json")
  if os.path.exists(fullPath):
    os.remove(fullPath)


def tarzip_logs(log_file_paths, cur_time):
  """Zips up a list of webots logs that are in the same directory.

  The zip file will be located in the same directory as the logs.

  Args:
    log_file_paths (list[str]) --
      A non-empty list of log file paths to zip up.

    cur_time (string) --
      Timestamp that will be included in the name of the zip file.
  """
  if not log_file_paths:
    return

  # Since all the log files should have the same directory just take the directory of the first file
  log_folder = os.path.dirname(log_file_paths[0])
  mkdir_p(log_folder)

  with tarfile.open(os.path.join(log_folder, "webots_out_{0}.tar.gz".format(cur_time)),
                    "w:gz") as tar:
    for log_file_path in log_file_paths:
      tar.add(log_file_path, arcname=os.path.basename(log_file_path))


def parse_output(log_level, log_file):
  with open(log_file, 'r') as f:
    lines = [line.strip() for line in f]
  crash_count = 0
  error_count = 0
  warning_count = 0

  for line in lines:
    if 'The process crashed some time after starting successfully.' in line:
      crash_count += 1

    # [Error] catches controller log errors
    # 'ERROR:' catches potential problems with the webots world/protos
    if '[Error]' in line or 'ERROR:' in line:
      error_count += 1
      if log_level is ForwardWebotsLogLevel.only_errors or log_level is ForwardWebotsLogLevel.only_teamcity_stats_and_errors:
        UtilLog.error("Webots log contained an error: '{error_msg}'".format(error_msg=line))
    if 'Warn' in line:
      warning_count += 1

    if log_level is ForwardWebotsLogLevel.only_teamcity_stats_and_errors:
      if "##teamcity[buildStatisticValue" in line:
        UtilLog.info(line)
    elif log_level is ForwardWebotsLogLevel.full_forwarding:
      UtilLog.info(line)

    # Stop parsing the output if we encounter the end of the webots run, since
    # there can be nonsense error messages while processes are terminating.
    if 'UiGameController.QuitWebots.Result' in line:
      break

  return (crash_count, error_count, warning_count)

def get_build_folder(build_type):
  """Build the build folder path (where the logs are exported)

  build_type (BuildType enum) --
    The targetted build type. Must be a member of the BuildType enum.
  """
  assert build_type in BuildType
  return get_subpath(os.path.join("_build","mac"), build_type.name)

def get_log_file_path(log_folder, test_name, world_file_name, timestamp="", retry_number=0):
  """Returns what the log file names should be.

  log_folder (string) --
    the path where the log files should live

  test_name (string) --
    name of the controller class for the test that is outputting the logs

  world_file_name (string) --
    name of the webots world file the test is running in

  timestamp (string, optional)--
    Timestamp of the test run. Will be included in the file name if provided.

  retry_number (integer)--
    Which retry of the run this is (0 if it is the original run).
  """

  retry_string = ""
  if retry_number > 0:
    retry_string = "_retry{0}".format(retry_number)

  if timestamp == "":
    file_name = "webots_out_{0}_{1}{2}.txt".format(test_name, world_file_name, retry_string)
  else:
    file_name = "webots_out_{0}_{1}_{2}{3}.txt".format(test_name, world_file_name, timestamp, retry_string)

  log_file_path = os.path.join(log_folder, file_name)

  return log_file_path

def any_test_failed(test_results):
  for test in test_results.values():
    if ResultCode.failed in test.values():
      return True
  return False

def any_test_succeeded(test_results):
  for test in test_results.values():
    if ResultCode.succeeded in test.values():
      return True
  return False

# executes main script logic
def main():
  parser = argparse.ArgumentParser(
    # formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description='Runs Webots functional tests'
    )

  parser.add_argument('--debug',
                      '-d',
                      '--verbose',
                      dest='debug',
                      action='store_true',
                      help='prints debug output')

  parser.add_argument('--buildType',
                      '-b',
                      dest='build_type',
                      action='store',
                      default=BuildType.Debug.name,
                      choices=list(map(lambda x: x.name, list(BuildType))),
                      help='The build type to be passed onto xcodebuild. (default: %(default)s)')

  parser.add_argument('--generator',
                      '-g',
                      dest='generator',
                      action='store',
                      default=GeneratorType.Ninja.name,
                      choices=list(map(lambda x: x.name, list(GeneratorType))),
                      help='Cmake generator name (default: %(default)s)')

  parser.add_argument('--showGraphics',
                      dest='show_graphics',
                      action='store_true',
                      help='display Webots window')

  parser.add_argument('--configFile',
                      dest='config_file',
                      action='store',
                      default='webotsTests.cfg',
                      type=str,
                      help='Name of .cfg file to use, should be located in the same directory as this file.')

  parser.add_argument('--numRuns',
                      dest='num_runs',
                      action='store',
                      default=1,
                      type=int,
                      help="""The number of times to run ALL tests again. It is as if the entire
                      script is re-run, but has additional logging features than just actually re-
                      running the script.""")

  parser.add_argument('--password',
                      dest='password',
                      action='store',
                      help="""Your password is needed to add the webots executables to the firewall exception list. Can
                      be omitted if your firewall is disabled. It is requested in plaintext so this script can be re-ran
                      easily and also for build server/steps reasons.""")

  parser.add_argument('--setupFirewall',
                      dest='setupFirewallAndExit',
                      action='store_true',
                      help="""Add the webots executables to the firewall exception list and exit.""")

  parser.add_argument('--setup-exe',
                      '-x',
                      dest='setup_exe_list',
                      action='append',
                      help="""Add webots executable to firewall exception list""")

  parser.add_argument('--forwardWebotsLogLevel',
                      dest='log_level',
                      action='store',
                      default=ForwardWebotsLogLevel.only_errors.name,
                      choices=list(map(lambda x: x.name, list(ForwardWebotsLogLevel))),
                      help="""Control how much of the webots logs to forward to the console stdout here.""")

  parser.add_argument('--timeout',
                      dest='default_timeout',
                      action='store',
                      default=180,
                      type=int,
                      help="""Default time limit for each webots test before marking it as failure and killing the webots instance.
                      This can be overridden in the configuration file for each individual test.""")

  parser.add_argument('--numRetries',
                      dest='num_retries',
                      action='store',
                      default=1,
                      type=int,
                      help="""When a test fails, the number of times to retry the test before
                      declaring it as failed. Be aware of off-by-one gotcha here, if numRetries ==
                      1, that means each test will be run for a maximum of two times, if the first
                      run of each test fails.""")

  parser.add_argument('--ignoreLogErrors',
                      dest='fail_on_error',
                      default='true',
                      action='store_false',
                      help="""If set, a test will not automatically fail just because an error
                      appears in its webots log.""")

  parser.add_argument('--doNotQuitWebots',
                      dest='quit_webots_after_test',
                      default='true',
                      action='store_false',
                      help="""If set, Webots will continue running after the test controller completes.
                      This is useful for local testing/debugging.""")

  options = parser.parse_args()

  options.generator = GeneratorType[options.generator]
  options.build_type = BuildType[options.build_type]
  options.log_level = ForwardWebotsLogLevel[options.log_level]

  if options.debug:
    UtilLog.setLevel(logging.DEBUG)
  else:
    UtilLog.setLevel(logging.INFO)

  os.chdir(COZMO_ENGINE_ROOT)

  cfg_path = get_subpath(os.path.join("project","build-scripts","webots"), options.config_file)
  assert os.path.isfile(cfg_path)

  UtilLog.debug(options)

  if options.setup_exe_list:
    setup_exes(options.generator, options.build_type, options.setup_exe_list)
    sys.exit(0)

  # build the project first
  if not build(options.generator, options.build_type):
    UtilLog.error("build failed")
    return 1

  if options.setupFirewallAndExit:
    print("Enter your password to set up firewall exceptions:")
    sign_webot_executables(options.generator, options.build_type, getpass.getpass()) # prompt for password
    sys.exit(0)

  if options.password:
    sign_webot_executables(options.generator, options.build_type, options.password)

  num_of_failed_runs = 0
  num_of_passed_runs = 0
  num_of_total_runs = 0

  tests = get_tests(cfg_path)
  build_folder = get_build_folder(options.build_type)

  return_value = 0

  global_test_results = {}
  for _ in range(0, options.num_runs):
    stop_webots()

    test_results, total_error_count, total_warning_count = run_tests(tests, build_folder,
                                                                     options.show_graphics,
                                                                     options.default_timeout,
                                                                     options.log_level,
                                                                     options.num_retries,
                                                                     options.fail_on_error,
                                                                     options.quit_webots_after_test)

    num_of_tests = sum(len(test_controller) for test_controller in test_results.values())

    if any_test_succeeded(test_results):
      UtilLog.info('Passed tests: ')
      for test_controller, results_of_each_world in test_results.items():
        for world, result in results_of_each_world.items():
          if result is ResultCode.succeeded:
            global_test_results.setdefault(test_controller, []).append("passed")
            UtilLog.info("{test_controller} in {world} passed.".format(test_controller=test_controller, world=world))

    if any_test_failed(test_results):
      UtilLog.info('Failed tests: ')
      for test_controller, results_of_each_world in test_results.items():
        for world, result in results_of_each_world.items():
          if result is ResultCode.failed:
            global_test_results.setdefault(test_controller, []).append("failed")
            UtilLog.info("{test_controller} in {world} failed.".format(test_controller=test_controller, world=world))

    if not options.show_graphics:
      # Only makes sense to print these when gui is not on because if the gui is on the webots logs
      # won't be piped to the log files and therefore errors and warnings will always be 0
      UtilLog.info("##teamcity[buildStatisticValue key='WebotsErrorCount' value='{errors}']".format(
                   errors=total_error_count))
      UtilLog.info("##teamcity[buildStatisticValue key='WebotsWarningCount' value='{warnings}']".format(
                   warnings=total_warning_count))

    UtilLog.info("##teamcity[buildStatisticValue key='WebotsTestCount' value='{num_of_tests}']".format(
                 num_of_tests=num_of_tests))

    if any_test_failed(test_results):
      UtilLog.error("*************************")
      UtilLog.error("SOME TESTS FAILED")
      UtilLog.error("*************************")
      return_value = 1
      num_of_failed_runs += 1
    else:
      UtilLog.info("*************************")
      UtilLog.info("ALL {num_of_tests} TESTS PASSED".format(num_of_tests=num_of_tests))
      UtilLog.info("*************************")
      num_of_passed_runs += 1

    num_of_total_runs = num_of_failed_runs + num_of_passed_runs

    if options.num_runs > 1:
      UtilLog.info("Run #{run_number}".format(run_number=num_of_total_runs))

    num_of_failed_tests_in_one_run = 0
    for test_controller in test_results.values():
      # Loop through each test controller
      for test_result in test_controller.values():
        # Loop through each webots world that the test controller runs in
        if test_result is ResultCode.failed:
          num_of_failed_tests_in_one_run += 1

    UtilLog.info("##teamcity[buildStatisticValue key='WebotsFailedTests' value='{num_of_failed_tests_in_one_run}']".format(
                 num_of_failed_tests_in_one_run=num_of_failed_tests_in_one_run))

  if options.num_runs > 1:
    UtilLog.info("{failed}/{total} ({percentage:.1f}%) runs failed".format(
                  failed=num_of_failed_runs, total=num_of_total_runs,
                  percentage=float(num_of_failed_runs)/num_of_total_runs*100))

    results_passed_msg=''
    results_failed_msg = ''
    for test_controller,results in global_test_results.items():
      if results.count('failed') > 0:
        results_failed_msg += "{} - {}/{} PASSED\n".format(test_controller, len(results)-results.count('failed'), len(results))
      elif results.count('passed') > 0:
        results_passed_msg += "{} - {}/{} PASSED\n".format(test_controller, results.count('passed'), len(results))

    UtilLog.info('results_passed_msg:\n{}'.format(results_passed_msg))
    UtilLog.info('results_failed_msg:\n{}'.format(results_failed_msg))
    payload = {
               "text":"*Webots Nightly Engine Test Results:*\n",
               "mrkdwn": True,
               "channel":"{}".format(os.environ['SLACK_CHANNEL']),
               "username":"buildbot",
               "attachments":[
                 {
                 "text":"{}".format(results_failed_msg),
                 "fallback":"{}".format(results_failed_msg),
                 "color":"danger"
                 },{
                 "text": "{}".format(results_passed_msg),
                 "fallback": "{}".format(results_passed_msg),
                 "color": "good"
                 }
               ]
             }
    slack_url = '{}'.format(os.environ['SLACK_TOKEN_URL'])
    cmd = ['curl', '-X', 'POST', '-d', 'payload={}'.format(payload), slack_url]
    process = subprocess.Popen(cmd, shell=False, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    dummy_stdout, dummy_stderr = process.communicate()
  return return_value



if __name__ == '__main__':
  sys.exit(main())
