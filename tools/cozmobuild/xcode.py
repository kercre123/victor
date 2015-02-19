
#import getpass
#import os
#import plistlib

import os.path
import sys

import shell

def xcodebuild(
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
    
    shell.execute(arguments)

'''
class XcodeWorkspace(object):
    def __init__(self, name):
        self.name = name
        self.projects = []

    def add_project(self, project_path):
        self.projects.append(project_path)

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
        with open(xc_contents, 'w') as f:
            f.write("\n".join(output))

        # generate settings if necessary
        if derived_data_path is not None:
            xc_settings = self.generate_workspace_settings(derived_data_path)
            username = getpass.getuser()
            xc_user_settings = os.path.join(path, 'xcuserdata', username + '.xcuserdatad', 'WorkspaceSettings.xcsettings')
            util.File.mkdir_p(os.path.dirname(xc_user_settings))
            plistlib.writePlist(xc_settings, xc_user_settings)

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

'''

