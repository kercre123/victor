#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import os.path
import platform
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(REPO_ROOT, 'lib/anki/cozmo-engine/tools'))
import ankibuild.cmake
import ankibuild.util
import ankibuild.xcode
#import ankibuild.unity

def parse_arguments():
    
    if platform.system() == 'Windows':
        unity_binary_path_prefix = r'C:\Program Files (x86)'
        unity_binary_path_suffix = 'Editor\Unity.exe'
        default_unity_dir = 'Unity'
    elif platform.system() == 'Darwin':
        unity_binary_path_prefix = '/Applications'
        unity_binary_path_suffix = 'Unity.app/Contents/MacOS/Unity'
        default_unity_dir = 'Unity'
    else:
        unity_binary_path_prefix = None
        unity_binary_path_suffix = None
        default_unity_dir = None
    
    parser = argparse.ArgumentParser(
        description='Issues commands to cmake and generated files.',
        formatter_class=argparse.RawTextHelpFormatter)
    
    parser.add_argument(
        'command',
        metavar='command',
        type=str.lower,
        nargs='?',
        default='generate',
        choices=['generate', 'build', 'clean', 'delete'],
        help=textwrap.dedent('''\
            generate (default) -- generate or regenerate projects
            build -- generate, then build the generated projects
            clean -- issue the clean command to projects
            delete -- delete all generated projects
            '''))
    parser.add_argument(
        '-m',
        '--mode',
        metavar='mode',
        type=str.lower,
        default='auto',
        choices=['auto', 'unity', 'prebuilt', 'library'],
        help=textwrap.dedent('''\
            when building, how much to build
            unity -- build unity (unity must be installed)
            prebuilt -- download prebuilt unity libraries (need prebuilt url)
            library -- only build libraries
            auto (default) -- do the most complete build available
            '''))
        
    parser.add_argument(
        '-p',
        '--platform',
        dest='platforms',
        metavar='platform(s)',
        type=str.lower,
        default='mac+ios',
        help=textwrap.dedent('''\
            which platform(s) to target {mac, ios}
            can concatenate with + (default is "%(default)s")
            '''))
    parser.add_argument(
        '-s',
        '--simulator',
        action='store_true',
        help='specify the iphone simulator when building (will add platform "ios")')
    
    group = parser.add_mutually_exclusive_group(required=False)
    if default_unity_dir:
        group.add_argument(
            '-u',
            '--unity-dir',
            metavar='path',
            default=default_unity_dir,
            help=textwrap.dedent('''\
                the name of the Unity folder (default "%(default)s)
                if it is in a standard location
                '''))
    group.add_argument(
        '--unity-binary-path',
        metavar='path',
        help='the full path to the Unity executable')
    
    parser.add_argument(
        '--prebuilt-url',
        metavar='url',
        default='http://example.com/',
        help='the url to the prebuilt Unity libraries')
    
    parser.add_argument(
        '-b',
        '--build-dir',
        metavar='path',
        default=os.path.relpath(os.path.join(REPO_ROOT, 'build/')),
        help='where to build intermediate files (default is "%(default)s")')
    parser.add_argument(
        '-o',
        '--output-dir',
        metavar='path',
        default=os.path.relpath(os.path.join(REPO_ROOT, 'generated/')),
        help='where to put the generated projects (default is "%(default)s")')
    
    configurations = ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel']
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument(
        '-c',
        '--config',
        dest='configuration',
        metavar='configuration',
        type=str.lower,
        required=False,
        choices=[configuration.lower() for configuration in configurations],
        help=textwrap.dedent('''\
            build type (only valid with command 'build')
            options are: {0}
            '''.format(', '.join(configurations))))
    group.add_argument(
        '-d',
        '--debug',
        dest='configuration',
        action='store_const',
        const='debug',
        help='shortcut for --config Debug')
    
    
    options = parser.parse_args()
    
    for configuration in configurations:
        if configuration.lower() == options.configuration:
            options.configuration = configuration
    
    if options.platforms == 'all':
        options.platforms = 'mac+ios'
    options.platforms = options.platforms.replace(' ', '').split('+')
    for plat in options.platforms:
        if plat not in ('mac', 'ios'):
            sys.exit('Invalid platform "{0}"'.format(plat))
    if options.simulator and 'ios' not in options.platforms:
        options.platforms.append('ios')
    if not options.platforms:
        sys.exit('No platforms specified.')
    
    if hasattr(options, 'unity_dir'):
        if not options.unity_binary_path and unity_binary_path_prefix:
            options.unity_binary_path = os.path.join(unity_binary_path_prefix, options.unity_dir, unity_binary_path_suffix)
        del options.unity_dir
    
    return options

