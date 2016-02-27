#!/usr/bin/env python -u

"""Issues commands to gyp and generated files."""

import inspect
import os
import os.path
import platform
import sys
import subprocess

GAME_ROOT = os.path.normpath(
    os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))

ENGINE_ROOT = os.path.join(GAME_ROOT, 'lib', 'anki', 'cozmo-engine')
CERT_ROOT = os.path.join(GAME_ROOT, 'project', 'ios', 'ProvisioningProfiles')
EXTERNAL_ROOT = os.path.join(GAME_ROOT, 'EXTERNALS')
sys.path.insert(0, ENGINE_ROOT)
PRODUCT_NAME = 'Cozmo'

from configure import BUILD_TOOLS_ROOT, print_header, print_status
from configure import ArgumentParser, generate_gyp, configure

sys.path.insert(0, BUILD_TOOLS_ROOT)
import ankibuild.ios_deploy
import ankibuild.util
import ankibuild.unity
import ankibuild.xcode
import importlib

dependencies = importlib.import_module("project.build-scripts.dependencies")


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
        default_unity_dir = 'place_holder' #if this was None -u would not be an option
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

    platforms = ['mac', 'ios']
    # NOTE: Both mac + ios here.
    default_platforms = ['mac', 'ios']
    parser.add_platform_arguments(platforms, default_platforms)

    add_unity_arguments(parser)
    parser.add_output_directory_arguments(GAME_ROOT)
    parser.add_configuration_arguments()
    parser.add_verbose_arguments()
    parser.add_argument('--mex', '-m', dest='mex', action='store_true',
                        help='builds mathlab\'s mex project')

    # signing_group = parser.add_mutually_exclusive_group(required=False)

    parser.add_argument(
        '--provision-profile',
        metavar='string',
        default=None,
        required=False,
        help='Provide the mobile provisioning profile name for signing')

    parser.add_argument(
            '--use-keychain',
            metavar='string',
            default=None,
            required=False,
            help='Provide the keychain to look for the signing cert.')

    parser.add_argument(
        '--do-not-check-dependencies',
        required=False,
        help='Use this flag to not pull down the latest dependencies(i.e. audio)')

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

        self.gyp_dir = os.path.join(GAME_ROOT, 'project', 'gyp')

        self.platform_build_dir = os.path.join(options.build_dir, self.platform)
        self.platform_output_dir = os.path.join(options.output_dir, self.platform)

        self.workspace_name = '{0}Workspace_{1}'.format(PRODUCT_NAME, self.platform.upper())
        self.workspace_path = os.path.join(self.platform_output_dir, '{0}.xcworkspace'.format(self.workspace_name))

        self.scheme = 'BUILD_WORKSPACE'
        self.derived_data_dir = os.path.join(self.platform_build_dir, 'derived-data')
        self.config_path = os.path.join(self.platform_output_dir, '{0}.xcconfig'.format(self.platform))

        self.gyp_project_path = os.path.join(self.platform_output_dir, 'cozmoGame.xcodeproj')
        if platform == 'ios':
            self.unity_xcode_project_dir = os.path.join(GAME_ROOT, 'unity', self.platform)
            self.unity_xcode_project_path = os.path.join(self.unity_xcode_project_dir,
                                                         '{0}Unity_{1}.xcodeproj'.format(PRODUCT_NAME, self.platform.upper()))
            self.unity_build_dir = os.path.join(self.platform_build_dir, 'unity-{0}'.format(self.platform))
            try:
                if self.options.provision_profile is not None:
                    tmp_pp = os.path.join(CERT_ROOT, self.options.provision_profile + '.mobileprovision')
                    self.provision_profile_uuid = subprocess.check_output(
                        '{0}/mpParse -f {1} -o uuid'.format(CERT_ROOT, tmp_pp), shell=True).strip()
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

            self.unity_opencv_symlink = os.path.join(self.unity_xcode_project_dir, 'opencv')

            if not os.environ.get("CORETECH_EXTERNAL_DIR"):
                sys.exit('ERROR: Environment variable "CORETECH_EXTERNAL_DIR" must be defined.')
            self.unity_opencv_symlink_target = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"), 'build', 'opencv-ios')
            self.unity_sphinx_symlink = os.path.join(self.unity_xcode_project_dir, 'sphinx')
            self.unity_sphinx_symlink_target = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"),
                                                            'pocketsphinx/pocketsphinx/generated/ios/DerivedData/Release-iphoneos')

            self.unity_build_symlink = os.path.join(self.unity_xcode_project_dir, 'UnityBuild')
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
            self.run()
        if self.options.command == 'uninstall':
            self.uninstall()
        if self.options.command in ('delete', 'wipeall!'):
            self.delete()

    def generate(self):
        if self.options.verbose:
            print_status('Generating files for platform {0}...'.format(self.platform))

        if not self.options.do_not_check_dependencies:
            assert isinstance(dependencies, object)
            dependencies.extract_dependencies("DEPS", EXTERNAL_ROOT)

        os.environ['EXTERNALS_DIR'] = "{0}".format(EXTERNAL_ROOT)

        ankibuild.util.File.mkdir_p(self.platform_build_dir)
        ankibuild.util.File.mkdir_p(self.platform_output_dir)

        generate_gyp(self.gyp_dir, self.platform, self.options)

        relative_gyp_project = os.path.relpath(self.gyp_project_path, self.platform_output_dir)
        workspace = ankibuild.xcode.XcodeWorkspace(self.workspace_name)
        workspace.add_project(relative_gyp_project)

        if self.platform == 'mac':
            workspace.add_scheme_gyp(self.scheme, relative_gyp_project)
            xcconfig = [
                'ANKI_BUILD_REPO_ROOT={0}'.format(GAME_ROOT),
                'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo', #{0}'.format(PRODUCT_NAME),
                'ANKI_BUILD_TARGET={0}'.format(self.platform),
                '// ANKI_BUILD_USE_PREBUILT_UNITY=1',
                '']
            ankibuild.util.File.write(self.config_path, '\n'.join(xcconfig))

        elif self.platform == 'ios':
            relative_unity_xcode_project = os.path.relpath(self.unity_xcode_project_path, self.platform_output_dir)
            workspace.add_project(relative_unity_xcode_project)

            workspace.add_scheme_ios(self.scheme, relative_unity_xcode_project)

            if not os.path.exists(self.unity_opencv_symlink_target):
                sys.exit('ERROR: opencv does not appear to have been built for ios. Please build opencv for ios.')

            xcconfig = [
                'ANKI_BUILD_REPO_ROOT={0}'.format(GAME_ROOT),
                'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo', #{0}'.format(PRODUCT_NAME),
                'ANKI_BUILD_UNITY_BUILD_DIR={0}'.format(self.unity_build_dir),
                'ANKI_BUILD_UNITY_XCODE_BUILD_DIR=${ANKI_BUILD_UNITY_BUILD_DIR}/${CONFIGURATION}-${PLATFORM_NAME}',
                'ANKI_BUILD_UNITY_EXE={0}'.format(self.options.unity_binary_path),
                'ANKI_BUILD_TARGET={0}'.format(self.platform),
                '// ANKI_BUILD_USE_PREBUILT_UNITY=1',
                'ANKI_BUILD_APP_PATH={0}'.format(self.artifact_path)]

            if self.other_cs_flags is not None:
                xcconfig += ['OTHER_CODE_SIGN_FLAGS="{0}"'.format(self.other_cs_flags)]
            if self.provision_profile_uuid is not None:
                xcconfig += ['PROVISIONING_PROFILE={0}'.format(self.provision_profile_uuid)]
            xcconfig += ['CODE_SIGN_IDENTITY="{0}"'.format(self.codesign_identity)]
            xcconfig += ['']

            ankibuild.util.File.mkdir_p(self.unity_build_dir)
            ankibuild.util.File.ln_s(self.platform_output_dir, self.unity_output_symlink)
            ankibuild.util.File.ln_s(self.unity_opencv_symlink_target, self.unity_opencv_symlink)
            ankibuild.util.File.ln_s(self.unity_sphinx_symlink_target, self.unity_sphinx_symlink)
            ankibuild.util.File.write(self.config_path, '\n'.join(xcconfig))
            ankibuild.util.File.mkdir_p(self.artifact_dir)
        else:
            sys.exit('Cannot generate for platform "{0}"'.format(self.platform))

        ankibuild.util.File.mkdir_p(self.derived_data_dir)
        workspace.generate(self.workspace_path, self.derived_data_dir)

    def build(self):
        if self.options.verbose:
            print_status('Building project for platform {0}...'.format(self.platform))

        if not os.path.exists(self.workspace_path):
            print_status(
                'Workspace {0} does not exist. (clean does not generate workspaces.)'.format(self.workspace_path))
            sys.exit(0)
        else:
            if self.options.command == 'clean':
                buildaction = 'clean'
            else:
                buildaction = 'build'

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





    def run(self):
        if self.options.verbose:
            print_status('Installing built binaries for platform {0}...'.format(self.platform))

        if self.platform == 'ios':
            if self.options.command == 'install':
                ankibuild.ios_deploy.install(self.artifact_path)
            elif self.options.command == 'run':
                # ankibuild.ios_deploy.noninteractive(self.artifact_path)
                ankibuild.ios_deploy.debug(self.artifact_path)

                # elif self.platform == 'mac':
                # run webots?
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
            print_status('Deleting generated files for platform {0}...'.format(self.platform))

        # reverse generation
        if self.platform == 'ios':
            ankibuild.util.File.rm(self.unity_build_symlink)
            ankibuild.util.File.rm(self.unity_output_symlink)
            ankibuild.util.File.rm(self.unity_opencv_symlink)
        ankibuild.util.File.rm_rf(self.platform_build_dir)
        ankibuild.util.File.rm_rf(self.platform_output_dir)



###############
# ENTRY POINT #
###############

def recursive_delete(options):
    "Calls delete on engine."
    print_header('RECURSING INTO {0}'.format(os.path.relpath(ENGINE_ROOT, GAME_ROOT)))
    args = [os.path.join(ENGINE_ROOT, 'configure.py'), 'delete']
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
