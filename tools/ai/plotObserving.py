#!/usr/bin/env python2

"""
* File: plotObserving.py
*
* Author: ross
* Created: Jan 11 2017
*
* Description: Plots observer transitions from json
*
* Copyright: Anki, Inc. 2018
*
**/

"""

import os
import sys
import tempfile as tmp
import subprocess
import functools
import graphviz as gv
import argparse
import re
import json

kRenderEngine = 'fdp'  #also try circo, but unset splines=curved

def getEdgeLabel( data ):
  if 'preDefinedStrategyName' in data:
    edgeLabel = data['preDefinedStrategyName']
  elif 'strategy' in data:
    edgeLabel = data['strategy']['strategyType']
  elif 'condition' in data and 'conditionType' in data['condition']:
    if 'Negate' in data['condition']['conditionType']:
      edgeLabel = 'not ' + data['condition']['operand']['conditionType']
    else:
      edgeLabel = data['condition']['conditionType']
  else:
    edgeLabel = ''
  return edgeLabel
def addEdges( options, fromNode, edges, transitionJson, edgeStyle='solid', color='black' ):
  for toData in transitionJson:
    toNode = toData['to']
    edgeLabel = getEdgeLabel( toData )
    if options.ignoreUserIntents and edgeLabel == 'UserIntentPending':
      continue
    if options.startNode is not None or options.endNode is not None:
      if options.startNode is not None and fromNode != options.startNode:
        continue
      elif options.endNode is not None and toNode != options.endNode:
        continue
    edge = (fromNode, toNode)
    props = {'style': edgeStyle, 'label': edgeLabel, 'color': color}
    if [fromNode.lower(), toNode.lower()] != sorted([fromNode.lower(), toNode.lower()]):
      props['minlen'] = '20'
    props['color'] = color
    props['fontcolor'] = color
    edges.append( (edge, props) )

def parseJson( nodes, edges, json, filename, options ):
  """ Loads json into nodes and edges """

  states = json['states']
  for state in states:
    if 'name' in state:
      name = state['name']
      if name not in nodes:
        nodes.append(name)
  for trans in json['transitionDefinitions']:
    fromObj = trans['from']
    if isinstance(fromObj, list):
      fromList = fromObj
    else:
      fromList = [fromObj]
    for fromNode in fromList:
      interruptingTrans = {} if 'interruptingTransitions' not in trans else trans['interruptingTransitions']
      nonInterruptingTrans = {} if 'nonInterruptingTransitions' not in trans else trans['nonInterruptingTransitions']
      exitTrans = {} if 'exitTransitions' not in trans else trans['exitTransitions']

      if fromNode not in nodes:
        nodes.append(fromNode)
      
      addEdges( options, fromNode, edges, interruptingTrans, 'solid', '#d55e00' ) #colors are rgb for colorblind ppl
      addEdges( options, fromNode, edges, nonInterruptingTrans, 'dashed', '#009e73' )
      addEdges( options, fromNode, edges, exitTrans, 'dotted','#0072b2' )

def loadFile( filename ):
  """ Loads json file, after removing comments """
  with open( filename, 'r' ) as content:
    data = content.read()
    # remove comments // like this
    data = re.sub( r'//.*$', '', data, flags=re.M )
    return json.loads( data )

def applyStyles(G, styles):
  G.graph_attr.update(
    ('graph' in styles and styles['graph']) or {}
  )
  G.node_attr.update(
    ('nodes' in styles and styles['nodes']) or {}
  )
  G.edge_attr.update(
    ('edges' in styles and styles['edges']) or {}
  )
  return G

def makeGraph( nodes, edges, ext ):
  """ Gathers nodes and edges into a graphviz graph"""
  digraph = functools.partial(gv.Digraph, format=ext, engine=kRenderEngine)
  G = digraph()

  for node in nodes:
    if isinstance(node, tuple):
      G.node(node[0], **node[1])
    else:
      G.node(node)
    
  for edge in edges:
    if isinstance(edge[0], tuple):
      G.edge(*edge[0], **edge[1])
    else: 
        G.edge(*edge)

    G.attr(sep="+40,40",overlap='scaledxy',splines='curved')

  styles = {
    'graph': {
      'label': 'A Fancy Graph',
      'fontsize': '16',
      'fontcolor': 'white'
    },
    'nodes': {
      'fontname': 'Helvetica',
      'shape': 'ellipse',
      'fontcolor': 'white',
      'color': 'white',
      'style': 'filled',
      'fillcolor': '#333333'
    },
    'edges': {
      'arrowhead': 'open',
      'fontname': 'Courier',
      'fontsize': '10',
      'lblstyle':'above, sloped'
    }
  }
  applyStyles(G, styles)

  return G

