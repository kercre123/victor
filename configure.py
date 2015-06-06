#!/usr/bin/env python

"""Issues commands to cmake and generated files."""

import argparse
import inspect
import os
import os.path
import platform
import sys
import time

ENGINE_ROOT = os.path.normpath(os.path.abspath(os.path.realpath(os.path.dirname(inspect.getfile(inspect.currentframe())))))

BUILD_TOOLS_ROOT = os.path.join(ENGINE_ROOT, 'tools', 'anki-util', 'tools', 'build-tools', 'tools')
sys.path.insert(0, BUILD_TOOLS_ROOT)
import ankibuild.cmake
import ankibuild.util
import ankibuild.xcode

def _monkeypatch_build_tools():
    "Monkeypatch build tools to reduce number of newlines output."
    @staticmethod
    def _raw_execute(func, arglists, ignore_result):
        if ECHO_ALL:
            print('')
            print(File._escape_piped(arglists))
        return File._run_subprocess(func, arglists, ignore_result)

    ankibuild.util._raw_execute = _raw_execute

_monkeypatch_build_tools()


##################
# COLORED OUTPUT #
##################

def initialize_colors():
    if platform.system() == 'Windows':
        try:
            import colorama
            colorama.init()
            print('If you want colored output, use:')
            print('pip install colorama')
        except ImportError:
            return
    
    # https://github.com/freelan-developers/chromalog/blob/master/chromalog/stream.py
    if platform.system() == 'Windows' or getattr(sys.stdout, 'isatty', lambda: False)():
        
        def print_header_colored(text):
            print('\033[1m{0}\033[0m'.format(text))
        def print_status_colored(text):
            print('\033[34m{0}\033[0m'.format(text))
        def raw_dialog_colored(prompt):
            return raw_input('\033[31m{0}\033[0m'.format(prompt))
    
        global print_header
        global print_status
        global raw_dialog
        print_header = print_header_colored
        print_status = print_status_colored
        raw_dialog = raw_dialog_colored
        
def print_header(text):
    print('\n{0}'.format(text))

def print_status(text):
    print(text)

def raw_dialog(prompt):
    return raw_input(prompt)

def dialog(prompt):
    result = None
    while not result:
        result = raw_dialog(prompt).strip().lower()
    return result in ('y', 'yes')


####################
# ARGUMENT PARSING #
####################

