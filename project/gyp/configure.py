#!/usr/bin/env python2

import logging
import os
import os.path
import sys
import subprocess
import argparse
import StringIO

class ArgParseUniqueStore(argparse.Action):
    def __call__(self, parser, namespace, values, option_string):
        if getattr(namespace, self.dest, self.default) is not None:
            parser.error(option_string + " appears multiple times.\n")
        setattr(namespace, self.dest, values)

def gypHelp():
  print "to install gyp:"
  print "cd ~/your_workspace/"
  print "git clone git@github.com:anki/anki-gyp.git"
  print "echo PATH=$HOME/your_workspace/gyp:$PATH >> ~/.bash_profile"
  print ". ~/.bash_profile"

def main(scriptArgs):
  # configure logging
  UtilLog = logging.getLogger('anki-util.configure')
  stdout_handler = logging.StreamHandler()
  formatter = logging.Formatter('%(name)s - %(message)s')
  stdout_handler.setFormatter(formatter)
  UtilLog.addHandler(stdout_handler)

  version = '1.0'
  parser = argparse.ArgumentParser(description='runs gyp to generate projects')
  parser.add_argument('--version', '-V', action='version', version=version)
  parser.add_argument('--debug', dest='debug', action='store_true',
                        help='prints debug output')
  parser.add_argument('--path', dest='pathExtension', action='store',
                        help='prepends to the environment PATH')
  parser.add_argument('--with-gyp', metavar='GYP_PATH', dest='gypPath', action='store', default=None,
                        help='Use gyp installation located at GYP_PATH')
  parser.add_argument('--with-clad', metavar='CLAD_PATH', dest='cladPath', action='store', default=None,
                        help='Use clad installation located at CLAD_PATH')
  parser.add_argument('--clean', '-c', dest='clean', action='store_true',
                        help='cleans all output folders')
  parser.add_argument('--buildTools', metavar='BUILD_TOOLS_PATH', dest='buildToolsPath', action='store', default=None,
                      help='Use build tools located at BUILD_TOOLS_PATH')
  parser.add_argument('--projectRoot', '-p', dest='projectRoot', action='store', default=None,
                      help='project location, assumed same as script location')
  parser.add_argument('--updateListsOnly', '-u', dest='updateListsOnly', action='store_true', default=False,
                      help='forces configure to only update .lst files and not run gyp project generation')


  # options controlling gyp output
  parser.add_argument('--arch', action='store',
                        choices=('universal', 'standard'),
                        default='universal',
                        help="Target set of architectures")
  parser.add_argument('--platform', action='append', dest='platforms',
                        choices=('ios', 'mac', 'android', 'linux', 'cmake'),
                        help="Generate output for a specific platform")
  parser.add_argument('--verbose', '-v', action='store_true', dest='verbose',
                        help="Extra debug output")
  parser.add_argument('--gyp-define',
                      action='append',
                      dest='gyp_defines',
                      default=[],
                      help="Set a variable that will be passed to gyp." \
                                "Format is <key>=<value>")
  try:
    (options, args) = parser.parse_known_args(scriptArgs)
  except Exception as e:
    UtilLog.error(e)

  if options.verbose:
    UtilLog.setLevel(logging.DEBUG)

  if options.platforms is None:
    options.platforms = ['ios', 'mac', 'android']

  if options.pathExtension:
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
    options.projectRoot = projectRoot

  if not options.buildToolsPath:
    options.buildToolsPath = os.path.join(projectRoot, 'tools/build-tools')
  if not os.path.exists(options.buildToolsPath):
    UtilLog.error('build tools not found [%s]' % (options.buildToolsPath) )
    return False

  sys.path.insert(0, os.path.join(options.buildToolsPath, 'tools/ankibuild'))
  import installBuildDeps
  import updateFileLists
  import generateCLAD
  import util

  # create gyp arg function
  getGypArgs = util.Gyp.getArgFunction(['--check', '--depth', '.', '--toplevel-dir', '../..'])

  if not options.cladPath:
      options.cladPath = util.Module.get_path('clad')

  # do not check for gyp if we are only updating list files
  if not options.updateListsOnly:

    gypPath = os.path.join(projectRoot, 'tools/build-tools/gyp')
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
      UtilLog('wrong version of gyp found')
      gypHelp()
      return False

  if options.clean:
    for folder in ['gyp-mac', 'gyp-ios', 'gyp-android', 'gyp-linux']:
      shutil.rmtree(os.path.join(projectRoot, 'project', folder), True)
    return True

  # Hack to avoid total failure when we don't have ninja/valgrind
  # also valgrind must be installed with --HEAD
  if 'linux' in options.platforms:
    print 'linux dependencies defined by Vagrant'
  elif 'cmake' not in options.platforms or not options.updateListsOnly:
    options.deps = ['ninja'] 
    options.verbose = options.debug
    installer = installBuildDeps.DependencyInstaller(options);

    if not installer.install():
      UtilLog.error("Failed to verify build tool dependencies")
      return False

  # pre build setup: unpackLibs
  os.chdir(os.path.join(projectRoot, 'libs/packaged'))
  if (subprocess.call(['./unpackLibs.sh']) != 0):
    UtilLog.error('error executing unpackLibs')
  os.chdir(os.path.join(projectRoot, 'project/gyp'))

  # generate CLAD source
  clad_src_dir = os.path.join(projectRoot, 'source', 'anki', 'clad')
  clad_dst_dir = os.path.join(projectRoot, 'source', 'anki', 'messages')
  generate_clad_result = generateCLAD.generateMessageBuffersCPP(clad_src_dir,
                                                                clad_dst_dir,
                                                                os.path.join(os.path.abspath(options.cladPath), 'emitters'),
                                                                options.verbose)

  # update file lists
  generator = updateFileLists.FileListGenerator(options)
  generator.processFolder(['source/anki/util' ], ['project/gyp/util.lst'])
  generator.processFolder(['source/anki/audioUtil' ], ['project/gyp/audioUtil.lst'])
  generator.processFolder(['source/anki/utilUnitTest'], ['project/gyp/utilUnitTest.lst'])
  generator.processFolder(['source/anki/networkApp'], ['project/gyp/networkApp.lst'])
  generator.processFolder(['source/anki/clad'], ['project/gyp/clad.lst'])
  generator.processFolder(['source/anki/messages'], ['project/gyp/messages.lst'])

  if options.updateListsOnly:
    return True

  configurePath = os.path.join(projectRoot, 'project/gyp')
  gypFile = 'util.gyp'

  # paths relative to gyp file
  clad_dir_rel = os.path.relpath(options.cladPath, configurePath)

  #################### GYP_DEFINES ####
  default_defines = {
    'jsoncpp_library_type': 'static_library',
    'kazmath_library_type': 'static_library',
    'libwebp_library_type': 'static_library',
    'cpufeatures_library_type': 'static_library',
    'util_library_type': 'static_library',
    'audioutil_library_type': 'static_library',
    'civetweb_library_type': 'static_library',
    'lua_library_type': 'static_library',
    'gyp_location': gypPath,
    'configure_location': configurePath,
    'clad_dir': clad_dir_rel,
    'ndk_root': 'INVALID',
    'android_toolchain': 'arm-linux-androideabi-4.9',
    'android_platform': 'android-18',
  }

  # mac
  if 'mac' in options.platforms:
    UtilLog.info('generating mac project')
    #################### GYP_DEFINES ####
    defines = default_defines.copy()
    defines.update({
            'OS': 'mac',
            'output_location': projectRoot+'/project/gyp-mac',
            'arch_group': 'standard'
    })

    os.environ['GYP_DEFINES'] = util.Gyp.getDefineString(defines, options.gyp_defines)

    gypArgs = getGypArgs('xcode', '../gyp-mac', gypFile)
    if options.verbose:
      gypArgs = ['-d', 'all'] + gypArgs
    gyp.main(gypArgs)
    # gyp driveEngine.gyp --check --depth . -f xcode --generator-output=../gyp-mac


  # ios
  if 'ios' in options.platforms:
    UtilLog.info('generating ios project')
    #################### GYP_DEFINES ####
    defines = default_defines.copy()
    defines.update({
            'OS': 'ios',
            'output_location': 'gyp-ios',
            'arch_group': options.arch
    })

    os.environ['GYP_DEFINES'] = util.Gyp.getDefineString(defines, options.gyp_defines)

    gypArgs = getGypArgs('xcode', '../gyp-ios', gypFile)
    if options.verbose:
      gypArgs = ['-d', 'all'] + gypArgs
    gyp.main(gypArgs)

  # linux
  if 'linux' in options.platforms:
    ##################### GYP_DEFINES ####
    defines = default_defines.copy()
    defines.update({
            'os_posix': 1,
            'GYP_CROSSCOMPILE': 1,
            'output_location': 'gyp-linux',
            'OS': 'linux',
            'target_arch': 'x64',
            'clang': 1,
            'component': 'static_library',
            'use_system_stlport': 0
    })

    os.environ['GYP_DEFINES'] = util.Gyp.getDefineString(defines, options.gyp_defines)

    os.environ['GYP_GENERATOR_FLAGS'] = 'output_dir=DerivedData'
    os.environ['CC_target'] = '/usr/bin/clang'
    os.environ['CXX_target'] = '/usr/bin/clang++'
    os.environ['LD_target'] = '/usr/bin/clang++'
    gypArgs = getGypArgs('ninja', 'project/gyp-linux', gypFile)
    gyp.main(gypArgs)

  if 'android' in options.platforms:
    UtilLog.info('generating android project')

    ### Install android build deps if necessary
    # TODO: We should only check for deps in configure.py, not actuall install anything
    deps = []
    if not 'ANDROID_HOME' in os.environ:
      deps.append('android-sdk')

    if not 'ANDROID_NDK_ROOT' in os.environ:
      deps.append('android-ndk')

    if len(deps) > 0:
      # Install Android build dependencies
      options.deps = deps
      installer = installBuildDeps.DependencyInstaller(options);
      if not installer.install():
        UtilLog.error("Failed to verify build tool dependencies")
        return False

    for dep in deps:
      if dep == 'android-sdk':
        os.environ['ANDROID_HOME'] = os.path.join('/', 'usr', 'local', 'opt', dep)
      elif dep == 'android-ndk':
        os.environ['ANDROID_NDK_ROOT'] = os.path.join('/', 'usr', 'local', 'opt', dep)
    ### android deps installed

    ndk_root = os.environ['ANDROID_NDK_ROOT']

    os.environ['ANDROID_BUILD_TOP'] = configurePath
    ##################### GYP_DEFINES ####
    defines = default_defines.copy()
    defines.update({
            'os_posix': 1,
            'GYP_CROSSCOMPILE': 1,
            'output_location': 'gyp-android',
            'OS': 'android',
            'target_arch': 'arm',
            'clang': 1,
            'component': 'static_library',
            'use_system_stlport': 0,
            'ndk_root': ndk_root
    })

    os.environ['GYP_DEFINES'] = util.Gyp.getDefineString(defines, options.gyp_defines)

    toolchain = defines['android_toolchain']

    os.environ['CC_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang')
    os.environ['CXX_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['AR_target'] = os.path.join(ndk_root, 'toolchains/%s/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc-ar' % toolchain)
    os.environ['LD_target'] = os.path.join(ndk_root, 'toolchains/llvm/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['NM_target'] = os.path.join(ndk_root, 'toolchains/%s/prebuilt/darwin-x86_64/arm-linux-androideabi/bin/nm' % toolchain)
    os.environ['READELF_target'] = os.path.join(ndk_root, 'toolchains/%s/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-readelf' % toolchain)
    gypArgs = getGypArgs('ninja-android', 'project/gyp-android', gypFile)
    if options.verbose:
      gypArgs = ['-d', 'all'] + gypArgs
    gyp.main(gypArgs)

      
  if 'cmake' in options.platforms:
    UtilLog.info('generating cmake project')

    ### Install android build deps if necessary
    # TODO: We should only check for deps in configure.py, not actuall install anything
    ##################### GYP_DEFINES ####
    defines = default_defines.copy()
    defines.update({
      'os_posix': 1,
      'OS': 'cmake',
      'output_location': 'gyp-android',
      'arch_group': options.arch
    })
    os.environ['GYP_DEFINES'] = util.Gyp.getDefineString(defines)
    gypArgs = ['-G', 'output_dir=.'] + getGypArgs('cmake', '..', gypFile)
    if options.verbose:
      gypArgs = ['-d', 'all'] + gypArgs
    gyp.main(gypArgs)



  # return from main
  return True




if __name__ == '__main__':

  args = sys.argv
  if main(args):
    sys.exit(0)
  else:
    sys.exit(1)
