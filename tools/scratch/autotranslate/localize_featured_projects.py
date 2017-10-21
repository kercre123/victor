#!/usr/bin/env python3

# Nicolas Kent 10-20-17
#

# expected usage: "./localize_featured_projects.py"
# This script is expected to be run in it's containing folder from the command line
# There are currently no command line arguments for this script
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

import json
import random
import copy
import os
from pprint import pprint

BASE_SRC_SCRATCH_PATH = "../../../unity/Cozmo/Assets/StreamingAssets"
LOCALIZATION_ROOT_PATH = "LocalizedStrings"
FEATURED_PROJECT_SOURCE = "Scratch/featured-projects.json"
FEATURED_PROJECT_TARGET = "Scratch/featured-projects.json"

# Plenty of string fields contain these values 
# but even if they were to be localization translations, it might break a lot ot reverse key them
IGNORE_TEXT_KEYS = ['', 'True', 'False', 'true', 'false']

# 
SOURCE_LANGUAGE = 'en-US'

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
    def augment_context_by_dict_key(self, key, context):
        if key == 'variables':
            context = copy.deepcopy(context)
            context['variable'] = True
        return context

    def test_and_apply_transform(self, src, context, transform):
        if 'variable' in context and type(src) == dict and 'name' in src:
            string = src['name']
            try:
                int(string)
            except ValueError:
                if string not in IGNORE_TEXT_KEYS:
                    src['name'] = transform(string, context)

# the vast majority of user editable strings are represented in scratch using this json structure
class FilterField:
    def augment_context_by_dict_key(self, key, context):
        return context

    def test_and_apply_transform(self, src, context, transform):
        for key in ['TEXT', 'VARIABLE']:
            if type(src) == dict and 'fields' in src and key in src['fields']:
                string = src['fields'][key]['value']
                try:
                    int(string)
                except ValueError:
                    if string not in IGNORE_TEXT_KEYS:
                        src['fields'][key]['value'] = transform(string, context)

allFilters = [FilterVariable(), FilterField()]

# --------------------------------------------------------------------------------------------------------
#                                               Translation Code
# --------------------------------------------------------------------------------------------------------
#
# These functions are concerned just with translating a particular project, isolated from any context 
# of how they are stored in the featured json
#

# Replace a string using the loc translation table in the supplied context
# NOTE: this function is not called directly, but fed into the following function, it could easily be replaced for other transformations
def translate( string, context ):
    if string in context['loc']:
        # uncomment the print statement to get a verbose log of everything translated
        print(string + " - " + context['loc'][string])
        return context['loc'][string]
    return string

# Traverses a hierarchy of json objects/lists, using the supplied filters to identify where to apply the supplied transform
def walk_json_tree_and_perform_transformations( node, filters, context, transform ):
    if type(node) == list:
        for subNode in node:
            walk_json_tree_and_perform_transformations( subNode, filters, context, transform )
    elif type(node) == dict:
        for key in node:
            passedCtx = context
            for f in filters:
                passedCtx = f.augment_context_by_dict_key(key, passedCtx)
            walk_json_tree_and_perform_transformations( node[key], filters, passedCtx, transform )
    for f in filters:
        f.test_and_apply_transform(node, context, transform)

def generate_new_uuid():
    a = ''.join(random.choice('0123456789abcdef') for i in range(8))
    b = ''.join(random.choice('0123456789abcdef') for i in range(4))
    c = ''.join(random.choice('0123456789abcdef') for i in range(4))
    d = ''.join(random.choice('0123456789abcdef') for i in range(4))
    e = ''.join(random.choice('0123456789abcdef') for i in range(12))
    return a + '-' + b + '-' + c + '-' + d + '-' + e

# Returns a list of transformed projects for all languages specified in the locTable, for a given input project
def generate_translated_projects( project, locTable ):
    resultList = []

    encodedProjectJson = json.loads(project["ProjectJSON"])
    projectLanguage = project['Language']
    encodingContext = { 'loc': locTable[projectLanguage]['encodings'] }
    walk_json_tree_and_perform_transformations( encodedProjectJson, allFilters, encodingContext, translate )
    for key in locTable:
        if key != projectLanguage:
            newProject = copy.deepcopy(project)
            newProject['Language'] = key
            newProject['ProjectUUID'] = generate_new_uuid()
            projectInOtherLanguageJson = copy.deepcopy(encodedProjectJson)
            #print("translating to " + key)
            decodingContext = { 'loc': locTable[key]['decodings'] }
            walk_json_tree_and_perform_transformations( projectInOtherLanguageJson, allFilters, decodingContext, translate )
            newProject['ProjectJSON'] = json.dumps(projectInOtherLanguageJson)
            resultList.append(newProject)
    return resultList


