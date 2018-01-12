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

import copy
import json
import os
from pprint import pprint
import random
import re
import sys
import argparse
import zipfile

BASE_SRC_SCRATCH_PATH = "../../../unity/Cozmo/Assets/StreamingAssets"
LOCALIZATION_ROOT_PATH = "LocalizedStrings"
LOCALIZATION_TARGET_FILES = [ "CodeLabStrings.json", "CodeLabFeaturedContentStrings.json" ]
PROJECT_CONFIGURATION_FILE = "Scratch/featured-projects.json"
TARGET_PROJECT_FOLDER = "Scratch/featuredProjects"

# Plenty of string fields contain these values 
# but even if they were to be localization translations, it might break a lot ot reverse key them
IGNORE_TEXT_KEYS = ['', 'True', 'False', 'true', 'false']

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
    arg_parser.add_argument('-d', '--desktop',
                        dest='target_desktop',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Specifies that scan exports should be sent to the desktop instead of the script\'s folder')

    arg_parser.add_argument('-o', '--source_language',
                        dest='source_language',
                        default='en-US',
                        action='store',
                        help='Specify the source language to be operated on')


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
            context['scope'][-1] = 'variable'

    def test_and_apply_transform(self, src, context, transform):
        if context['scope'][-1] == 'variable' and type(src) == dict and 'name' in src:
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
        return context['loc'][string]
    return string

# version of the translate function the checks a pre-lowercased loc mapping for lowercased versions of the
# target strings.  This will compensate for things like "points" and "Points" resolving to the same key
def translate_lower(string, context):
    lowercasedString = string.lower()
    if lowercasedString in context['loc']:
        # uncomment the print statement to get a verbose log of everything translated
        return context['loc'][lowercasedString]
    return lowercasedString

# Traverses a hierarchy of json objects/lists, using the supplied filters to identify where to apply the supplied transform
def walk_json_tree_and_perform_transformations(node, filters, context, transform):
    if len( context['scope'] ) == 0 :
        context['scope'].append('none')
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

    projectJson = json.loads(project['ProjectJSON'])

    # encode the original project into localization keys, with matches lowercased
    encodingContext = { 'loc': {}, 'scope': [] }#{ 'loc': sourceLanguageTable['encodings'] }
    for key in sourceLanguageTable['encodings']:
        encodingContext['loc'][key.lower()] = sourceLanguageTable['encodings'][key]
    walk_json_tree_and_perform_transformations(projectJson, allFilters, encodingContext, translate_lower)

    # decode the localization keys into the proper language
    decodingContext = { 'loc': destinationLanguageTable['decodings'], 'scope': [] }
    walk_json_tree_and_perform_transformations(projectJson, allFilters, decodingContext, translate)

    newProject = {}
    newProject['ProjectUUID'] = project['ProjectUUID']
    newProject['ProjectJSON'] = json.dumps(projectJson)

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
    return (base_file_name + '_' + language + '.json').lower()

def run_translation_target(target, loc_table):
    with open(target['source_file_name']) as input_file:
        project = json.load(input_file)
        translated = translate_project(project, loc_table[target['source_language']], loc_table[target['target_language']])
        with open(target['target_file_name'], 'w') as output_file:
            json.dump(translated, output_file, indent=4)
            output_file.write('\n')

    return True

def execute_translations_on_project(project, source_language, loc_table):
    source_project_file = get_file_path(project['base_file_name'], source_language)
    targets = []
    for language in loc_table:
        if language != source_language:
            target_project_file = get_file_path(project['base_file_name'], language)
            target_entry = {}
            target_entry['source_language'] = source_language
            target_entry['target_language'] = language
            target_entry['source_file_name'] = source_project_file
            target_entry['target_file_name'] = target_project_file
            targets.append(target_entry)

    for target in targets:
        print('translating ' + project['project_name'] + ' ' + target['source_language'] + ' -> ' + target['target_language'])
        if not run_translation_target(target, loc_table):
            return False

    return True

# --------------------------------------------------------------------------------------------------------
#                                          Loc String Scanning Code
# --------------------------------------------------------------------------------------------------------

