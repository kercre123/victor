#!/usr/bin/python
#
# purgeDuplicateFaceFrames.py
#
# Desciption: Given a source directory, this script recursively traverses its folders
#             and in each one traverses the png files contained therein in alphanumeric order and deletes
#             all duplicates that occur in a continuous set, leaving only the first instance of that image.
#

import os
import os.path
import shutil
import errno
import sys
import argparse
import string


# executes main script logic
if __name__ == '__main__':
  
  # parse arguments
  parser = argparse.ArgumentParser()
  
  parser.add_argument('image_dir', help='Directory of face images. (Searched recursively)')
  args = parser.parse_args()

  filesToDelete = []

  # Traverse image directory recursively and keep track of duplicate files in filesToDelete
  print 'LIST OF FILES TO BE DELETED'
  print '=================================='
  for root, dirs, files in os.walk(args.image_dir):
    path = root.split('/')
    print 'CHECKING DIR: ', root
    
    # filter out non-png files
    files = [x for x in files if x.endswith('.png')]
    
    # There must be at least 2 files in folder otherwise don't bother
    if len(files) > 1:
      # Get first file
      f = open(root + '/' + files.pop(0), 'rb')
      img1 = f.read()
      f.close()
      for file in files:
        # Compare against next image
        f = open(root + '/' + file, 'rb')
        img2 = f.read()
        f.close()
        
        if img1 == img2:
          # Add to delete list if the same
          filesToDelete.append(os.path.join(root,file))
          print len(path)*'---', file
        else:
          # Update img1 if different
          img1 = img2



  # Confirm files to be deleted
  confirmation = "?"
  validAnswers = ["y", "n"]
  while confirmation not in validAnswers:
    confirmation = string.lower(str(raw_input("Delete these files? [y/n]: ")))

  if confirmation == 'y':
    print 'DELETING FILES'
    for x in filesToDelete:
      print 'Deleting ', x
      os.remove(x)
  else:
    print 'Aborting file deletion'