class ArgumentParser(argparse.ArgumentParser):
    "Specialized form of the argument parser."
    
    class Command(object):
        def __init__(self, name, help_text):
            self.name = name
            self.help_text = help_text

    def __init__(self, description=None):
        super(ArgumentParser, self).__init__(
            description=(description or 'Issues commands to cmake and generated files.'),
            formatter_class=argparse.RawTextHelpFormatter)
        self.postprocess_callbacks = []
    
    def parse_known_args(self, args=None, namespace=None):
        args, argv = super(ArgumentParser, self).parse_known_args(args, namespace)
        for callback in self.postprocess_callbacks:
            callback(args)
        return args, argv
    
    @classmethod
    def format_options_list(cls, options_list):
        if not options_list:
            return '-'
        elif len(options_list) == 1:
            return options_list[0]
        else:
            return '{{{0}}}'.format(', '.join(options_list))
    
    def add_postprocess_callback(self, callback):
        self.postprocess_callbacks.append(callback)
    
    def add_command_arguments(self, commands):
        assert(commands)
        default = commands[0].name
        def default_text(command):
            if command == default:
                return '(default) '
            else:
                return ''
    
        help_lines = ['{name} {default_indicator}-- {help}'.format(
                name=command.name,
                default_indicator=default_text(command),
                help=command.help_text)
            for command in commands]
    
        self.add_argument(
            'command',
            metavar='command',
            type=str.lower,
            nargs='?',
            default=default,
            choices=[command.name for command in commands],
            help='\n'.join(help_lines))

    def add_platform_arguments(self, platforms, default_platforms):
    
        assert(platforms)
        assert(len(platforms) > 1)
        assert(default_platforms)
    
        help_text = 'Which platform(s) to target, of: {choices}. Can concatenate with +. Defaults to {default}.'.format(
          choices=self.format_options_list(platforms), default='+'.join(default_platforms))
        
        self.add_argument(
            '-p',
            '--platform',
            dest='platforms',
            metavar='platforms',
            type=str.lower,
            help=help_text)
        
        if 'ios' in platforms:
            self.add_argument(
                '-s',
                '--simulator',
                action='store_true',
                help='Specify the iphone simulator when building (implicitly adds platform "ios").')
        
        def postprocess_platforms(args):
            if not args.platforms:
                args.platforms = default_platforms
            
            elif args.platforms == 'all':
                args.platforms = platforms
            
            else:
                platform_list = []
                args.platforms = args.platforms.split('+')
                for platform in args.platforms:
                    platform = platform.strip()
                    if platform not in platforms:
                        sys.exit('Invalid platform "{0}"'.format(platform))
                    if platform not in platform_list:
                        platform_list.append(platform)
                args.platforms = platform_list
            
                if 'ios' in platforms and args.simulator and 'ios' not in args.platforms:
                    args.platforms.append('ios')
            
            assert(args.platforms)
        
        self.add_postprocess_callback(postprocess_platforms)

    def add_output_directory_arguments(self, root_path):
        self.add_argument(
            '-o',
            '--output-dir',
            metavar='path',
            default=os.path.relpath(os.path.join(root_path, 'build')),
            help='Where to generate projects and build (default is "%(default)s").')

        def postprocess_output_directory(args):
            args.output_dir = os.path.normpath(os.path.abspath(args.output_dir))
        
        self.add_postprocess_callback(postprocess_output_directory)

    def add_configuration_arguments(self):
        # defined in cmake, so hardcoded for now
        configurations = ['Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel']
        default_configuration = 'Debug'
        group = self.add_mutually_exclusive_group(required=False)
        group.add_argument(
            '-c',
            '--config',
            dest='configuration',
            metavar='configuration',
            type=str.lower,
            required=False,
            choices=[configuration.lower() for configuration in configurations],
            default=default_configuration.lower(),
            help='Configuration when building. Options are: {options}, defaults to {default}.'.format(
              options=self.format_options_list(configurations),
              default=default_configuration))
        group.add_argument(
            '-d',
            '--debug',
            dest='configuration',
            action='store_const',
            const='debug',
            help='shortcut for --config Debug')

        def postprocess_configuration(args):
            "Fixes casing on configuration."
            for configuration in configurations:
                if configuration.lower() == args.configuration:
                    args.configuration = configuration
                    break
            else:
                assert(false)
        
        self.add_postprocess_callback(postprocess_configuration)
    
    def add_verbose_arguments(self):
        self.add_argument(
            '-v',
            '--verbose',
            dest='verbose',
            action='store_true',
            help='Echo all commands to the console.')
        
        def postprocess_verbose(args):
            # TODO: Pass to cmake ANKI_OUTPUT_VERBOSE
            # global variables are Fun!
            ankibuild.util.ECHO_ALL = args.verbose
            ankibuild.xcode.RAW_OUTPUT = args.verbose
        
        self.add_postprocess_callback(postprocess_verbose)

def parse_engine_arguments():
    parser = ArgumentParser()
    
    commands = [
        ArgumentParser.Command('generate', 'generate or regenerate projects'),
        ArgumentParser.Command('build', 'generate, then build the generated projects'),
        ArgumentParser.Command('clean', 'issue the clean command to projects'),
        ArgumentParser.Command('delete', 'delete all generated projects'),
        ArgumentParser.Command('wipeall!', 'wipe all ignored files in the entire repo (including generated projects)')]
    parser.add_command_arguments(commands)
      
    platforms = ['mac', 'ios']
    default_platforms = ['mac']
    parser.add_platform_arguments(platforms, default_platforms)
    
    parser.add_output_directory_arguments(ENGINE_ROOT)
    parser.add_configuration_arguments()
    parser.add_verbose_arguments()
    
    return parser.parse_args()


