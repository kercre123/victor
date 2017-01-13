#!/usr/bin/env python -u

"""Issues commands to gyp and generated files."""

import inspect
import os
import os.path
import platform
import sys
import subprocess
import shutil
import hashlib

GAME_ROOT = os.path.normpath(
        os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))

ENGINE_ROOT = GAME_ROOT
CERT_ROOT = os.path.join(GAME_ROOT, 'project', 'ios', 'ProvisioningProfiles')
EXTERNALS_ROOT = os.path.join(ENGINE_ROOT, 'EXTERNALS')
CTE_ROOT = os.path.join(EXTERNALS_ROOT, 'coretech_external')
sys.path.insert(0, ENGINE_ROOT)
PRODUCT_NAME = 'Cozmo'

from configure_engine import BUILD_TOOLS_ROOT, print_header, print_status
from configure_engine import ArgumentParser, generate_gyp, configure

sys.path.insert(0, BUILD_TOOLS_ROOT)
import ankibuild.android
import ankibuild.ios_deploy
import ankibuild.util
import ankibuild.unity
import ankibuild.xcode


####################
# HELPER FUNCTIONS #
####################

def check_unity_version():
    unity_project_dir = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME)
    project_ver = ankibuild.unity.UnityBuildConfig.get_unity_project_version(unity_project_dir)

    app_path = os.path.join(os.path.dirname(options.with_unity), '..', '..')
    app = ankibuild.unity.UnityApp(app_path)
    app_ver = app.bundle_version()

    return project_ver == app_ver


def find_unity_app_path():
    unity_project_dir = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME)
    project_ver = ankibuild.unity.UnityBuildConfig.get_unity_project_version(unity_project_dir)
    project_ver_short = project_ver.replace('.', '')

    unity_search_paths = [
        os.path.join('/', 'Applications', 'Unity'),
        os.path.join('/', 'Applications', 'Unity%s' % project_ver_short),
        os.path.join('/', 'Applications', 'Unity%s' % project_ver),
        os.path.join('/', 'Applications', 'Unity-%s' % project_ver),
        os.path.join('/', 'Applications', 'Unity-%s' % project_ver_short)
    ]

    unity_app_path = None
    for unity_path in unity_search_paths:
        if not os.path.exists(unity_path):
            continue

        app_path = os.path.join(unity_path, 'Unity.app')
        app = ankibuild.unity.UnityApp(app_path)
        app_ver = app.bundle_version()

        if app_ver == project_ver:
            unity_app_path = app_path
            break

    if not unity_app_path:
        sys.exit("[ERROR] Could not find Unity.app match project version %s" % project_ver)

    return unity_app_path

def get_android_device():
    process = subprocess.Popen("adb devices -l", shell=True, stdout=subprocess.PIPE)
    output = process.communicate()[0]
    device_list = output.splitlines()
    # get usb devices from strings that look like:
    # 85b6394a36495334       device usb:336592896X product:nobleltejv model:SM_N920C device:noblelte
    device_list = [x for x in device_list if "usb" in x]
    if len(device_list) == 0:
        return ""
    # return id of first device in list
    return device_list[0].split(' ')[0]

