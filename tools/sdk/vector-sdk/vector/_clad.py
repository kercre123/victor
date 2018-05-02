#!/usr/bin/env python3

import sys, os
# @TODO: we should import clad properly when we standardize the build process
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'vector-clad', 'clad', 'externalInterface'))
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'message-buffers', 'support', 'python'))

import messageExternalComms
_clad_message = messageExternalComms.Anki.Cozmo.ExternalComms
