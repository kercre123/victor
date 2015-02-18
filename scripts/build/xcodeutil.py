import argparse
import glob
import os
import os.path
import shutil
import textwrap
import sys
try:
  import subprocess32 as subprocess
except ImportError:
  import subprocess

root_path = None
CMAKE_IOS_PATH = os.path.join(os.path.dirname(os.path.abspath(os.path.realpath(__file__))), 'iOS.cmake')
ECHO_ALL = True
projects = []
workspaces = []

class Project(object):

  def __init__(self, project_path, source_path, platform):
    self.project_path = project_path
    self.build_path = os.path.dirname(project_path)
    self.platform = platform
    self.source_path = source_path

  def generate(self):
    cmake(self.source_path, self.build_path, self.platform)

  def build(self, **kwargs):
    xcodebuild(project=self.project_path, platform=self.platform, **kwargs)

  def delete(self):
    rmdirs(self.build_path)


class Workspace(object):

  def __init__(self, workspace_path, build_scheme, extra_project_paths, platform):
    self.workspace_path = workspace_path
    self.platform = platform
    self.build_scheme = build_scheme
    self.extra_project_paths = extra_project_paths

  def generate(self, projects):
    generated_project_paths = [ project.project_path for project in projects ]
    generate_workspace(self.workspace_path, generated_project_paths, self.extra_project_paths)

  def build(self, **kwargs):
    xcodebuild(workspace=self.workspace_path, scheme=self.build_scheme, platform=self.platform, **kwargs)

  def delete(self):
    rmdirs(self.workspace_path)


def run_xcode(root_path, projects, workspaces = None, downstream_scripts = None, use_unity = False):
  set_root_path(root_path)
  args = parse_args(use_workspaces=workspaces, use_unity=use_unity)
  if downstream_scripts:
    for script in downstream_scripts:
      execute_downstream(script)

  print 'Running command {0} on platforms {1}.'.format(args.command, ', '.join(args.platforms))
  run_command(workspaces, projects, **vars(args))
  print 'DONE'


def run_command(workspaces, projects, command, platforms, unity = 'full', **kwargs):
  workspaces = workspaces or []
  projects = projects or []
  projects = [ project for project in projects if project.platform in platforms ]
  workspaces = [ workspace for workspace in workspaces if workspace.platform in platforms ]
  if command == 'delete':
    for workspace in workspaces:
      workspace.delete()

    for project in projects:
      project.delete()

    return
  if command != 'clean':
    for project in projects:
      project.generate()

    for workspace in workspaces:
      workspace_projects = [ project for project in projects if project.platform == workspace.platform ]
      workspace.generate(workspace_projects)

  if command in ('build', 'clean'):
    if workspaces:
      for workspace in workspaces:
        workspace.build(buildaction=command, **kwargs)

    else:
      for project in projects:
        project.build(buildaction=command, **kwargs)


def set_root_path(path):
  global root_path
  root_path = os.path.abspath(os.path.realpath(path))


