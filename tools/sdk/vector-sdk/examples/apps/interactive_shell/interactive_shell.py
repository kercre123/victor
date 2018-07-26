#!/usr/bin/env python3

# Copyright (c) 2018 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Command Line Interface for Vector

This is an example of integrating Vector with an ipython-based command line interface.
'''

import argparse
from pathlib import Path
import os
import sys

try:
    from IPython.terminal.embed import InteractiveShellEmbed
except ImportError:
    sys.exit('Cannot import from ipython: Do `pip3 install ipython` to install')

import vector

usage = ('This is an IPython interactive shell for Vector.\n'
         'All commands are executed within Vector\'s running connection loop.\n'
         'Use the [tab] key to auto-complete commands, and see all available methods.\n'
         'All IPython commands work as usual. See below for some useful syntax:\n'
         '  ?         -> Introduction and overview of IPython\'s features.\n'
         '  object?   -> Details about \'object\'.\n'
         '  object??  -> More detailed, verbose information about \'object\'.')


parser = argparse.ArgumentParser()
parser.add_argument("-n", "--name", nargs='?', default=os.environ.get('VECTOR_ROBOT_NAME', None))
parser.add_argument("-i", "--ip", nargs='?', default=os.environ.get('VECTOR_ROBOT_IP', None))
parser.add_argument("-c", "--cert_file", nargs='?', default=os.environ.get('VECTOR_ROBOT_CERT', None))
parser.add_argument("--port", nargs='?', default="443")
args = parser.parse_args()

cert = Path(args.cert_file)
args.cert = cert.resolve()

ipyshell = InteractiveShellEmbed(banner1='\nWelcome to the Vector Shell!',
                                 exit_msg='Goodbye\n')


with vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
    # Invoke the ipython shell while connected to vector
    ipyshell(usage)
