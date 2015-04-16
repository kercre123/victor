#!/usr/bin/env python

from __future__ import absolute_import
from __future__ import print_function

import inspect
import os
import sys
import textwrap

def _modify_path():
    currentpath = os.path.dirname(inspect.getfile(inspect.currentframe()))
    searchpath = os.path.join(currentpath, '..', '..', 'lib', 'anki', 'cozmo-engine', 'tools', 'message-buffers')
    searchpath = os.path.normpath(os.path.abspath(os.path.realpath(searchpath)))
    if searchpath not in sys.path:
        sys.path.insert(0, searchpath)
_modify_path()

if __name__ == '__main__':
    from clad import emitterutil
    from emitters import CSharp_emitter

    language = 'C#'
    extension = '.cs'
    
    option_parser = emitterutil.StandardArgumentParser(language)
    option_parser.add_argument('--output-file-override',
        help='The actual file path to write to, overriding other options.')
    
    options = option_parser.parse_args()
    
    tree = emitterutil.parse(options)
    
    output_file = emitterutil.get_output_file(options, extension)
    if options.output_file_override is not None:
        output_file = options.output_file_override
        output_file = emitterutil.make_path_relative(output_file, options.output_directory)
    
    comment_lines = emitterutil.get_comment_lines(options, language)
    
    emitter_types = [CSharp_emitter.NamespaceEmitter]
    use_inclusion_guards = False
    
    emitterutil.write_c_file(tree, options.output_directory, output_file, emitter_types,
        None, None, None,
        comment_lines, use_inclusion_guards)
