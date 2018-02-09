#!/usr/bin/env python3

# Copyright (c) 2017 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Create collapsed feature-project json files for each language

Reads all pretty-printed featured project json, and creates one
featured project json per language with encoded project strings.
'''

import argparse
import copy
import json
import os

def parse_command_args():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-s', '--streaming_assets_path',
                            dest='streaming_assets_path',
                            type=str,
                            default='../../unity/Cozmo/Assets/StreamingAssets/',
                            action='store',
                            help='Specify the root path of the StreamingAssets')

    options = arg_parser.parse_args()
    return options

def generate_json_for_all_projects_by_language(language, project_metadata_map, project_json_list):

    output_data = []
    for project_json in project_json_list:
        uuid = project_json['ProjectUUID']
        active_project = copy.deepcopy(project_metadata_map[uuid])


        if not active_project:
            raise "Could not find project with uuid " + uuid

        project_content = json.dumps(project_json['ProjectJSON'], ensure_ascii=False, separators=(',',':'))
        project_content = project_content.replace("\\\"", "\"").replace("\\\\u", "\\u")
        if project_content.startswith('"') and project_content.endswith('"'):
            project_content = project_content[1:-1]
        active_project['ProjectJSON'] = project_content

        output_data.append(active_project)
    return output_data

def generate_project_metadata_map(project_manifest_json):
    result = {}
    for project in project_manifest_json:
        project = copy.deepcopy(project)
        #print( str(project) )
        del project['ProjectJSONFile']
        result[project['ProjectUUID']] = project
    return result

def get_all_featured_projects_by_language(language, project_folder):
    result = []
    for filename in os.listdir(project_folder):
        filename_lower = filename.lower()
        #print( 'PROJECT! ' + filename )
        if not filename.startswith('.') and language.lower() in filename_lower and filename_lower.endswith('.json'):
            target_file = os.path.join(project_folder, filename)
            #print( target_file )
            with open(target_file, 'r', encoding='utf8') as file_data:
                json_data = json.load(file_data)
                result.append(json_data)
    return result

def write_file_from_json(filename, data):
    with open(filename, 'w', encoding='utf8') as output_file:
        json.dump(data, output_file, indent=4, sort_keys=True)
        output_file.write('\n')

def main():
    # get folders
    command_args = parse_command_args()

    localization_root_folder=os.path.join(command_args.streaming_assets_path, 'Scratch', "LocalizedStrings")
    featured_project_json=os.path.join(command_args.streaming_assets_path, 'Scratch', "featured-projects.json")
    featured_project_folder=os.path.join(command_args.streaming_assets_path, 'Scratch', "featuredProjects")
    output_project_folder=os.path.join(command_args.streaming_assets_path, 'Scratch', "encodedFeaturedProjects")

    # Get a list of source languages
    languages = []
    for language in os.listdir(localization_root_folder):
        if '.' not in language:
            languages.append(language.lower())

    # Read the source featured project json config file
    project_manifest_json=[]
    with open(featured_project_json, 'r', encoding='utf8') as json_data:
        project_manifest_json = json.load(json_data)

    # Generate target folder if necessary
    if not os.path.exists(output_project_folder):
        os.makedirs(output_project_folder)

    # export project file for each language
    for language in languages:
        project_metadata_map = generate_project_metadata_map(project_manifest_json)
        project_json_list = get_all_featured_projects_by_language(language, featured_project_folder)

        compiled_json_for_language = generate_json_for_all_projects_by_language(language, project_metadata_map, project_json_list)

        target_file_name = os.path.join(output_project_folder, 'featured-projects_' + language + '.json')
        write_file_from_json( target_file_name, compiled_json_for_language )

if __name__ == "__main__": 
    main()
