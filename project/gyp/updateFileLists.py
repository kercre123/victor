#!/usr/bin/env python

import fnmatch
import os
import os.path
import re
import subprocess
import sys
import argparse



def main(scriptArgs):
  version = '1.0'
  parser = argparse.ArgumentParser(description='updates gyp list files based on the source files found on disk')
  parser.add_argument('--version', '-V', action='version', version=version)
  parser.add_argument('--debug', dest='debug', action='store_true',
                      help='prints debug output')
  parser.add_argument('--projectRoot', dest='projectRoot', action='store',
                      help='prints debug output')
  (options, args) = parser.parse_known_args(scriptArgs)


  projectRoot = options.projectRoot
  assert(projectRoot)
  os.chdir(os.path.join(projectRoot, 'project/gyp'))

  # target data
  settings = {
      'util': {
        'knownFiles' : [], # this is list of all currently listed sources for this target (from all lst files)
        'foundFiles' : [], # this is a list of fils found on disk that e think belong to this target
        'filesToAdd' : [], # files is 'found' but not in 'know'
        'filesToRemove' : [], # files in 'known' but not in 'found'
        'files' : [
          ['util.lst', []], # second list is the contents of the named file
        ]
      },
      'utilUnitTest': {
        'knownFiles' : [],
        'foundFiles' : [],
        'filesToAdd' : [],
        'filesToRemove' : [],
        'files': [
          ['utilUnitTest.lst', []]
        ]
      },
      'networkApp': {
        'knownFiles' : [],
        'foundFiles' : [],
        'filesToAdd' : [],
        'filesToRemove' : [],
        'files': [
          ['networkApp.lst', []]
        ]
      }
  }

  # clasification rules:
  # any file with /test/i => unittest
  # files within known targets => 'target'.'files'.[0]
  # files in unkown targets => ignore

  # define fnmatch-style shell glob patterns to exclude.
  fn_exclude_patterns = [
    '.*',
    '~*',
    '*~',
    '*.swp',
    '*.orig',
    'version.h',
  ]

  # compile fnmatch patterns into regular expressions
  re_exclude_patterns = [re.compile(fnmatch.translate(p)) for p in fn_exclude_patterns]

  # read in known sources
  for targetKey, targetSettings in settings.iteritems():
    # open each .lst file and read in files into the 'knownFiles' key
    for targetListFileName, targetList in targetSettings['files']:
      if (options.debug):
        print 'reading files from ' + targetListFileName
      
      filename = os.path.join(projectRoot, 'project/gyp', targetListFileName) 
      if os.path.exists(filename):
        with open(filename, 'r') as file:
          for line in file:
            targetSettings['knownFiles'].append(line.rstrip("\r\n"))
            targetList.append(line.rstrip("\r\n"))
      

  # if (options.debug):
  #   print settings

  # find all c/cpp/mm files in source/anki
  for dirPath, dirNames, fileNames in os.walk(os.path.join(projectRoot, 'source/anki')):
    
    #short path starts at 'sources' and ends at .cpp
    shortPath = re.sub(r"^.*(source\/anki)(.*)$", r"\1\2", dirPath)
    # target name is the folder name immediately after 'sources/anki'
    targetName = re.sub(r"^.*source\/anki\/(\w*)(\/.*)?$", r"\1", dirPath)


    # for every found file that matches c/cpp/mm
    for fileName in fileNames:
      # if not ( fileName.endswith('.cpp') or fileName.endswith('.c') or fileName.endswith('.mm') ):
      #   if (options.debug):
      #     print "not a source file " + fileName
      #   continue

      # ignore files that match any of the exclude patterns
      is_ignored = any((exclude_re.match(fileName) is not None) for exclude_re in re_exclude_patterns)
      if is_ignored:
        if (options.debug):
          print("ignoring file: %s" % fileName)
        continue 

      # this is the gyp path
      fullFilePath = os.path.join('../..', shortPath, fileName)

      # fix target names :
      #  - util and metagame => basestation
      fixedTargetName = targetName
      # if ('anki/util' in fullFilePath):
      #   fixedTargetName = 'basestation'
      # if ('anki/metagame' in fullFilePath):
      #   fixedTargetName = 'basestation'
      # if ('anki/shared' in fullFilePath):
      #   fixedTargetName = 'basestation'

      # if this is a known target, process it (aka skip unknown targets)
      if (fixedTargetName in settings):
        # fix target names :
        #  - any file with 'test' => unitTest
        # if ('test' in fullFilePath or 'Test' in fullFilePath):
        #   fixedTargetName = 'utilUnitTest'

        # look through 'knownFiles'
        foundInTarget = ''
        if (fullFilePath in settings[fixedTargetName]['knownFiles']):
          foundInTarget = fixedTargetName
        if (targetName in settings and fullFilePath in settings[targetName]['knownFiles']):
          foundInTarget = targetName

        # not found in 'knownFiles' - add it
        if (foundInTarget == ''):
          if (options.debug):
            print 'new file found: ' + fullFilePath + ' target: ' + fixedTargetName
          settings[fixedTargetName]['filesToAdd'].append(fullFilePath)
        else:
          # put file in 'foundFiles'
          settings[fixedTargetName]['foundFiles'].append(fullFilePath)


  # now check if any know files has been removed
  for targetKey, targetSettings in settings.iteritems():
    for fileName in targetSettings['knownFiles']:
      if ('foundFiles' not in targetSettings or fileName not in targetSettings['foundFiles']):
        # if target is prod test, also check unit test's found files, as they share them
        if (targetKey == 'prodTest'):
          if (fileName not in settings['unitTest']['foundFiles']):
            if (options.debug):
              print 'missing file: ' + fileName + ' target: ' + targetKey
            settings[targetKey]['filesToRemove'].append(fileName)
        else:
          if (options.debug):
            print 'missing file: ' + fileName + ' target: ' + targetKey
          settings[targetKey]['filesToRemove'].append(fileName)

  modifications = False
  # finally write updated file lists
  for targetKey, targetSettings in settings.iteritems():
    if (targetSettings['filesToAdd'] or targetSettings['filesToRemove']):
      targeListFileName, targetList = targetSettings['files'][0]

      # additions
      for fileName in targetSettings['filesToAdd']:
        print 'adding ' + fileName + ' to ' + targeListFileName
        targetList.append(fileName)

      # removals
      for fileName in targetSettings['filesToRemove']:
        print 'removing ' + fileName + ' from ' + targeListFileName
        # some targets have multiple list files, for now we only know how to remove from the FIRST
        # secondary list files need to be updated manually
        assert(fileName in targetList)
        targetList.remove(fileName)

      # write list changes to the file
      file = open(os.path.join(projectRoot, 'project/gyp', targeListFileName), 'w')
      for fileName in targetList:
        file.write(fileName+'\n')
      file.close()
      modifications = True

  if (modifications):
    print "[anki-util] File list changed."
  else:
    print "[anki-util] File list unchanged."

  return modifications



if __name__ == '__main__':

  # go to the script dir, so that we can find the project dir
  # in the future replace this with command line param
  selfPath = os.path.dirname(os.path.realpath(__file__))
  os.chdir(selfPath)

  # find project root path
  projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")
  args = sys.argv
  args.extend(['--projectRoot', projectRoot])
  sys.exit(main(args))


