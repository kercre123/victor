#!/usr/bin/env python3

# Nicolas Kent 10-20-17
#

# This script is expected to be run in it's containing folder from the command line
#  command line parameters:
#    -p PROJECT_NAME    ... Specifies a project to be operated on by DASProjectName
#    -a                 ... Operates on all projects specified in codelab
#
#    -s                 ... Scans the specified project(s) and exports localizable strings into the local folder
#    -t                 ... Translates the specified projects
#    -l                 ... Lists projects to console
#
# example usages:
#    "./localize_featured_projects.py -al"  ... Lists all codelab featured projects
#    "./localize_featured_projects.py -s -p LaserSmile"  ... Scans laser smile project for strings
#    "./localize_featured_projects.py -t -p LaserSmile"  ... Translates the laser smile project
#

#
# This script will parse cozmo codelab's featured projects, and relocalize them into different languages
# the script will only act on projects that specify a Language field, others will be ignored
#
# 1) The source-language version is reverse-localized back to loc keys using a table of all localization keys in that language of translation:key
# 2) Versions of the project are generated for every other language found in the localization folder, localized with a table of that languages key:translations
# 3) The translated projects are inserted into the project list with modified language keys, next to the source projects, with all non-language, non-projectJSON fields unchanged
#
# The intent is that the cozmo app will then mask on locale for any featured project that specifies a language.
#

import argparse
import collections
import copy
import json
import os
from pprint import pprint
import random
import re
import sys
import zipfile

LOCALIZATION_ROOT_PATH = "LocalizedStrings"
LOCALIZATION_TARGET_FILES = [ "CodeLabStrings.json", "CodeLabFeaturedContentStrings.json" ]

# Plenty of string fields contain these values 
# but even if they were to be localization translations, it might break a lot ot reverse key them
IGNORE_TEXT_KEYS = ['', 'True', 'False', 'true', 'false']

# Certain keys should be treated as project agnostic so they are only translated once
# These are primarily default fields that can be buried under variables.
PROJECT_AGNOSTIC_KEYS = ['text', 'anki', 'cozmo']

