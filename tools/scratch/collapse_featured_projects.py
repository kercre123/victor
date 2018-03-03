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

'''Create collapsed feature-project json files

Reads each pretty-printed featured project json and collapses each one
to a new one where the serialized project json is a single line of encoded data.
'''

import argparse
import json
import os
import shutil

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

def main():
    command_args = parse_command_args()

    featured_project_folder=os.path.join(command_args.streaming_assets_path, 'Scratch', "featuredProjects")
    output_project_folder=os.path.join(command_args.streaming_assets_path, 'Scratch', "encodedFeaturedProjects")

    # Delete target folder to remove old contents
    if os.path.exists(output_project_folder):
        shutil.rmtree(output_project_folder)

    # Generate target folder 
    os.makedirs(output_project_folder)

    # For each featured project pretty-printed file, create a new file of the same name with the same data, but replace
    # the pretty-printed data for key 'ProjectJSON' with non-pretty-printed data.
    for filename in os.listdir(featured_project_folder):
        filename_lower = filename.lower()
        if filename_lower.endswith('.json'):
            source_file = os.path.join(featured_project_folder, filename_lower)

            with open(source_file, 'r', encoding='utf8') as file_data:                
                json_data = json.load(file_data)
                for key, value in json_data.items():
                    project_content = json.dumps(json_data['ProjectJSON'], ensure_ascii=False, separators=(',',':'))
                    project_content = project_content.replace("\\\"", "\"").replace("\\\\u", "\\u")
                    if project_content.startswith('"') and project_content.endswith('"'):
                        project_content = project_content[1:-1]
                    json_data['ProjectJSON'] = project_content

                    # Create new json file in output folder with same file name.
                    dest_file = os.path.join(output_project_folder, filename_lower)
                    with open(dest_file, 'w', encoding='utf8') as output_file:                    
                        json.dump(json_data, output_file, indent=4, sort_keys=True)
                        output_file.write('\n')

if __name__ == "__main__": 
    main()
