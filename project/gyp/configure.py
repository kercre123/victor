#!/usr/bin/python

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
UtilLog = logging.getLogger('cozmo.engine.configure')
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
  parser.add_argument('--xcode', dest='xcode', action='store_true',
                      help='opens xcode projects')
  parser.add_argument('--path', dest='pathExtension', action='store',
                      help='prepends to the environment PATH')
  parser.add_argument('--clean', '-c', dest='clean', action='store_true',
                      help='cleans all output folders')
  parser.add_argument('--withGyp', metavar='GYP_PATH', dest='gypPath', action='store', default=None,
                      help='Use gyp installation located at GYP_PATH')
  parser.add_argument('--buildTools', metavar='BUILD_TOOLS_PATH', dest='buildToolsPath', action='store', default=None,
                      help='Use build tools located at BUILD_TOOLS_PATH')
  parser.add_argument('--ankiUtil', metavar='ANKI_UTIL_PATH', dest='ankiUtilPath', action='store', default=None,
                      help='Use anki-util repo checked out at ANKI_UTIL_PATH')
  parser.add_argument('--projectRoot', '-p', dest='projectRoot', action='store', default=None,
                      help='project location, assumed to be same as git repo root')

  # options controlling gyp output
  parser.add_argument('--arch', action='store',
                      choices=('universal', 'standard'),
                      default='universal',
                      help="Target set of architectures")
  parser.add_argument('--platform', action='append', dest='platforms',
                      choices=('ios', 'mac', 'android'),
                      help="Generate output for a specific platform")
  (options, args) = parser.parse_known_args(scriptArgs)

  if options.verbose:
    UtilLog.setLevel(logging.DEBUG)

  if options.platforms is None:
    options.platforms = ['ios', 'mac', 'android']

  if (options.pathExtension):
    os.environ['PATH'] = options.pathExtension + ':' + os.environ['PATH']

  # go to the script dir, so that we can find the project dir
  # in the future replace this with command line param
  selfPath = os.path.dirname(os.path.realpath(__file__))
  os.chdir(selfPath)

  # find project root path
  if options.projectRoot:
    projectRoot = options.projectRoot
  else:
    projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")

  if not options.buildToolsPath:
    options.buildToolsPath = os.path.join(options.projectRoot, 'tools/anki-util/tools/build-tools')
  if not os.path.exists(options.buildToolsPath):
    UtilLog.error('build tools not found [%s]' % (options.buildToolsPath) )
    return False

  sys.path.insert(0, os.path.join(options.buildToolsPath, 'tools/ankibuild'))
  # import ankibuild.cmake
  # import ankibuild.util
  # import ankibuild.xcode
  import installBuildDeps
  import updateFileLists


  if not options.ankiUtilPath:
    options.ankiUtilPath = os.path.join(options.projectRoot, 'tools/anki-util')
  if not os.path.exists(options.ankiUtilPath):
    UtilLog.error('anki-util not found [%s]' % (options.ankiUtilPath) )
    return False

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
      print("Wrong version of gyp")
  sys.stdout = oldStdOut
  gypVersion = stdOutCapture.getvalue();
  #subprocess.check_output([gypLocation, '--version']).rstrip("\r\n")
  if ('ANKI' not in gypVersion):
    print 'wrong version of gyp found'
    gypHelp()
    return False

  if options.clean:
    shutil.rmtree(os.path.join(projectRoot, 'generated', folder), True)
    return True

  # update file lists
  generator = updateFileLists.FileListGenerator(options)
  generator.processFolder(['basestation/src/anki/cozmo/basestation', 'basestation/include/anki/cozmo'], ['project/gyp/cozmoEngine.lst'])

  # update subprojects
  for platform in options.platforms:
    if (subprocess.call([os.path.join(options.ankiUtilPath, 'project/gyp/configure.py') , '--platform', platform ]) != 0):
      print "error executing submodule configure"
      return False


  configurePath = os.path.join(projectRoot, 'project/gyp')

  # mac
  if 'mac' in options.platforms:
      print 'generating mac project'
                                  # audio_library_type=static_library
                                  # audio_library_build=profile
                                  # bs_library_type=static_library
                                  # de_library_type=shared_library
                                  # kazmath_library_type=static_library
                                  # jsoncpp_library_type=static_library
                                  # util_library_type=static_library
                                  # worldviz_library_type=static_library
                                  # gyp_location={1}
                                  # configure_location={2}
                                  # output_location={0}
      #################### GYP_DEFINES ####
      os.environ['GYP_DEFINES'] = """ 
                                  OS=mac
                                  energy_library_type=static_library
                                  arch_group={0}
                                  ndk_root=INVALID
                                  """.format(options.arch)
      gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../../generated/mac']
      gyp.main(gypArgs)
      # gyp driveEngine.gyp --check --depth . -f xcode --generator-output=../gyp-mac



  # ios
  if 'ios' in options.platforms:
      print 'generating ios project'
      #################### GYP_DEFINES ####
      os.environ['GYP_DEFINES'] = """  
                                  audio_library_type=static_library
                                  audio_library_build=profile
                                  bs_library_type=static_library
                                  de_library_type=static_library
                                  kazmath_library_type=static_library
                                  jsoncpp_library_type=static_library
                                  util_library_type=static_library
                                  worldviz_library_type=static_library
                                  basestation_target_name=Basestation
                                  driveengine_target_name=DriveEngine
                                  OS=ios
                                  das_location=
                                  output_location={0}
                                  gyp_location={1}
                                  configure_location={2}
                                  arch_group={3}
                                  ndk_root=INVALID
                                  """.format('gyp-ios', gypPath, configurePath, options.arch)
      gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../gyp-ios']
      gyp.main(gypArgs)

  if 'android' in options.platforms:
      print 'generating android project'

      ### Install android build deps if necessary
      # TODO: We should only check for deps in configure.py, not actuall install anything
      # TODO: We should not install any deps here, only check that valid depdendencies exist
      deps = ['ninja']
      if not 'ANDROID_HOME' in os.environ:
        deps.append('android-sdk')

      if not 'ANDROID_NDK_ROOT' in os.environ:
        deps.append('android-ndk')

      if len(deps) > 0:
        # Install Android build dependencies
        installBuildDeps.main(deps)

      for dep in deps:
        if dep == 'android-sdk':
          os.environ['ANDROID_HOME'] = os.path.join('/', 'usr', 'local', 'opt', dep)
        elif dep == 'android-ndk':
          os.environ['ANDROID_NDK_ROOT'] = os.path.join('/', 'usr', 'local', 'opt', dep)
      ### android deps installed

      ndk_root = os.environ['ANDROID_NDK_ROOT']

      os.environ['ANDROID_BUILD_TOP'] = configurePath
      ##################### GYP_DEFINES ####
      os.environ['GYP_DEFINES'] = """ 
                                      audio_library_type=shared_library
                                      audio_library_build=profile
                                      bs_library_type=static_library
                                      de_library_type=shared_library
                                      kazmath_library_type=static_library
                                      jsoncpp_library_type=static_library
                                      util_library_type=static_library
                                      worldviz_library_type=static_library
                                      das_library_type=shared_library
                                      basestation_target_name=Basestation
                                      driveengine_target_name=DriveEngine
                                      os_posix=1
                                      OS=android
                                      GYP_CROSSCOMPILE=1
                                      das_location={0}
                                      output_location={1}
                                      gyp_location={2}
                                      configure_location={3}
                                      target_arch=arm
                                      clang=1
                                      component=static_library
                                      use_system_stlport=0
                                      ndk_root={4}
                                      use_das={5}
                                  """.format(dasPath, 'gyp-android', gypPath, configurePath, ndk_root, useDas)
      os.environ['CC_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang')
      os.environ['CXX_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang++')
      os.environ['AR_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc-ar')
      os.environ['LD_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang++')
      os.environ['NM_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/arm-linux-androideabi/bin/nm')
      os.environ['READELF_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-readelf')
      gypArgs = ['--check', '--depth', '.', '-f', 'ninja-android', '--toplevel-dir', '../..', '--generator-output', 'project/gyp-android']
      gyp.main(gypArgs)


  # # pre build setup: versionGenerator
  # if (subprocess.call([os.path.join(projectRoot, 'tools/build/versionGenerator/versionGenerator.sh')]) != 0):
  #   print "error executing versionGenerator"

  # #pre build setup: decompress audio libs
  # if (subprocess.call([os.path.join(projectRoot, 'libs/drive-audio/gyp/decompressAudioLibs.sh')]) != 0):
  #   print "error decompressing audio libs"
    
  if (options.xcode):
    os.chdir(os.path.join(projectRoot, 'project'))
    subprocess.call(['open', 'gyp-ios/driveEngine.xcodeproj'])
    subprocess.call(['open', 'gyp-mac/driveEngine.xcodeproj'])


  return True


if __name__ == '__main__':

  # go to the script dir, so that we can find the project dir
  # in the future replace this with command line param
  selfPath = os.path.dirname(os.path.realpath(__file__))
  os.chdir(selfPath)

  # find project root path
  projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")
  args = sys.argv
  args.extend(['--projectRoot', projectRoot])
  if main(args):
    sys.exit(0)
  else:
    sys.exit(1)
