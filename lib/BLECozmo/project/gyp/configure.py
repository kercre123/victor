#!/usr/bin/python

#Try running from the same directory like: ./configure.py --projectRoot ../.. --buildTools ../../../../tools/anki-util/tools/build-tools

import os
import os.path
import sys
import re
import subprocess
import argparse
import StringIO
import shutil
import logging

#set up default logger
UtilLog = logging.getLogger('configure.BLECozmo')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
UtilLog.addHandler(stdout_handler)

def gypHelp():
  print "to install gyp:"
  print "cd ~/your_workspace/"
  print "git clone git@github.com:anki/anki-gyp.git"
  print "echo PATH=$HOME/your_workspace/gyp:$PATH >> ~/.bash_profile"
  print ". ~/.bash_profile"

def main(scriptArgs):
  version = '1.0'
  parser = argparse.ArgumentParser(description='runs gyp to generate projects', version=version)
  parser.add_argument('--debug', '--verbose', '-d', dest='verbose', action='store_true',
                      help='prints debug output')
  parser.add_argument('--path', dest='pathExtension', action='store',
                      help='prepends to the environment PATH')
  parser.add_argument('--clean', '-c', dest='clean', action='store_true',
                      help='cleans all output folders')
  parser.add_argument('--withGyp', metavar='GYP_PATH', dest='gypPath', action='store', default=None,
                      help='Use gyp installation located at GYP_PATH')
  parser.add_argument('--buildTools', metavar='BUILD_TOOLS_PATH', dest='buildToolsPath', action='store', default=None,
                      help='Use build tools located at BUILD_TOOLS_PATH')
  parser.add_argument('--projectRoot', dest='projectRoot', action='store', default=None,
                      help='project location')
  parser.add_argument('--updateListsOnly', dest='updateListsOnly', action='store_true', default=False,
                      help='forces configure to only update .lst files and not run gyp project generation')


  # options controlling gyp output
  parser.add_argument('--arch', action='store',
                      choices=('universal', 'standard'),
                      default='universal',
                      help="Target set of architectures")
  parser.add_argument('--platform', action='append', dest='platforms',
                      choices=('ios', 'mac'),
                      help="Generate output for a specific platform")
  (options, args) = parser.parse_known_args(scriptArgs)

  if options.verbose:
    UtilLog.setLevel(logging.DEBUG)

  if options.platforms is None:
    options.platforms = ['ios', 'mac']

  if (options.pathExtension):
    os.environ['PATH'] = options.pathExtension + ':' + os.environ['PATH']

  # go to the script dir, so that we can find the project dir
  # in the future replace this with command line param
  selfPath = os.path.dirname(os.path.realpath(__file__))
  os.chdir(selfPath)

  # find project root path
  if not options.projectRoot:
    UtilLog.error('project root not found')
    return False

  if not options.buildToolsPath:
    UtilLog.error('build tools not found' )
    return False

  sys.path.insert(0, os.path.join(options.buildToolsPath, 'tools/ankibuild'))
  import installBuildDeps
  import updateFileLists

  # do not check for coretech external, and gyp if we are only updating list files
  if not options.updateListsOnly:

    gypPath = os.path.join(options.buildToolsPath, 'gyp')
    if (options.gypPath is not None):
      gypPath = options.gypPath

    # import gyp
    sys.path.insert(0, os.path.join(gypPath, 'pylib'))
    import gyp

    #check gyp version
    stdOutCapture = StringIO.StringIO()
    oldStdOut = sys.stdout
    sys.stdout = stdOutCapture
    try:
        gyp.main(['--version'])
    except:
        UtilLog.error("Wrong version of gyp")
    sys.stdout = oldStdOut
    gypVersion = stdOutCapture.getvalue();
    #subprocess.check_output([gypLocation, '--version']).rstrip("\r\n")
    if ('ANKI' not in gypVersion):
      UtilLog.error('wrong version of gyp found')
      gypHelp()
      return False

  if options.clean:
    shutil.rmtree(os.path.join(projectRoot, 'generated', folder), True)
    return True


  # update file lists
  generator = updateFileLists.FileListGenerator(options)
  generator.processFolder(['shared', 'ios' ], ['project/gyp/BLECozmo_ios.lst'])

  if options.updateListsOnly:
    return True


  configurePath = os.path.join(options.projectRoot, 'project/gyp')
  gypFile = 'BLECozmo.gyp'

  # mac
  if 'mac' in options.platforms:
      os.environ['GYP_DEFINES'] = """ 
                                  OS=mac
                                  ndk_root=INVALID
                                  library_type=static_library
                                  arch_group={0}
                                  output_location={1}
                                  """.format(
                                    options.arch, 
                                    os.path.join(options.projectRoot, 'generated/mac')
                                  )
      gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../../generated/mac', gypFile]
      gyp.main(gypArgs)
      



  # ios
  if 'ios' in options.platforms:
    os.environ['GYP_DEFINES'] = """  
                                OS=ios
                                ndk_root=INVALID
                                library_type=static_library
                                arch_group={0}
                                output_location={1}
                                """.format(
                                  options.arch, 
                                  os.path.join(options.projectRoot, 'generated/mac')
                                )
    gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../../generated/ios', gypFile]
    gyp.main(gypArgs)



  return True


if __name__ == '__main__':

  args = sys.argv
  if main(args):
    sys.exit(0)
  else:
    sys.exit(1)
