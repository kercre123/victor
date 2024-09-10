#!/usr/bin/env python2

# Author: Jordan Rivas
# Date:   04/03/17


import argparse
import fnmatch
import json
import logging
import os
import shutil
import sys
import textwrap
import zipfile


#set up default logger
Logger = logging.getLogger('organize_soundbank_products')
stdout_handler = logging.StreamHandler()
formatter = logging.Formatter('%(name)s - %(message)s')
stdout_handler.setFormatter(formatter)
Logger.addHandler(stdout_handler)

BUNDLE_INFO_FILENAME = 'SoundbankBundleInfo.json'


# Return list of files that match pattern
def find_with_pattern(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result


# Find assets for soundbanks in list and add them to the asset map
# Example: {'bankname' : ['bankname_English.zip', 'bankname_German.zip']}
# Return True if all soundbanks have assets
def find_soundbank_in_list(source_dir, soundbank_list, out_asset_map):
    SOUNDBANK_NAME_KEY = 'name'
    missing_asset = False
    # Open soundbank_list json file
    with open(soundbank_list) as json_file:
        jsonData = json.load(json_file)

        for soundbank_obj in jsonData:
            soundbank_name = soundbank_obj[SOUNDBANK_NAME_KEY]
            results = []
            # SFX bank
            results = find_with_pattern(soundbank_name + '.zip', source_dir)
            # VO Local banks
            if len(results) == 0:
                results = find_with_pattern(soundbank_name + '_*.zip', source_dir)

            out_asset_map[soundbank_name] = results
            logging.debug('{0} : {1}'.format(soundbank_name, results))

            if len(results) == 0:
                missing_asset = True
                logging.warning('Missing Soundbank assets \'{0}\''.format(soundbank_name))

    return not missing_asset


# Copy assets to destination dir
def copy_assets(soundbank_map, dest_dir):
    for key in soundbank_map:
        asset_list = soundbank_map[key]
        for an_asset in asset_list:
            # Create paths
            path, file_name = os.path.split(an_asset)
            shutil.copy2(an_asset, os.path.join(dest_dir, file_name))


# Create copy of bundle info JSON file that only contains info for banks in assets map
def copy_stripped_bundle_info(soundbank_list_file, source_dir, dest_dir):
    # bank-list.json keys
    BANK_LIST_NAME_KEY = 'name'
    BANK_LIST_TYPE_KEY = 'type'
    BUNDLE_INFO_SOUNDBANK_NAME_KEY = 'soundbank_name'

    # Open bank-list.json and create map of soundbanks
    bank_list_map = {}
    with open(soundbank_list_file) as json_data:
        bank_list_data = json.load(json_data)

        for obj in bank_list_data:
            bank_list_map[obj[BANK_LIST_NAME_KEY]] = obj

    # Open SoundbankBundleInfo.json to copy info into new info file
    new_bundle_info = []
    path = os.path.join(source_dir, BUNDLE_INFO_FILENAME)
    with open(path) as json_data:
        bundle_info_json = json.load(json_data)
        # Loop through soundbank list and to copy bundle info if bank is in bank_list_map
        for a_bundle_info in bundle_info_json:
            bank_name = a_bundle_info[BUNDLE_INFO_SOUNDBANK_NAME_KEY ]
            if bank_name in bank_list_map:
                # Extend obj with type from sound bank list
                if BANK_LIST_TYPE_KEY in bank_list_map[bank_name]:
                    a_bundle_info[BANK_LIST_TYPE_KEY] = bank_list_map[bank_name][BANK_LIST_TYPE_KEY]
                # Add object to new bundle info
                new_bundle_info.append(a_bundle_info)

    # Write new SoundbankBundleInfo.json file which only contains info from soundbanks in bank-list.json
    dest_bundle_info_path = os.path.join(dest_dir, BUNDLE_INFO_FILENAME)
    with open(dest_bundle_info_path , 'w') as json_file:
        json.dump(new_bundle_info, json_file, sort_keys=True, ensure_ascii=False)


# For some platforms we want the loose files rather then zip files
def unzip_soundbank_bundles(dest_dir):
    # Find all zip files
    for (dir_path, dir_names, file_names) in os.walk(dest_dir):
        all_files = map(lambda x: os.path.join(dir_path, x), file_names)
        zip_files = [a_file for a_file in all_files if a_file.endswith('.zip')]
        # Unzip everything in place
        for a_zip in zip_files:
            zip_ref = zipfile.ZipFile(a_zip, 'r')
            zip_ref.extractall(dest_dir)
            zip_ref.close()
            os.remove(a_zip)


# parse command arguments
def parse_args(argv=[], print_usage=False):
    # version = '1.0'
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description='Organize soundbank zip files for a project',
        epilog=textwrap.dedent('''
          options description:
            [path/to/sounds] [path/for/output]
      '''),
        # version=version
    )

    parser.add_argument('--debug', '-d', '--verbose', dest='debug', action='store_true',
                        help='Print debug output')
    parser.add_argument('--allow-missing', dest='allow_missing', action='store_true',
                        help='do not quit for a missing file')
    parser.add_argument('--meta-only', dest='meta_only', action='store_true',
                        help='Only generate soundbank bundle metadata file' )
    parser.add_argument('--unzip-bundles', dest='unzip_bundles', action='store_true',
                        help='Unzip soundbank bundles')
    parser.add_argument('source_dir', action='store',
                        help='Path to generated soundbank directory')
    parser.add_argument('output_dir', action='store',
                        help='Directory to store output in')
    parser.add_argument('soundbank_list', action='store',
                        help='Path to json file with a list of soundbanks')


    if print_usage:
        parser.print_help()
        sys.exit(2)

    args = parser.parse_args(argv)

    if (args.debug):
        Logger.setLevel(logging.DEBUG)
        Logger.debug(args)
    else:
        Logger.setLevel(logging.INFO)

    return args


def run(args):
    source_dir = args.source_dir
    output_dir = args.output_dir
    soundbank_list = args.soundbank_list

    # Check argument paths
    # Input Dir
    if not os.path.isdir(source_dir):
        Logger.error('source_dir \'{0}\' does NOT exist'.format(source_dir))
        return 2


    # Output Dir
    if not os.path.isdir(output_dir):
        logging.info("Create directory \'{0}\'".format(output_dir))
        os.makedirs(output_dir)

    # Soundbank list
    if not os.path.exists(soundbank_list):
        Logger.error('soundbank_list \'{0}\' does NOT exist'.format(soundbank_list))
        return 2

    # Find and Copy assets in soundbank metadata list
    if not args.meta_only:
      assets_map = {}
      success = find_soundbank_in_list(source_dir, soundbank_list, assets_map)
      if not success and not args.allow_missing:
        return 2
        
      copy_assets(assets_map, output_dir)

    # Create soundbank metadata file for specified soundbank list
    copy_stripped_bundle_info(soundbank_list, source_dir, output_dir)

    if args.unzip_bundles:
        unzip_soundbank_bundles(output_dir)

    # Success!
    return 0


def main(args):
    args = parse_args(args)
    return run(args)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
