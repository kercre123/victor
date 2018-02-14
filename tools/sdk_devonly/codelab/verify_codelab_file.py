#!/usr/bin/env python3

'''Verify any ".codelab" files to test if they're valid encoded JSON files.

To be used to whitelist submitted CodeLab files, to ensure they're valid
and not a virus or other malicious file.
'''

import argparse
import codecs
import json
import logging
import os
import sys
import urllib.parse


CODELAB_FILE_HEADER = "CODELAB:"

# Set the log level requested
logger = logging.getLogger('codelab.verify_file')

def parse_command_args():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-tf', '--test_file',
                        dest='test_file',
                        default=None,
                        action='store',
                        help='The single .codelab file to test')
    arg_parser.add_argument('-td', '--test_directory',
                        dest='test_directory',
                        default=None,
                        action='store',
                        help='The directory to test all .codelab files in')
    arg_parser.add_argument('-l', '--log_level',
                        dest='log_level',
                        default='WARNING',
                        action='store',
                        help='The logging level (e.g. "CRITICAL", "ERROR",'
                             '"WARNING", "INFO", "DEBUG")')

    options = arg_parser.parse_args()
    return options


def json_bool_to_python_bool(js_bool):
    # Handle various string representations for booleans from JSON
    js_bool_str = str(js_bool).lower()
    if js_bool_str == "true":
        return True
    elif js_bool_str == "false":
        return False
    else:
        logging.warning("Unexpected js_bool value == '%s'" % repr(js_bool))
        return False


def is_valid_codelab_file(file_path):
    try:
        file_encoding = "utf8"
        file_bom = codecs.BOM_UTF8.decode()

        logging.debug("Testing File '%s' encoding=%s", file_path, file_encoding)

        with open(file_path, encoding=file_encoding) as data_file:
            try:
                file_contents = data_file.read()

                # Files can include an optional Byte Order Marker (which behaves as a zero
                # width character when printed), at the start - remove it if present
                if file_contents.startswith(file_bom):
                    logging.debug("Removing BOM at start of File '%s'", file_path)
                    file_contents = file_contents[len(file_bom):]

                unencoded_contents = urllib.parse.unquote_plus(file_contents)

                # Check the header
                if not unencoded_contents.startswith(CODELAB_FILE_HEADER):
                    logger.error("Invalid Header: File='%s' doesn't start with '%s'",
                                  file_path, CODELAB_FILE_HEADER)
                    return False

                # Skip past the header to the contents
                unencoded_contents = unencoded_contents[len(CODELAB_FILE_HEADER):]

                # Decode the contents to JSON
                try:
                    json_contents = json.loads(unencoded_contents)
                except json.decoder.JSONDecodeError as e:
                    logger.error("Invalid JSON: File='%s' error='%s'", file_path, e)
                    return False

                # Verify critical fields are valid / present
                try:
                    project_data = json_contents["ProjectJSON"]
                except KeyError:
                    logger.error("Missing ProjectJSON entry: File='%s'", file_path)
                    return False

                try:
                    is_vertical = json_contents["IsVertical"]
                except KeyError:
                    logger.error("Missing IsVertical entry: File='%s'", file_path)
                    return False
                # We don't accept horizontal submissions - the grammar is too simple to make
                # them worthwhile featuring.
                is_vertical = json_bool_to_python_bool(is_vertical)
                if not is_vertical:
                    logger.error("Horizontal project - ignore: File='%s'", file_path)
                    return False

                logger.debug("File '%s' size=%s is a Valid codelab file",
                              file_path, len(unencoded_contents))
                return True
            except Exception as e:
                logger.error("Unhandled Error loading: File='%s' error='%s'", file_path, e)
                return False
    except Exception as e:
        logger.error("Unhandled Error opening: File='%s' error='%s'", file_path, e)
        return False


def test_all_files_in_directory(directory_name):
    num_valid = 0
    num_invalid = 0
    for root, dirs, files in os.walk(directory_name):
        for filename in files:
            if filename.lower().endswith(".codelab"):
                full_path = os.path.join(root, filename)
                is_valid = is_valid_codelab_file(full_path)
                logger.debug("Test file '%s' valid=%s", filename, is_valid)
                if is_valid:
                    num_valid += 1
                else:
                    num_invalid += 1

    logger.info("Tested %s files in '%s', %s valid, %s invalid",
                 (num_valid + num_invalid), directory_name, num_valid, num_invalid)

    if num_invalid > 0:
        sys.exit(1)


if __name__ == '__main__':
    command_args = parse_command_args()

    # Set the log level requested
    logger = logging.getLogger()
    logger.setLevel(command_args.log_level)

    # Test the requested file(s), return error code if not all are valid

    if command_args.test_directory is not None:
        test_all_files_in_directory(command_args.test_directory)
    elif command_args.test_file is not None:
        if is_valid_codelab_file(command_args.test_file):
            logger.info("Tested single file - it was valid")
        else:
            logger.info("Tested single file - it was INVALID")
            sys.exit(1)