def parse_command_args():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('-t', '--translate',
                            dest='translate_project',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Automatically generates foreign language projects from the english source')
    arg_parser.add_argument('-s', '--scan',
                            dest='scan_project',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Scans the json of a project and exports a list of localizable strings')
    arg_parser.add_argument('-l', '--list',
                            dest='list_project',
                            default=False,
                            action='store_const',
                            const=True,
                            help='lists all projects matching the filter')

    arg_parser.add_argument('--exclude_project_agnostic_strings',
                            dest='exclude_project_agnostic_strings',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Avoid exporing project agnostic strings during scan')
    arg_parser.add_argument('-a', '--all',
                            dest='all_projects',
                            default=False,
                            action='store_const',
                            const=True,
                            help='Applies over all featured projects')
    arg_parser.add_argument('-p', '--project',
                            dest='project',
                            default=None,
                            action='store',
                            help='Specifies featured project to be scanned or translated')
    arg_parser.add_argument('-b', '--blacklist',
                            dest='blacklist',
                            default=None,
                            action='store',
                            help='Specifies a json blacklist to exclude specific projects')

    arg_parser.add_argument('-o', '--source_language',
                            dest='source_language',
                            default='en-US',
                            action='store',
                            help='Specify the source language to be operated on')
    arg_parser.add_argument('--scan_loc_target',
                            dest='scan_loc_target',
                            default=None,
                            action='store',
                            help='Specify the target file to merge new localizable strings into')

    arg_parser.add_argument('--streaming_assets_path',
                            dest='streaming_assets_path',
                            type=str,
                            default='',
                            action='store',
                            help='Specify the root path of the StreamingAssets')
    arg_parser.add_argument('--project_config_file',
                            dest='project_config_file',
                            type=str,
                            default='',
                            action='store',
                            help='Specify the project configuration file')
    arg_parser.add_argument('--project_folder',
                            dest='project_folder',
                            type=str,
                            default='',
                            action='store',
                            help='Specify the project folder')
    arg_parser.add_argument('--scan_output_path',
                            dest='scan_output_path',
                            type=str,
                            default=None,
                            action='store',
                            help='Path to export project scans to')

    options = arg_parser.parse_args()
    return options

# --------------------------------------------------------------------------------------------------------
#                                                   Filters
# --------------------------------------------------------------------------------------------------------
# If any additional types of fields are identified that need localization, these filters should be able to
# isolate them on both transforms without having to touch any of the actual translation code.
# Since the filters only map fed in transforms onto nodes of a json tree, ther is nothing specific to translation,
# string extraction, etc, in these classes.

# TO ADD A NEW FILTER:
# 1) Create a new class
# 2) implement a new test_and_apply_transform function which will be invoked recursively on every node of the featured project's JSON
#    - src: the node being operated on
#    - context: current state of the traversal manager
#    - transform: function reference to be invoked when a match is found
# 3) Optionally create a augment_context_by_dict_key to allow specialized behavior under specific key headers in the json heirarchy by modifying context state
#    - key: the dictionary key being passed over
#    - context: the current context
#    returns a new context that will persist for all subnodes under this key
# 4) Add a generated instance from that class to the allFilters list

# variable names are the all represented in scratch using this json structure, inside of a dictionary named 'variables'
class FilterVariable:
    def augment_context_by_dict_key(self, src, key, context):
        if key == 'variables':
            context['scope'][-1] = 'parentedUnderVariables'

    def test_and_apply_transform(self, src, context, transform):
        if 'parentedUnderVariables' in context['scope'] and type(src) == dict and 'name' in src:
            string = src['name']
            try:
                if len(string) > 1 and string[0] == '.':
                    int(string[1:])
                else:
                    int(string)
            except ValueError:
                if string not in IGNORE_TEXT_KEYS:
                    src['name'] = transform(string, context)

# the vast majority of user editable strings are represented in scratch using this json structure
class FilterField:
    def augment_context_by_dict_key(self, src, key, context):
        pass

    def test_and_apply_transform(self, src, context, transform):
        for key in ['TEXT', 'VARIABLE']:
            if type(src) == dict and 'fields' in src and key in src['fields']:
                string = src['fields'][key]['value']
                try:
                    if len(string) > 1 and string[0] == '.':
                        float(string[1:])
                    else:
                        float(string)
                except ValueError:
                    if string not in context['textIdMap'] or context['textIdMap'][string] not in context['sdkAnimBlockIds']:
                        if string not in IGNORE_TEXT_KEYS:
                            src['fields'][key]['value'] = transform(string, context)
                        
class FilterSDKAnimation:
    def augment_context_by_dict_key(self, src, key, context):
        if 'sdkAnimBlockIds' not in context:
            context['sdkAnimBlockIds'] = []
        if 'textIdMap' not in context:
            context['textIdMap'] = {}

        if 'opcode' in src and 'inputs' in src and 'ANIM_NAME' in src['inputs']:
            if src['opcode'] == 'cozmo_play_animation_by_name' or src['opcode'] == 'cozmo_play_animation_by_triggername':
                valueText = src['inputs']['ANIM_NAME']['block']
                context['sdkAnimBlockIds'].append( valueText )

        if 'id' in src and 'opcode' in src and src['opcode'] == 'text' and 'fields' in src and 'TEXT' in src['fields']:
            idText = src['id']
            valueText = src['fields']['TEXT']['value']
            context['textIdMap'][valueText] = idText

    def test_and_apply_transform(self, src, context, transform):
        pass

allFilters = [FilterVariable(), FilterField(), FilterSDKAnimation()]

# --------------------------------------------------------------------------------------------------------
#                                               Translation Code
# --------------------------------------------------------------------------------------------------------
#
# These functions are concerned just with translating a particular project, isolated from any context 
# of how they are stored in the featured json
#

# Replace a string using the loc translation table in the supplied context
# NOTE: this function is not called directly, but fed into the following function, it could easily be replaced for other transformations
def translate(string, context):
    if string in context['loc']:
        # uncomment the print statement to get a verbose log of everything translated
        #print(string + ' -> ' + context['loc'][string] + '\n')
        return context['loc'][string]
    return string

# version of the translate function the checks a pre-lowercased loc mapping for lowercased versions of the
# target strings.  This will compensate for things like "points" and "Points" resolving to the same key
def translate_lower(string, context):
    lowercasedString = string.lower()
    if lowercasedString in context['loc']:
        # uncomment the print statement to get a verbose log of everything translated
        #print(string + ' -> ' + context['loc'][lowercasedString] + '\n')
        return context['loc'][lowercasedString]
    return lowercasedString

# Traverses a hierarchy of json objects/lists, using the supplied filters to identify where to apply the supplied transform
def walk_json_tree_and_perform_transformations(node, filters, context, transform):
    if type(node) == list:
        for subNode in node:
            walk_json_tree_and_perform_transformations( subNode, filters, context, transform )
    elif type(node) == dict:
        for key in node:
            context['scope'].append('none')
            for f in filters:
                f.augment_context_by_dict_key(node, key, context)
            walk_json_tree_and_perform_transformations( node[key], filters, context, transform )
            context['scope'] = context['scope'][:-1]
    for f in filters:
        f.test_and_apply_transform(node, context, transform)

# Returns a list of transformed projects for all languages specified in the locTable, for a given input project
def translate_project(project, sourceLanguageTable, destinationLanguageTable):
    resultList = []

    cloned_source_project = copy.deepcopy(project)

    projectJson = load_project_json(cloned_source_project['ProjectJSON'])

    #with open('testIn.txt', 'w') as output_file:
    #    json.dump(projectJson, output_file, indent=4)

    # encode the original project into localization keys, with matches lowercased
    encodingContext = { 'loc': {}, 'scope': ['none'] }#{ 'loc': sourceLanguageTable['encodings'] }
    for key in sourceLanguageTable['encodings']:
        encodingContext['loc'][key.lower()] = sourceLanguageTable['encodings'][key]
    walk_json_tree_and_perform_transformations(projectJson, allFilters, encodingContext, translate_lower)

    # decode the localization keys into the proper language
    decodingContext = { 'loc': destinationLanguageTable['decodings'], 'scope': ['none'] }
    walk_json_tree_and_perform_transformations(projectJson, allFilters, decodingContext, translate)

    newProject = {}
    newProject['ProjectUUID'] = cloned_source_project['ProjectUUID']
    newProject['ProjectJSON'] = projectJson
    #with open('testOut.txt', 'w') as output_file:
    #    json.dump(projectJson, output_file, indent=4)

    return newProject


def pull_string(string, context):
    if string not in context['outputList']:
        context['outputList'].append(string)

# --------------------------------------------------------------------------------------------------------
#                                           Translation Management
# --------------------------------------------------------------------------------------------------------
# 
# These functions identify which projects in the featured list to process, how to modify that list,
# where to import and export that list from and to, and where to pull in localization data

def get_file_path(base_file_name, language):
    return base_file_name + '_' + language.lower() + '.json'

def run_translation_target(target, loc_table):
    source_loc = loc_table[target['source_language']]
    target_loc = loc_table[target['target_language']]
    translated = translate_project(target['source_json'], source_loc, target_loc)
    with open(target['target_file_name'], 'w', encoding='utf8') as output_file:
        json.dump(translated, output_file, indent=4, sort_keys=True)
        output_file.write('\n')

    return True

def execute_translations_on_project(project, source_language, loc_table):
    project_json = {}
    source_project_file = get_file_path(project['base_file_name'], source_language)
    with open(source_project_file, encoding='utf8') as input_file:
        project_json = json.load(input_file)

    targets = []
    for language in loc_table:
        if language != source_language:
            target_project_file = get_file_path(project['base_file_name'], language)
            target_entry = {}
            target_entry['source_language'] = source_language
            target_entry['source_json'] = project_json
            target_entry['target_language'] = language
            target_entry['target_file_name'] = target_project_file
            targets.append(target_entry)

    for target in targets:
        print('translating ' + project['project_name'] + ' ' + source_project_file + ' -> ' + target['target_file_name'] + ' using ' + target['source_language'] + ' -> ' + target['target_language'] + ' mapping')
        if not run_translation_target(target, loc_table):
            return False

    return True

# --------------------------------------------------------------------------------------------------------
#                                          Loc String Scanning Code
# --------------------------------------------------------------------------------------------------------

def load_project_json(json_in):
    if isinstance( json_in, str ):
        return json.loads(json_in)
    else:
        return json_in

def scan_project(project, source_language, target_folder, exclude_project_agnostic_strings):
    source_project_file = get_file_path(project['base_file_name'], source_language)

    json_out = {}
    output_list = []

    print('scanning ' + project['project_name'] + ' ' + source_project_file)
    with open(source_project_file, encoding='utf8') as input_file:
        json_contents = json.load(input_file)
        if 'ProjectJSON' in json_contents:
            extraction_context = { 'outputList' : output_list, 'scope': ['none'] }
            internal_json = load_project_json(json_contents['ProjectJSON'])
            walk_json_tree_and_perform_transformations(internal_json, allFilters, extraction_context, pull_string)
        else:
            print("ERROR: could not find ProjectJSON field in source file %s" % source_project_file)

    for content in output_list:
        simplified_content = re.sub('[^a-z^0-9^_]+', '', content.lower().replace(' ', '_'))
        if simplified_content != '':
            if simplified_content in PROJECT_AGNOSTIC_KEYS:
                if not exclude_project_agnostic_strings:
                    key = 'codeLabFeaturedProject.' + simplified_content
                    json_out[key] = { 'translation': content }
            else:
                key = 'codeLabFeaturedProject.' + project['project_name'] + '.' + simplified_content
                json_out[key] = { 'translation': content }

    if target_folder is not None:
        export_filename = os.path.join(target_folder, os.path.basename(project['project_name']) + '_loc_strings.json')

        sorted_output = collections.OrderedDict(sorted(json_out.items(), key=lambda t: t[0]))

        with open(export_filename, 'w', encoding='utf8') as output_file:
            json.dump(sorted_output, output_file, indent=4, sort_keys=True)
            output_file.write('\n')

        print('scanning ' + project['project_name'] + ' and found ' + str(len(output_list)) + ' strings -> ' + export_filename)

    return json_out

# --------------------------------------------------------------------------------------------------------
#                                          Local State Construction
# --------------------------------------------------------------------------------------------------------

# collect all localization key->translation and translation->key mappings from a language folder
def load_localization_for_language(streaming_assets_path, language, active_project_name):
    resultDict = {'encodings':{}, 'decodings':{}}
    for file in os.listdir(os.path.join(streaming_assets_path, language)):
        if file.endswith('.json') and file in LOCALIZATION_TARGET_FILES:
            filePath = os.path.join(streaming_assets_path, language, file)
            with open(filePath, encoding='utf8') as dataFile:
                #print( 'loading language file ' + filePath )
                rootData = json.load(dataFile)
                for key in rootData:
                    if key != 'smartling':
                        value = rootData[key]['translation']
                        resultDict['decodings'][key.lower()] = value
                        if value not in resultDict['encodings'] or active_project_name in key:
                            # NOTE: The following can be useful for debugging whether the correct strings are being prioritized for projects
                            #if value in resultDict['encodings']:
                            #    print("Overriding loc encoding target: " + value + " (was: " +
                            #          resultDict['encodings'][value] + ", becoming: " + key.lower() +
                            #          ") - within scope of active_project_name " + active_project_name )
                            resultDict['encodings'][value] = key.lower()
    return resultDict

# build a dictionary of all loc tables by crawling the loc base path
def load_and_build_localization_dictionary(streaming_assets_path, target_project):
    resultDict = {}
    loc_path = os.path.join(streaming_assets_path, LOCALIZATION_ROOT_PATH)
    for language in os.listdir(loc_path):
        if '.' not in language:
            #print( 'loading language path: ' + language )
            resultDict[language] = load_localization_for_language(loc_path, language, target_project['project_name'])
    return resultDict

def build_project_translation_queue(streaming_assets_path, project_config_file, project_folder, specified_project, use_all_projects, blacklist):
    result = []

    with open(project_config_file, 'r', encoding='utf8') as json_data:
        featured_config = json.load(json_data)

    blacklisted_files = []
    if blacklist != None:
        with open(blacklist, encoding='utf8') as input_file:
            blacklisted_files = json.load(input_file)

    for featured_project_config in featured_config:
        base_file_name = os.path.join(project_folder, featured_project_config['ProjectJSONFile'])
        entry = { 'project_name': featured_project_config['DASProjectName'], 'base_file_name': base_file_name }
        if entry['project_name'] == specified_project or use_all_projects:
            if entry['project_name'] not in blacklisted_files:
                result.append( entry )
    return result

def modify_base_loc_file(extracted_json, file_path):
    #grab loc data
    loc_file_json = {}
    with open(file_path, 'r') as input_file:
        loc_file_json = json.load(input_file)

    #filter existing loc keys out of extracted_json
    for key in loc_file_json:
        extracted_json.pop(key, None)

    #inject new keys into the loc target's json
    for key in extracted_json:
        loc_file_json[key] = extracted_json[key]

    loc_file_json.pop('smartling')

    ordered_loc_file_json = collections.OrderedDict(sorted(loc_file_json.items(), key=lambda t: t[0]))

    raw_smartling_heading=collections.OrderedDict()
    raw_smartling_heading.update({"translate_paths": [ "*/translation" ]})
    raw_smartling_heading.update({"variants_enabled": True})
    raw_smartling_heading.update({"translate_mode": "custom"})
    raw_smartling_heading.update({"placeholder_format_custom": [
          "%%[^%]+?%%",
          "%[^%]+?%",
          "##[^#]+?##",
          "__[^_]+?__",
          "[$]\\{[^\\}\\{]+?\\}",
          "\\{\\{[^\\}\\{]+?\\}\\}",
          "(?<!\\{)\\{[^\\}\\{]+?\\}(?!\\})"
        ]})
    raw_smartling_heading.update({"source_key_paths": [ "/{*}" ]})

    header=collections.OrderedDict({"smartling": raw_smartling_heading})
    ordered_loc_file_json.update(header)
    ordered_loc_file_json.move_to_end('smartling', last=False)

    #replace the loc target's json
    with open(file_path, 'w') as output_file:
        json.dump(ordered_loc_file_json, output_file, indent=4)
        output_file.write('\n')

# --------------------------------------------------------------------------------------------------------
#                                               Core Path
# --------------------------------------------------------------------------------------------------------

def main():
    command_args = parse_command_args()

    if command_args.streaming_assets_path == '':
        sys.exit("Must specify streaming_assets_path")

    if command_args.project_config_file == '':
        sys.exit("Must specify project_config_file")

    if command_args.project_folder == '':
        sys.exit("Must specify project_folder")

    if command_args.project is None and not command_args.all_projects:
        sys.exit("Must specify a project to operate on")

    if not command_args.scan_project and not command_args.translate_project and not command_args.list_project:
        sys.exit("Must specify either list, scan or translate")

    project_list = build_project_translation_queue(command_args.streaming_assets_path, command_args.project_config_file, command_args.project_folder, command_args.project, command_args.all_projects, command_args.blacklist)

    if len(project_list) <= 0:
        sys.exit("Could not find source project %s" % command_args.project)

    if command_args.scan_project:
        if command_args.scan_output_path is None and command_args.scan_loc_target is None:
            sys.exit("Must specify a scan_output_path or scan_log_target to use scan_project functionality")

        generated_json = {}
        for project_entry in project_list:
            generated_json.update( scan_project(project_entry, command_args.source_language, command_args.scan_output_path, command_args.exclude_project_agnostic_strings) )
        print(str(len(project_list)) + ' project(s) scanned')

        if command_args.scan_loc_target:
            modify_base_loc_file(generated_json, command_args.scan_loc_target)

    if command_args.translate_project:
        projects_translated = 0
        for project_entry in project_list:
            loc_table = load_and_build_localization_dictionary(command_args.streaming_assets_path, project_entry)

            if command_args.source_language not in loc_table:
                sys.exit("Unexpected source_language %s" % command_args.source_language)

            if execute_translations_on_project(project_entry, command_args.source_language, loc_table):
                projects_translated += 1
        print(str(projects_translated) + ' project(s) translated')

    if command_args.list_project:
        for project_entry in project_list:
            print(project_entry['project_name'])


if __name__ == "__main__": 
    main()
