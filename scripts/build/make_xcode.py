#!/usr/bin/env python

import os.path
import sys

from xcodeutil import run_xcode, Project, Workspace

project_osx = Project('build_xcode_osx/CozmoEngine.xcodeproj', '.', 'osx')
project_ios = Project('build_xcode_ios/CozmoEngine_iOS.xcodeproj', '.', 'ios')
project_sim = Project('build_xcode_sim/CozmoEngine_sim.xcodeproj', '.', 'sim')
projects = [project_osx, project_ios, project_sim]

if __name__ == '__main__':
  up = os.path.dirname
  root_path = up(up(up(os.path.abspath(os.path.realpath(__file__)))))
  run_xcode(root_path, projects)
