
"ios-deploy script running."

import os.path
import util
import sys
import textwrap

def _run(*args):
    output = util.File.evaluate(['which', 'ios-deploy'], ignore_result=True)
    if not output or output.isspace():
        sys.exit(textwrap.dedent('''\
            ERROR: ios-deploy not found. ios-deploy is used to run ios apps on device.
            To install, enter the following commands into the OSX terminal:
            brew install node
            npm install -g ios-deploy'''))
    util.File.execute(['killall', 'lldb'], ignore_result=True)
    util.File.execute(['killall', 'ios-deploy'], ignore_result=True)
    util.File.execute(['ios-deploy'] + list(args), ignore_result=True)
    util.File.execute(['killall', 'lldb'], ignore_result=True)
    util.File.execute(['killall', 'ios-deploy'], ignore_result=True)

def install(app_path):
    "Install the app without debugging. Not sure if this one works."
    _run('--bundle', app_path)
    #_run('--nostart', '--debug', '--bundle', app_path)

def uninstall(app_path):
    _run('--uninstall', '--bundle', app_path)
    #_run('--nostart', '--uninstall', '--debug', '--bundle', app_path)

def debug(app_path):
    _run('--debug', '--bundle', app_path)
    #_run('--noinstall', '--debug', '--bundle', app_path)

def noninteractive(app_path):
    _run('--noninteractive', '--debug', '--bundle', app_path)

def justlaunch(app_path):
    _run('--justlaunch', '--debug', '--bundle', app_path)

