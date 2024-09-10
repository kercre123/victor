#!/usr/bin/python

import sys
import argparse
import json
import logging
from os import path


from ParseWWiseIds import ParseWWiseIds
from GenerateAudioMetadataSheet import GenerateAudioMetadataSheet
from GenerateCladJsonFromMetadata import GenerateCladJsonFromMetadata
from JsonToCladGenerator import JsonToCladGenerator


__wwiseIdsJson_command = 'wwiseIdsJson'
__metadata_command = 'metadata'
__clad_command = 'clad'
__meta_to_json_command = 'metadata-to-json'


def __parse_input_arguments():
    argv = sys.argv

    parser = argparse.ArgumentParser(description="I like stuff!")

    subparsers = parser.add_subparsers(dest='commands', help='commands')

    # Creat JSON from WWise Ids File
    wwise_ids_json_parser = subparsers.add_parser(__wwiseIdsJson_command, help='Generate WWiseIds.json')
    wwise_ids_json_parser.add_argument('wwiseIds', action="store", help="WWiseIds.h file generated with soundbands")
    wwise_ids_json_parser.add_argument('output', action='store', help='Output WWiseIds.json output file')

    # Generate & update Audio Meteadata
    audio_metadata_parser = subparsers.add_parser(__metadata_command, help='Generate audio metadata.csv')
    audio_metadata_parser.add_argument('--merge', '-m', action='store', help='Merge audio metadata.csv fille into new output')
    audio_metadata_parser.add_argument('wwiseIds', action="store", help="WWiseIds.h file generated with soundbands")
    audio_metadata_parser.add_argument('output', action='store', help='Output Audio Metadata.csv output file')

    # Generate Clad files
    clad_parser = subparsers.add_parser(__clad_command, help='Generate audio.clad Files')
    clad_parser.add_argument('wwiseIds', action="store", help="WWiseIds.h file generated with soundbands")
    clad_parser.add_argument('metadata', action="store", help="Audio Metadata.csv file")
    clad_parser.add_argument('clad_dir', action='store', help='Clad output Directory')
    clad_parser.add_argument('namespaces', nargs='+', help='List of Namespaces to add to clad files')

    # Generate JSON from Audio Metadata
    meta_to_jason = subparsers.add_parser(__meta_to_json_command, help='Generate event.json file')
    meta_to_jason.add_argument('metadata', action="store", help='Audio Metadata.csv file')
    meta_to_jason.add_argument('outputFilePath', action='store', help='Output file.json')
    meta_to_jason.add_argument('groups', nargs='+', help='List of Event Group names to insert into json file')

    # TODO: Add argument that takes a list of arguments

    options = parser.parse_args(sys.argv[1:])

    return  options


def main():
    options = __parse_input_arguments()
    # print "Options:\n%s" % options

    if __metadata_command == options.commands:
        # Parse WWise header file into a Json formated dictionary
        wwiseIdsData = ParseWWiseIds().parseWWiseIdFile(options.wwiseIds)
        # Merge currnet metadata.csv file and generate new metadata.csv sheet
        errorLog = GenerateAudioMetadataSheet().generateMetadataFile(wwiseIdsData, options.merge, options.output)
        if len(errorLog) > 0:
            logging.warning("Please check errors before committing")
            for aLog in errorLog:
                logging.warning(aLog)


    elif __clad_command == options.commands:
        metadata = GenerateAudioMetadataSheet().readMetadataSheet(options.metadata)
        generator = GenerateCladJsonFromMetadata()
        # Parse WWise header file into a Json formated dictionary
        wwiseIdsData = ParseWWiseIds().parseWWiseIdFile(options.wwiseIds)
        generator.importMetadata(metadata, wwiseIdsData)
        cladJsonData = generator.generateCladJsonData(options.namespaces)

        JsonToCladGenerator().generateCladFile(cladJsonData, options.clad_dir)

    elif __wwiseIdsJson_command == options.commands:
        # Parse WWise header file into a Json formated dictionary
        wwiseIdsData = ParseWWiseIds().parseWWiseIdFile(options.wwiseIds)
        ParseWWiseIds().createWWiseIdJsonFile(wwiseIdsData, options.output)

    elif __meta_to_json_command == options.commands:
        metadata = GenerateAudioMetadataSheet().readMetadataSheet(options.metadata)
        jsonData = __CreateJsonForEventTypes(metadata, options.groups)
        if len(jsonData) == 0:
            print "No Events available"
            return

        with open(options.outputFilePath, 'w') as outfile:
            json.dump(jsonData, outfile, indent=4, sort_keys=True, separators=(',', ': '))


# Temp: Put this in it's own class
def __CreateJsonForEventTypes(metadata, eventList):
    jsonData = []
    # Gather Events that belong to a group that is in Event List
    for anItem in metadata:
        if anItem.groupName in eventList:
            jsonData.append(anItem.jsonData())

    return  jsonData


if __name__ == '__main__':
    main()