def hasGraphViz():
  fnull = open(os.devnull, 'w')
  try:
    subprocess.check_call('neato -V',shell=True, stdout=fnull, stderr=subprocess.STDOUT)
    fnull.close()
    return True
  except:
    fnull.close()
    return False

def main():

  if not hasGraphViz():
    print('This script requires GraphViz. Run "brew install graphviz" and try again')
    sys.exit(1)

  parser = argparse.ArgumentParser(description='Draws a graph of Victor behaviors.')
  parser.add_argument('-o', '--output', action="store", dest='outputFilename', metavar='OUTPUTFILE',
                      help='output file (extension determines filetype)')
  parser.add_argument('-u', '--ignore-user-intents', action="store_true", dest='ignoreUserIntents',
                      help='ignores transitions due to user intents')
  parser.add_argument('inputFilename', metavar='INPUTFILE',
                      help='the file victorObservingDemo.json')
  mutex = parser.add_mutually_exclusive_group(required=False)
  mutex.add_argument('--begin', action="store", dest='startNode', metavar='BEGINBEHAVIOR',
                     help='only draws edges that originate from BEGINBEHAVIOR')
  mutex.add_argument('--end', action="store", dest='endNode', metavar='ENDBEHAVIOR',
                     help='only draws edges that end at ENDBEHAVIOR')
  

  args = parser.parse_args()

  inputFilename = args.inputFilename
  outputFilename = args.outputFilename
  
  if not os.path.isfile(inputFilename):
    print('Cannot find input file ' + inputFilename)
    sys.exit(1)
  if outputFilename:
    if len(outputFilename) < 3 or '.' not in outputFilename:
      print('Cannot determine output format from extension')
      sys.exit(1)

  nodes = []
  edges = []

  jsonData = loadFile( inputFilename )
  parseJson( nodes, edges, jsonData, inputFilename, args )

  if args.startNode is not None and args.startNode not in nodes:
    print('Behavior {0} was not found'.format(args.startNode))
    sys.exit(1)
  if args.endNode is not None and args.endNode not in nodes:
    print('Behavior {0} was not found'.format(args.endNode))
    sys.exit(1)

  # render 

  ext = 'pdf'
  G = makeGraph( nodes, edges, ext )
  
  writeSuccess = False
  unflattenRet = 0

  if outputFilename:
    fname = outputFilename
  else:
    tempFile = tmp.NamedTemporaryFile(delete=False)
    fname = tempFile.name + '.' + ext

  try:
    with tmp.NamedTemporaryFile(delete=False) as dotFile:
      dotFile.write( str(G) )
      dotFile.flush()

      # cmd = 'unflatten -c 2 {0} | {1} -T{2} -o {3}'.format(dotFile.name, kRenderEngine, ext, fname)
      cmd = 'cat {0} | {1} -T{2} -o {3}'.format(dotFile.name, kRenderEngine, ext, fname)
      unflattenRet = subprocess.call(cmd, shell=True)
      writeSuccess = (unflattenRet == 0)

      # todo: edge-aligned labels are possible with dot2tex, but bc pip hasnt been updated with
      # the latest (https://github.com/kjellmf/dot2tex/issues/44) node edges are not drawn. Maybe
      # patch dot2tex for this?
      
      dotFile.close()
  finally:
    if not outputFilename:
      tempFile.close()

  


  if writeSuccess:
    print( 'Output saved to {0}'.format( fname ) )

    print(' red solid    == interrupting transitions')
    print(' green dashed == non-interrupting transitions')
    print(' blue dotted  == natural exits')

    os.system( 'open ' + fname )
  elif not writeSuccess:
    print('Error!')
    sys.exit(unflattenRet)
  
if __name__ == "__main__":
  main()
