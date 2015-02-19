#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import os.path
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(REPO_ROOT, 'tools'))
import cozmobuild.cmake
import cozmobuild.shell
import cozmobuild.xcode

def parse_arguments():
    
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
        '-p',
        '--platform',
        dest='platforms',
        metavar='platform(s)',
        type=str.lower,
        default='mac',
        help=textwrap.dedent('''\
            which platform(s) to target {mac, ios}
            can concatenate with + (default is "%(default)s")
            '''))
    parser.add_argument(
        '-s',
        '--simulator',
        action='store_true',
        help='specify the iphone simulator when building (will add platform "ios")')
    
    parser.add_argument(
        '-b',
        '--build-dir',
        metavar='path',
        default=os.path.relpath(os.path.join(REPO_ROOT, 'build/')),
        help='where to build the generated projects (default is "%(default)s")')
    
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
    
    return options


def generate(options):
    for platform in options.platforms:
        project_dir = os.path.join(options.build_dir, platform)
        cozmobuild.cmake.generate(project_dir, REPO_ROOT, platform)

def build(options):
    for platform in options.platforms:
        if platform == 'ios':
            suffix = '_iOS'
        elif platform == 'mac':
            suffix = ''
        else:
        	sys.exit('Cannot {1} platform of type "{0}"'.format(platform, options.command))
        project_path = os.path.join(options.build_dir, platform, 'CozmoEngine{0}.xcodeproj'.format(suffix))
        
        if not os.path.exists(project_path):
        	sys.exit('Project {0} does not exist. (clean does not generate projects.)'.format(project_path))
        else:
			cozmobuild.xcode.xcodebuild(
				buildaction=options.command,
				project=project_path,
				target='ALL_BUILD',
				platform=platform,
				configuration=options.configuration,
				simulator=options.simulator)

def delete(options):
    for platform in options.platforms:
        project_dir = os.path.join(options.build_dir, platform)
        cozmobuild.shell.rm_rf(project_dir)
    cozmobuild.shell.rmdir(options.build_dir)

if __name__ == '__main__':
    options = parse_arguments()
    
    if len(options.platforms) != 1:
        print('Running command {0} on platforms {{{1}}}.'.format(options.command, ','.join(options.platforms)))
    else:
        print('Running command {0} on platform {1}.'.format(options.command, ','.join(options.platforms)))
    if options.command in ('generate', 'build'):
        generate(options)
    if options.command in ('build', 'clean'):
        build(options)
    if options.command == 'delete':
        delete(options)
    print('DONE')