#################################
# PLATFORM-INDEPENDENT COMMANDS #
#################################

def wipe_all(options, root_path, kill_unity=False):
    print_header('Running command {0}'.format(options.command))
    print_status('Checking what to remove:')
    old_dir = ankibuild.util.File.pwd()
    ankibuild.util.File.cd(root_path)
    ankibuild.util.File.execute(['git', 'clean', '-Xdn'])
    ankibuild.util.File.execute(['git', 'submodule', 'foreach', '--recursive', 'git', 'clean', '-Xdn'])
    prompt = 'Are you sure you want to wipe all ignored files and folders from the entire repository?'
    if kill_unity:
        prompt += ('\nYou will need to do a full reimport in Unity and rebuild everything from scratch.' +
            '\nIf Unity is open, it will also be closed without saving.')
    prompt += ' (Y/N) '
    if not dialog(prompt):
        ankibuild.util.File.cd(old_dir)
        sys.exit('Operation cancelled.')
    else:
        if kill_unity:
            print_status('Killing Unity...')
            ankibuild.util.File.execute(['killall', 'Unity'], ignore_result=True)
        print_status('Wiping all ignored files from the entire repository...')
        ankibuild.util.File.execute(['git', 'clean', '-Xdf'])
        ankibuild.util.File.execute(['git', 'submodule', 'foreach', '--recursive', 'git', 'clean', '-Xdf'])
        ankibuild.util.File.cd(old_dir)
        print_header('DONE command {0}'.format(options.command))

def unpack_anki_util_dependencies(options):
    if options.verbose:
        print_status('Unpacking anki-util dependencies...')
    saved_cwd = ankibuild.util.File.pwd()
    ankibuild.util.File.cd(os.path.join(ENGINE_ROOT, 'tools', 'anki-util', 'libs', 'packaged'))
    # HACK: Noisy; use evaluate instead of execute to suppress output.
    ankibuild.util.File.evaluate(['./unpackLibs.sh'])
    ankibuild.util.File.cd(saved_cwd)

def generate_anki_util_cmake(options):
    if options.verbose:
        print_status('Generating anki-util CMake file...')
    
    gyp_path = os.path.join(ENGINE_ROOT, 'tools', 'anki-util', 'project', 'gyp')
    output_path = os.path.join(ENGINE_ROOT, 'tools', 'CMakeLists.txt')
    
    # OK, this is a bit weird: I couldn't figure out a good way to generate to a different filename or directory,
    # so instead we reset the file modification time if the contents are unchanged.
    if options.verbose:
        print('stat {filename}'.format(filename=output_path))
    stat = os.stat(output_path)
    
    if os.path.exists(output_path):
        old_contents = ankibuild.util.File.read(output_path)
    else:
        old_contents = None
    
    cwd = ankibuild.util.File.pwd()
    ankibuild.util.File.cd(gyp_path)
    # HACK: Noisy; use evaluate instead of execute to suppress output.
    ankibuild.util.File.evaluate(['./configure.py', '--platform', 'cmake'])
    ankibuild.util.File.cd(cwd)
    
    new_contents = ankibuild.util.File.read(output_path)
    
    if old_contents == new_contents:
        if options.verbose:
            print('touch -t "{time}" {filename}'.format(time=time.ctime(stat.st_mtime), filename=output_path))
        os.utime(output_path, (stat.st_atime, stat.st_mtime))

def configure_anki_util(options):
    unpack_anki_util_dependencies(options)
    generate_anki_util_cmake(options)


###############################
# PLATFORM-DEPENDENT COMMANDS #
###############################

