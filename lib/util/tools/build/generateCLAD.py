#!/usr/bin/env python2

import subprocess
import os.path

projectRoot = subprocess.check_output(['git', 'rev-parse', '--show-toplevel']).rstrip("\r\n")

print '** Generating source from CLAD files **'
result = subprocess.call(['make', '-C', os.path.join(projectRoot, 'source/anki/messageBuffers'), 'OUTPUT_DIR={0}'.format('..'), 'cpp'])
if result != 0:
  print "Failed to generate message-buffer sources"
  exit(1)
