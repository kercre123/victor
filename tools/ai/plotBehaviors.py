#!/usr/bin/env python2

"""
* File: plotBehaviors.py
*
* Author: ross / brad
* Created: Jan 4 2017
*
* Description: Plots a directed graph of behaviors, informed by
*              recursive calls to the IBehavior::GetAllDelegates() method. For
*              this, the script looks for a behavior_transitions.txt file
*              that is output from the test_engine unit test.
*              Supply a behavior name as an arg for a graph of just that behavior
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

kShowDisconnectedNodes = True
kRenderEngine = 'dot' #'neato'

font_size = '12'

## set to true to show points at the invisible joint nodes
debugInvisibleJoints = False

kGlobalMaxDepth = 100

def cropTree( inputEdges, root, crops ):
  """ Crop the tree by cutting off any branches below nodes named in args.crop
  """
  edges = {}
  for k in inputEdges:
    if k not in crops:
      # add " ..." to the name if there are hidden children
      edges[k] = [x + ' ...' if x in crops else x for x in inputEdges[k]]

  return edges

def extractTree( inputEdges, root, nodes=None, edges=None, depth=0 ):
  """ Recursively pull out tree from root. Only supply first 2 args
  """

  if depth > kGlobalMaxDepth:
    print('WARNING: depth ({0}) unexpectedly large'.format(depth))
    return nodes,edges

  if nodes is None:
    nodes = []
    edges = {}
  if root not in nodes:
    nodes.append(root)
  if root in inputEdges:
    for outNode in inputEdges[root]:
      if root not in edges:
        edges[root] = []
      if outNode not in edges[root]: # TODO: use a set / dict here??
        edges[root].append( outNode )
      extractTree( inputEdges, outNode, nodes, edges, 1 + depth )
  return nodes, edges

def flattenTree( inputEdges, root, nodes_lvl=None, edges_lvl=None, depth=0 ):
  """ Recursively duplicate nodes to flatten the DAG into a simpler tree
  """

  if depth > kGlobalMaxDepth:
    print('WARNING: depth ({0}) unexpectedly large'.format(depth))
    return nodes_lvl,edges_lvl

  global flattenTreeNodeCtr

  if depth == 0:
    flattenTreeNodeCtr = 0

  root_lvl = '{0}__{1}'.format(root, flattenTreeNodeCtr)

  if nodes_lvl is None:
    nodes_lvl = []
    edges_lvl = {}

  if root_lvl not in nodes_lvl:
    nodes_lvl.append(root_lvl)

  if root in inputEdges:
    for outNode in inputEdges[root]:
      flattenTreeNodeCtr += 1
      outNode_lvl = '{0}__{1}'.format(outNode, flattenTreeNodeCtr)
      if root_lvl not in edges_lvl:
        edges_lvl[root_lvl] = []
      if outNode_lvl not in edges_lvl[root_lvl]:
        edges_lvl[root_lvl].append( outNode_lvl )
      flattenTree( inputEdges, outNode, nodes_lvl, edges_lvl, 1 + depth )
  return nodes_lvl, edges_lvl

def makeGraph( nodes, edges, ext, startNodes, args ):
  """ Gathers nodes and edges into a graphviz graph"""
  digraph = functools.partial(gv.Digraph, format=ext, engine=kRenderEngine)
  G = digraph()

  G.attr(overlap='false')

  if args.horizontal:
    G.attr(splines='polyline')
    G.attr(rankdir='LR')
    G.attr(concentrate='true')
  else:
    G.attr(splines='true')

  for node in nodes:
    behavior_name = node
    if node.find('__') > 0:
      behavior_name = node.split('__')[0]
    if node in startNodes:
      G.attr('node', shape='box', fillcolor='black', fontcolor='white', style='filled', label=behavior_name, fontsize=font_size)
    else:
      G.attr('node', shape='box', fillcolor='white', fontcolor='black', label=behavior_name, fontsize=font_size)
    G.node( node )
  for node, edgeList in edges.iteritems():

    if args.flatten and args.horizontal and len(edgeList) > 1:
      ## add hidden nodes to straighten edges
      last = None
      with G.subgraph() as g2:
        g2.attr(rank='same')
        for node2 in edgeList:
          joint = '{}_jnt'.format(node2)

          if debugInvisibleJoints:
            G.node( joint, shape='point', label='', width='0.1', height='0.1' )
          else :
            G.node( joint, shape='none', label='', width='0', height='0' )

          if last:
            g2.edge( last, joint, arrowhead='none' )
          G.edge( joint, node2 )
          last = joint

      idx = 0

      ## NOTE:(BN) I tried making it connect to the "middle" joint instead of the first once but it caused
      ## chaos with the layout for some reason (dot file looked fine....), so for now keep it at zero

      # if node == 'HighLevelAI__0':
      #   idx = 0 len(edgeList) / 2
      #   print('{}_jnt'.format( edgeList[ idx ] ))

      middleJoint = '{}_jnt'.format( edgeList[ idx ] )
      G.edge( node, middleJoint, arrowhead='none' )
    else:
      for node2 in edgeList:
        G.edge( node, node2 )

  return G

def tryLoadCpp(filename, nodes, edges):
  """ If the cpp dump file exists, parse it
  todo: anonymousBehaviors?
  """
  if not os.path.isfile(filename):
    return
  with open( filename, 'r' ) as f:
    content = [x.strip() for x in f.readlines()]
  for line in content:
    segs = line.split(' ')
    if len(segs) == 2:
      nodeId = segs[0]
      outId = segs[1]
      if nodeId not in nodes:
        nodes.append(nodeId)
      if nodeId not in edges:
        edges[nodeId] = []
      if outId not in edges[nodeId]:
        edges[nodeId].append(outId)
      if outId not in nodes:
        nodes.append(outId)

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
  parser.add_argument('-r', '--root', action="store", dest='rootBehavior', metavar='ROOTNODE',
                      help='draw a tree from this root node, ignoring other nodes')
  parser.add_argument('-o', '--output', action="store", dest='outputFilename', metavar='OUTPUTFILE',
                      help='output file (extension determines filetype)')
  parser.add_argument('inputFilename', metavar='INPUTFILE', nargs='?',
                      default='./_build/mac/Debug/test/engine/behavior_transitions.txt',
                      help='the file generated by the unit test that dumps all behaviors')
  parser.add_argument('-v', '--view', action='store_true',
                      help='also open the output image file')
  parser.add_argument('-f', '--flatten', action='store_true',
                      help='only valid with -r, flatten the DAG into a tree by duplicating nodes')
  parser.add_argument('-z', '--horizontal', action='store_true',
                      help='plot horizontal tree instead of veritcal (espcially useful with -f on large plot)')
  parser.add_argument('-t', '--text', action="store", dest='textFilename', metavar='TEXTFILE',
                      help='load the given text file and add it as a label node to the output')
  parser.add_argument('-c', '--crop', metavar='BEHAVIOR', nargs='+',
                      help='List (seperated by space) behavior to "crop" (i.e. don\'t descend into)')

  args = parser.parse_args()

  if args.crop and not args.rootBehavior:
    print('Cannoy specify crop without root behavior')
    sys.exit(1)

  inputFilename = args.inputFilename
  outputFilename = args.outputFilename
  textFilename = args.textFilename
  rootBehavior = args.rootBehavior

  if not os.path.isfile(inputFilename):
    print('Cannot find input file ' + inputFilename)
    sys.exit(1)
  if outputFilename:
    if len(outputFilename) < 3 or '.' not in outputFilename:
      print('Cannot determine output format from extension')
      sys.exit(1)

  nodes = []
  edges = {}

  tryLoadCpp( inputFilename, nodes, edges )

  # if we're only showing a partial tree in the graph, find that tree

  if rootBehavior:
    if rootBehavior not in edges:
      print( 'Root {0} not found in edges (if it exists, does it have outbound edges?)'.format(rootBehavior) )
      sys.exit(1)

    if args.crop:
      edges = cropTree( edges, rootBehavior, args.crop )

    if args.flatten:
      nodes, edges = flattenTree( edges, rootBehavior )
    else:
      nodes, edges = extractTree( edges, rootBehavior )

  # makes the job of graphviz easier if unconnected nodes are ignored

  if not kShowDisconnectedNodes:
    # remove nodes with no edges
    nodes = [node for node in nodes if node in edges]

  # identify source nodes

  nonStartNodes = set()
  for node, edgeList in edges.iteritems():
    for outNode in edgeList:
      nonStartNodes.add(outNode)
  startNodes = [x for x in nodes if x not in nonStartNodes]

  # render

  ext = 'pdf'
  if outputFilename:
    idx = outputFilename.rindex('.') + 1
    ext = outputFilename[idx:]

  G = makeGraph( nodes, edges, ext, startNodes, args )

  if textFilename:
    with open(textFilename, 'r') as infile:
      text = infile.read()
      G.node( 'description', style='dashed', label=text, rank='min' )

  if outputFilename:
    fname = outputFilename
  else:
    tempFile = tmp.NamedTemporaryFile(delete=False)
    fname = tempFile.name + '.' + ext

  writeSuccess = False
  unflattenRet = 0
  preserveDotFile = True

  try:
    with tmp.NamedTemporaryFile(delete=not preserveDotFile) as dotFile:
      if preserveDotFile:
        print('writing raw (before unflattening) dot file to {}'.format(dotFile.name))

      dotFile.write( str(G) )
      dotFile.flush()

      if not args.flatten:
        # run unflatted on it to avoid a (SUPER WIDE by short) aspect ratio
        cmd = 'unflatten -c 2 {0} | {1} -T{2} -o {3}'.format(dotFile.name, kRenderEngine, ext, fname)
        ## cmd = 'unflatten -c 4 -l 2 -f {0} | {1} -T{2} -o {3}'.format(dotFile.name, kRenderEngine, ext, fname)
      else:
        cmd = 'cat {0} | {1} -T{2} -o {3}'.format(dotFile.name, kRenderEngine, ext, fname)

      unflattenRet = subprocess.call(cmd, shell=True)
      writeSuccess = (unflattenRet == 0)

      dotFile.close()
  finally:
    if not outputFilename:
      tempFile.close()

  if writeSuccess and not outputFilename:
    print( 'Output saved to {0}'.format( fname ) )
    if args.view:
      cmd = 'open {}'.format(fname)
      subprocess.call(cmd, shell=True)
  elif not writeSuccess:
    print('Error saving file ' + fname)
    sys.exit(unflattenRet)

if __name__ == "__main__":
  main()