def pull_string( string, context ):
    if string not in context['outputList']:
        context['outputList'].append(string)

def extract_all_translatable_values(resultTable, project):
    outputList = []
    extractionContext = { 'outputList' : outputList }
    encodedProjectJson = json.loads(project["ProjectJSON"])
    walk_json_tree_and_perform_transformations( encodedProjectJson, allFilters, extractionContext, pull_string )

    resultTable[project['ProjectName']] = outputList


# --------------------------------------------------------------------------------------------------------
#                                           Translation Management
# --------------------------------------------------------------------------------------------------------
# 
# These functions identify which projects in the featured list to process, how to modify that list,
# where to import and export that list from and to, and where to pull in localization data
#

# export either the input project, or append translations based on existence of 'Language' key
def append_project_results_to_list( resultList, project, locTable ):
    if 'Language' in project: 
        projectLanguage = project['Language']
        if projectLanguage not in locTable:
            raise Exception('Language ' + projectLanguage + ' not found in localization table')

        if projectLanguage == SOURCE_LANGUAGE:
            resultList.append(project)
            newProjects = generate_translated_projects( project, locTable )
            for prj in newProjects:
                resultList.append(prj)
            return True
    else:
        resultList.append(project)
    return False

def export_all_strings_from_project( resultTable, project ):
    if 'Language' in project: 
        if project['Language'] == SOURCE_LANGUAGE:
            extract_all_translatable_values( resultTable, project )
    else:
        extract_all_translatable_values( resultTable, project )

# read all featured projects, process each one, and export them to another json file
def process_all_projects( inputFile, outputFile, locTable ):
    resultProjects = []
    projectsTranslated = 0

    with open(inputFile) as dataFile:
        allProjects = json.load(dataFile)
        for project in allProjects:
            if append_project_results_to_list(resultProjects, project, locTable):
                projectsTranslated += 1
    
    with open(outputFile, 'w') as outputFile:
        json.dump(resultProjects, outputFile, indent=4)
    return projectsTranslated

# collect all localization key->translation and translation->key mappings from a language folder
def load_localization_for_language( rootPath, language ):
    resultDict = {'encodings':{}, 'decodings':{}}
    for file in os.listdir(os.path.join(rootPath, language)):
        if file.endswith('.json') and file == 'CodeLabFeaturedContentStrings.json':
            filePath = os.path.join(rootPath, language, file)
            with open(filePath) as dataFile:
                #print( 'loading language file ' + filePath )
                rootData = json.load(dataFile)
                for key in rootData:
                    if key != 'smartling':
                        value = rootData[key]['translation']
                        resultDict['decodings'][key] = value
                        resultDict['encodings'][value] = key
    return resultDict

# build a dictionary of all loc tables by crawling the loc base path
def load_and_build_localization_dictionary():
    resultDict = {}
    locPath = os.path.join(BASE_SRC_SCRATCH_PATH, LOCALIZATION_ROOT_PATH);
    for language in os.listdir(locPath):
        if '.' not in language:
            #print( 'loading language path: ' + language )
            resultDict[language] = load_localization_for_language(locPath, language)
    return resultDict

def export_all_strings_file(inputFile):
    jsonOut = {}
    stringTable = {}

    with open(inputFile) as dataFile:
        allProjects = json.load(dataFile)
        for project in allProjects:
            export_all_strings_from_project(stringTable, project)

    for projectKey in stringTable:
        for content in stringTable[projectKey]:
            simplifiedContent = content.lower().replace(' ', '_').replace('.', '').replace('\'', '').replace('\"', '').replace('(', '').replace(')', '')
            projectKeySanitized = projectKey.replace('.projectName','').replace('_','.')
            jsonOut[projectKeySanitized + '.' + simplifiedContent] = { 'translation' : content }
    with open('allStrings.json', 'w') as outputFile:
        json.dump(jsonOut, outputFile, indent=4)


def main():
    locTable = load_and_build_localization_dictionary()

    inputFile = os.path.join(BASE_SRC_SCRATCH_PATH, FEATURED_PROJECT_SOURCE)
    outputFile = os.path.join(BASE_SRC_SCRATCH_PATH, FEATURED_PROJECT_TARGET)
    projectsTranslated = process_all_projects(inputFile, outputFile, locTable)
    print('projects translated: ' + str(projectsTranslated))

    export_all_strings_file(inputFile)

if __name__ == "__main__": 
    main()
