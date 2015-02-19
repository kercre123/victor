#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import os.path
import platform
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(REPO_ROOT, 'lib/anki/cozmo-engine/tools'))
import cozmobuild.cmake
import cozmobuild.shell
import cozmobuild.xcode
#import cozmobuild.unity

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
        help='where to build the generated projects (default is "%(default)s")')
    parser.add_argument(
        '-o',
        '--output-dir',
        metavar='path',
        default=os.path.relpath(os.path.join(REPO_ROOT, 'build/generated')),
        help='where to put the final unity build (default is "%(default)s")')
    
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
    for platform in options.platforms:
        if platform not in ('mac', 'ios'):
            sys.exit('Invalid platform "{0}"'.format(platform))
    if options.simulator and 'ios' not in options.platforms:
        options.platforms.append('ios')
    if not options.platforms:
        sys.exit('No platforms specified.')
    
    if hasattr(options, 'unity_dir'):
        if not options.unity_binary_path and unity_binary_path_prefix:
            options.unity_binary_path = os.path.join(unity_binary_path_prefix, options.unity_dir, unity_binary_path_suffix)
        del options.unity_dir
    
    return options


def generate(options):
    for platform in options.platforms:
        if platform == 'ios':
            project_suffix = '_iOS'
            workspace_suffix = '_iOS'
        elif platform == 'mac':
            project_suffix = ''
            workspace_suffix = '_OSX'
        else:
        	sys.exit('Cannot {1} platform of type "{0}"'.format(platform, options.command))
        
        cmake_project_dir = os.path.join(options.build_dir, '{0}-engine'.format(platform))
        cmake_project_path = os.path.join(cmake_project_dir, 'CozmoGame{0}.xcodeproj'.format(project_suffix))
        unity_project_path = os.path.join(options.build_dir, '{0}-unity', 'CozmoUnity{0}.xcodeproj'.format(workspace_suffix))
        projects = [cmake_project_path, unity_project_path]
        
        workspace_path = os.path.join(options.build_dir, 'CozmoWorkspace{0}.xcworkspace'.format(workspace_suffix))
        
        xcconfig = [
            'COZMO_BUILD_REPO_ROOT={0}'.format(ROOT_PATH),
            'COZMO_BUILD_UNITY_PROJECT_PATH=${COZMO_BUILD_REPO_ROOT}/unity/Cozmo',
            'COZMO_BUILD_UNITY_BUILD_DIR={0}'.format(''),
            'COZMO_BUILD_UNITY_XCODE_BUILD_DIR=${COZMO_BUILD_UNITY_BUILD_DIR}/${CONFIGURATION}-${PLATFORM_NAME}',
        ]
        if options.unity_binary_path:
            xcconfig.append('COZMO_BUILD_UNITY_BINARY_PATH={0}'.format(options.unity_binary_path))
        else:
            xcconfig.append('// COZMO_BUILD_UNITY_BINARY_PATH=')
        
        if options.prebuilt_url:
            xcconfig.append('COZMO_BUILD_PREBUILT_URL={0}'.format(options.prebuilt_url))
        else:
            xcconfig.append('// COZMO_BUILD_PREBUILT_URL=')
        
        cozmobuild.cmake.generate(cmake_project_dir, REPO_ROOT, platform)
        cozmobuild.xcode.generate_workspace(workspace_path, projects)
        cozmobuild.xcode.generate_config(workspace_path, xcconfig)

def build(options):
    
    scheme = 'MODE_{0}'.format(options.mode.upper())
    
    for platform in options.platforms:
        if platform == 'ios':
            workspace_suffix = '_iOS'
        elif platform == 'mac':
            workspace_suffix = '_OSX'
        else:
        	sys.exit('Cannot {1} platform of type "{0}"'.format(platform, options.command))
        workspace_path = os.path.join(options.build_dir, 'CozmoWorkspace{0}.xcworkspace'.format(workspace_suffix))
        
        if not os.path.exists(workspace_path):
        	sys.exit('Workspace {0} does not exist. (clean does not generate workspaces.)'.format(workspace_path))
        else:
			cozmobuild.xcode.xcodebuild(
				buildaction=options.command,
				workspace=workspace_path,
				scheme=scheme,
				platform=platform,
				configuration=options.configuration,
				simulator=options.simulator)

def delete(options):
    for platform in options.platforms:
        project_dir = os.path.join(options.build_dir, platform)
        cozmobuild.shell.rm_rf(project_dir)

if __name__ == '__main__':
    options = parse_arguments()
    
    if len(options.platforms) != 1:
        platforms_text = 'platforms {{{0}}}'
    else:
        platforms_text = 'platform {0}'
    platforms_text = platforms_text.format(','.join(options.platforms))
    
    print('')
    print('Running command {0} on {1}'.format(options.command, platforms_text, options.mode))
    if options.command in ('generate', 'build'):
        generate(options)
    if options.command in ('build', 'clean'):
        build(options)
    if options.command == 'delete':
        delete(options)
    print('DONE command {0} on {1}'.format(options.command, platforms_text))
    