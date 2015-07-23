#!/usr/bin/env python

from __future__ import absolute_import
from __future__ import print_function

import inspect
import os
import sys
import textwrap

def _modify_path():
    currentpath = os.path.dirname(inspect.getfile(inspect.currentframe()))
    searchpath = os.path.join(currentpath, '..', '..', 'tools', 'anki-util', 'tools', 'message-buffers')
    searchpath = os.path.normpath(os.path.abspath(os.path.realpath(searchpath)))
    if searchpath not in sys.path:
        sys.path.insert(0, searchpath)
_modify_path()

from clad import ast
from clad import clad
from emitters import CPP_emitter

class UnionDeclarationEmitter(ast.NodeVisitor):
    "An emitter that generates the handler declaration."
    
    def __init__(self, buf=sys.stdout, include_extension=None):
        self.buf = buf
    
    def visit_UnionDecl(self, node):
        for member in node.members():
            self.buf.write('void Process_{member_name}(const {member_type}& msg);\n'.format(
                member_name=member.name, member_type=CPP_emitter.cast_type(member.type)))

if __name__ == '__main__':
    from clad import emitterutil
    from emitters import CPP_emitter
    
    emitterutil.c_main(language='C++', extension='_declarations.def',
        emitter_types=[UnionDeclarationEmitter],
        use_inclusion_guards=False)