def generate_xcode_workspace_settings(workspace_path, derived_data_dir):
    import getpass
    import plistlib
    # HACK: Should be a static method.
    workspace = ankibuild.xcode.XcodeWorkspace('temp')
    xc_settings = workspace.generate_workspace_settings(derived_data_dir)
    username = getpass.getuser()
    xc_user_settings = os.path.join(workspace_path, 'xcuserdata', username + '.xcuserdatad', 'WorkspaceSettings.xcsettings')
    ankibuild.util.File.mkdir_p(os.path.dirname(xc_user_settings))
    plistlib.writePlist(xc_settings, xc_user_settings)

class EnginePlatformConfiguration(object):
    
    def __init__(self, platform, options):
        if options.verbose:
            print_status('Initializing paths for platform {0}...'.format(platform))
        
        self.platform = platform
        self.options = options
        
        self.project_dir = os.path.join(options.output_dir, 'cmake-{0}'.format(self.platform))
        self.project_path = os.path.join(self.project_dir, 'CozmoEngine_{0}.xcodeproj'.format(self.platform.upper()))
        self.derived_data_dir = os.path.join(self.options.output_dir, 'derived-data-{0}'.format(self.platform))
        
        self.workspace_path = os.path.join(self.project_path, 'project.xcworkspace')
    
    def process(self):
        if self.options.command in ('generate', 'build'):
            self.generate()
        if self.options.command in ('build', 'clean'):
            self.build()
        if self.options.command == 'delete':
            self.delete()
    
    def generate(self):
        if self.options.verbose:
            print_status('Generating files for platform {0}...'.format(self.platform))
        
        ankibuild.cmake.generate(self.project_dir, ENGINE_ROOT, self.platform)
        ankibuild.util.File.mkdir_p(os.path.join(self.project_dir, 'Xcode', 'lib', self.options.configuration))
        generate_xcode_workspace_settings(self.workspace_path, self.derived_data_dir)

    def build(self):
        if self.options.verbose:
            print_status('Building project for platform {0}...'.format(self.platform))
        
        if not os.path.exists(self.project_path):
            sys.exit('Project {0} does not exist. (Note: clean does not generate projects.)'.format(self.project_path))
        else:
            ankibuild.xcode.build(
                buildaction=self.options.command,
                project=self.project_path,
                target='ALL_BUILD',
                platform=self.platform,
                configuration=self.options.configuration,
                simulator=self.options.simulator)
    
    def delete(self):
        if self.options.verbose:
            print_status('Deleting generated files for platform {0}...'.format(self.platform))
        
        ankibuild.util.File.rm_rf(self.project_dir)

def configure_platforms(options, platform_configuration_type, shared_generated_folders):
    if len(options.platforms) != 1:
        platforms_text = 'PLATFORMS {{{0}}}'
    else:
        platforms_text = 'PLATFORM {0}'
    platforms_text = platforms_text.format(','.join(options.platforms).upper())
    
    print_header('RUNNING COMMAND {0} ON {1}'.format(options.command.upper(), platforms_text))
    for platform in options.platforms:
        print_header('PLATFORM {0}:'.format(platform.upper()))
        config = platform_configuration_type(platform, options)
        config.process()
    
    if shared_generated_folders:
        if options.command == 'delete':
            if options.verbose:
                print_status('Deleting generated folders (if empty)...')
            for folder in shared_generated_folders:
                ankibuild.util.File.rmdir(folder)
    
    print_header('DONE COMMAND {0} ON {1}'.format(options.command.upper(), platforms_text))


###############
# ENTRY POINT #
###############

def main():
    initialize_colors()
    options = parse_engine_arguments()
    
    if options.command == 'wipeall!':
        return wipe_all(options, ENGINE_ROOT, kill_unity=True)
    
    if options.command in ('generate', 'build'):
        configure_anki_util(options)
    
    configure_platforms(options, EnginePlatformConfiguration, [options.output_dir])

if __name__ == '__main__':
    main()
