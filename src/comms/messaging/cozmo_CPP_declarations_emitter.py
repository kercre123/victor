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

from CPP_emitter import cast_type

class UnionDeclarationEmitter(ast.NodeVisitor):
    "An emitter that generates the handler declaration."
    
    def __init__(self, buf=sys.stdout):
        self.buf = buf
    
    def visit_UnionDecl(self, node):
        for member in node.members():
            self.buf.write('\tvoid Process_{member_name}(const {member_type}& msg);\n'.format(
                member_name=member.name, member_type=cast_type(member.type)))

if __name__ == '__main__':
    import emitterutil
    emitterutil.c_main(language='C++',
        emitters=[UnionDeclarationEmitter],
        use_inclusion_guards=False)
