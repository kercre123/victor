#!/usr/bin/python

"""
Author: Jordan Rivas
Date:   12/06/17

Description: This script allows sound designers to use locally generated sound banks in local Victor project builds.
             Audio build server scripts are used to generate metadata and package assets similar to how they are
             delivered by Victor's package manager process. This script will simply replace the assets in the
             EXTERNALS/victor-audio-assets directory. After replacing the audio assets build-victor.sh must be ran to
             copy the assets to the projects deliverables. Likewise, you must run deploy-assets.py to copy new assets
             to robot. For convenience this script has arguments to update audio CLAD files, build victor and deploy
             changes.
"""

import argparse
import logging
import os
import shutil
import subprocess
import sys
import textwrap
from os import path

import UpdateAudioAssets


#set up default logger
Logger = logging.getLogger('UseLocalAudioAssets')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

__script_dir = path.dirname(path.realpath(__file__))
__project_root_path = path.join(__script_dir, '..', '..')
__audio_lib_path = path.join(__project_root_path, 'lib', 'audio')
__audio_build_script_path = path.join(__audio_lib_path, 'build-scripts')
__victor_external_path = path.join(__project_root_path, 'EXTERNALS', 'local-audio-assets')


# Import Audio Build Scripts
sys.path.append(__audio_build_script_path)
import bundle_metadata_products
import bundle_soundbank_products
import organize_soundbank_products


# Victor project defines
__audio_project_name = 'VictorAudio'
__sound_bank_dir_name = 'GeneratedSoundBanks'
__audio_metadata_dir_name = 'metadata'
__victor_bank_list_file_name = 'victor-banks-list.json'



