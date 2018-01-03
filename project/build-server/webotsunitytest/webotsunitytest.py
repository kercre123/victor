import os
import errno
import inspect
import subprocess
import sys
import time
import threading
import logging
from functools import lru_cache


COZMO_ENGINE_ROOT = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip(b"\r\n").decode("utf-8")
BUILD_AUTOMATION_ROOT = os.path.join(COZMO_ENGINE_ROOT, 'build', 'automation')
BUILD_TOOLS_ROOT = os.path.join(COZMO_ENGINE_ROOT, 'tools', 'build', 'tools')
sys.path.insert(0, BUILD_TOOLS_ROOT)
WEBOTS_UNITY_ROOT = os.path.join(COZMO_ENGINE_ROOT, 'project', 'build-server', 'webotsunitytest')
LANGUAGES = ["de-DE", "en-US"]

from ankibuild import util

#set up default logger
UtilLog = logging.getLogger('webots.test')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('{name} {levelname} - {message}', style='{')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)

WORLD_FILE_TEST_NAME_PLACEHOLDER = r'%COZMO_SIM_TEST%'
COZMOBOT_PLACEHOLDER = r'%COZMOBOT%'
WORLD_FILE_NAME = 'PatternPlay.wbt'

# Name of user created certificate for signing webots executables to be
# excepted by the firewall
CERTIFICATE_NAME = "WebotsFirewall"

class ThreadOutput(object):
  test_return_code = None

def mkdir_p(path):
  try:
    os.makedirs(path)
  except OSError as exc: # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise

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
  forward_args = partial_path.split("/") + list(args)
  if forward_args:
    return os.path.join(COZMO_ENGINE_ROOT, *(forward_args))
  else:
    return COZMO_ENGINE_ROOT

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
    '/Applications/Webots.app/webots',
    '--stdout',
    '--stderr',
    '--minimize',  # Ability to start without graphics is on the wishlist
    '--mode=fast',
    get_subpath("simulator/worlds", WORLD_FILE_NAME)
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
  process = subprocess.Popen("ps -ax | grep [/]Applications/Webots.app/webots", shell=True, stdout=subprocess.PIPE)
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
  ps   = subprocess.Popen(('ps', 'Aux'), stdout=subprocess.PIPE)
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
  run_command = [get_subpath("simulator/kill_ipcs.sh")]
  result = subprocess.call(run_command)
  if result != 0:
    return False
  return True

def clear_old_files():
  if os.path.isdir(BUILD_AUTOMATION_ROOT):
    subprocess.run('rm {0}/*_Results.log'.format(BUILD_AUTOMATION_ROOT), shell=True)

def main(args):
  GENERATED_FILE_PATH = get_subpath("simulator/worlds", WORLD_FILE_NAME)
  show_graphics = True
  log_file_name = os.path.join(WEBOTS_UNITY_ROOT, 'webotsunitytest.txt')
  smoke_test_path = os.path.join(COZMO_ENGINE_ROOT, 'project', 'build-server', 'steps', '59_smoke_test.sh')
  clear_old_files()
  output = ThreadOutput()
  for language in LANGUAGES:
    run_webots_thread = threading.Thread(target=run_webots, args=[output, GENERATED_FILE_PATH, 
                                                                        show_graphics, log_file_name])
    run_webots_thread.start()  
    subprocess.call([smoke_test_path, language])
    stop_webots()
  return 0

if __name__ == '__main__':
  ARGS = sys.argv
  sys.exit(main(ARGS))
