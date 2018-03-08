#!/usr/bin/env python2

from __future__ import absolute_import
from __future__ import print_function

import inspect
import os
import sys
import textwrap

def _modify_path():
    currentpath = os.path.dirname(inspect.getfile(inspect.currentframe()))
    searchpath = os.path.normpath(os.path.abspath(os.path.join(currentpath, '..', '..')))
    searchpath = os.path.normpath(os.path.abspath(os.path.realpath(searchpath)))
    if searchpath not in sys.path:
        sys.path.insert(0, searchpath)
    sys.path.insert(0, os.path.join('..', '..', 'tools', 'message-buffers'))
_modify_path()

from clad import ast
from clad import clad
from emitters import CPP_emitter

class UnionSwitchEmitter(ast.NodeVisitor):
    "An emitter that generates the handler switch statement."

    groupSwitchPrefix = None
    groupedSwitchMembers = []
    startID = None
    endID = None

    def __init__(self, output=sys.stdout, include_extension=None):
        self.output = output

    def visit_UnionDecl(self, node):
        globals = dict(
            union_name=node.name,
            qualified_union_name=node.fully_qualified_name(),
        )

        self.writeHeader(node, globals)
        self.writeMemberCases(node, globals)
        self.writeFooter(node, globals)

    def writeHeader(self, node, globals):
        pass

    def writeFooter(self, node, globals):
        if self.groupedSwitchMembers:
            for member in self.groupedSwitchMembers:
                self.output.write('case 0x{member_tag:x}:\n'.format(member_tag=member.tag))
            self.output.write('\tProcess_{group_prefix}(msg);\n\tbreak;\n'.format(group_prefix=self.groupSwitchPrefix))

    def writeMemberCases(self, node, globals):
        for member in node.members():
            if self.groupSwitchPrefix is not None and member.name.startswith(self.groupSwitchPrefix):
                self.groupedSwitchMembers.append(member)
            elif self.startID is not None and member.tag < self.startID:
                continue
            elif self.endID is not None and member.tag > self.endID:
                continue
            else:
                self.output.write(textwrap.dedent('''\
                    case 0x{member_tag:x}:
                    \tProcess_{member_name}(msg.{member_name});
                    \tbreak;
                    ''').format(member_tag=member.tag, member_name=member.name, **globals))

if __name__ == '__main__':
    from clad import emitterutil
    from emitters import CPP_emitter

    suffix = '_switch'
    argv = []

    for a in sys.argv:
        if a.startswith('--group='):
            arg, param = a.split('=')
            UnionSwitchEmitter.groupSwitchPrefix = param
            suffix += '_group_' + param
        elif a.startswith('--start='):
            arg, param = a.split('=')
            UnionSwitchEmitter.startID = int(eval(param))
            suffix += '_from_' + param
        elif a.startswith('--end='):
            arg, param = a.split('=')
            UnionSwitchEmitter.endID = int(eval(param))
            suffix += '_to_' + param
        else:
            argv.append(a)
    sys.argv = argv

    emitterutil.c_main(language='C++', extension=suffix+'.def',
        emitter_types=[UnionSwitchEmitter],
        use_inclusion_guards=False)
