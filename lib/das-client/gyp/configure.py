#!/usr/bin/env python

import os
import os.path
import sys
import re
import subprocess
import argparse
import StringIO

def gypHelp():
  print "to install gyp:"
  print "cd ~/your_workspace/"
  print "git clone git@github.com:anki/anki-gyp.git"
  print "echo PATH=$HOME/your_workspace/gyp:$PATH >> ~/.bash_profile"
  print ". ~/.bash_profile"

version = '1.0'
parser = argparse.ArgumentParser(description='runs gyp to generate projects', version=version)
parser.add_argument('--debug', dest='debug', action='store_true',
                    help='prints debug output')
parser.add_argument('--xcode', dest='xcode', action='store_true',
                    help='opens xcode projects')
parser.add_argument('--path', dest='pathExtension', action='store',
                    help='prepends to the environment PATH')
parser.add_argument('--with-gyp', metavar='GYP_PATH', dest='gypPath', action='store', default=None,
                    help='Use gyp installation located at GYP_PATH')

# options controlling gyp output
parser.add_argument('--arch', action='store',
                    choices=('universal', 'standard'),
                    default='universal',
                    help="Target set of architectures")
parser.add_argument('--platform', action='append', dest='platforms',
                    choices=('ios', 'mac', 'android', 'linux'),
                    help="Generate output for a specific platform")
(options, args) = parser.parse_known_args()

if options.platforms is None:
  options.platforms = ['ios', 'mac', 'android']

if (options.pathExtension):
  os.environ['PATH'] = options.pathExtension + ':' + os.environ['PATH']

# find project root path
projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")

gypPath = os.path.join(projectRoot, '../drive-basestation/tools/build-tools/gyp')
if (options.gypPath is not None):
    gypPath = options.gypPath

# import gyp
if (options.debug):
  print 'gyp-path = %s' % gypPath
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
  exit (1)


# go to the script dir, so that we can find the project dir
# in the future replace this with command line param
selfPath = os.path.dirname(os.path.realpath(__file__))
os.chdir(selfPath)

print "[das-client] configuring libDAS..."

# update file lists
# sys.path.insert(0, os.path.join(projectRoot, 'project/gyp'))
#import updateFileLists
#args = sys.argv
#args.extend(['--projectRoot', projectRoot])
#updateFileLists.main(args)

# internal das location
configurePath = os.path.join(projectRoot, 'gyp')

# mac
if 'mac' in options.platforms:
    print '[das-client] generating mac project...'
    #################### GYP_DEFINES ####
    os.environ['GYP_DEFINES'] = """ lua_library_type=static_library
                                bs_library_type=shared_library
                                de_library_type=shared_library
                                kazmath_library_type=static_library
                                worldviz_library_type=static_library
                                das_library_type=shared_library
                                basestation_target_name=Basestation
                                driveengine_target_name=DriveEngine
                                output_location={0}
                                gyp_location={1}
                                configure_location={2}
                                arch_group={3}
                                """.format(projectRoot+'/gyp-mac', gypPath, configurePath, options.arch)
    gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '..', '--generator-output', '../gyp-mac']
    gyp.main(gypArgs)
    # gyp driveEngine.gyp --check --depth . -f xcode --generator-output=../gyp-mac



# ios
if 'ios' in options.platforms:
    print '[das-client] generating ios project...'
    #################### GYP_DEFINES ####
    os.environ['GYP_DEFINES'] = """
                                lua_library_type=static_library
                                bs_library_type=static_library
                                de_library_type=static_library
                                kazmath_library_type=static_library
                                worldviz_library_type=static_library
                                das_library_type=static_library
                                basestation_target_name=Basestation
                                driveengine_target_name=DriveEngine
                                OS=ios
                                output_location={0}
                                gyp_location={1}
                                configure_location={2}
                                arch_group={3}
                                """.format(projectRoot+'/gyp-ios', gypPath, configurePath, options.arch)
    gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '..', '--generator-output', '../gyp-ios']
    gyp.main(gypArgs)

if 'android' in options.platforms:
    print '[das-client] generating android project...'
    ndk_root = os.environ['ANDROID_NDK_ROOT']
    os.environ['ANDROID_BUILD_TOP'] = configurePath
    ##################### GYP_DEFINES ####
    os.environ['GYP_DEFINES'] = """ das_library_type=shared_library
                                    os_posix=1
                                    OS=android
                                    GYP_CROSSCOMPILE=1
                                    output_location={0}
                                    gyp_location={1}
                                    configure_location={2}
                                    target_arch=arm
                                    clang=1
                                    component=static_library
                                    use_system_stlport=0
                                    ndk_root={3}
                                """.format(projectRoot+'/gyp-android', gypPath, configurePath, ndk_root)
    os.environ['CC_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang')
    os.environ['CXX_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['AR_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc-ar')
    os.environ['LD_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['NM_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/arm-linux-androideabi/bin/nm')
    os.environ['READELF_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-readelf')
    gypArgs = ['--check', '--depth', '.', '-f', 'ninja-android', '--toplevel-dir', '..', '--generator-output', 'gyp-android']
    gyp.main(gypArgs)

if (options.xcode):
  os.chdir(os.path.join(projectRoot, 'project'))
  subprocess.call(['open', 'gyp-ios/driveEngine.xcodeproj'])
  subprocess.call(['open', 'gyp-mac/driveEngine.xcodeproj'])


exit(0)
