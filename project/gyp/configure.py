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
UtilLog = logging.getLogger('cozmo.game.configure')
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
  parser.add_argument('--mex', '-m', dest='mex', action='store_true',
                      help='builds mathlab\'s mex project')
  parser.add_argument('--withGyp', metavar='GYP_PATH', dest='gypPath', action='store', default=None,
                      help='Use gyp installation located at GYP_PATH')
  parser.add_argument('--buildTools', metavar='BUILD_TOOLS_PATH', dest='buildToolsPath', action='store', default=None,
                      help='Use build tools located at BUILD_TOOLS_PATH')
  parser.add_argument('--ankiUtil', metavar='ANKI_UTIL_PATH', dest='ankiUtilPath', action='store', default=None,
                      help='Use anki-util repo checked out at ANKI_UTIL_PATH')
  parser.add_argument('--webots', metavar='WEBOTS_PATH', dest='webotsPath', action='store', default=None,
                      help='Use webots aplication at WEBOTS_PATH')
  parser.add_argument('--coretechExternal', metavar='CORETECH_EXTERNAL_DIR', dest='coretechExternalPath', action='store', default=None,
                      help='Use coretech-external repo checked out at CORETECH_EXTERNAL_DIR')
  parser.add_argument('--coretechInternal', metavar='CORETECH_INTERNAL_DIR', dest='coretechInternalPath', action='store', default=None,
                      help='Use coretech-internal repo checked out at CORETECH_INTERNAL_DIR')
  parser.add_argument('--cozmoEngine', metavar='COZMO_ENGINE_PATH', dest='cozmoEnginePath', action='store', default=None,
                      help='Use cozmo-engine repo checked out at COZMO_ENGINE_PATH')
  parser.add_argument('--cozmoAssets', metavar='COZMO_ASSET_PATH', dest='cozmoAssetPath', action='store', default=None,
                      help='Use cozmo-asset repo checked out at COZMO_ASSET_PATH')
  parser.add_argument('--projectRoot', dest='projectRoot', action='store', default=None,
                      help='project location, assumed to be same as git repo root')
  parser.add_argument('--updateListsOnly', dest='updateListsOnly', action='store_true', default=False,
                      help='forces configure to only update .lst files and not run gyp project generation')


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

  if not options.cozmoAssetPath:
    options.cozmoAssetPath = os.path.join(options.projectRoot, 'lib/anki/products-cozmo-assets')
  if not os.path.exists(options.cozmoAssetPath):
    UtilLog.error('cozmo-asset not found [%s]' % (options.cozmoAssetPath) )
    return False

  if not options.cozmoEnginePath:
    options.cozmoEnginePath = os.path.join(options.projectRoot, 'lib/anki/cozmo-engine')
  if not os.path.exists(options.cozmoEnginePath):
    UtilLog.error('cozmo-engine not found [%s]' % (options.cozmoEnginePath) )
    return False
  cozmoEngineProjectPath = os.path.join(options.cozmoEnginePath, 'project/gyp/cozmoEngine.gyp')

  if not options.coretechInternalPath:
    options.coretechInternalPath = os.path.join(options.projectRoot, 'lib/anki/cozmo-engine/coretech')
  if not os.path.exists(options.coretechInternalPath):
    UtilLog.error('coretech-internal not found [%s]' % (options.coretechInternalPath) )
    return False
  coretechInternalProjectPath = os.path.join(options.coretechInternalPath, 'project/gyp/coretech-internal.gyp')

  if not options.buildToolsPath:
    options.buildToolsPath = os.path.join(options.projectRoot, 'lib/anki/cozmo-engine/tools/anki-util/tools/build-tools')
  if not os.path.exists(options.buildToolsPath):
    UtilLog.error('build tools not found [%s]' % (options.buildToolsPath) )
    return False

  sys.path.insert(0, os.path.join(options.buildToolsPath, 'tools/ankibuild'))
  import installBuildDeps
  import updateFileLists
  import generateUnityMeta

  if not options.ankiUtilPath:
    options.ankiUtilPath = os.path.join(options.projectRoot, 'lib/anki/cozmo-engine/tools/anki-util')
  if not os.path.exists(options.ankiUtilPath):
    UtilLog.error('anki-util not found [%s]' % (options.ankiUtilPath) )
    return False
  ankiUtilProjectPath = os.path.join(options.ankiUtilPath, 'project/gyp/util.gyp')
  gtestPath = os.path.join(options.ankiUtilPath, 'libs/framework')

  if not options.webotsPath:
    options.webotsPath = '/Applications/Webots'
  if not os.path.exists(options.webotsPath):
    UtilLog.error('webots not found [%s]' % options.webotsPath)
    return False
  webotsPath = options.webotsPath

  # do not check for coretech external, and gyp if we are only updating list files
  if not options.updateListsOnly:

    if not options.coretechExternalPath or not os.path.exists(options.coretechExternalPath):
      UtilLog.error('coretech-external not found [%s]' % (options.coretechExternalPath) )
      return False
    coretechExternalPath = options.coretechExternalPath

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

  #run clad's make
  unityGeneratedPath=os.path.join(projectRoot, 'unity/Cozmo/Assets/Scripts/Generated')
  if (subprocess.call(['make', '--silent', 'OUTPUT_DIR_CSHARP=' + unityGeneratedPath, 'csharp'],
    cwd=os.path.join(options.cozmoEnginePath, 'clad')) != 0):
    UtilLog.error("error compiling clad files")
    return False

  #generate unity's metafiles
  if (generateUnityMeta.generateMetaFiles(unityGeneratedPath, options.verbose)):
    UtilLog.error("error generating unity meta files")
    return False

  # update file lists
  generator = updateFileLists.FileListGenerator(options)
  generator.processFolder(['game/src/anki/cozmo', 'game/include', 
    'lib/anki/products-cozmo-assets/animations', 'lib/anki/products-cozmo-assets/faceAnimations', 
    'lib/anki/products-cozmo-assets/sounds'], ['project/gyp/cozmoGame.lst'])
  generator.processFolder(['simulator/controllers/webotsCtrlKeyboard', 'lib/anki/cozmo-engine/simulator/src/game'], ['project/gyp/ctrlKeyboard.lst'])
  generator.processFolder(['simulator/controllers/webotsCtrlBuildServerTest', 'lib/anki/cozmo-engine/simulator/src/game'], ['project/gyp/ctrlBuildServerTest.lst'])
  generator.processFolder(['simulator/controllers/webotsCtrlGameEngine'], ['project/gyp/ctrlGameEngine.lst'])
  generator.processFolder(['unity/CSharpBinding/src'], ['project/gyp/csharp.lst'])

  if options.updateListsOnly:
    # TODO: remove dependency on abspath. 
    # there is a bug due to 'os.chdir' and user passed rel path
    if (subprocess.call([os.path.join(options.cozmoEnginePath, 'project/gyp/configure.py'),
     '--updateListsOnly', '--buildTools', options.buildToolsPath, '--ankiUtil', options.ankiUtilPath]) != 0):
      UtilLog.error("error executing cozmo-engine configure")
      return False
    return True

  # update subprojects
  for platform in options.platforms:
    # TODO: we should pass in our own options with any additional overides..
    # subproject might need to know about the build-tools location, --verbose, and other args...
    if (subprocess.call([os.path.join(options.cozmoEnginePath, 'project/gyp/configure.py'),
     '--platform', platform, '--buildTools', options.buildToolsPath, '--ankiUtil', options.ankiUtilPath, '--updateListsOnly']) != 0):
      UtilLog.error("error executing submodule configure")
      return False


  configurePath = os.path.join(projectRoot, 'project/gyp')
  cozmoEngineConfigurePath = os.path.join(options.cozmoEnginePath, 'project/gyp')
  coretechInternalConfigurePath = os.path.join(options.coretechInternalPath, 'project/gyp')
  gypFile = 'cozmoGame.gyp'
  # gypify path names
  cgGtestPath = os.path.relpath(gtestPath, configurePath)
  ceGtestPath = os.path.relpath(gtestPath, cozmoEngineConfigurePath)
  ctiGtestPath = os.path.relpath(gtestPath, coretechInternalConfigurePath)
  cgAnkiUtilProjectPath = os.path.relpath(ankiUtilProjectPath, configurePath)
  ceAnkiUtilProjectPath = os.path.relpath(ankiUtilProjectPath, cozmoEngineConfigurePath)
  ctiAnkiUtilProjectPath = os.path.relpath(ankiUtilProjectPath, coretechInternalConfigurePath)
  cgCoretechInternalProjectPath = os.path.relpath(coretechInternalProjectPath, configurePath)
  ceCoretechInternalProjectPath = os.path.relpath(coretechInternalProjectPath, cozmoEngineConfigurePath)
  cgCozmoEngineProjectPath = os.path.relpath(os.path.join(options.cozmoEnginePath, 'project/gyp/cozmoEngine.gyp'), configurePath)
  cgMexProjectPath = os.path.relpath(os.path.join(options.cozmoEnginePath, 'project/gyp/cozmoEngineMex.gyp'), configurePath)
  cozmoConfigPath = os.path.join(options.cozmoEnginePath, 'resources')
  buildMex = 'no'
  if options.mex:
    buildMex = 'yes'

  # symlink coretech external resources 
  if subprocess.call(['mkdir', '-p',
    os.path.join(projectRoot, 'generated/resources')]) != 0 :
    UtilLog.error("error creating generated/resources")
    return False

  # symlink coretech external resources 
  if subprocess.call(['ln', '-s', '-f', '-h',
    os.path.join(coretechExternalPath, 'pocketsphinx/pocketsphinx/model/en-us'),
    os.path.join(projectRoot, 'generated/resources/pocketsphinx')]) != 0 :
    UtilLog.error("error symlinking pocket sphinx resources")
    return False
  
  # mac
  if 'mac' in options.platforms:
      os.environ['GYP_DEFINES'] = """ 
                                  OS=mac
                                  ndk_root=INVALID
                                  kazmath_library_type=static_library
                                  jsoncpp_library_type=static_library
                                  util_library_type=static_library
                                  worldviz_library_type=static_library
                                  arch_group={0}
                                  output_location={1}
                                  coretech_external_path={2}
                                  webots_path={3}
                                  cti-gtest_path={4}
                                  cti-util_gyp_path={5}
                                  cozmo_engine_path={6}
                                  cti-cozmo_engine_path={6}
                                  ce-gtest_path={7}
                                  ce-util_gyp_path={8}
                                  ce-cti_gyp_path={9}
                                  cg-gtest_path={10}
                                  cg-util_gyp_path={11}
                                  cg-cti_gyp_path={12}
                                  cg-ce_gyp_path={13}
                                  cg-mex_gyp_path={14}
                                  build-mex={15}
                                  cozmo_asset_path={16}
                                  """.format(
                                    options.arch, 
                                    os.path.join(options.projectRoot, 'generated/mac'),
                                    coretechExternalPath, 
                                    webotsPath,
                                    ctiGtestPath, 
                                    ctiAnkiUtilProjectPath,
                                    options.cozmoEnginePath,
                                    ceGtestPath, 
                                    ceAnkiUtilProjectPath, 
                                    ceCoretechInternalProjectPath,
                                    cgGtestPath, 
                                    cgAnkiUtilProjectPath, 
                                    cgCoretechInternalProjectPath,
                                    cgCozmoEngineProjectPath,
                                    cgMexProjectPath,
                                    buildMex,
                                    options.cozmoAssetPath
                                  )
      gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../../generated/mac', gypFile]
      gyp.main(gypArgs)
      



  # ios
  if 'ios' in options.platforms:
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
                                cg-mex_gyp_path=blah
                                cozmo_asset_path=blah
                                cozmo_config_path=blah
                                build-mex=no
                                arch_group={0}
                                output_location={1}
                                coretech_external_path={2}
                                webots_path={3}
                                cti-gtest_path={4}
                                cti-util_gyp_path={5}
                                cti-cozmo_engine_path={6}
                                ce-gtest_path={7}
                                ce-util_gyp_path={8}
                                ce-cti_gyp_path={9}
                                cg-gtest_path={10}
                                cg-util_gyp_path={11}
                                cg-cti_gyp_path={12}
                                cg-ce_gyp_path={13}
                                """.format(
                                  options.arch, 
                                  os.path.join(options.projectRoot, 'generated/ios'),
                                  coretechExternalPath, 
                                  webotsPath,
                                  ctiGtestPath, 
                                  ctiAnkiUtilProjectPath,
                                  options.cozmoEnginePath,
                                  ceGtestPath, 
                                  ceAnkiUtilProjectPath, 
                                  ceCoretechInternalProjectPath,
                                  cgGtestPath, 
                                  cgAnkiUtilProjectPath, 
                                  cgCoretechInternalProjectPath,
                                  cgCozmoEngineProjectPath,
                                )
    gypArgs = ['--check', '--depth', '.', '-f', 'xcode', '--toplevel-dir', '../..', '--generator-output', '../../generated/ios', gypFile]
    gyp.main(gypArgs)



  if 'android' in options.platforms:
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
    os.environ['GYP_DEFINES'] = """ 
                                kazmath_library_type=static_library
                                jsoncpp_library_type=static_library
                                util_library_type=static_library
                                worldviz_library_type=static_library
                                os_posix=1
                                OS=android
                                GYP_CROSSCOMPILE=1
                                target_arch=arm
                                clang=1
                                component=static_library
                                cg-mex_gyp_path=blah
                                cozmo_asset_path=blah
                                cozmo_config_path=blah
                                build-mex=no
                                use_system_stlport=0
                                arch_group={0}
                                output_location={1}
                                coretech_external_path={2}
                                webots_path={3}
                                cti-gtest_path={4}
                                cti-util_gyp_path={5}
                                cti-cozmo_engine_path={6}
                                ce-gtest_path={7}
                                ce-util_gyp_path={8}
                                ce-cti_gyp_path={9}
                                cg-gtest_path={10}
                                cg-util_gyp_path={11}
                                cg-cti_gyp_path={12}
                                cg-ce_gyp_path={13}
                                ndk_root={14}
                                """.format(
                                  options.arch, 
                                  os.path.join(options.projectRoot, 'generated/android'),
                                  coretechExternalPath, 
                                  webotsPath,
                                  ctiGtestPath, 
                                  ctiAnkiUtilProjectPath,
                                  options.cozmoEnginePath,
                                  ceGtestPath, 
                                  ceAnkiUtilProjectPath, 
                                  ceCoretechInternalProjectPath,
                                  cgGtestPath, 
                                  cgAnkiUtilProjectPath, 
                                  cgCoretechInternalProjectPath,
                                  cgCozmoEngineProjectPath,
                                  ndk_root,
                                )
    os.environ['CC_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang')
    os.environ['CXX_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['AR_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc-ar')
    os.environ['LD_target'] = os.path.join(ndk_root, 'toolchains/llvm-3.5/prebuilt/darwin-x86_64/bin/clang++')
    os.environ['NM_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/arm-linux-androideabi/bin/nm')
    os.environ['READELF_target'] = os.path.join(ndk_root, 'toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-readelf')
    gypArgs = ['--check', '--depth', '.', '-f', 'ninja-android', '--toplevel-dir', '../..', '--generator-output', 'generated/android', gypFile]
    gyp.main(gypArgs)



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
