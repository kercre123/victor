#!/usr/bin/env python3

'''Convert between ".codelab" files and the underlying JSON.

Convert / unpack a ".codelab" file into 2 JSON files (one
metadata, one for the project contents), where the contents
can more easily be viewed and edited by hand. These can
then be re-packed back into a ".codelab" file.
'''

import argparse
import codecs
import json
import logging
import os
import urllib.parse


CODELAB_FILE_HEADER = "CODELAB:"
PROJECT_DATA_FILENAME = "_project_data.json"
PROJECT_JSON_FILENAME = "_project_json.json"


def parse_command_args():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-unpack',
                        dest='file_to_unpack',
                        default=None,
                        action='store',
                        help='The single .codelab file to unpack')
    arg_parser.add_argument('-output_directory',
                            dest='output_directory',
                            default=".",
                            action='store',
                            help='The directory to store any output files.')
    arg_parser.add_argument('-pack',
                        dest='file_to_pack',
                        default=None,
                        action='store',
                        help='The unpacked project to re-pack back to a .codelab file')
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


def get_file_contents(file_path):
    file_encoding = "utf8"
    file_bom = codecs.BOM_UTF8.decode()

    logging.debug("get_file_contents('%s') encoding=%s", file_path, file_encoding)

    with open(file_path, encoding=file_encoding) as data_file:
        file_contents = data_file.read()

        # Files can include an optional Byte Order Marker (which behaves as a zero
        # width character when printed), at the start - remove it if present
        if file_contents.startswith(file_bom):
            logging.debug("Removing BOM at start of File '%s'", file_path)
            file_contents = file_contents[len(file_bom):]

        return file_contents


def extract_field_from_json(json_contents, key_name, file_path):
    try:
        json_field = json_contents[key_name]
        return json_field
    except KeyError:
        logging.error("JSON file is missing %s entry: File='%s'", key_name, file_path)
        return None


def unpack_codelab_file(file_path, output_directory):
    logging.debug("unpack_codelab_file('%s')", file_path)
    file_contents = get_file_contents(file_path)

    unencoded_contents = urllib.parse.unquote_plus(file_contents)

    # Check the header
    if not unencoded_contents.startswith(CODELAB_FILE_HEADER):
        logging.error("Invalid Header: File='%s' doesn't start with '%s'",
                      file_path, CODELAB_FILE_HEADER)
        return

    # Skip past the header to the contents
    unencoded_contents = unencoded_contents[len(CODELAB_FILE_HEADER):]

    # Decode the contents to JSON
    try:
        json_contents = json.loads(unencoded_contents)
    except json.decoder.JSONDecodeError as e:
        logging.error("Invalid JSON: File='%s' error='%s'", file_path, e)
        return

    # Grab key fields (and make sure they're present)
    project_name = extract_field_from_json(json_contents, "ProjectName", file_path)
    project_json = extract_field_from_json(json_contents, "ProjectJSON", file_path)
    project_uuid = extract_field_from_json(json_contents, "ProjectUUID", file_path)
    if (project_name is None) or (project_json is None) or (project_uuid is None):
        return

    # Convert the ProjectJSON field to JSON (from the encoded string)
    project_json = json.loads(project_json)

    # And remove it from the data so that's it's not
    json_contents["ProjectJSON"] = "See " + PROJECT_JSON_FILENAME

    # Make sure the output directory exists
    try:
        os.makedirs(output_directory)
    except FileExistsError:
        pass  # directory already exists - this is fine

    # Create the base output filename
    # Replace spaces and slashes to avoid confusing the OS / filesystem
    output_filename = project_name
    output_filename = output_filename.replace(" ", "_")
    output_filename = output_filename.replace("/", "_")

    project_data_path_name = os.path.join(output_directory, output_filename + PROJECT_DATA_FILENAME)
    with open(project_data_path_name, 'w') as out_file:
        json.dump(json_contents, out_file, indent=4)

    project_json_path_name = os.path.join(output_directory, output_filename + PROJECT_JSON_FILENAME)
    with open(project_json_path_name, 'w') as out_file:
        json.dump(project_json, out_file, indent=4)


def pack_codelab_file(file_path, output_directory):
    logging.debug("pack_codelab_file('%s', '%s')", file_path)

    if PROJECT_DATA_FILENAME in file_path:
        file_path_project_data = file_path
        file_path_project_json = file_path.replace(PROJECT_DATA_FILENAME, PROJECT_JSON_FILENAME)
    elif PROJECT_JSON_FILENAME in file_path:
        file_path_project_json = file_path
        file_path_project_data = file_path.replace(PROJECT_JSON_FILENAME, PROJECT_DATA_FILENAME)
    else:
        logger.error("Unexpected filename %s - expected to end in %s or %s",
                     file_path, PROJECT_DATA_FILENAME, PROJECT_JSON_FILENAME)
        return

    logging.debug("pack_codelab_file('%s', '%s')", file_path_project_data, file_path_project_json)
    project_data_contents = get_file_contents(file_path_project_data)
    project_json_contents = get_file_contents(file_path_project_json)

    # Load the contents to JSON
    try:
        json_contents = json.loads(project_data_contents)
    except json.decoder.JSONDecodeError as e:
        logging.error("Invalid JSON: File='%s' error='%s'", file_path_project_data, e)
        return

    try:
        project_json_contents = json.loads(project_json_contents)
    except json.decoder.JSONDecodeError as e:
        logging.error("Invalid JSON: File='%s' error='%s'", file_path_project_json, e)
        return

    # Grab key fields (and make sure they're present)
    project_name = extract_field_from_json(json_contents, "ProjectName", file_path)
    project_uuid = extract_field_from_json(json_contents, "ProjectUUID", file_path)
    if (project_name is None) or (project_uuid is None):
        return

    # Put ProjectJSON, as a string, in the ProjectJSON of the main contents
    json_contents["ProjectJSON"] = json.dumps(project_json_contents)

    # Make sure the output directory exists
    try:
        os.makedirs(output_directory)
    except FileExistsError:
        pass  # directory already exists - this is fine

    # Create the base output filename
    # Replace spaces and slashes to avoid confusing the OS / filesystem
    project_name = project_name.replace(" ", "_")
    project_name = project_name.replace("/", "_")
    project_extension = ".codelab"

    # find a new unique output filename (to avoid clobbering an existing file)
    output_filename = os.path.join(output_directory, project_name + project_extension)
    suffix = 1
    while (os.path.exists(output_filename)):
        output_filename = os.path.join(output_directory, project_name + "_" + str(suffix) + project_extension)
        suffix += 1

    logging.debug("Exporting project '%s' UUID %s to %s", project_name, project_uuid, output_filename)

    encoded_contents = urllib.parse.quote_plus(json.dumps(json_contents))
    with open(output_filename, 'w') as out_file:
        out_file.write(CODELAB_FILE_HEADER)
        out_file.write(encoded_contents)


if __name__ == '__main__':
    command_args = parse_command_args()

    # Set the log level requested
    logger = logging.getLogger()
    logger.setLevel(command_args.log_level)

    if command_args.file_to_unpack is not None:
        unpack_codelab_file(command_args.file_to_unpack, command_args.output_directory)

    if command_args.file_to_pack is not None:
        pack_codelab_file(command_args.file_to_pack, command_args.output_directory)

