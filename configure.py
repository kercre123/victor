#!/usr/bin/env python -u

"""Issues commands to cmake and generated files."""

import inspect
import os
import os.path
import platform
import sys

GAME_ROOT = os.path.normpath(os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))

ENGINE_ROOT = os.path.join(GAME_ROOT, 'lib', 'anki', 'cozmo-engine')
sys.path.insert(0, ENGINE_ROOT)
from configure import BUILD_TOOLS_ROOT, initialize_colors
from configure import ArgumentParser, wipe_all, configure_anki_util, configure_platforms

sys.path.insert(0, BUILD_TOOLS_ROOT)
import ankibuild.cmake
import ankibuild.ios_deploy
import ankibuild.util
import ankibuild.xcode


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
        default_unity_dir = 'Unity'
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
            help='The name of the Unity folder (default "%(default)s) if it is in a standard location.')
    group.add_argument(
        '--unity-binary-path',
        metavar='path',
        help='the full path to the Unity executable')
    
    def postprocess_unity(args):
        if hasattr(args, 'unity_dir'):
            if not args.unity_binary_path and unity_binary_path_prefix:
                args.unity_binary_path = os.path.join(unity_binary_path_prefix, args.unity_dir, unity_binary_path_suffix)
            del args.unity_dir
    
    parser.add_postprocess_callback(postprocess_unity)
    
    # TODO: Prebuilt Unity.
    #parser.add_argument(
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
        ArgumentParser.Command('wipeall!', 'wipe all ignored files in the entire repo (including generated projects)')]
    parser.add_command_arguments(commands)
    
    platforms = ['mac', 'ios']
    # NOTE: Both mac + ios here.
    default_platforms = ['mac', 'ios']
    parser.add_platform_arguments(platforms, default_platforms)
    
    add_unity_arguments(parser)

    parser.add_output_directory_arguments(GAME_ROOT)
    parser.add_configuration_arguments()
    parser.add_verbose_arguments()
    
    return parser.parse_args()


###############################
# PLATFORM-DEPENDENT COMMANDS #
###############################

def add_xcode_ios_scheme(workspace, scheme_name, project_path, mode='auto'):
    scheme = dict(template=ankibuild.xcode.SCHEME_TEMPLATE_IOS, name=scheme_name, mode=mode, project_path=project_path)
    workspace.schemes.append(scheme)
    	