def scan_project(project, source_language, target_desktop):
    source_project_file = project['base_file_name'] + '_' + source_language + '.json'

    json_out = {}
    output_list = []

    with open(source_project_file) as input_file:
        json_contents = json.load(input_file)
        if 'ProjectJSON' in json_contents:
            extraction_context = { 'outputList' : output_list, 'scope': [] }
            encoded_project_json = json.loads(json_contents['ProjectJSON'])
            walk_json_tree_and_perform_transformations(encoded_project_json, allFilters, extraction_context, pull_string)
        else:
            print("ERROR: could not find ProjectJSON field in source file %s" % source_project_file)

    for content in output_list:
        simplified_content = re.sub('[^a-z^0-9^_]+', '', content.lower().replace(' ', '_'))
        if simplified_content != '':
            key = 'codeLabFeaturedProject.' + project['project_name'] + '.' + simplified_content
            json_out[key] = { 'translation': content }

    if target_desktop:
        export_folder = os.path.expanduser('~/Desktop/CodeLabLocStrings')
        try:
            os.stat(export_folder)
        except:
            os.mkdir(export_folder)
        export_filename = os.path.join(export_folder, os.path.basename(project['project_name']) + '_loc_strings.json')
    else:
        export_filename = project['project_name'] + '_loc_strings.json'

    with open(export_filename, 'w') as output_file:
        json.dump(json_out, output_file, indent=4)
        output_file.write('\n')

    print('scanning ' + project['project_name'] + ' and found ' + str(len(output_list)) + ' strings -> ' + export_filename)
    return True

# --------------------------------------------------------------------------------------------------------
#                                          Local State Construction
# --------------------------------------------------------------------------------------------------------

# collect all localization key->translation and translation->key mappings from a language folder
def load_localization_for_language(rootPath, language):
    resultDict = {'encodings':{}, 'decodings':{}}
    for file in os.listdir(os.path.join(rootPath, language)):
        if file.endswith('.json') and file in LOCALIZATION_TARGET_FILES:
            filePath = os.path.join(rootPath, language, file)
            with open(filePath) as dataFile:
                #print( 'loading language file ' + filePath )
                rootData = json.load(dataFile)
                for key in rootData:
                    if key != 'smartling':
                        value = rootData[key]['translation']
                        resultDict['decodings'][key.lower()] = value
                        resultDict['encodings'][value] = key.lower()
    return resultDict

# build a dictionary of all loc tables by crawling the loc base path
def load_and_build_localization_dictionary():
    resultDict = {}
    script_path = os.path.dirname(os.path.realpath(__file__))
    loc_path = os.path.join(script_path, BASE_SRC_SCRATCH_PATH, LOCALIZATION_ROOT_PATH)
    for language in os.listdir(loc_path):
        if '.' not in language:
            #print( 'loading language path: ' + language )
            resultDict[language] = load_localization_for_language(loc_path, language)
    return resultDict

def build_project_translation_queue(specified_project, use_all_projects, blacklist):
    result = []
    script_path = os.path.dirname(os.path.realpath(__file__))

    project_config_path = os.path.join(script_path, BASE_SRC_SCRATCH_PATH, PROJECT_CONFIGURATION_FILE)
    with open(project_config_path, 'r') as json_data:
        featured_config = json.load(json_data)

    project_folder = os.path.join(script_path, BASE_SRC_SCRATCH_PATH, TARGET_PROJECT_FOLDER)

    blacklisted_files = []
    if blacklist != None:
        with open(blacklist) as input_file:
            blacklisted_files = json.load(input_file)

    for featured_project_config in featured_config:
        base_file_name = os.path.join(project_folder, featured_project_config['ProjectJSONFile'])
        entry = { 'project_name': featured_project_config['DASProjectName'], 'base_file_name': base_file_name }
        if entry['project_name'] == specified_project or use_all_projects:
            if entry['project_name'] not in blacklisted_files:
                result.append( entry )
    return result

# --------------------------------------------------------------------------------------------------------
#                                               Core Path
# --------------------------------------------------------------------------------------------------------

def main():
    command_args = parse_command_args()

    loc_table = load_and_build_localization_dictionary()

    if command_args.source_language not in loc_table:
        sys.exit("Unexpected source_language %s" % command_args.source_language)

    if command_args.project is None and not command_args.all_projects:
        sys.exit("Must specify a project to operate on")

    if not command_args.scan_project and not command_args.translate_project and not command_args.list_project:
        sys.exit("Must specify either list, scan or translate")

    project_list = build_project_translation_queue(command_args.project, command_args.all_projects, command_args.blacklist)

    if len(project_list) <= 0:
        sys.exit("Could not find source project %s" % command_args.project)

    if command_args.scan_project:
        projects_scanned = 0
        for project_entry in project_list:
            if scan_project(project_entry, command_args.source_language, command_args.target_desktop):
                projects_scanned += 1
        print(str(projects_scanned) + ' project(s) scanned')

    if command_args.translate_project:
        projects_translated = 0
        for project_entry in project_list:
            if execute_translations_on_project(project_entry, command_args.source_language, loc_table):
                projects_translated += 1
        print(str(projects_translated) + ' project(s) translated')

    if command_args.list_project:
        for project_entry in project_list:
            print(project_entry['project_name'])


if __name__ == "__main__": 
    main()
