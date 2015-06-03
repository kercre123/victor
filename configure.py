#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import os.path
import sys
import textwrap

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(REPO_ROOT, 'tools', 'anki-util', 'tools', 'build-tools', 'tools'))
import ankibuild.cmake
import ankibuild.util
import ankibuild.xcode

def unpack_libraries():
	saved_cwd = ankibuild.util.File.pwd()
	ankibuild.util.File.cd(os.path.join(REPO_ROOT, 'tools', 'anki-util', 'libs', 'packaged'))
	ankibuild.util.File.execute(['./unpackLibs.sh'])
	ankibuild.util.File.cd(saved_cwd)

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
        choices=['generate', 'build', 'clean', 'delete', 'wipeall!'],
        help=textwrap.dedent('''\
            generate (default) -- generate or regenerate projects
            build -- generate, then build the generated projects
            clean -- issue the clean command to projects
            delete -- delete all generated projects
            wipeall! -- wipe all ignored files in the entire repo (including generated projects)
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
    parser.add_argument(
        '-v',
        '--verbose',
        dest='verbose',
        action='store_true',
        help='echo all commands to the console')
    
    
    options = parser.parse_args()
    
    for configuration in configurations:
        if configuration.lower() == options.configuration:
            options.configuration = configuration
            break
    else:
      options.configuration = configurations[0]

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
				cwd = ankibuild.util.File.pwd()
				ankibuild.util.File.cd(os.path.join(REPO_ROOT, 'tools', 'anki-util', 'project', 'gyp'))
				ankibuild.util.File.execute(['./configure.py', '--platform', 'cmake'])
				ankibuild.util.File.cd(cwd)
        
				ankibuild.cmake.generate(self.project_dir, REPO_ROOT, self.platform)
				ankibuild.util.File.mkdir_p(os.path.join(self.project_dir, 'Xcode', 'lib', options.configuration))

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

def dialog(prompt):
    result = None
    while not result:
        result = raw_input(prompt).strip().lower()
    return result in ('y', 'yes')

def wipe_all():
	print('')
	print('Checking what to remove:')
	ankibuild.util.File.execute(['git', 'clean', '-Xdn'])
	ankibuild.util.File.execute(['git', 'submodule', 'foreach', '--recursive', 'git', 'clean', '-Xdn'])
	if not dialog('Are you sure you want to wipe all ignored files from the entire repository?\n' +
			'You will need to do a full reimport in Unity and rebuild everything from scratch.\n' +
			'If Unity is open, it will also be closed without saving. (Y/N)'):
		sys.exit('Operation cancelled.')
	else:
		print('')
		print('Wiping all ignored files from the entire repository...')
		old_dir = ankibuild.util.File.pwd()
		ankibuild.util.File.cd(REPO_ROOT)
		ankibuild.util.File.execute(['git', 'clean', '-Xdf'])
		ankibuild.util.File.execute(['git', 'submodule', 'foreach', '--recursive', 'git', 'clean', '-Xdf'])
		ankibuild.util.File.cd(old_dir)
		ankibuild.util.File.execute(['killall', 'Unity'], ignore_result=True)
		print('DONE command {0}'.format(options.command))

if __name__ == '__main__':
    options = parse_arguments()
    ankibuild.util.ECHO_ALL = options.verbose
    ankibuild.xcode.RAW_OUTPUT = options.verbose
    
    if options.command == 'wipeall!':
    	wipe_all()
    	sys.exit()
    
    if options.command in ['generate', 'build']:
        unpack_libraries()
    
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
        print('')
        print('Deleting generated folders (if empty)...')
        ankibuild.util.File.rmdir(options.output_dir)
    
    print('DONE command {0} on {1}'.format(options.command, platforms_text))
