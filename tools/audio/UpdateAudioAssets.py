#!/usr/bin/python

import sys
import subprocess
import argparse
import json
import os
import tempfile
import logging
from os import path


__scriptDir = path.dirname(path.realpath(__file__))
__projectRoot = path.join(__scriptDir , '..', '..')
__projectScriptDir = path.join(__projectRoot, 'project', 'build-scripts')
__engineScriptDir = path.join(__projectRoot, 'project', 'buildScripts')
sys.path.append(__projectScriptDir)
sys.path.append(__engineScriptDir)
import dependencies


# Project specific files and directors to perform scripts
__externalsDir = path.join(__projectRoot, 'EXTERNALS')
__wwiseToAppMetadataScript = path.join(__projectRoot, 'lib', 'audio', 'tools', 'WWiseToAppMetadata', 'WwiseToAppMetadata.py')
__wwiseIdFileName = 'Wwise_IDs.h'
__wwiseIdsFilePath = path.join(__externalsDir, 'cozmosoundbanks', 'GeneratedSoundBanks', __wwiseIdFileName)
__audioMetadataFileName= 'audioEventMetadata.csv'
__audioMetadataFilePath = path.join(__scriptDir, __audioMetadataFileName)
__audioCladDir = path.join(__projectRoot, 'clad', 'src', 'clad', 'audio')
__depsFilePath = path.join(__projectRoot, 'DEPS')
__namespaceList = ['Anki', 'Cozmo', 'Audio']

__errorMsg = '\'{}\' DOES NOT EXIST. Find your closest project Nerd!!'

# Available Command
__update_command = 'update'
__generate_command = 'generate'
__update_alt_workspace = 'update-alt-workspace'
__generate_maya_cozmo_data = 'generate-maya-cozmo-data'

# Parse input
def __parse_input_args():
    argv = sys.argv

    parser = argparse.ArgumentParser(description='Update Audio event metadata.csv file & generate project CLAD files')

    subparsers = parser.add_subparsers(dest='commands', help='commands')

    # Update sound banks and metadata
    update_parser = subparsers.add_parser(__update_command, help='Update project sound banks and metadata.csv')
    update_parser.add_argument('version', action='store', help='Update Soundbank Version. Pass \'-\' to not update version')
    update_parser.add_argument('--metadata-merge', action='store', help='Merge generate metadata.csv with other_metadata.csv file')

    # Generate project clad
    subparsers.add_parser(__generate_command, help='Generate Cozmo project audio clad files')

    # Update meta data in alternate location
    update_alt_workspace_parser = subparsers.add_parser(__update_alt_workspace, help='Update project meta data in an alternative workspace')
    update_alt_workspace_parser.add_argument('soundBankDir', action='store', help='Location of the Sound banks root directory')
    update_alt_workspace_parser.add_argument('--metadata-merge', action='store', help='Merge generate metadata.csv with other_metadata.csv file')

    # Update metadata in Maya Animations
    generate_maya_data_parser = subparsers.add_parser(__generate_maya_cozmo_data, help='Generate json file for maya plug-in')
    generate_maya_data_parser.add_argument('outputFilePath', action='store', help='Location the CozmoMayaPlugIn.json will be stored')
    generate_maya_data_parser.add_argument('groups', nargs='+', help='List of Event Group names to insert into json file')

    options = parser.parse_args(sys.argv[1:])

    return options


# MAIN!
def main():

    logging.basicConfig(level=logging.INFO)

    # Verify Paths
    if path.exists(__wwiseToAppMetadataScript) == False:
        __abort((__errorMsg.format(__wwiseToAppMetadataScript)))

    if path.exists(__audioCladDir) == False:
        __abort((__errorMsg.format(__audioCladDir)))

    if path.exists(__depsFilePath) == False:
        __abort((__errorMsg.format(__depsFilePath)))

    if path.exists(__externalsDir) == False:
        __abort((__errorMsg.format(__externalsDir)))

    # Parse input
    options = __parse_input_args()

    # Perform command
    if __update_command == options.commands:
        __updateSoundbanks(options.version, options.metadata_merge)

    elif __generate_command == options.commands:
        __generateProjectFiles()

    elif __update_alt_workspace == options.commands:
        __updateAltWorkspace(options.soundBankDir, options.metadata_merge)

    elif __generate_maya_cozmo_data == options.commands:
        __generateMayaCozmoData(options.outputFilePath, options.groups)