class PlatformConfiguration(object):
    
    def __init__(self, platform, options):
        self.platform = platform
        self.options = options
        
        if platform == 'ios':
            game_suffix = '_iOS'
            normal_suffix = '_iOS'
        elif platform == 'mac':
            game_suffix = ''
            normal_suffix = '_OSX'
        else:
            sys.exit('Cannot {1} platform of type "{0}"'.format(self.platform, self.options.command))
        
        self.cmake_project_dir = os.path.join(self.options.output_dir, 'game-{0}'.format(self.platform))
        self.cmake_project_path = os.path.join(self.cmake_project_dir, 'CozmoGame{0}.xcodeproj'.format(game_suffix))
        
        if platform == 'ios':
            self.unity_xcode_project_dir = os.path.join(REPO_ROOT, 'unity', self.platform)
            self.unity_xcode_project_path = os.path.join(self.unity_xcode_project_dir, 'CozmoUnity{0}.xcodeproj'.format(normal_suffix))
            self.unity_build_dir = os.path.join(self.options.build_dir, 'unity', self.platform)
            self.unity_opencv_symlink = os.path.join(self.unity_build_dir, 'opencv')
            if not os.environ.get("CORETECH_EXTERNAL_DIR"):
                sys.exit('ERROR: Environment variable "CORETECH_EXTERNAL_DIR" is not defined.')
            self.unity_opencv_symlink_target = os.path.join(os.environ.get("CORETECH_EXTERNAL_DIR"), "build/opencv-ios/multiArchLibs/")
        else:
            self.unity_xcode_project_dir = None
            self.unity_xcode_project_path = None
            self.unity_build_dir = None
            self.unity_opencv_symlink = None
            self.unity_opencv_symlink_target = None
        
        self.workspace_name = 'CozmoWorkspace{0}'.format(normal_suffix)
        self.workspace_dir = self.options.output_dir
        self.workspace_path = os.path.join(self.workspace_dir, '{0}.xcworkspace'.format(self.workspace_name))
        self.derived_data_path = os.path.join(self.options.build_dir, 'DerivedData')
        self.config_path = os.path.join(self.options.output_dir, '{0}.xcconfig'.format(self.platform))
        
        self.scheme = 'MODE_{0}'.format(self.options.mode.upper())
    
    def run(self):
        if self.options.command in ('generate', 'build'):
            self.generate()
        if self.options.command in ('build', 'clean'):
            self.build()
        if self.options.command == 'delete':
            self.delete()
    
    def generate(self):
        rel_cmake_project = os.path.relpath(self.cmake_project_path, self.workspace_dir)
        if self.unity_xcode_project_path:
            rel_unity_xcode_project = os.path.relpath(self.unity_xcode_project_path, self.workspace_dir)
            projects = [rel_cmake_project, rel_unity_xcode_project]
            target_project = rel_unity_xcode_project
            fixed_target = 'Unity-iPhone'
        else:
            projects = [rel_cmake_project]
            target_project = rel_cmake_project
            fixed_target = 'ALL_BUILD'
        
        workspace = ankibuild.xcode.XcodeWorkspace(self.workspace_name)
        workspace.add_project(rel_cmake_project)
        if self.unity_xcode_project_path:
            workspace.add_project(rel_unity_xcode_project)
        
        for mode in ['auto', 'unity', 'prebuilt', 'library']:
            workspace.add_scheme(mode, target_project, fixed_target)
        
        xcconfig = [
            'ANKI_BUILD_REPO_ROOT={0}'.format(REPO_ROOT),
            'ANKI_BUILD_UNITY_PROJECT_PATH=${ANKI_BUILD_REPO_ROOT}/unity/Cozmo',
            'ANKI_BUILD_UNITY_BUILD_DIR={0}'.format(self.unity_build_dir),
            'ANKI_BUILD_UNITY_XCODE_BUILD_DIR=${ANKI_BUILD_UNITY_BUILD_DIR}/${CONFIGURATION}-${PLATFORM_NAME}',
        ]
        if self.options.unity_binary_path:
            xcconfig.append('ANKI_BUILD_UNITY_BINARY_PATH={0}'.format(self.options.unity_binary_path))
        else:
            xcconfig.append('// ANKI_BUILD_UNITY_BINARY_PATH=')
        if self.options.prebuilt_url:
            xcconfig.append('ANKI_BUILD_PREBUILT_URL={0}'.format(self.options.prebuilt_url))
        else:
            xcconfig.append('// ANKI_BUILD_PREBUILT_URL=')
        xcconfig.append('')
        
        
        ankibuild.cmake.generate(self.cmake_project_dir, REPO_ROOT, self.platform)
        
        if self.unity_build_dir:
            ankibuild.util.File.mkdir_p(self.unity_build_dir)
        if self.unity_opencv_symlink:
            ankibuild.util.File.ln_s(self.unity_opencv_symlink_target, self.unity_opencv_symlink)
        
        workspace.generate(self.workspace_path, self.derived_data_path)
        ankibuild.util.File.write(self.config_path, '\n'.join(xcconfig))
        
        ankibuild.util.File.mkdir_p(self.derived_data_path)
    
    def build(self):
        
        if not os.path.exists(self.workspace_path):
            sys.exit('Workspace {0} does not exist. (clean does not generate workspaces.)'.format(self.workspace_path))
        else:
            ankibuild.xcode.build(
                buildaction=self.options.command,
                workspace=self.workspace_path,
                scheme=self.scheme,
                platform=self.platform,
                configuration=self.options.configuration,
                simulator=self.options.simulator)
        
    def delete(self):
        ankibuild.util.File.rm_rf(self.workspace_path)
        ankibuild.util.File.rm_rf(self.derived_data_path)
        if self.unity_opencv_symlink:
            ankibuild.util.File.rm(self.unity_opencv_symlink)
        if self.unity_build_dir:
            ankibuild.util.File.rm_rf(self.unity_build_dir)
        ankibuild.util.File.rm_rf(self.cmake_project_path)


if __name__ == '__main__':
    options = parse_arguments()
    
    if len(options.platforms) != 1:
        platforms_text = 'platforms {{{0}}}'
    else:
        platforms_text = 'platform {0}'
    platforms_text = platforms_text.format(','.join(options.platforms))
    
    print('')
    print('Running command {0} on {1}'.format(options.command, platforms_text))
    for platform in options.platforms:
        config = PlatformConfiguration(platform, options)
        config.run()
        
    if options.command == 'delete':
        ankibuild.util.File.rmdir(options.build_dir)
        ankibuild.util.File.rmdir(options.output_dir)
    
    print('DONE command {0} on {1}'.format(options.command, platforms_text))
    
    