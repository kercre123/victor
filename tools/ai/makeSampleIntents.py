#!/usr/bin/env python2

"""
* File: makeSampleIntents.py
*
* Author: ross
* Created: Mar 10 2018
*
* Description: Grabs the latest cloud intents from the 
*              voice-intent-resolution-config repo and generates a list
*              of intents that could arise 
*
* Copyright: Anki, Inc. 2018
*
**/
"""

import os
import sys
import json
import itertools

USAGE_MSG = 'USAGE: {0} <dialogFlowDataDir> <outputFile.json>'.format( sys.argv[0] )

ANKI_ENTITIES = [ "entity_anki_words", "entity_bad_words", "entity_behavior",
                  "entity_behavior_deletable", "entity_behavior_stoppable",
                  "entity_charger_words", "entity_clothes_words", "entity_cube_words",
                  "entity_exploring_words", "entity_forecast_words", "entity_photo_selfie",
                  "entity_robotnames", "entity_topic", "entity_weather_words" ]

# These are from https://dialogflow.com/docs/reference/system-entities
SYSTEM_ENTITIES = { "sys.any": ["flower"],
                    "sys.date-time": ["2017-07-12T16:30:00Z"],
                    "sys.duration": [{"amount":10, "unit":"min"}],
                    "sys.time": ["16:30:00", "08:20:01"],
                    "sys.temperature": [{"amount":25,"unit":"F"}],
                    "sys.percentage": ["50%"],
                    "sys.given-name": ["Mary"],
                    "sys.date": ["2017-07-12"],
                    "sys.geo-city": ["New York"],
                    "sys.geo-state-us": ["California"],
                    "sys.geo-country": ["United States of America"] }

# - - - - - - - - - - - - - - - - - - - - - -
def Fail(msg):
  sys.stderr.write( '{0}\n'.format(msg) )
  sys.exit( 1 )

# - - - - - - - - - - - - - - - - - - - - - -
def CheckDirectoryExists( dirPath ):
  try:
    os.stat( dirPath )
  except OSError:
    Fail( 'Directory missing: {0}'.format( dirPath ) )

# - - - - - - - - - - - - - - - - - - - - - -
def LoadFile( filename ):
  """ Loads json file """
  if not os.path.isfile( filename ):
    Fail( 'Could not find {0}'.format( filename ) )
  with open( filename, 'r' ) as content:
    data = content.read()
    return json.loads( data )

# - - - - - - - - - - - - - - - - - - - - - -
def GetIntents(path):
  """ Loads and returns a list of intents """
  intents = []

  files = []
  for file in os.listdir( path ):
    if file.endswith( '.json' ):
      filename = os.path.join( path, file )
      if 'usersays_en' not in filename:
        files.append( filename )

  for file in files:
    intent = {}
    data = LoadFile( file )
    intent["name"] = data["name"]
    paramList = data["responses"][0]["parameters"]
    intent["params"] = [ {"name": param["name"], "dataType": param["dataType"]} for param in paramList ]

    intents.append( intent )

  return intents

# - - - - - - - - - - - - - - - - - - - - - -
def GetSamplesForDataType(paramName, dataType, entities):
  """ Map from dataType to a sample, either from entities or system types """

  dataType = dataType[1:] # drop the "@"

  paramSamples = []

  if dataType in ANKI_ENTITIES:
    paramSamples = entities[dataType]["values"]
  # the rest from https://dialogflow.com/docs/reference/system-entities
  elif dataType in SYSTEM_ENTITIES:
    paramSamples = SYSTEM_ENTITIES[dataType]

  if len(paramSamples) == 0:
    # if no match, this script needs updating to match the
    # newly-found data type
    Fail( 'Unknown dataType "{0}"'.format(dataType) )

  sampleList = []
  for param in paramSamples:
    # note: cast everything to string, bc that's how we receive it from the cloud
    if isinstance( param, dict ):
      # change dicts {A: {x:1, y:2}} into {A.x:1 and A.y:2}
      fullParam = {}
      for key, value in param.iteritems():
        fullParam[paramName + '.' + key] = str(value)
      sampleList.append( fullParam )
    else:
      sampleList.append( {paramName: str(param)} );
  return sampleList

# - - - - - - - - - - - - - - - - - - - - - -
def GetEntities(path):
  """ Loads and returns a list of entities """

  entities = {}

  files = []
  for file in os.listdir( path ):
    if file.endswith( '.json' ):
      filename = os.path.join( path, file )
      if 'entries_en' not in filename:
        files.append( filename )

  for file in files:
    entity = {}

    valueFile = os.path.splitext( file )[0] + '_entries_en.json'

    nameData = LoadFile( file )
    valueData = LoadFile( valueFile )

    entity["name"] = nameData["name"]
    entity["values"] = [ entry["value"] for entry in valueData ]

    entities[entity["name"]] = entity

  return entities

# - - - - - - - - - - - - - - - - - - - - - -
def MakeSamples( intents, entities ):
  """ Merges intents, entities, and samples of system data types to
      generate and return a list of sample cloud intents """

  sampleList = []

  for intent in intents:
    paramSamples = []
    for param in intent["params"]:
      paramSamples.append( GetSamplesForDataType( param["name"], param["dataType"], entities ) )
    if len( paramSamples ) == 0:
      sample = {}
      sample["intent"] = intent["name"]
      sampleList.append( sample )
    else:
      # for each combo in caretesian product of param samples
      for elems in itertools.product( *paramSamples ):
        sample = {}
        sample["intent"] = intent["name"]
        sample["params"] = {}
        for elem in elems:
          sample["params"].update( elem )
        sampleList.append( sample )
  return sampleList

# - - - - - - - - - - - - - - - - - - - - - -
def SaveSamples( sampleList, outputFilename ):
  """ Saves sampleList into outputFilename """
  with open( outputFilename, 'w' ) as outfile:
    json.dump( sampleList, outfile )

# - - - - - - - - - - - - - - - - - - - - - -
def ParseArgs():
  """ Parses sys.argv and returns a three-item tuple of:
      1. path to the intents directory
      2. path to the entities directory
      3. path to the json output file """

  if len( sys.argv ) != 3:
    Fail( USAGE_MSG )

  dialogFlowDataDir = sys.argv[1]
  CheckDirectoryExists( dialogFlowDataDir )

  intentsDir = os.path.join( dialogFlowDataDir, 'intents' )
  CheckDirectoryExists( intentsDir )

  entitiesDir = os.path.join( dialogFlowDataDir, 'entities' )
  CheckDirectoryExists( entitiesDir )

  outputFilename = sys.argv[2]

  return (intentsDir, entitiesDir, outputFilename)

# - - - - - - - - - - - - - - - - - - - - - -
def main():

  intentsDir, entitiesDir, outputFilename = ParseArgs()

  intents = GetIntents( intentsDir )
  entities = GetEntities( entitiesDir )

  sampleList = MakeSamples( intents, entities )

  SaveSamples( sampleList, outputFilename )

# - - - - - - - - - - - - - - - - - - - - - -
if __name__ == "__main__":
  main()

