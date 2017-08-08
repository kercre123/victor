#!/usr/bin/env python2

# Based off answer at http://stackoverflow.com/questions/2412816/how-can-i-get-the-source-lines-inline-with-the-assembly-output-using-gcc

import re
import sys
import os
import time

def addSource(inputFilename, outputFilename, silent):
  f = open(inputFilename)
  lines = f.readlines()
  f.close()

  FILE_RE=re.compile(r"\s+\.file (\d+) \"(.*)\"")
  LOC_RE =re.compile(r"\s+\.loc (\d+) (\d+)")

  if not silent:
    print "Commenting " + file + " which has " + str(len(lines)) + " lines...",
    #print("Parsing" + len(lines) + lines)

  output = []
  files = {}

  numIncludedFiles = 0
  for i, line in enumerate(lines):
    mo = FILE_RE.match(line)
    if mo is not None:
       #print(i, line)
       #print(mo.group(1), mo.group(2))
       numIncludedFiles += 1
       if "<built-in>" == mo.group(2):
         files[mo.group(1)] = ""
       else:
         files[mo.group(1)] = [mo.group(2), open(mo.group(2)).readlines()]

      #print mo.group(1),"=",mo.group(2)

    continue

  numCommented = 0
  for line in lines:

    #print(line)

    mo = LOC_RE.match(line)
    #print(mo)

    if mo is not None:
      #print mo.group(1),"=",mo.group(2)
      numCommented += 1
      source = files[mo.group(1)][1][int(mo.group(2))-1]
      
      slashIndex = files[mo.group(1)][0].rfind('/')

      if slashIndex is None:
        slashIndex = -1

      sourceFilename = files[mo.group(1)][0][(slashIndex+1):]

      output.append(line[:-1] + "\t#" + " " + sourceFilename + " - " + source.lstrip())
      #print(source)
    else:
      output.append(line)

  f = open(outputFilename,"w")
  f.writelines(output)
  f.close()

  if not silent:
    print("Parsed " + str(numIncludedFiles) + " inputs files for " + str(numCommented) + " commented lines")

if __name__ == "__main__":
  rootdir = sys.argv[1]

  numCommented = 0

  for root, subFolders, files in os.walk(rootdir):
    for file in files:
      if len(file) > 1 and file[-2] == "." and file[-1] == "s":
        #print("Commenting " + file)
        inputFilename = os.path.join(root, file)
        outputFilename = inputFilename + ".commented"

        inputTime = time.ctime(os.path.getmtime(inputFilename))

        runOnFile = False

        if os.path.exists(outputFilename):
          outputTime = time.ctime(os.path.getmtime(outputFilename))

          if outputTime < inputTime:
            runOnFile = True
        else:
          runOnFile = True

        if runOnFile:
          addSource(inputFilename, outputFilename, True)
          numCommented += 1

  print("Added source comments to " + str(numCommented) + " files")



        
