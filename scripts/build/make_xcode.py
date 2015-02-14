#!/usr/bin/env python

import os.path
import sys

up = os.path.dirname
ROOT_PATH = up(up(up(os.path.abspath(os.path.realpath(__file__)))))

print(ROOT_PATH)
XCODE_UTIL_PATH = 'lib/anki/cozmo-engine/scripts/build'
XCODE_FULL_PATH = os.path.join(ROOT_PATH, XCODE_UTIL_PATH)
sys.path.insert(0, XCODE_FULL_PATH)
try:
  from xcodeutil import run_xcode, Project, Workspace
except ImportError:
  sys.exit('Could not find xcodeutil python script at {0}.'.format(XCODE_FULL_PATH))

game_osx = Project('build_xcode/game_osx/CozmoGame.xcodeproj', '.', 'osx')
game_ios = Project('build_xcode/game_ios/CozmoGame_iOS.xcodeproj', '.', 'ios')
game_sim = Project('build_xcode/game_sim/CozmoGame_sim.xcodeproj', '.', 'sim')
projects = [game_osx, game_ios, game_sim]

extra_projects_osx = []
extra_projects_ios = []
extra_projects_sim = []

workspace_osx = Workspace('build_xcode/CozmoWorkspace_OSX.xcworkspace', 'ALL_BUILD', extra_projects_osx, 'osx')
workspace_ios = Workspace('build_xcode/CozmoWorkspace_iOS.xcworkspace', 'ALL_BUILD', extra_projects_ios, 'ios')
workspace_sim = Workspace('build_xcode/CozmoWorkspace_sim.xcworkspace', 'ALL_BUILD', extra_projects_sim, 'sim')
workspaces = [workspace_osx, workspace_ios, workspace_sim]

#downstream_script = 'lib/anki/cozmo-engine/make_xcode.py'
downstream_scripts = []

if __name__ == '__main__':
  run_xcode(ROOT_PATH, projects, workspaces, downstream_scripts, use_unity=True)
