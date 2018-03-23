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

# - - - - - - - - - - - - - - - - - - - - - -
def Fail(msg):
  sys.stderr.write( '{0}\n'.format(msg) )
  sys.exit( 1 )

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

  path = os.path.join( path, 'intents' )
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

  if dataType == "entity_behavior" \
   or dataType == "word" \
   or dataType == "entity_bad_words" \
   or dataType == "entity_topic":
    paramSamples = entities[dataType]["values"]
  # the rest from https://dialogflow.com/docs/reference/system-entities
  elif dataType == "sys.date-time":
    paramSamples = ["2017-07-12T16:30:00Z"]
  elif dataType == "sys.duration":
    paramSamples = [{"amount":10,"unit":"min"}]
  elif dataType == "sys.time":
    paramSamples = ["16:30:00", "08:20:01"]
  elif dataType == "sys.temperature":
    paramSamples = [{"amount":25,"unit":"F"}]
  elif dataType == "sys.percentage":
    paramSamples = ["50%"]
  elif dataType == "sys.given-name":
    paramSamples = ["Mary"]
  elif dataType == "sys.date":
    paramSamples = ["2017-07-12"]
  elif dataType == "sys.geo-city":
    paramSamples = ["New York"]

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

  path = os.path.join( path, 'entities' )
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
def main():
  if len( sys.argv ) != 2:
    Fail( 'USAGE: {0} <outputFile.json>'.format( sys.argv[0] ) )

  outputFilename = sys.argv[1]

  # todo: download DialogFlow data using DEPS or submodules.
  # For now, get the put the contents of the file victor-prd-lab-04.zip
  # from nishkar into the "json" dir.
  pathToIntentData = 'json'

  intents = GetIntents( pathToIntentData )
  entities = GetEntities( pathToIntentData )

  sampleList = MakeSamples( intents, entities )

  SaveSamples( sampleList, outputFilename )

# - - - - - - - - - - - - - - - - - - - - - -
if __name__ == "__main__":
  main()