#stolen from http://stackoverflow.com/questions/1868714/how-do-i-copy-an-entire-directory-of-files-into-an-existing-directory-using-pyth
def copytree(src, dst, symlinks=False, ignore=None):
    if not os.path.exists(dst):
        os.makedirs(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            copytree(s, d, symlinks, ignore)
        else:
            if not os.path.exists(d) or os.stat(s).st_mtime - os.stat(d).st_mtime > 1:
                shutil.copy2(s, d)

#stolen from above
def getfilesrecursive(src):
    files = []
    if not os.path.exists(src):
        return files
    for item in os.listdir(src):
        s = os.path.join(src, item)
        if (os.path.isdir(s)):
            files.extend(getfilesrecursive(s))
        else:
            files.append(s)
    return files

####################
# ARGUMENT PARSING #
####################

def add_unity_arguments(parser):
    if platform.system() == 'Windows':
        unity_binary_path_prefix = r'C:\Program Files'
        unity_binary_path_suffix = r'Editor\Unity.exe'
        default_unity_dir = 'Unity'
    elif platform.system() == 'Darwin':
        unity_binary_path_prefix = '/Applications'
        unity_binary_path_suffix = 'Unity.app/Contents/MacOS/Unity'
        default_unity_dir = 'place_holder'  # if this was None -u would not be an option
    else:
        unity_binary_path_prefix = None
        unity_binary_path_suffix = None
        default_unity_dir = None

    group = parser.add_mutually_exclusive_group(required=False)
    if default_unity_dir:
        group.add_argument(
                '-u',
                '--unity-dir',
                metavar='path',
                default=default_unity_dir,
                help='Choose unity default directory.  Bypasses version check.')
    group.add_argument(
            '--unity-binary-path',
            metavar='path',
            help='the full path to the Unity executable. Bypasses version check.')

    def postprocess_unity(args):
        if hasattr(args, 'unity_dir'):
            if args.unity_dir is "place_holder":
                args.unity_binary_path = os.path.join(find_unity_app_path(), 'Contents', 'MacOS', 'Unity')
            else:
                if not args.unity_binary_path and unity_binary_path_prefix:
                    args.unity_binary_path = os.path.join(unity_binary_path_prefix, args.unity_dir,
                                                          unity_binary_path_suffix)
            del args.unity_dir

    parser.add_postprocess_callback(postprocess_unity)

    # TODO: Prebuilt Unity.
    # parser.add_argument(
    #    '--prebuilt-url',
    #    metavar='url',
    #    default='http://example.com/',
    #    help='the url to the prebuilt Unity libraries')


def parse_game_arguments():
    parser = ArgumentParser()

    commands = [
        ArgumentParser.Command('generate', 'generate or regenerate projects'),
        ArgumentParser.Command('build', 'generate, then build the generated projects'),
        ArgumentParser.Command('install', 'generate, build, then install the app on device'),
        ArgumentParser.Command('run', 'generate, build, install, then run the app on device'),
        ArgumentParser.Command('uninstall', 'uninstall the app from device'),
        ArgumentParser.Command('clean', 'issue the clean command to projects'),
        ArgumentParser.Command('delete', 'delete all generated projects'),
        ArgumentParser.Command('wipeall!',
                               'delete, then wipe all ignored files in the entire repo (including generated projects)')]
    parser.add_command_arguments(commands)

    platforms = ['mac', 'ios', 'android']
    # TODO: add support for v8a and x86
    # NOTE: Both mac + ios here.
    default_platforms = ['mac', 'ios', 'android']
    parser.add_platform_arguments(platforms, default_platforms)

    add_unity_arguments(parser)
    parser.add_output_directory_arguments(GAME_ROOT)
    parser.add_configuration_arguments()
    parser.add_verbose_arguments()
    parser.add_argument('--mex', '-m', dest='mex', action='store_true',
                        help='builds mathlab\'s mex project')

    # signing_group = parser.add_mutually_exclusive_group(required=False)

    parser.add_argument('--features', action='append', dest='features',
                      choices=['factoryTest', 'factoryTestDev', 'sdkOnly', 'standalone'], nargs='+',
                      help="Generates feature flags for project")

    parser.add_argument(
            '--provision-profile',
            metavar='string',
            default=None,
            required=False,
            help='Provide the mobile provisioning profile name for signing')

    parser.add_argument(
            '--codesign-force-dev',
            required=False,
            action='store_true',
            help='Force the mobileprovision to be signed with iPhone Developer')

    parser.add_argument(
            '--set-build-number',
            metavar='string',
            default='1',
            required=False,
            help='Set the Android build number')

    parser.add_argument(
            '--google-play',
            action='store_true',
            dest='google_play',
            help='Generate binaries for google-play store. (build obb files)')

    parser.add_argument(
            '--use-keychain',
            metavar='string',
            default=None,
            required=False,
            help='Provide the keychain to look for the signing cert.')

    parser.add_argument(
            '--do-not-check-dependencies',
            required=False,
            action='store_true',
            help='Use this flag to NOT pull down the latest dependencies (i.e. audio and animation)')

    parser.add_argument(
            '--use-external',
            required=False,
            default=EXTERNALS_ROOT,
            metavar='path',
            help='Use this flag to specify external dependency location.')

    parser.add_argument(
            '-l', '--logcat',
            required=False,
            action='store_true',
            help='Launch logcat for Android devices')

    parser.add_argument(
           '-n', '--nobuild',
           required=False,
           action='store_true',
           help='Skip build step, i.e. to install/run without building when no code has changed')

    return parser.parse_args()


###############################
# PLATFORM-DEPENDENT COMMANDS #
###############################

class GamePlatformConfiguration(object):
    def __init__(self, platform, options):
        if options.verbose:
            print_status('Initializing paths for platform {0}...'.format(platform))

        self.platform = platform
        self.options = options

        self.platform_build_dir = os.path.join(options.build_dir, self.platform)
        self.platform_output_dir = os.path.join(options.output_dir, self.platform)
        self.unity_project_root = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME)
        self.unity_plugin_dir = os.path.join(self.unity_project_root, 'Assets', 'Plugins')
        self.unity_scripts_dir = os.path.join(self.unity_project_root, 'Assets', 'Scripts')

        # TODO: When processors has more than one option make a parameter.
        self.processors = ["armeabi-v7a"]
        self.engine_generated = os.path.join(ENGINE_ROOT, "generated", self.platform)

        #because these is being deleted no matter what it must exist for all configurations.
        self.android_unity_plugin_dir = os.path.join(self.unity_plugin_dir, 'Android', 'libs', 'generated')
        self.android_lib_dir = os.path.join(self.platform_build_dir, 'libs')
        ankibuild.util.File.mkdir_p(self.android_lib_dir)
        self.android_prestrip_lib_dir = os.path.join(self.platform_build_dir, 'libs-prestrip')


        self.symlink_keys = ['opencv', 'HockeyApp']
        # The keys defined in symlink must exist in the following dictionaries
        self.unity_symlink = {}
        self.unity_target = {}
        self.gyp_dir = os.path.join(GAME_ROOT, 'project', 'gyp')

        if platform != 'android':
            self.workspace_name = '{0}Workspace_{1}'.format(PRODUCT_NAME, self.platform.upper())
            self.workspace_path = os.path.join(self.platform_output_dir, '{0}.xcworkspace'.format(self.workspace_name))

            self.scheme = 'BUILD_WORKSPACE'
            self.derived_data_dir = os.path.join(self.platform_build_dir, 'derived-data')
            self.config_path = os.path.join(self.platform_output_dir, '{0}.xcconfig'.format(self.platform))
            self.gyp_project_path = os.path.join(self.platform_output_dir, 'cozmoGame.xcodeproj')

        if platform == 'android':
            ankibuild.android.setup_android_ndk_and_sdk()
            if os.environ.get("ANDROID_NDK_ROOT"):
                self.android_ndk_root = os.environ.get("ANDROID_NDK_ROOT")
            else:
                sys.exit("Cannot find ANDROID_NDK_ROOT env var, script should have installed it. Perhaps internet is not available?")

            self.android_opencv_target = os.path.join(CTE_ROOT, 'build', 'opencv-android',
                                                      'OpenCV-android-sdk', 'sdk', 'native', 'libs')

        if platform == 'ios':
            self.unity_xcode_project_dir = os.path.join(GAME_ROOT, 'unity', self.platform)
            self.unity_xcode_project_path = os.path.join(self.unity_xcode_project_dir,
                                                         '{0}Unity_{1}.xcodeproj'.format(PRODUCT_NAME,
                                                                                         self.platform.upper()))
            self.unity_build_dir = os.path.join(self.platform_build_dir, 'unity-{0}'.format(self.platform))
            try:
                if self.options.provision_profile is not None:
                    tmp_pp = os.path.join(CERT_ROOT, self.options.provision_profile + '.mobileprovision')
                    self.provision_profile_uuid = subprocess.check_output(
                            '{0}/mpParse -f {1} -o uuid'.format(CERT_ROOT, tmp_pp), shell=True).strip()
                    if self.options.codesign_force_dev:
                        self.codesign_identity = "iPhone Developer"
                    else:
                        self.codesign_identity = subprocess.check_output(
                                '{0}/mpParse -f {1} -o codesign_identity'.format(CERT_ROOT, tmp_pp), shell=True).strip()
                else:
                    self.provision_profile_uuid = None
                    self.codesign_identity = "iPhone Developer"
                if self.options.use_keychain is not None:
                    self.other_cs_flags = '--keychain ' + self.options.use_keychain
                else:
                    self.other_cs_flags = None
            except TypeError or AttributeError:
                self.provision_profile_uuid = None
                self.codesign_identity = "iPhone Developer"
                self.other_cs_flags = None

            self.unity_output_symlink = os.path.join(self.unity_xcode_project_dir, 'generated')
            #there should be a 1-to-1 with self.symlink_list for _symlink
            for sl in self.symlink_keys:
                self.unity_symlink[sl] = os.path.join(self.unity_xcode_project_dir, sl)
            self.unity_target['opencv'] = os.path.join(CTE_ROOT, 'build', 'opencv-ios')
            self.unity_target['HockeyApp'] = os.path.join(GAME_ROOT, 'lib/HockeySDK-iOS')

            self.artifact_dir = os.path.join(self.platform_build_dir, 'app-{0}'.format(self.platform))
            self.artifact_path = os.path.join(self.artifact_dir, '{0}.app'.format(PRODUCT_NAME.lower()))

    def process(self):
        if self.options.verbose:
            print_status('Running {0} for configuration {1}...'.format(self.options.command, self.platform))
        if self.options.command in ('generate', 'build', 'install', 'run'):
            self.generate()
        if self.options.command in ('build', 'clean', 'install', 'run'):
            self.build()
        if self.options.command in ('install', 'run'):
            self.install()
        if self.options.command == 'run':
            self.run()
        if self.options.command == 'uninstall':
            self.uninstall()
        if self.options.command in ('delete', 'wipeall!'):
            self.delete()

        if self.platform == 'android' and self.options.logcat:
            subprocess.call("./tools/android/logcatAllDevices.sh", shell=True)

    def generate(self):
        if self.options.verbose:
            print_status('Generating files for platform {0}...'.format(self.platform))

        ankibuild.util.File.mkdir_p(self.platform_build_dir)
        ankibuild.util.File.mkdir_p(self.platform_output_dir)
        # START ENGINE GENERATE
        generate_gyp(self.gyp_dir, './configure.py', self.platform, self.options, os.path.join(ENGINE_ROOT, 'DEPS'))

        if self.platform != 'android':
            # Calling generate gyp instead of generate() for engine.
            relative_gyp_project = os.path.relpath(self.gyp_project_path, self.platform_output_dir)
            workspace = ankibuild.xcode.XcodeWorkspace(self.workspace_name)
            workspace.add_project(relative_gyp_project)
        else:
            self.call_engine('generate')
        # END ENGINE GENERATE

        #FEATURES BUILD FLAGS

        #writes to smcs file based on feature flags
        unityAssetsPath = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME, 'Assets');
        smcsFile = open(os.path.join(unityAssetsPath, 'smcs.rsp'), 'w')
        smcsSettings = "-warnaserror\n";

        if self.options.features != None and 'factoryTest' in self.options.features[0]:
            smcsSettings = smcsSettings + "-define:FACTORY_TEST"
        elif self.options.features != None and 'factoryTestDev' in self.options.features[0]:
            smcsSettings = smcsSettings + "-define:FACTORY_TEST" + "\n-define:FACTORY_TEST_DEV"
        elif self.options.features != None and 'sdkOnly' in self.options.features[0]:
            smcsSettings = smcsSettings + "-define:SDK_ONLY"
 

        smcsFile.write(smcsSettings + '\n');

        smcsFile.close()

        class_code = 'public class BuildFlags { \n'
        git_variable = '  public const string kGitHash = \"' + subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip() + '\";\n'
        buildFlagsPath = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME, 'Assets', 'Scripts', 'Generated')
        ankibuild.util.File.mkdir_p(buildFlagsPath)

        file = open(os.path.join(buildFlagsPath, 'BuildFlags.cs'), 'w')

        # writes a hash for the contents of the smcs file to force unity editor to recompile if the editor is already open.
        file.write(class_code + git_variable + '  public const string kSmcsFileHash = \"' + hashlib.sha256(smcsSettings).hexdigest() + '\";'+'\n'+'}')

        file.close()
        #NEED OF DEMO BUILD

        if self.platform == 'mac':
            workspace.add_scheme_gyp(self.scheme, relative_gyp_project)
            xcconfig = [
                'ANKI_BUILD_REPO_ROOT={0}'.format(GAME_ROOT),
                'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo',  # {0}'.format(PRODUCT_NAME),
                'ANKI_BUILD_TARGET={0}'.format(self.platform),
                '// ANKI_BUILD_USE_PREBUILT_UNITY=1',
                '']
            ankibuild.util.File.write(self.config_path, '\n'.join(xcconfig))

        elif self.platform == 'ios':
            relative_unity_xcode_project = os.path.relpath(self.unity_xcode_project_path, self.platform_output_dir)
            workspace.add_project(relative_unity_xcode_project)

            workspace.add_scheme_ios(self.scheme, relative_unity_xcode_project)

            xcconfig = [
                'ANKI_BUILD_REPO_ROOT={0}'.format(GAME_ROOT),
                'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo',  # {0}'.format(PRODUCT_NAME),
                'ANKI_BUILD_UNITY_BUILD_DIR={0}'.format(self.unity_build_dir),
                'ANKI_BUILD_UNITY_XCODE_BUILD_DIR=${ANKI_BUILD_UNITY_BUILD_DIR}/${CONFIGURATION}-${PLATFORM_NAME}',
                'CORETECH_EXTERNAL_DIR={0}'.format(CTE_ROOT),
                'ANKI_BUILD_UNITY_EXE={0}'.format(self.options.unity_binary_path),
                'ANKI_BUILD_TARGET={0}'.format(self.platform),
                '// ANKI_BUILD_USE_PREBUILT_UNITY=1',
                'ANKI_BUILD_APP_PATH={0}'.format(self.artifact_dir),
                'ANKI_BUILD_APP_PKG={0}'.format(self.artifact_path)]

            if self.other_cs_flags is not None:
                xcconfig += ['OTHER_CODE_SIGN_FLAGS="{0}"'.format(self.other_cs_flags)]
            if self.provision_profile_uuid is not None:
                xcconfig += ['PROVISIONING_PROFILE={0}'.format(self.provision_profile_uuid)]
	    if self.options.provision_profile is not None:
		xcconfig += ['PROVISIONING_PROFILE_SPECIFIER="{0}"'.format(self.options.provision_profile.replace ("_", " "))]
            xcconfig += ['CODE_SIGN_IDENTITY="{0}"'.format(self.codesign_identity)]
            xcconfig += ['']

            ankibuild.util.File.mkdir_p(self.unity_build_dir)
            ankibuild.util.File.ln_s(self.platform_output_dir, self.unity_output_symlink)
            for sl in self.symlink_keys:
                ankibuild.util.File.ln_s(self.unity_target[sl], self.unity_symlink[sl])
            ankibuild.util.File.write(self.config_path, '\n'.join(xcconfig))
            ankibuild.util.File.mkdir_p(self.artifact_dir)
        elif self.platform == 'android':
            ankibuild.util.File.mkdir_p(self.android_prestrip_lib_dir)
            for chipset in self.processors:
                # TODO: create a cp_r in util.
                libfolder = os.path.join(self.android_prestrip_lib_dir, chipset)
                # TODO: change cp -r steps to linking steps for android assets
                copytree(os.path.join(self.android_opencv_target, chipset), libfolder)
        else:
            sys.exit('Cannot generate for platform "{0}"'.format(self.platform))

        if self.platform != 'android':
            ankibuild.util.File.mkdir_p(self.derived_data_dir)
            workspace.generate(self.workspace_path, self.derived_data_dir)

        DASSource = os.path.join(GAME_ROOT, 'unity', 'Common', 'DASConfig', self.options.configuration, 'DASConfig.json')
        DASTarget = os.path.join(self.unity_project_root, 'Assets', 'StreamingAssets', 'DASConfig.json')
        ankibuild.util.File.rm(DASTarget)
        ankibuild.util.File.cp(DASSource, DASTarget)

        if self.options.verbose:
            print_status('Moved {0} to {1}'.format(DASSource, DASTarget))

    def build(self):
        if self.options.command == 'clean':
            buildaction = 'clean'
        else:
            buildaction = 'build'

        if self.options.nobuild and buildaction != 'clean':
            print_status('Skipping \'build\' step due to nobuild option')
            return

        if self.options.verbose:
            print_status('Building project for platform {0}...'.format(self.platform))

        if self.platform == 'android':
            self.call_engine(buildaction)
            # move files.
            # TODO: When cozmoEngine is built for different self.processors This will need to change to a for loop.
            ankibuild.util.File.cp(os.path.join(self.engine_generated, "out", self.options.configuration, "lib",
                                                "libcozmoEngine.so"),
                                   os.path.join(self.android_prestrip_lib_dir, self.processors[0]));
            ankibuild.util.File.cp(os.path.join(self.engine_generated, "out", self.options.configuration, "lib",
                                                "libDAS.so"),
                                   os.path.join(self.android_prestrip_lib_dir, self.processors[0]));
            # build android-specific java files
            self.build_java(buildaction)
            # move third ndk libs.
            self.move_ndk()
            # strip libraries and copy into unity
            self.strip_libs()
            # Call unity for game
            self.call_unity()


        elif not os.path.exists(self.workspace_path):
            print_status(
                    'Workspace {0} does not exist. (clean does not generate workspaces.)'.format(self.workspace_path))
            sys.exit(0)
        else:

            # Other cs flags and codesigning identity have default values that will work no matter what.
            try:
                ankibuild.xcode.build(
                        buildaction=buildaction,
                        workspace=self.workspace_path,
                        scheme=self.scheme,
                        platform=self.platform,
                        configuration=self.options.configuration,
                        simulator=self.options.simulator,
                        other_code_sign_flags=self.other_cs_flags,
                        provision_profile=self.provision_profile_uuid,
                        code_sign_identity=self.codesign_identity)
            except AttributeError:
                ankibuild.xcode.build(
                        buildaction=buildaction,
                        workspace=self.workspace_path,
                        scheme=self.scheme,
                        platform=self.platform,
                        configuration=self.options.configuration,
                        simulator=self.options.simulator)
        
        if self.options.features is not None and 'standalone' in self.options.features[0]:
            print("Building standalone-apk")
            ankibuild.util.File.execute(['./standalone-apk/stage-assets.sh'])
            ankibuild.util.File.execute(['buck', 'build', ':cozmoengine_standalone_app'])

    def call_engine(self, command):
        args = [os.path.join(ENGINE_ROOT, 'configure_engine.py'), command]
        args += ['--platform', '+'.join(self.options.platforms)]
        args += ['--config', self.options.configuration]
        if self.options.verbose:
            args += ['--verbose']
        if self.options.do_not_check_dependencies:
            args += ['--do-not-check-dependencies']
        ankibuild.util.File.execute(args)

    def move_ndk(self):
        #this must be more dynamic. But I'm not sure how to do this at the moment.
        ndk = ['stl', 'cxx', 'gnu']
        ndk_values = {'stl': "libstlport_shared.so", 'stl_path': "sources/cxx-stl/stlport/libs",
                      'cxx': "libc++_shared.so", 'cxx_path': "sources/cxx-stl/llvm-libc++/libs",
                      'gnu': "libgnustl_shared.so", 'gnu_path': "sources/cxx-stl/gnu-libstdc++/4.9/libs"}
        for chipset in self.processors:
            for lib_type in ndk:
                original = os.path.join(self.android_ndk_root, ndk_values['{0}_path'.format(lib_type)],
                                        chipset, ndk_values[lib_type])
                copy = os.path.join(self.android_prestrip_lib_dir, chipset)
                if os.path.isfile(original):
                    ankibuild.util.File.cp(original, copy)
                else:
                    sys.exit("Cannot locate {0}".format(original))

    def build_java(self, command):
        buck_args = ['buck', 'build', ':cozmojava']
        ankibuild.util.File.execute(buck_args)
        built_lib_path = os.path.join(GAME_ROOT, 'buck-out', 'gen', 'cozmojava.aar')
        if not os.path.exists(self.android_unity_plugin_dir):
            os.makedirs(self.android_unity_plugin_dir)
        ankibuild.util.File.cp(built_lib_path, self.android_unity_plugin_dir)

    def strip_libs(self):
        print_status('Stripping library symbols (if necessary)...')
        args = [os.path.join(self.gyp_dir, 'android-strip-libs.py')]
        args += ["--ndk-toolchain", os.path.join(self.android_ndk_root, 'toolchains',
                                                 'arm-linux-androideabi-4.9', 'prebuilt', 'darwin-x86_64')]
        args += ["--source-libs-dir", self.android_prestrip_lib_dir]
        args += ["--target-libs-dir", self.android_lib_dir]
        ankibuild.util.File.execute(args)

        # copy libs to unity plugin folder
        copytree(self.android_lib_dir, self.android_unity_plugin_dir)

    def call_unity(self):
        if self.platform != 'android':
            print ('Error: invalid call to unity. only allowed for android builds.')
            return False
        args = [self.options.unity_binary_path, "-quit", "-batchmode", "-nographics"]
        args += ['-projectPath', self.unity_project_root]
        args += ['-executeMethod', 'CommandLineBuild.Build']
        args += ['--platform', self.platform]
        args += ['--build-number', self.options.set_build_number]
        args += ['--build-path', os.path.join(self.options.build_dir, self.platform)]
        args += ['--build-type', 'PlayerAndAssets']
        args += ['--asset-path', 'Assets/StreamingAssets/cozmo_resources']
        if self.options.google_play:
            args += ['--config', "googleplay"]
        else:
            args += ['--config', self.options.configuration]
        ankibuild.util.File.execute(args)
        # TODO: Modify unity.py to handle this logic.  Problem is that unity.py is too coupled to xcode.
        # example of what this function should look like.
        # config = ankibuild.unity.UnityBuildConfig()
        # config.parse_arguments(self.options)
        # android_build = ankibuild.unity.UnityBuild(config)
        # ret = android_build.run()
        # TODO: Generate android gradle project here. Or just post this function call.

    def install(self):
        if self.options.verbose:
            print_status('Installing built binaries for platform {0}...'.format(self.platform))

        if self.platform == 'ios':
            if self.options.command == 'install':
                ankibuild.ios_deploy.install(self.artifact_path)

        elif self.platform == 'android':
            device = get_android_device()
            if len(device) > 0:
                subprocess.call("adb install -r -d ./build/android/Cozmo.apk", shell=True)
            else:
                print('{0}: No attached devices found via adb'.format(self.options.command))

        else:
            print('{0}: Nothing to do on platform {1}'.format(self.options.command, self.platform))


    def run(self):
        if self.options.verbose:
            print_status('Executing on platform {0}...'.format(self.platform))

        if self.platform == 'ios':
            if self.options.command == 'run':
                # ankibuild.ios_deploy.noninteractive(self.artifact_path)
                ankibuild.ios_deploy.debug(self.artifact_path)

                # elif self.platform == 'mac':
                # run webots?

        elif self.platform == 'android':
            device = get_android_device()
            if len(device) > 0:
                activity = "com.anki.cozmo/com.unity3d.player.UnityPlayerActivity"
                cmd = "adb -s {0} shell am start -n {1}".format(device, activity)
                subprocess.call(cmd, shell=True)
            else:
                print('{0}: No attached devices found via adb'.format(self.options.command))

        else:
            print('{0}: Nothing to do on platform {1}'.format(self.options.command, self.platform))

    def uninstall(self):
        if self.options.verbose:
            print_status('Uninstalling for platform {0}...'.format(self.platform))

        if self.platform == 'ios':
            ankibuild.ios_deploy.uninstall(self.artifact_path)
        else:
            print('{0}: Nothing to do on platform {1}'.format(self.options.command, self.platform))

    def delete(self):
        if self.options.verbose:
            print_status('Deleting generated files ...')

        for sl in self.symlink_keys:
            if os.path.exists(os.path.join('unity', 'ios', sl)):
                ankibuild.util.File.rm(sl)
        if os.path.exists(self.android_unity_plugin_dir):
            ankibuild.util.File.rm_rf(self.android_unity_plugin_dir)
        if os.path.exists(os.path.join(self.unity_scripts_dir, 'Generated')):
            ankibuild.util.File.rm_rf(os.path.join(self.unity_scripts_dir, 'Generated'))
            ankibuild.util.File.rm(os.path.join(self.unity_scripts_dir, 'Generated.meta'))

        ankibuild.util.File.rm_rf(self.options.build_dir)
        ankibuild.util.File.rm_rf(self.options.output_dir)


###############
# ENTRY POINT #
###############

def recursive_delete(options):
    "Calls delete on engine."
    print_header('RECURSING INTO {0}'.format(os.path.relpath(ENGINE_ROOT, GAME_ROOT)))
    args = [os.path.join(ENGINE_ROOT, 'configure_engine.py'), 'delete']
    args += ['--platform', '+'.join(options.platforms)]
    if options.verbose:
        args += ['--verbose']
    ankibuild.util.File.execute(args)


def main():
    options = parse_game_arguments()

    clad_csharp = os.path.join(GAME_ROOT, 'unity', PRODUCT_NAME, 'Assets', 'Scripts', 'Generated')
    clad_folders = [os.path.join(options.output_dir, 'clad'), clad_csharp, clad_csharp + '.meta']
    shared_generated_folders = [options.build_dir, options.output_dir]
    configure(options, GAME_ROOT, GamePlatformConfiguration, clad_folders, shared_generated_folders)

    # recurse on delete
    if options.command == 'delete':
        recursive_delete(options)


if __name__ == '__main__':
    main()
