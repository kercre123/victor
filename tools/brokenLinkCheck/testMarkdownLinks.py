#!/usr/bin/env python3

# testMarkdownLinks.py
# Author: Matt Michini
# Copyright 2018 Anki, Inc.
# Description: Search for *.md files as specified in the config file
#              and check them for broken external and internal links.

import os, sys, subprocess
import glob, re, json

if len(sys.argv) != 2:
  print("Usage: {} <config_file>".format(os.path.basename(sys.argv[0])))
  sys.exit(1)

gitRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).decode('utf-8').strip()

# load config
configFile = sys.argv[1]
config = json.load(open(os.path.join(gitRoot, configFile),'r'))

checkExternalLinks = config['testExternalLinks']
dirsToSearch = config['directoriesToCheck']

# Grab all matching md files
files = []
for dirEntry in dirsToSearch:
  dir = os.path.join(gitRoot, dirEntry['dirName'])
  recursive = dirEntry['recursive']
  searchPath = os.path.join(dir, '**/*.md' if recursive else '*.md')
  files = files + glob.glob(searchPath, recursive=recursive)

print("Checking {} markdown files for broken links, {}including external links.".format(len(files), "" if checkExternalLinks else "NOT "))

failedFiles = set()
for file in files:
  repoRelativeFilePath = os.path.relpath(file, start=gitRoot)
  print("checking file \"{}\"".format(repoRelativeFilePath))
  
  if checkExternalLinks:
    try:
      output = subprocess.check_output(['awesome_bot', file, '--allow-redirect', '--skip-save-results', '--allow-dupe', '--allow-ssl', '-w', 'github.com/anki']).decode('utf-8')
    except FileNotFoundError:
      print("ERROR: awesome_bot: command not found. Try \"gem install awesome_bot\"")
      sys.exit(1)
    except subprocess.CalledProcessError as e:
      print("  Broken external link. " + "awesome_bot output:\n\n" + e.output.decode('utf-8'))
      failedFiles.add(file)
  
  # Check internal (repository) links
  excludeTerms = ['://', 'mailto:']
  with open(file, 'r', encoding='utf-8') as f:
    for num, line in enumerate(f, 1):
      # find pattern "[link text](some/link)"
      for result in re.finditer("\[([^\[\]]+)\]\(([^)]+)\)", line):
        matchedText, linkText, linkPath = result.group(0), result.group(1), result.group(2)
        if not any(x in linkPath for x in excludeTerms):
          # remove any trailing "#" references and any trailing '/' characters
          linkPath = linkPath.split('#')[0].rstrip('/')
          if not linkPath:
            continue
          if linkPath[0] == '/':
            # 'absolute' repo link
            fullPath = os.path.join(gitRoot, linkPath.lstrip('/'))
          else: 
            # 'relative' repo link
            fullPath = os.path.join(os.path.dirname(file), linkPath)
          # Check if this file/folder exists (use listdir for case sensitivity):
          dirName, baseName = os.path.split(fullPath)
          if not os.path.exists(fullPath) or not baseName in os.listdir(dirName):
            print("  Broken link in file \"{}\", line {}: \"{}\". File or folder \"{}\" does not exist".format(repoRelativeFilePath, num, matchedText, os.path.relpath(fullPath, start=gitRoot)))
            failedFiles.add(file)

if failedFiles:
  print("\nERROR: The following files contain broken links:")
  for file in failedFiles:
    print("  " + os.path.relpath(file, start=gitRoot))
else:
  print("No broken links detected.")

sys.exit(1 if failedFiles else 0)