def parse_args(use_workspaces = False, use_unity = False):
  if use_workspaces:
    single_text = 'xcode workspaces'
    both_text = 'xcode projects and workspaces'
  else:
    single_text = 'xcode projects'
    both_text = 'xcode projects'
  parser = argparse.ArgumentParser(description='Generates xcode project files.',
                                   formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument('command', default='generate', metavar='command', type=str.lower, nargs='?',
                      choices=['generate', 'build', 'clean','delete'], help=textwrap.dedent('''\
                      generate (default) -- generate or regenerate the {0}
                      build -- build the generated {1}
                      clean -- issue the clean command to {1}
                      delete -- delete all generated {0}
                      ''').format(both_text, single_text))
  parser.add_argument('-p', '--platform', type=str.lower, dest='platforms', metavar='platforms', default='osx+ios',
                      help=textwrap.dedent('''\
                      which platform(s) to target {osx, ios}
                      can concatenate with + (default is %(default)s)
                      ''')
  parser.add_argument('-s', '--simulator', action='store_true',
                      help='use the iphone simulator build (will add platform "ios")')
  group = parser.add_mutually_exclusive_group(required=False)
  group.add_argument('-c', '--config', type=str.lower, required=False, dest='configuration', metavar='configuration',
                     choices=['debug', 'release', 'relwithdebinfo', 'minsizerel'],
                     help=textwrap.dedent('''\
                     build type (only valid with command 'build')
                     options are: Debug, Release, RelWithDebInfo and MinSizeRel
                     '''))
  group.add_argument('-d', '--debug', action='store_const', const='debug', dest='configuration',
                     help='shortcut for --config Debug')
  if use_unity:
    parser.add_argument('-u', '--unity', choices=['full', 'none', 'dl'], default='full',
                        help=textwrap.dedent('''\
                        full (default) -- fully build unity
                        none -- do not build unity
                        dl -- download and build with prebuilt unity files
                        '''))
  args = parser.parse_args()
  if args.platforms in 'all':
    args.platforms = 'osx+ios'
  args.platforms = args.platforms.replace(' ', '').split('+')
  for platform in args.platforms:
    if platform not in ('osx', 'ios'):
      exit('Invalid platform "{0}".'.format(platform))

  return args


def complete_path(path):
  if root_path is None:
    sys.exit('ERROR: Root path not set!')
  return os.path.join(root_path, path)


def cd(path = '.'):
  path = complete_path(path)
  if ECHO_ALL:
    print 'cd ' + path
  try:
    os.chdir(path)
  except OSError as e:
    sys.exit('ERROR: Failed to change to directory {0}: {1}'.format(path, e.strerror))


def makedirs(path):
  path = complete_path(path)
  if ECHO_ALL:
    print 'mkdir -p ' + path
  try:
    os.makedirs(path)
  except OSError as e:
    if not os.path.isdir(path):
      sys.exit('ERROR: Failed to recursively create directories to {0}: {1}'.format(path, e.strerror))


def rmdirs(path):
  path = complete_path(path)
  if ECHO_ALL:
    print 'rm -rf ' + path
  try:
    shutil.rmtree(path)
  except OSError as e:
    if os.path.exists(path):
      sys.exit('ERROR: Failed to recursively remove directory {0}: {1}'.format(path, e.strerror))


def rm(path):
  path = complete_path(path)
  if ECHO_ALL:
    print 'rm ' + path
  try:
    os.remove(path)
  except OSError as e:
    if os.path.exists(path):
      sys.exit('ERROR: Failed to remove file {0}: {1}'.format(path, e.strerror))


def write(path, contents):
  path = complete_path(path)
  if ECHO_ALL:
    print 'writing to ' + path
  try:
    with open(path, 'w') as file:
      file.write(contents)
  except IOError as e:
    sys.exit('ERROR: Could not write to file {0}: {1}'.format(path, e.strerror))


def escape_shell(args):
  try:
    import pipes
    return ' '.join((pipes.quote(arg) for arg in args))
  except ImportError:
    return ' '.join(args)


def raw_execute(args):
  if not args:
    raise ValueError('No arguments to execute.')
  try:
    result = subprocess.call(args)
  except OSError as e:
    sys.exit('ERROR: Error in subprocess `{0}`: {1}'.format(escape_shell(args), e.strerror))

  if result:
    sys.exit('ERROR: Subprocess `{0}` exited with code {1}.'.format(escape_shell(args), result))


def execute(args):
  print ''
  if ECHO_ALL:
    raw_execute(['echo'] + args)
  raw_execute(args)


def execute_downstream(script_path):
  script_path = complete_path(script_path)
  arguments = [script_path] + sys.argv
  
  cd(os.path.dirname(script_path))
  execute(arguments)


def cmake(source_path, build_path, platform):
  arguments = ['cmake', '-GXcode']
  if platform == 'ios':
    arguments += ['-DANKI_IOS_BUILD=1']
    arguments += ['-DCMAKE_TOOLCHAIN_FILE=' + CMAKE_IOS_PATH]
  arguments += [complete_path(source_path)]
  
  makedirs(build_path)
  cd(build_path)
  execute(arguments)


def xcodebuild(project = None, workspace = None, scheme = None,
               configuration = None, platform = None, simulator = False,
               buildaction = 'build'):
  if not project and not workspace:
    raise ValueError('You must specify either a project or workspace to xcodebuild.')
  arguments = ['xcodebuild']
  
  if project:
    arguments += ['-project', complete_path(project)]
  if workspace:
    arguments += ['-workspace', complete_path(workspace)]
  if scheme:
    arguments += ['-scheme', scheme]
  if configuration:
    arguments += ['-configuration', configuration]

  if platform == 'ios':
    if simulator:
      arguments += ['-sdk', 'iphoneos', 'ARCHS=armv6 armv7 arm64', 'ONLY_ACTIVE_ARCH=NO']
    else:
      arguments += ['-sdk', 'iphonesimulator']

  arguments += [buildaction]
  
  cd()
  execute(arguments)


def generate_workspace(workspace_path, generated_project_paths, extra_project_paths):
  workspace_full = complete_path(os.path.dirname(workspace_path))
  for path in generated_project_paths:
    relpath = os.path.relpath(complete_path(path), os.path.dirname(workspace_full))
    scheme_contents = XCSCHEME_XML.format(relpath)
    scheme_path = os.path.join(path, 'xcshareddata/xcschemes/ALL_BUILD.xcscheme')
    
    makedirs(os.path.dirname(scheme_path))
    write(scheme_path, scheme_contents)

  entries = [WORKSPACE_TOP_XML]
  for path in generated_project_paths + extra_project_paths:
    relpath = os.path.relpath(complete_path(path), workspace_full)
    entry = WORKSPACE_ENTRY_XML.format(relpath)
    entries.append(entry)

  entries.append(WORKSPACE_BOTTOM_XML)
  workspace_file = os.path.join(workspace_path, 'contents.xcworkspacedata')
  
  makedirs(workspace_path)
  write(workspace_file, ''.join(entries))


WORKSPACE_TOP_XML = '''\
<?xml version="1.0" encoding="UTF-8"?>
<Workspace
  version = "1.0">
'''
WORKSPACE_BOTTOM_XML = '''\
</Workspace>
'''
WORKSPACE_ENTRY_XML = '''\
  <FileRef
      location = "group:{0}">
  </FileRef>
'''
XCSCHEME_XML = '''\
<?xml version="1.0" encoding="UTF-8"?>
<Scheme
   LastUpgradeVersion = "0600"
   version = "1.3">
   <BuildAction
      parallelizeBuildables = "YES"
      buildImplicitDependencies = "YES">
      <BuildActionEntries>
         <BuildActionEntry
            buildForTesting = "YES"
            buildForRunning = "YES"
            buildForProfiling = "YES"
            buildForArchiving = "YES"
            buildForAnalyzing = "YES">
            <BuildableReference
               BuildableIdentifier = "primary"
               BlueprintIdentifier = "CBA5B968ADF64E8D92D8349D"
               BuildableName = "ALL_BUILD"
               BlueprintName = "ALL_BUILD"
               ReferencedContainer = "container:{0}">
            </BuildableReference>
         </BuildActionEntry>
      </BuildActionEntries>
   </BuildAction>
   <TestAction
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      shouldUseLaunchSchemeArgsEnv = "YES"
      buildConfiguration = "Debug">
      <Testables>
      </Testables>
   </TestAction>
   <LaunchAction
      selectedDebuggerIdentifier = "Xcode.DebuggerFoundation.Debugger.LLDB"
      selectedLauncherIdentifier = "Xcode.DebuggerFoundation.Launcher.LLDB"
      launchStyle = "0"
      useCustomWorkingDirectory = "NO"
      buildConfiguration = "Debug"
      ignoresPersistentStateOnLaunch = "NO"
      debugDocumentVersioning = "YES"
      allowLocationSimulation = "YES">
      <MacroExpansion>
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "CBA5B968ADF64E8D92D8349D"
            BuildableName = "ALL_BUILD"
            BlueprintName = "ALL_BUILD"
            ReferencedContainer = "container:{0}">
         </BuildableReference>
      </MacroExpansion>
      <AdditionalOptions>
      </AdditionalOptions>
   </LaunchAction>
   <ProfileAction
      shouldUseLaunchSchemeArgsEnv = "YES"
      savedToolIdentifier = ""
      useCustomWorkingDirectory = "NO"
      buildConfiguration = "Release"
      debugDocumentVersioning = "YES">
      <MacroExpansion>
         <BuildableReference
            BuildableIdentifier = "primary"
            BlueprintIdentifier = "CBA5B968ADF64E8D92D8349D"
            BuildableName = "ALL_BUILD"
            BlueprintName = "ALL_BUILD"
            ReferencedContainer = "container:{0}">
         </BuildableReference>
      </MacroExpansion>
   </ProfileAction>
   <AnalyzeAction
      buildConfiguration = "Debug">
   </AnalyzeAction>
   <ArchiveAction
      buildConfiguration = "Release"
      revealArchiveInOrganizer = "YES">
   </ArchiveAction>
</Scheme>
'''
