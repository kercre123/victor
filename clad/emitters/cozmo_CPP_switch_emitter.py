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

class UnionSwitchEmitter(ast.NodeVisitor):
    "An emitter that generates the handler switch statement."
    
    def __init__(self, buf=sys.stdout, include_extension=None):
        self.buf = buf
    
    def visit_UnionDecl(self, node):
        globals = dict(
            union_name=node.name,
            qualified_union_name=node.fully_qualified_name(),
        )
        
        self.writeHeader(node, globals)
        self.writeMemberCases(node, globals)
        self.writeFooter(node, globals)
        
    def writeHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
        switch(msg.GetTag()) {{
        default:
        \tProcessBadTag_{union_name}(msg.GetTag());
        \tbreak;
        ''').format(**globals))
    
    def writeFooter(self, node, globals):
        self.buf.write('}\n')
        
    def writeMemberCases(self, node, globals):
        for member in node.members():
            self.buf.write(textwrap.dedent('''\
                case {qualified_union_name}::Tag::{member_name}:
                \tProcess_{member_name}(msg.Get_{member_name}());
                \tbreak;
                ''').format(member_name=member.name, **globals))

if __name__ == '__main__':
    from clad import emitterutil
    from emitters import CPP_emitter
    
    emitterutil.c_main(language='C++', extension='_switch.def',
        emitter_types=[UnionSwitchEmitter],
        use_inclusion_guards=False)
