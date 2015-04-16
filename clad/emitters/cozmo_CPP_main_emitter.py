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
    print(searchpath)
    if searchpath not in sys.path:
        sys.path.insert(0, searchpath)
_modify_path()

if __name__ == '__main__':
    from clad import emitterutil
    from emitters import CPP_emitter
    
    option_parser = emitterutil.StandardArgumentParser('C++')
    option_parser.add_argument('-r', '--header-output-directory',
        help='The directory to output the C++ header file(s) to.')
    option_parser.add_argument('--header-path-prefix', default='',
        help='The prefix to add to all header includes.')
    option_parser.add_argument('--header-output-extension', default='.h',
        help='The extension to use for header files.')
    
    options = option_parser.parse_args()
    if not options.header_output_directory:
        options.header_output_directory = options.output_directory
    
    tree = emitterutil.parse(options)
    
    union_discoverer = CPP_emitter.UnionDiscoverer()
    union_discoverer.visit(tree)
    
    comment_lines = emitterutil.get_comment_lines(options, 'C++')
    
    if union_discoverer.found:
        tag_output_header = emitterutil.get_output_file(options, 'Tag' + options.header_output_extension)
        
        emitterutil.write_c_file(tree=tree, 
            output_directory=options.header_output_directory, output_file=tag_output_header,
            emitter_types=[CPP_emitter.TagHUnionTagEmitter, CPP_emitter.TagHHashEmitter],
            system_headers=['functional'],
            comment_lines=comment_lines,
            use_inclusion_guards=True)
        
        main_output_header_local_headers = [options.header_path_prefix + tag_output_header]
    else:
        main_output_header_local_headers = None
    
    main_output_header = emitterutil.get_output_file(options, options.header_output_extension)
    emitterutil.write_c_file(tree=tree,
        output_directory=options.header_output_directory, output_file=main_output_header,
        emitter_types=[CPP_emitter.HNamespaceEmitter],
        system_headers=['array', 'cassert', 'cstdint', 'string', 'vector', 'CLAD/SafeMessageBuffer.h'],
        local_headers=main_output_header_local_headers,
        comment_lines=comment_lines,
        use_inclusion_guards=True)
    
    main_output_source = emitterutil.get_output_file(options, '.cpp')
    emitterutil.write_c_file(tree=tree,
        output_directory=options.output_directory, output_file=main_output_source,
        emitter_types=[CPP_emitter.CPPNamespaceEmitter],
        local_headers=[options.header_path_prefix + main_output_header],
        comment_lines=comment_lines,
        use_inclusion_guards=False)
