#!/usr/bin/env python -u

import sys
import subprocess
import argparse
import json
from os import path

__projectRoot = path.join('..', '..')
__buildScripDir = path.join(__projectRoot, 'project', 'build-scripts')
sys.path.append(__buildScripDir)
import dependencies


# Project specific files and directors to perform scripts
__wwiseToAppMetadataScript = path.join(__projectRoot, 'lib', 'anki', 'cozmo-engine', 'lib', 'audio', 'tools', 'WWiseToAppMetadata', 'WWiseToAppMetadata.py')
__wwiseIdsFilePath = path.join(__projectRoot, 'EXTERNALS', 'cozmosoundbanks', 'GeneratedSoundBanks', 'Wwise_IDs.h')
__audioMetadataFilePath = "audioEventMetadata.csv"
__audioCladDir = path.join(__projectRoot, 'lib', 'anki', 'cozmo-engine', 'clad', 'src', 'clad', 'audio')
__depsFilePath = path.join(__projectRoot, 'DEPS')
__externalsDir = path.join(__projectRoot, 'EXTERNALS')

__errorMsg = "%s DOES NOT EXIST. Find your closest project Nerd!!"

# Available Command
__update_command = "update"
__generate_command = "generate"

# Parse input
def __parse_input_args():
    argv = sys.argv

    parser = argparse.ArgumentParser(description="Update Audio event metadata.csv file & generate project CLAD files")

    subparsers = parser.add_subparsers(dest='commands', help='commands')

    # Update sound banks and metadata
    update_parser = subparsers.add_parser(__update_command, help='Update project sound banks and metadata.csv')
    update_parser.add_argument('version', action="store", help="Update Soundbank Version")
    update_parser.add_argument('--metadataMerge', action='store', help='Merge generate metadata.csv with other_metadata.csv file')

    # Generate project clad
    subparsers.add_parser(__generate_command, help='Generate audio metadata.csv')

    options = parser.parse_args(sys.argv[1:])

    return options


# MAIN!
def main():

    # Verify Paths
    if path.exists(__wwiseToAppMetadataScript) == False:
        __abort((__errorMsg % __wwiseToAppMetadataScript))

    if path.exists(__audioCladDir) == False:
        __abort((__errorMsg % __audioCladDir))

    if path.exists(__depsFilePath) == False:
        __abort((__errorMsg % __depsFilePath))

    if path.exists(__externalsDir) == False:
        __abort((__errorMsg % __externalsDir))

    # Parse input
    options = __parse_input_args()

    # Perform command
    if __update_command == options.commands:
        __updateSoundbanks(options.version, options.metadataMerge)

    elif __generate_command == options.commands:
        __generateProjectFiles()



def __updateSoundbanks(version, mergeMetadataPath):
    # Update sound bank version in DEPS
    deps_json = None
    with open(__depsFilePath) as deps_file:
        deps_json = json.load(deps_file)

        print "Update Soundbanks Version"
        svn_key = "svn"
        repo_names_key = "repo_names"
        cozmosoundbanks_key = "cozmosoundbanks"
        version_key = "version"

        abortMsg = "Can not update Soundbank Version!!!!!! Can't Find "

        if svn_key not in deps_json:
            __abort(abortMsg + " Can't Find " + svn_key)

        if repo_names_key not in deps_json[svn_key]:
            __abort(abortMsg + repo_names_key)

        if cozmosoundbanks_key not in deps_json[svn_key][repo_names_key]:
            __abort(abortMsg + cozmosoundbanks_key)

        if version_key not in deps_json[svn_key][repo_names_key][cozmosoundbanks_key]:
            __abort(abortMsg + version_key)

    # Set soundbank version value
    deps_json[svn_key][repo_names_key][cozmosoundbanks_key][version_key] = version
     # Write file
    with open(__depsFilePath, "w") as deps_file:
        json.dump(deps_json, deps_file, indent = 4, sort_keys = True)
        print "DEPS file has been updated"

    # Download Soundbanks
    dependencies.extract_dependencies(__depsFilePath, __externalsDir)

    # Generate metadata
    previousMetadataFilePath = __audioMetadataFilePath
    # Merge specific file if provided
    if mergeMetadataPath != None:
        previousMetadataFilePath = mergeMetadataPath

    # Verify wwise id header is available
    if path.exists(__wwiseIdsFilePath) == False:
        __abort((__errorMsg % __wwiseIdsFilePath))

    # Update Audio Metadata.csv
    subprocess.call([__wwiseToAppMetadataScript, 'metadata', __wwiseIdsFilePath, __audioMetadataFilePath, '-m', previousMetadataFilePath])
    print "Metadata CSV has been updated and is ready for manual updates, file is located at:\n%s" % path.realpath(__audioMetadataFilePath)



def __generateProjectFiles():
    # Update Project Clad files
    subprocess.call([__wwiseToAppMetadataScript, 'clad', __wwiseIdsFilePath, __audioMetadataFilePath, __audioCladDir])
    print "Project has been updated"



def __abort(msg):
    print "Error: " + msg
    exit(1)


if __name__ == '__main__':
    main()
