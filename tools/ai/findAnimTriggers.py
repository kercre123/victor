#!/usr/bin/env python2

"""
* File: findAnimTriggers.py
*
* Author: ross
* Created: Mar 26 2018
*
* Description: Searches the provided path for any .cpp or .h file that contains
*              the string "AnimationTrigger::"
*              Optionally you can provide a list of behavior class names, and 
*              it will only search in those class files. Python is needed since
*              no other built-in osx tool has negative lookbehind regex.
*   Some useful commands for posterity:
*    List triggers in animationTriggerMap.json
*     $ grep Clad AnimationTriggerMap.json | tr "\"" " " | awk '{print $3}' | sort | uniq
*    List lines in sorted file2 that arent in sorted file1
*     $ comm -13i file1 file2
*    Get the anim groups from the clad events in file.txt using AnimationTriggerMap.json
*     $ while read in; do awk -v pat="$in" '$0~pat{ getline; print $0 }' AnimationTriggerMap.json | tr "\"" " " | awk '{print $3}'; done < file.txt
*    Find and replace animation triggers listed in unused.txt in all files in the engine directory (youll have to remove .bak files)
*     while read trigger; do  grep -rl --include \*.h --include \*.cpp "AnimationTrigger::$trigger" engine | xargs sed -i.bak "s/AnimationTrigger::$trigger/AnimationTrigger::DEPRECATED_$trigger/g";  done < unused.txt
*    Similar, but for json file usage
*     while read trigger; do  grep -rl --include \*.json "\"$trigger\"" resources | xargs sed -i.bak "s/\"$trigger\"/\"DEPRECATED_$trigger\"/g";  done < unused.txt
*    Similar, but for the single .clad file
*     while read trigger; do  sed -i.bak -E "s/([[:space:]]+)($trigger)(,*[[:space:]]*)((\/\/.*)*)/\1DEPRECATED_\2\3\4/g" ./clad/src/clad/types/animationTrigger.clad;  done < unused.txt
*
* Copyright: Anki, Inc. 2018
*
**/
"""



import os
import sys
import re
import argparse

# - - - - - - - - - - - - - - - - - - - - - -
def Fail(msg):
  sys.stderr.write( '{0}\n'.format(msg) )
  sys.exit( 1 )

def ScourFilesForRegex( files, pattern, verbose=True ):
  
  matches = {}

  for file in files:
    filePrinted = False
    for i, line in enumerate(open(file)):
      for match in re.finditer(pattern, line):
        if verbose:
          if not filePrinted:
            filePrinted = True
            print(file)
          print( 'Found on line {0}: {1}'.format(i+1, match.groups()[0]) )
        if match.groups()[0] not in matches:
          matches[match.groups()[0]] = []
        if file not in matches[match.groups()[0]]:
          matches[match.groups()[0]].append( file )
  return matches

# - - - - - - - - - - - - - - - - - - - - - -
def GetFiles( path, extension ):
  """ Returns a list of files in path with extension """

  if not os.path.exists( path ):
    Fail( 'Path does not exist: {0}'.format(path) )

  path = os.path.join( path )
  outFiles = []
  for root, dirs, files in os.walk( path ):
    for file in files:
      if file.endswith( extension ):
        filename = os.path.join( root, file )
        outFiles.append( filename )

  return outFiles

# - - - - - - - - - - - - - - - - - - - - - -
def main():

  parser = argparse.ArgumentParser(description='Find "AnimationTrigger::" usage')
  parser.add_argument('--behaviors', nargs='*', dest='behaviors', help='List of behavior class names')
  parser.add_argument('directory', help='directory to search recursively for .cpp and .h')
  parser.add_argument('-v', '--verbose', action='store_true', dest='verbose')

  args = parser.parse_args()

  directory = args.directory
  behaviors = args.behaviors
  verbose = args.verbose

  files = GetFiles( directory, '.cpp' )
  files += GetFiles( directory, '.h' )

  if behaviors:
    behaviors = [beh.lower() if beh.lower().startswith("behavior") else ("behavior" + beh.lower()) for beh in behaviors]
    tmp = []
    for file in files:
      match = False
      for beh in behaviors:
        if beh in file.lower():
          tmp.append(file)
          match = True
          break
      if verbose and not match:
        print('skipping ' + file)
    files = tmp


  pattern = re.compile("(?<!Cube)AnimationTrigger::([a-zA-Z0-9_]+)")
  matches = ScourFilesForRegex( files, pattern, verbose )
  
  matches.pop('Count', None)

  for key,val in sorted(matches.iteritems()):
    if verbose:
      print( '{0}: {1}'.format(key, val) )
    else:
      print( key )

# - - - - - - - - - - - - - - - - - - - - - -
if __name__ == "__main__":
  main()
