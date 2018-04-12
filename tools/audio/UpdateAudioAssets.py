#!/usr/bin/python

import argparse
import json
import logging
import os
import subprocess
import sys
import tempfile
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
__wwiseIdsFilePath = path.join(__externalsDir, 'victor-audio-assets', 'metadata', __wwiseIdFileName)
__audioMetadataFileName= 'audioEventMetadata.csv'
__audioMetadataFilePath = path.join(__scriptDir, __audioMetadataFileName)
__audioCladDir = path.join(__projectRoot, 'robot', 'clad', 'src', 'clad', 'audio')
__depsFilePath = path.join(__projectRoot, 'DEPS')
__namespaceList = ['Anki', 'AudioMetaData']

# DEPS JSON keys
__svn_key = 'svn'
__repo_names_key = 'repo_names'
__soundbanks_key = 'victor-audio-assets'
__version_key = 'version'

__errorMsg = '\'{}\' DOES NOT EXIST. Find your closest project Nerd!!'


# Available Command
__update_command = 'update'
__generate_command = 'generate'
__update_alt_workspace = 'update-alt-workspace'
__generate_maya_cozmo_data = 'generate-maya-cozmo-data'
__current_version = 'current-version'

# Parse input
def __parse_input_args(argv):

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

    # Return the current asset version
    subparsers.add_parser(__current_version, help='Get the current sound bank version')

    options = parser.parse_args(argv)

    return options


# MAIN!
def main(argv):

    logging.basicConfig(level=logging.INFO)

    # Verify Paths
    if not path.exists(__wwiseToAppMetadataScript):
        logging.error(__errorMsg.format(__wwiseToAppMetadataScript))
        return os.EX_SOFTWARE

    if not path.exists(__audioCladDir):
        logging.error(__errorMsg.format(__audioCladDir))
        return os.EX_SOFTWARE

    if not path.exists(__depsFilePath):
        logging.error(__errorMsg.format(__depsFilePath))
        return os.EX_SOFTWARE

    if not path.exists(__externalsDir):
        logging.error(__errorMsg.format(__externalsDir))
        return os.EX_SOFTWARE

    # Parse input
    options = __parse_input_args(argv)

    # Perform command
    if __update_command == options.commands:
        if not __updateSoundbanks(options.version, options.metadata_merge):
            return os.EX_SOFTWARE

    elif __generate_command == options.commands:
        __generateProjectFiles()

    elif __update_alt_workspace == options.commands:
        if not __updateAltWorkspace(options.soundBankDir, options.metadata_merge):
            return os.EX_SOFTWARE

    elif __generate_maya_cozmo_data == options.commands:
        __generateMayaCozmoData(options.outputFilePath, options.groups)

    elif __current_version == options.commands:
        if not __getSoundBankVersion():
            return os.EX_SOFTWARE

    return os.EX_OK


def __checkSoundBankDepsPath(deps_json):
    abortMsg = 'Can not update Soundbank Version!!!!!! Can\'t Find'
    if __svn_key not in deps_json:
        logging.error('{} {}'.format(abortMsg, __svn_key))
        return False

    if __repo_names_key not in deps_json[__svn_key]:
        logging.error('{} {}'.format(abortMsg, __repo_names_key))
        return False

    if __soundbanks_key not in deps_json[__svn_key][__repo_names_key]:
        logging.error('{} {}'.format(abortMsg, __soundbanks_key))
        return False

    if __version_key not in deps_json[__svn_key][__repo_names_key][__soundbanks_key]:
        logging.error('{} {}'.format(abortMsg, __version_key))
        return False
    return True


def __getSoundBankVersion():
    with open(__depsFilePath) as deps_file:
            deps_json = json.load(deps_file)
            if not __checkSoundBankDepsPath(deps_json):
                return False
            version = deps_json[__svn_key][__repo_names_key][__soundbanks_key][__version_key]
            print ('version: {}'.format(version))
    return True



def __updateSoundbanks(version, mergeMetadataPath):
    # Update sound bank version in DEPS
    deps_json = None
    if version != '-':
        with open(__depsFilePath) as deps_file:
            deps_json = json.load(deps_file)

            logging.info('Update Soundbanks Version')

            if not __checkSoundBankDepsPath(deps_json):
                return False

        # Set soundbank version value
        deps_json[__svn_key][__repo_names_key][__soundbanks_key][__version_key] = version
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
    if not path.exists(__wwiseIdsFilePath):
        logging.error(__errorMsg.format(__wwiseIdsFilePath))
        return False

    # Update Audio Metadata.csv
    updateMetadataCmd = [__wwiseToAppMetadataScript, 'metadata', __wwiseIdsFilePath, __audioMetadataFilePath, '-m', previousMetadataFilePath]
    logging.debug("Running: {}".format(' '.join(updateMetadataCmd)))
    subprocess.call(updateMetadataCmd)
    logging.info('Metadata CSV has been updated and is ready for manual updates, file is located at: \'{}\''.format(path.realpath(__audioMetadataFilePath)))

    return True


def __generateProjectFiles():
    # Update Project Clad files
    generateCladCmd = [__wwiseToAppMetadataScript, 'clad', __wwiseIdsFilePath, __audioMetadataFilePath, __audioCladDir] + __namespaceList
    logging.debug("Running: {}".format(' '.join(generateCladCmd)))
    subprocess.call(generateCladCmd)
    logging.info('Project has been updated')


def __updateAltWorkspace(soundBankDir, mergeMetaFilePath):

    previousMetaPath = None

    # Check if sound bank dir exist
    if not path.exists(soundBankDir):
        logging.info('Sound Bank directory \'{}\' DOES NOT exist!'.format(soundBankDir))
        return False

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

    return True


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



if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
