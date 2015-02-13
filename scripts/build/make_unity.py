#!/usr/bin/env python

# TEMPORARY COPY TO UNITY SCRIPT

# Usage: make_iOS_libs.py [build directory]    # Default build directory is build_ios/

import glob, re, os, os.path, shutil, string, sys

import subprocess

ECHO_ALL = True

def escape_shell(args):
  try:
    import pipes
    return ' '.join(pipes.quote(arg) for arg in args)
  except ImportError:
    return ' '.join(args)

def raw_execute(args):
  if not args:
    raise ValueError('No arguments to execute.')
  
  try:
    result = subprocess.call(args)
  except OSError as e:
    sys.exit('ERROR: Error in subprocess `{0}`: {1}'.format(escape_shell(args), e.strerror))
  if result:
    sys.exit('ERROR: Subprocess `{0}` exited with code {1}.'.format(escape_shell(args), result))

def execute(args):
  print('')
  if ECHO_ALL:
    raw_execute(['echo'] + args)
  raw_execute(args)

ROOT_PATH = os.path.abspath(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))))
def complete_path(path):
  return os.path.join(ROOT_PATH, path)

config = "Debug"

execute([complete_path('scripts/build/make_xcode.py'), '-p', 'ios+osx', '--config', config, 'build'])

##
## Cozmo_Game
##
print "Copying libraries to unity."

unityLoc = complete_path("unity/Cozmo/Assets/Plugins/iOS/")

cozmoGameLibsDir = complete_path(os.path.join("build_xcode/game_ios/Xcode/lib", config))
cozmoGameLibFiles = os.listdir(cozmoGameLibsDir)
for libFileName in cozmoGameLibFiles:
  fullLibFileName = os.path.join(cozmoGameLibsDir, libFileName)
  shutil.copy(fullLibFileName, unityLoc)

cozmoEngineLibsDir = complete_path(os.path.join("build_xcode/game_ios/lib/anki/cozmo-engine/Xcode/lib", config))
cozmoEngineLibFiles = os.listdir(cozmoEngineLibsDir)
for libFileName in cozmoEngineLibFiles:
  fullLibFileName = os.path.join(cozmoEngineLibsDir, libFileName)
  shutil.copy(fullLibFileName, unityLoc)

opencvLibsDir = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"), "build/opencv-ios/multiArchLibs/")
opencvLibFiles = os.listdir(opencvLibsDir)
for libFileName in opencvLibFiles:
    fullLibFileName = os.path.join(opencvLibsDir, libFileName)
    shutil.copy(fullLibFileName, unityLoc)

print "Done."


  