class GamePlatformConfiguration(object):
    
    def __init__(self, platform, options):
        if options.verbose:
            print_status('Initializing paths for platform {0}...'.format(platform))
        
        self.platform = platform
        self.options = options
        
        self.scheme = 'BUILD_WORKSPACE'
        self.derived_data_dir = os.path.join(self.options.output_dir, 'derived-data-{0}'.format(self.platform))
        self.config_path = os.path.join(self.options.output_dir, '{0}.xcconfig'.format(self.platform))
        
        self.workspace_name = 'CozmoWorkspace_{0}'.format(self.platform.upper())
        self.workspace_path = os.path.join(self.options.output_dir, '{0}.xcworkspace'.format(self.workspace_name))
        
        self.cmake_project_dir = os.path.join(self.options.output_dir, 'cmake-{0}'.format(self.platform))
        self.cmake_project_path = os.path.join(self.cmake_project_dir, 'CozmoGame_{0}.xcodeproj'.format(self.platform.upper()))
        
        if platform == 'ios':
            self.unity_xcode_project_dir = os.path.join(GAME_ROOT, 'unity', self.platform)
            self.unity_xcode_project_path = os.path.join(self.unity_xcode_project_dir, 'CozmoUnity_{0}.xcodeproj'.format(self.platform.upper()))
            self.unity_output_dir = os.path.join(self.options.output_dir, 'unity-{0}'.format(self.platform))
            self.unity_opencv_symlink = os.path.join(self.unity_output_dir, 'opencv')
            if not os.environ.get("CORETECH_EXTERNAL_DIR"):
                sys.exit('ERROR: Environment variable "CORETECH_EXTERNAL_DIR" must be defined.')
            self.unity_opencv_symlink_target = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"), 'build', 'opencv-ios', 'multiArchLibs')
            
            self.unity_output_symlink = os.path.join(self.unity_xcode_project_dir, 'UnityBuild')
            self.artifact_dir = os.path.join(self.options.output_dir, 'app-{0}'.format(self.platform))
            self.artifact_path = os.path.join(self.artifact_dir, 'cozmo.app')
    
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
        if self.options.command == 'delete':
            self.delete()
    
    def generate(self):
        if self.options.verbose:
            print_status('Generating files for platform {0}...'.format(self.platform))
        
        ankibuild.cmake.generate(self.cmake_project_dir, GAME_ROOT, self.platform)
        # to get rid of annoying warning
        ankibuild.util.File.mkdir_p(os.path.join(self.cmake_project_dir, 'Xcode', 'lib', self.options.configuration))
        
        rel_cmake_project = os.path.relpath(self.cmake_project_path, self.options.output_dir)
        workspace = ankibuild.xcode.XcodeWorkspace(self.workspace_name)
        workspace.add_project(rel_cmake_project)
        
        if self.platform == 'mac':
            workspace.add_scheme_cmake(self.scheme, rel_cmake_project)
        
        elif self.platform == 'ios':
            rel_unity_xcode_project = os.path.relpath(self.unity_xcode_project_path, self.options.output_dir)
            workspace.add_project(rel_unity_xcode_project)
            
            add_xcode_ios_scheme(workspace, self.scheme, rel_unity_xcode_project)
        
            if not os.path.exists(self.unity_opencv_symlink_target):
                sys.exit('ERROR: opencv does not appear to have been built for ios. Please build opencv for ios.')
            
            xcconfig = [
                'ANKI_BUILD_REPO_ROOT={0}'.format(GAME_ROOT),
                'ANKI_BUILD_GENERATED_XCODE_PROJECT_DIR={0}'.format(self.cmake_project_dir),
                'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo',
                'ANKI_BUILD_UNITY_OUTPUT_DIR={0}'.format(self.unity_output_dir),
                'ANKI_BUILD_UNITY_XCODE_BUILD_DIR=${ANKI_BUILD_UNITY_OUTPUT_DIR}/${CONFIGURATION}-${PLATFORM_NAME}',
                'ANKI_BUILD_UNITY_BINARY_PATH={0}'.format(self.options.unity_binary_path),
                #'ANKI_BUILD_PREBUILT_URL={0}'.format(self.options.prebuilt_url),
                'ANKI_BUILD_APP_PATH={0}'.format(self.artifact_path),
                '']
            
            ankibuild.util.File.mkdir_p(self.unity_output_dir)
            ankibuild.util.File.ln_s(self.unity_opencv_symlink_target, self.unity_opencv_symlink)
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
            sys.exit('Workspace {0} does not exist. (clean does not generate workspaces.)'.format(self.workspace_path))
        else:
            if self.options.command == 'clean':
                buildaction = 'clean'
            else:
                buildaction = 'build'
            
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
                #ankibuild.ios_deploy.noninteractive(self.artifact_path)
                ankibuild.ios_deploy.debug(self.artifact_path)
            
        #elif self.platform == 'mac':
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
        ankibuild.util.File.rm_rf(self.workspace_path)
        ankibuild.util.File.rm_rf(self.derived_data_dir)
        ankibuild.util.File.rm_rf(self.cmake_project_path)
        if self.platform == 'ios':
            ankibuild.util.File.rm_rf(self.artifact_dir)
            ankibuild.util.File.rm(self.unity_opencv_symlink)
            ankibuild.util.File.rm_rf(self.unity_output_dir)
            ankibuild.util.File.rm(self.unity_output_symlink)


###############
# ENTRY POINT #
###############

def main():
    initialize_colors()
    options = parse_game_arguments()
    
    if options.command == 'wipeall!':
        return wipe_all(options, GAME_ROOT, kill_unity=True)
    
    if options.command in ('generate', 'build', 'install', 'run'):
        configure_anki_util(options)
    
    configure_platforms(options, GamePlatformConfiguration, [options.output_dir])

if __name__ == '__main__':
    main()
