
"A series of shell commands that either successfully run or call sys.exit."

import os
import os.path
import shutil
import textwrap
import sys
try:
  import subprocess32 as subprocess
except ImportError:
  import subprocess

ECHO_ALL = True

def pwd():
    "Returns the absolute path of the current working directory."
    try:
        return os.path.abspath(os.getcwd())
    except OSError as e:
        sys.exit('ERROR: Failed to get working directory: {0}'.format(e.strerror))

def cd(path = '.'):
    "Changes the current working directory to the given path."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print('cd ' + path)
    try:
        os.chdir(path)
    except OSError as e:
        sys.exit('ERROR: Failed to change to directory {0}: {1}'.format(path, e.strerror))


def makedirs(path):
    "Recursively creates new directories up to and including the given directory, if needed."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print('mkdir -p ' + path)
    try:
        os.makedirs(path)
    except OSError as e:
        if not os.path.isdir(path):
            sys.exit('ERROR: Failed to recursively create directories to {0}: {1}'.format(path, e.strerror))


def rm_rf(path):
    "Removes all files and directories including the given path."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print('rm -rf ' + path)
    try:
        shutil.rmtree(path)
    except OSError as e:
        if os.path.exists(path):
            sys.exit('ERROR: Failed to recursively remove directory {0}: {1}'.format(path, e.strerror))


def rm(path):
    "Removes a single specific file."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print('rm ' + path)
    try:
        os.remove(path)
    except OSError as e:
        if os.path.exists(path):
            sys.exit('ERROR: Failed to remove file {0}: {1}'.format(path, e.strerror))


def rmdir(path):
    "Removes a directory if empty (or if it just contains directories)."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print 'rmdir ' + path
    try:
        for file in os.listdir(path):
            if file != '.DS_Store':
                return
        shutil.rmtree(path)
    except OSError:
    	pass


def read(path):
    "Reads the contents of the file at the given path."
    path = os.path.abspath(path)
    if ECHO_ALL:
        print('reading all from {0}'.format(path))
    try:
        with open(path, 'r') as file:
            return file.read()
    except IOError as e:
        sys.exit('ERROR: Could not read from file {0}: {1}'.format(path, e.strerror))

def write(path, contents):
    "Writes the specified contents to the given path."
    path = os.path.abspath(path)
    contents = str(contents)
    if ECHO_ALL:
        print('writing {0} bytes to {1}'.format(len(contents), path))
    try:
        with open(path, 'w') as file:
            file.write(contents)
    except IOError as e:
        sys.exit('ERROR: Could not write to file {0}: {1}'.format(path, e.strerror))


def _escape(args):
    try:
        import pipes
        return ' '.join([pipes.quote(arg) for arg in args])
    except ImportError:
        return ' '.join(args)


def _run_subprocess(func, args):
    if not args:
        raise ValueError('No arguments to execute.')
    try:
        return func(args)
    except subprocess.CalledProcessError as e:
        sys.exit('ERROR: Subprocess `{0}` exited with code {1}.'.format(_escape(args), e.returncode)) 
    except OSError as e:
        sys.exit('ERROR: Error in subprocess `{0}`: {1}'.format(_escape(args), e.strerror))


def _raw_execute(func, args):
    print('')
    if ECHO_ALL:
        _run_subprocess(subprocess.check_call, ['echo'] + args)
    _run_subprocess(func, args)


def execute(args):
    "Executes the given list of arguments as a shell command, returning nothing."
    _raw_execute(subprocess.check_call, args)


def evaluate(args):
    "Executes the given list of arguments as a shell command, returning stdout."
    return _raw_execute(subprocess.check_output, args)

