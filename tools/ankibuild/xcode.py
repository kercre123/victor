
import getpass
import os
import plistlib

import util

class XcodeWorkspace(object):
    def __init__(self, name):
        self.name = name
        self.projects = []
        self.schemes = []

    def add_project(self, project_path):
        self.projects.append(project_path)
    
    def add_scheme(self, mode, project_path, target):
    	scheme = dict(name='MODE_{0}'.format(mode.upper()), mode=mode, project_path=project_path, target=target)
    	self.schemes.append(scheme)

    def generate(self, path, derived_data_path):
        header = '<?xml version="1.0" encoding="UTF-8"?>'
        workspace_begin = '<Workspace version="1.0">'
        workspace_end = '</Workspace>'

        output = []
        output.append(header)
        output.append(workspace_begin)
        for project in self.projects:
            print project
            fileref = self.generate_file_ref(project)
            output.append(fileref)
        output.append(workspace_end)

        # make workspace bundle path
        util.File.mkdir_p(path)

        # generate contents
        xc_contents = os.path.join(path, 'contents.xcworkspacedata')
        util.File.write(xc_contents, "\n".join(output))

        # generate settings if necessary
        if derived_data_path is not None:
            xc_settings = self.generate_workspace_settings(derived_data_path)
            username = getpass.getuser()
            xc_user_settings = os.path.join(path, 'xcuserdata', username + '.xcuserdatad', 'WorkspaceSettings.xcsettings')
            util.File.mkdir_p(os.path.dirname(xc_user_settings))
            plistlib.writePlist(xc_settings, xc_user_settings)
        
        if self.schemes:
        	scheme_dir = os.path.join(path, 'xcshareddata', 'xcschemes')
        	util.File.mkdir_p(scheme_dir)
        	for scheme in self.schemes:
        		scheme_path = os.path.join(scheme_dir, '{0}.xcscheme'.format(scheme['name']))
        		scheme_data = SCHEME_TEMPLATE.format(**scheme)
        		util.File.write(scheme_path, scheme_data)

    def generate_file_ref(self, path, tag='group'):
        location = "%s:%s" % (tag, path)
        xml = "<FileRef location = \"%s\"></FileRef>" % location
        return xml

    def generate_workspace_settings(self, derived_data_path):
        settings = {
            'BuildLocationStyle': 'UseAppPreferences',
            'CustomBuildIntermediatesPath': 'Build/Intermediates',
            'CustomBuildLocationType': 'RelativeToDerivedData',
            'CustomBuildProductsPath': 'Build/Products',
            'DerivedDataCustomLocation': derived_data_path,
            'DerivedDataLocationStyle': 'AbsolutePath',
            'IssueFilterStyle': 'ShowActiveSchemeOnly',
            'LiveSourceIssuesEnabled': True,
            'SnapshotAutomaticallyBeforeSignificantChanges': False,
            'SnapshotLocationStyle': 'Default',
        }
        return settings


def build(
    project = None,
    workspace = None,
    target = None,
    scheme = None,
    configuration = None,
    platform = None,
    simulator = False,
    buildaction = 'build'):
    
    if not project and not workspace:
        raise ValueError('You must specify either a project or workspace to xcodebuild.')
    
    arguments = ['xcodebuild']
    
    if project:
        arguments += ['-project', os.path.abspath(project)]
    if workspace:
        arguments += ['-workspace', os.path.abspath(workspace)]
    if target:
        arguments += ['-target', target]
    if scheme:
        arguments += ['-scheme', scheme]
    if configuration:
        arguments += ['-configuration', configuration]

    if platform == 'ios':
        if simulator:
            arguments += ['-sdk', 'iphonesimulator']
        else:
            arguments += ['-sdk', 'iphoneos', 'ARCHS=armv6 armv7 arm64', 'ONLY_ACTIVE_ARCH=NO']
    
    arguments += [buildaction]
    
    util.File.execute(arguments)

SCHEME_TEMPLATE = '''\
<?xml version="1.0" encoding="UTF-8"?>
<Scheme
   LastUpgradeVersion = "0600"
   version = "1.3">
   <BuildAction
      parallelizeBuildables = "YES"
      buildImplicitDependencies = "YES">
      <PreActions>
         <ExecutionAction
            ActionType = "Xcode.IDEStandardExecutionActionsCore.ExecutionActionType.ShellScriptAction">
            <ActionContent
               title = "Run Script"
               scriptText = "export ANKI_BUILD_MODE=&quot;{mode}&quot;"
               shellToInvoke = "/bin/bash">
            </ActionContent>
         </ExecutionAction>
      </PreActions>
      <BuildActionEntries>
         <BuildActionEntry
            buildForTesting = "YES"
            buildForRunning = "YES"
            buildForProfiling = "YES"
            buildForArchiving = "YES"
            buildForAnalyzing = "YES">
            <BuildableReference
               BuildableIdentifier = "primary"
               BlueprintIdentifier = "6F942CD0023C40FDBB818758"
               BuildableName = "{target}"
               BlueprintName = "{target}"
               ReferencedContainer = "container:{project_path}">
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
            BlueprintIdentifier = "6F942CD0023C40FDBB818758"
            BuildableName = "{target}"
            BlueprintName = "{target}"
            ReferencedContainer = "container:{project_path}">
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
            BlueprintIdentifier = "6F942CD0023C40FDBB818758"
            BuildableName = "{target}"
            BlueprintName = "{target}"
            ReferencedContainer = "container:{project_path}">
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
