#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import os.path
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(REPO_ROOT, 'tools'))
import ankibuild.cmake
import ankibuild.util
import ankibuild.xcode

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
    for platform in options.platforms:
        if platform not in ('mac', 'ios'):
            sys.exit('Invalid platform "{0}"'.format(platform))
    if options.simulator and 'ios' not in options.platforms:
        options.platforms.append('ios')
    if not options.platforms:
        sys.exit('No platforms specified.')
    
    return options


class PlatformConfiguration(object):
    
    def __init__(self, platform, options):
        self.platform = platform
        self.options = options
        
        if platform == 'ios':
            suffix = '_iOS'
        elif platform == 'mac':
            suffix = ''
        else:
            sys.exit('Cannot {1} platform of type "{0}"'.format(platform, options.command))
        
        self.project_dir = os.path.join(options.output_dir, self.platform)
        self.project_path = os.path.join(self.project_dir, 'CozmoEngine{0}.xcodeproj'.format(suffix))
    
    def run(self):
        if self.options.command in ('generate', 'build'):
            self.generate()
        if self.options.command in ('build', 'clean'):
            self.build()
        if self.options.command == 'delete':
            self.delete()
    
    def generate(self):
        ankibuild.cmake.generate(self.project_dir, REPO_ROOT, self.platform)

    def build(self):
        if not os.path.exists(self.project_path):
            sys.exit('Project {0} does not exist. (clean does not generate projects.)'.format(self.project_path))
        else:
            ankibuild.xcode.build(
                buildaction=self.options.command,
                project=self.project_path,
                target='ALL_BUILD',
                platform=self.platform,
                configuration=self.options.configuration,
                simulator=self.options.simulator)

    def delete(self):
        ankibuild.util.File.rm_rf(self.project_dir)


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
        ankibuild.util.File.rmdir(options.output_dir)
    
    print('DONE command {0} on {1}'.format(options.command, platforms_text))


