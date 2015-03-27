#! /usr/bin/python

# here is a quick snippet to allow including the parser stuff.

import os, sys, inspect
# realpath() will make your script run, even if you symlink it :)
cmd_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

# use this if you want to include modules from a subfolder
library_folder = os.path.realpath(os.path.join(cmd_folder, "../../../lib/anki/cozmo-engine/tools/message-buffers"))
for folder_name in ('clad', 'emitters'):
    folder = os.path.join(library_folder, folder_name)
    if folder not in sys.path:
        sys.path.insert(0, folder)

import clad
import ast
import sys
import textwrap

class UnionSwitchEmitter(ast.NodeVisitor):
    "An emitter that generates the handler switch statement."
    
    def __init__(self, buf=sys.stdout):
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
        switch(msg.GetType()) {{
        default:
        \tProcessBadType_{union_name}(msg.GetType());
        \tbreak;
        #''')[:-1].format(**globals))
    
    def writeFooter(self, node, globals):
        self.buf.write('}\n')
        
    def writeMemberCases(self, node, globals):
        for member in node.members():
            self.buf.write(textwrap.dedent('''\
                case {qualified_union_name}::Type::{member_name}:
                \tProcess_{member_name}(msg.Get_{member_name}());
                #''')[:-1].format(member_name=member.name, **globals))

if __name__ == '__main__':
    import emitterutil
    emitterutil.c_main(language='C++',
        emitters=[UnionSwitchEmitter],
        use_inclusion_guards=False)