def main(args):
    # Check paths
    # Wwise project
    if not path.isdir(args.audio_project_repo_dir):
        Logger.error('Audio project does NOT exist: \'{0}\''.format(args.audio_project_repo_dir))
    else:
        Logger.debug('Audio project found: \'{0}\''.format(args.audio_project_repo_dir))

    # Sound bank dir
    sound_bank_path = path.join(args.audio_project_repo_dir, __audio_project_name, __sound_bank_dir_name)
    if not path.isdir(sound_bank_path):
        Logger.error('Sound Bank dir does NOT exist: \'{0}\''.format(sound_bank_path))
    else:
        Logger.debug('Sound Bank dir found: \'{0}\''.format(sound_bank_path))

    # Bank list file path
    bank_list_filepath = path.join(args.audio_project_repo_dir, __audio_metadata_dir_name, __victor_bank_list_file_name)
    if not path.exists(bank_list_filepath):
        Logger.error('Victor robot sound bank list does NOT exist: \'{0}\''.format(bank_list_filepath))
    else:
        Logger.debug('Victor robot sound bank list found: \'{0}\''.format(bank_list_filepath))

    #  Remove old Victor External Assets
    if path.isdir(__victor_external_path):
        Logger.info('Delete old audio assets: \'{0}\''.format(__victor_external_path))
        shutil.rmtree(__victor_external_path)

    # Create Directory structure
    tmp_dir = path.join(__victor_external_path, 'tmp')
    tmp_dev_mac_dir = path.join(tmp_dir, 'dev_mac')
    tmp_linux_dir = path.join(tmp_dir, 'victor_linux')
    dest_metadata_dir = path.join(__victor_external_path, 'metadata')
    dest_dev_mac_dir = path.join(__victor_external_path, 'victor_robot', 'dev_mac')
    dest_linux_dir = path.join(__victor_external_path, 'victor_robot', 'victor_linux')
    os.makedirs(tmp_dev_mac_dir)
    os.makedirs(tmp_linux_dir)
    os.makedirs(dest_metadata_dir)
    os.makedirs(dest_dev_mac_dir)
    os.makedirs(dest_linux_dir)
    Logger.info('Complete: Make asset directories')

    # Perform build steps
    # Copy meta data
    bundle_metadata_products.main([sound_bank_path, dest_metadata_dir])
    Logger.info('Complete: bundle_metedata_products')

    # Create sound bank metadata and zip assets together
    script_commands = []
    if args.debug:
        script_commands.append('--debug')

    if args.allow_missing:
        script_commands.append('--allow-missing')

    # Linux
    # Victor Robot
    bundle_soundbank_products.main([path.join(sound_bank_path, 'Victor_Linux'), tmp_linux_dir] + script_commands)
    # Dev Mac - Webots
    bundle_soundbank_products.main([path.join(sound_bank_path, 'Dev_Mac'), tmp_dev_mac_dir] + script_commands)
    Logger.info('Complete: bundle_soundbank_products')

    # Organize Sound Banks in project
    organize_soundbank_products.main([tmp_linux_dir, dest_linux_dir, bank_list_filepath] + script_commands)
    organize_soundbank_products.main([tmp_dev_mac_dir, dest_dev_mac_dir, bank_list_filepath] + script_commands)
    Logger.info('Complete: organize_soundbank_products')


    # Remove tmp dir
    if path.isdir(tmp_dir):
        Logger.debug('Remove Tmp Dir: \'{0}\''.format(tmp_dir))
        shutil.rmtree(tmp_dir)


    # Update & generate audio CLAD
    if args.update_clad:
        Logger.info('Updating audio event list and CLAD files. Do NOT check in updated audio files!!!')

        # Update audio event list
        result = UpdateAudioAssets.main(['update', '-'])
        if result != os.EX_OK:
            Logger.error('\'{0}\' Failed'.format('UpdateAudioAssets.py update'))
            return os.EX_SOFTWARE
        # Generate audio CLAD
        result = UpdateAudioAssets.main(['generate'])
        if result != os.EX_OK:
            Logger.error('\'{0}\' Failed'.format('UpdateAudioAssets.py generate'))
            return os.EX_SOFTWARE


    # Build & Deploy
    if args.build_deploy:
        # Force build project
        Logger.info('Run build-victor.sh')
        victor_project_dir = path.join('project', 'victor')
        victor_build_script = path.join(victor_project_dir, 'build-victor.sh')
        build_commands = [victor_build_script, '-f', '-a', '-DUSE_LOCAL_AUDIO_ASSETS=ON']
        is_mac_build = False;
        if args.configuration:
            build_commands.append('-c')
            build_commands.append(args.configuration)
        if args.platform:
            build_commands.append('-p')
            build_commands.append(args.platform)
            if args.platform.lower() == 'mac':
                is_mac_build = True

        result = subprocess.call(build_commands, cwd=__project_root_path)
        if result != os.EX_OK:
            Logger.error('\'{0}\' Failed'.format(victor_build_script))
            return os.EX_SOFTWARE

        if not is_mac_build:
            # Deploy bins
            victor_project_script_dir = path.join(victor_project_dir, 'scripts')
            Logger.info('Stop victor process')
            victor_stop_script = path.join(victor_project_script_dir, 'victor_stop.sh')
            result = subprocess.call([victor_stop_script], cwd=__project_root_path)
            if result != os.EX_OK:
                Logger.error('\'{0}\' Failed'.format(victor_stop_script))
                return os.EX_SOFTWARE

            Logger.info('Run victor_deploy.sh')
            victor_deploy_script = path.join(victor_project_script_dir, 'victor_deploy.sh')
            build_commands = [victor_deploy_script]
            if args.configuration:
                build_commands.append('-c')
                build_commands.append(args.configuration)

            result = subprocess.call(build_commands, cwd=__project_root_path)
            if result != os.EX_OK:
                Logger.error('\'{0}\' Failed'.format(build_commands))
                return os.EX_SOFTWARE

    # Success!
    return os.EX_OK


def parse_args(argv=[], print_usage=False):

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='Use locally generated sound banks in Victor build',
        epilog=textwrap.dedent('''
          options description:
            [path/to/audio_project_repo]
      '''),
    )

    parser.add_argument('audio_project_repo_dir', action='store',
                        help='Root director to audio project repo')

    parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                        help='Print debug output')
    parser.add_argument('--allow-missing', dest='allow_missing', action='store_true',
                        help='Do NOT quit for a missing file')

    parser.add_argument('--update-clad', '-u', dest='update_clad', action='store_true',
                        help='Update audio event list and generate audio CLAD files')
    # Build & Deploy with options
    parser.add_argument('--build-deploy', '-b', dest='build_deploy', action='store_true',
                        help='Force Build Victor and deploy updated assets')
    parser.add_argument('--configuration', '-c', dest='configuration', metavar='configuration',
                        help='Build configuration {Debug,Release}')
    parser.add_argument('--platform', '-p', dest='platform', metavar='platform',
                        help='Build target platform {android,mac}')

    if print_usage:
        parser.print_help()
        sys.exit(os.EX_NOINPUT)

    args = parser.parse_args(argv)

    if (args.debug):
        Logger.setLevel(logging.DEBUG)
        Logger.debug(args)
    else:
        Logger.setLevel(logging.INFO)

    return args



if __name__ == '__main__':
    args = parse_args(sys.argv[1:])
    sys.exit(main(args))
