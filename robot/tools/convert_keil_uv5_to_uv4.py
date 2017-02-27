#!/usr/bin/python

from __future__ import print_function #print compatibility for 2.7
import sys
import xml.etree.ElementTree as ET

if len(sys.argv) != 3:
  raise NameError('Usage: ' + sys.argv[0] + ' [uvprojx file] [uvproj file]')

projxFile = sys.argv[1].rstrip()
projFile = sys.argv[2].rstrip()
print("Export", projxFile, "->", projFile)

# open projx:
tree = ET.parse(projxFile)
root = tree.getroot()

# change some stuff:
root.find('SchemaVersion').text = '1.1'
root.remove(root.find('RTE'))

# overwrite proj file
tree.write(projFile, encoding='utf-8', method='html', xml_declaration=True)

# tree.write() doesn't write the one-line xml declaration header for some reason, so copy it from projx:
with open(projxFile) as projx:
  with open(projFile, 'r+') as proj:
    header = projx.readline()
    theRest = proj.read()
    proj.seek(0)
    proj.write(header + theRest)
    proj.truncate()