def __updateSoundbanks(version, mergeMetadataPath):
    # Update sound bank version in DEPS
    deps_json = None
    if version != '-':
        with open(__depsFilePath) as deps_file:
            deps_json = json.load(deps_file)

            logging.info('Update Soundbanks Version')
            svn_key = 'svn'
            repo_names_key = 'repo_names'
            cozmosoundbanks_key = 'cozmosoundbanks'
            version_key = 'version'

            abortMsg = 'Can not update Soundbank Version!!!!!! Can\'t Find'

            if svn_key not in deps_json:
                __abort('{} {}'.format(abortMsg, svn_key))

            if repo_names_key not in deps_json[svn_key]:
                __abort('{} {}'.format(abortMsg, repo_names_key))

            if cozmosoundbanks_key not in deps_json[svn_key][repo_names_key]:
                __abort('{} {}'.format(abortMsg, cozmosoundbanks_key))

            if version_key not in deps_json[svn_key][repo_names_key][cozmosoundbanks_key]:
                __abort('{} {}'.format(abortMsg, version_key))

        # Set soundbank version value
        deps_json[svn_key][repo_names_key][cozmosoundbanks_key][version_key] = version
         # Write file
        with open(__depsFilePath, 'w') as deps_file:
            json.dump(deps_json, deps_file, indent=4, sort_keys=True, separators=(',', ': '))
            deps_file.write(os.linesep)

        logging.info('DEPS file has been updated \'{}\''.format(path.realpath(__depsFilePath)))

        # Download Soundbanks
        dependencies.extract_dependencies(__depsFilePath, __externalsDir)

    # Generate metadata
    previousMetadataFilePath = __audioMetadataFilePath
    # Merge specific file if provided
    if mergeMetadataPath != None:
        previousMetadataFilePath = mergeMetadataPath

    # Verify wwise id header is available
    if path.exists(__wwiseIdsFilePath) == False:
        __abort((__errorMsg.format(__wwiseIdsFilePath)))

    # Update Audio Metadata.csv
    updateMetadataCmd = [__wwiseToAppMetadataScript, 'metadata', __wwiseIdsFilePath, __audioMetadataFilePath, '-m', previousMetadataFilePath]
    logging.debug("Running: {}".format(' '.join(updateMetadataCmd)))
    subprocess.call(updateMetadataCmd)
    logging.info('Metadata CSV has been updated and is ready for manual updates, file is located at: \'{}\''.format(path.realpath(__audioMetadataFilePath)))


def __generateProjectFiles():
    # Update Project Clad files
    generateCladCmd = [__wwiseToAppMetadataScript, 'clad', __wwiseIdsFilePath, __audioMetadataFilePath, __audioCladDir] + __namespaceList
    logging.debug("Running: {}".format(' '.join(generateCladCmd)))
    subprocess.call(generateCladCmd)
    logging.info('Project has been updated')


def __updateAltWorkspace(soundBankDir, mergeMetaFilePath):

    previousMetaPath = None

    # Check if sound bank dir exist
    if path.exists(soundBankDir) == False:
        logging.info('Sound Bank directory \'{}\' DOES NOT exist!'.format(soundBankDir))
        return

    if mergeMetaFilePath == None:
        # 1st check if there is one in the Sound Bank Dir
        aPath = path.join(soundBankDir, __audioMetadataFilePath)
        if path.exists(aPath):
            previousMetaPath = aPath
        else:
            # 2nd check if there is one in the Cozmo project
            if path.exists(__audioMetadataFilePath):
                previousMetaPath = __audioMetadataFilePath
            else:
                # 3rd create a new metadata file
                previousMetaPath = None

    # Generate Metadata.csv
    altWwiseHeaderIdPath = path.join(soundBankDir, __wwiseIdFileName)
    altMetadataPath = path.join(soundBankDir, __audioMetadataFilePath)
    subprocess.call([__wwiseToAppMetadataScript, 'metadata', altWwiseHeaderIdPath, altMetadataPath, '-m', previousMetaPath])
    logging.info('Metadata CSV has been updated and is ready for manual updates, file is located at: \'{}\''.format(path.realpath(altMetadataPath)))


def __generateMayaCozmoData(outputFilePath, groups):

    # Generate tmp metadata.csv by using current WwiseId.h and merging the projects current wwiseId.h events
    tempMetaFilePath = tempfile.mkstemp(dir='/tmp', prefix='tempAudioEventMetadata-', suffix='.csv')[1]
    tempMetaDataScriptArgs = [__wwiseToAppMetadataScript, 'metadata', __wwiseIdsFilePath, tempMetaFilePath, '-m', __audioMetadataFilePath]
    subprocess.call(tempMetaDataScriptArgs)

    # Use temp metadata to generate Maya Json file
    mayaDataScriptArgs = [__wwiseToAppMetadataScript, 'metadata-to-json', tempMetaFilePath, outputFilePath]
    mayaDataScriptArgs += groups
    subprocess.call(mayaDataScriptArgs)

    # Delete temp metadata.csv file
    os.remove(tempMetaFilePath)


def __abort(msg):
    logging.error(msg)
    exit(1)


if __name__ == '__main__':
    main()

