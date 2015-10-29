#!/usr/bin/env python

from __future__ import absolute_import
from __future__ import print_function

import inspect
import os
import sys
import textwrap

def _modify_path():
    currentpath = os.path.dirname(inspect.getfile(inspect.currentframe()))
    searchpath = os.path.join(currentpath, '..')
    searchpath = os.path.normpath(os.path.abspath(os.path.realpath(searchpath)))
    if searchpath not in sys.path:
        sys.path.insert(0, searchpath)
_modify_path()

from clad import ast
from clad import clad
from clad import emitterutil

type_translations = {
    'bool': 'uint8_t',
    'int_8': 'int8_t',
    'int_16': 'int16_t',
    'int_32': 'int32_t',
    'int_64': 'int64_t',
    'uint_8': 'uint8_t',
    'uint_16': 'uint16_t',
    'uint_32': 'uint32_t',
    'uint_64': 'uint64_t',
    'float_32': 'float',
    'float_64': 'double'
}

size_t = 'size_t'
byte = type_translations['uint_8']

class HGlobalEmitter(ast.NodeVisitor):
    
    def __init__(self, buf=sys.stdout, include_extension='.h'):
        self.buf = buf
        self.include_extension = include_extension
    
    def visit_IncludeDecl(self, node, *args, **kwargs):
        new_header_file_name = emitterutil.get_included_file(node.name, self.include_extension)
        self.buf.write('#include "{0}"\n\n'.format(new_header_file_name))

    def visit_EnumDecl(self, node, *args, **kwargs):
        HEnumEmitter(self.buf).visit(node, *args, **kwargs)
    
    def visit_MessageDecl(self, node, *args, **kwargs):
        HStructEmitter(self.buf).visit(node, *args, **kwargs)
    
    def visit_UnionDecl(self, node, *args, **kwargs):
        HUnionEmitter(self.buf).visit(node, *args, **kwargs)

class HEnumEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_EnumDecl(self, node):
        # print the header
        self.buf.write('// ENUM {enum_name}\n'.format(enum_name=node.name))
        self.buf.write('enum {\n')
        
        starts = []
        ends = []
        enum_val = 0
        for i, member in enumerate(node.members()):
            start = '\t{enum_name}_{member_name}'.format(enum_name=node.name, member_name=member.name)
            if member.initializer:
                enum_val = member.initializer.value
                initializer = hex(enum_val) if member.initializer.type == "hex" else str(enum_val)
                start += ' = {initializer}'.format(initializer=initializer)
            if i < len(node.members()) - 1:
                start += ','
            
            end = ' // {value}\n'.format(value=enum_val)
            enum_val += 1
            
            starts.append(start)
            ends.append(end)
        
        full_length = max(len(start) for start in starts)
        for start, end in zip(starts, ends):
            self.buf.write(start)
            self.buf.write(' ' * (full_length - len(start)))
            self.buf.write(end)
        
        self.buf.write('};\n')
        
        enum_storage = type_translations[node.storage_type.builtin_type().name]
        self.buf.write('typedef {enum_storage} {enum_name};\n\n'.format(enum_name=node.name, enum_storage=enum_storage))
        self.buf.write('\n')

    def emitEnum(self, member, enum_val, trailing_comma=True, prefix=''):
        separator = ',' if trailing_comma else ''
        if not member.initializer:
            self.buf.write('\t{prefix}{member_name}{separator}\t// {value}\n'.format(
                prefix=prefix, member_name=member.name, separator=separator, value=enum_val))
        else:
            initial_value = member.initializer
            enum_val = initial_value.value
            enum_str = hex(enum_val) if initial_value.type == "hex" else str(enum_val)
            self.buf.write('\t{prefix}{member_name} = {assignment}{separator}\t// {value}\n'.format(
                prefix=prefix, member_name=member.name, separator=separator, assignment=enum_str, value=enum_val))
        return enum_val + 1

class HStructEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_MessageDecl(self, node):
        self.checkFixedLength(node)
        self.checkSubUnions(node)
        self.checkAlignment(node)
        self.checkForGaps(node)
        
        self.emitStruct(node)
        self.emitMethods(node)
        
        self.buf.write('\n')
    
    def checkFixedLength(self, node):
        for member in node.members()[:-1]:
            if not member.type.is_message_size_fixed():
                sys.exit('Error: All struct members, other than the last, must be fixed length.')
    
    def checkSubUnions(self, node):
        for member in node.members():
            if isinstance(member.type, ast.CompoundType):
                if isinstance(member.type.type_decl, ast.UnionDecl) and member.type.alignment() > 1:
                    sys.exit('Error: Unable to nest union types in C emitter due to padding issues.')
    
    def checkEmptyStructsInStructs(self, node):
        for member in node.members():
            if isinstance(member.type, ast.CompoundType):
                if isinstance(member.type.type_decl, ast.MessageDecl) and member.type.max_message_size() == 0:
                    sys.exit('Error: Unable to have 0-length structs as members of structs ' +
                        'in C emitter due to padding issues.')
    
    def checkAlignment(self, node):
        "Ensures that all members are properly aligned so that doubles only appear on 8-byte boundaries, etc."
        currentOffset = 0
        for member in node.members():
            if currentOffset % member.type.alignment() != 0:
                sys.exit('Error: Cannot put a {member_type_name} at byte offset {current_offset} in a C struct. ' +
                    '(It has alignment {member_alignment} and may get padded.) ' +
                    'You can place members in order of decreasing alignment to avoid alignment issues.').format(
                    member_type_name=member.type.name,
                    current_offset=currentOffset,
                    member_alignment=member.type.alignment())
            currentOffset += member.type.max_message_size()
    
    def checkForGaps(self, node):
        """
        Ensures that structs used as members have a size that is a multiple of their alignment,
        so that there are no gaps in the current struct layout.
        (An exception is made for the last member unless it is in an array of length 2 or more.)
        """
        for i, member in enumerate(node.members()):
            gaps_allowed = (i == len(node.members()) - 1)
            
            member_type = member.type
            while isinstance(member_type, ast.FixedArrayType):
                gaps_allowed = gaps_allowed and member.type.length <= 1
                member_type = member_type.member_type
            
            if not gaps_allowed and isinstance(member_type, ast.CompoundType):
                if isinstance(member_type.type_decl, ast.UnionDecl):
                    member_size = None
                    for union_member in member_type.type_decl.members():
                        if member_size is not None and member_size != union_member.type.max_message_size():
                            sys.exit('Error: The C emitter does not support union members of varying length, except at the end of a struct. (Otherwise gaps could occur.)')
                elif isinstance(member_type.type_decl, ast.MessageDecl):
                    if member_type.max_message_size() % member_type.alignment() != 0:
                        sys.exit('Error: The C emitter does not support struct members that have trailing padding, except at the end of a struct.')
                else:
                    # unknown case
                    pass
    
    def emitStruct(self, node):
        self.buf.write('// {obj_type} {struct_name}\n'.format(
            obj_type=node.object_type().upper(), struct_name=node.name))
        self.buf.write('typedef struct\n{\n')
        
        is_empty = True
        if node.members():
            visitor = MemberVisitor(buf=self.buf)
            for member in node.members():
                self.buf.write('\t')
                if member.type.max_message_size() > 0:
                    is_empty = False
                else:
                    self.buf.write('// ')
                visitor.visit(member.type, member_name=member.name)
                self.buf.write(';\n')
        
        if is_empty:
            self.buf.write('\t// To conform to C99 standard (6.7.2.1)\n')
            self.buf.write('\tchar _empty;\n')
        
        self.buf.write('}} {struct_name};\n\n'.format(struct_name=node.name))
    
    def emitMethods(self, node):
        
        # sanity assert: alignment is power-of-two
        assert(node.alignment() != 0)
        assert((node.alignment() & (node.alignment() - 1)) == 0)
        
        globals = dict(
            struct_name=node.name,
            max_size=node.max_message_size(),
            alignment=node.alignment(),
            size_t=size_t,
            byte=byte)
        
        self.buf.write(textwrap.dedent('''\
            static const {size_t} {struct_name}_MaxSize = {max_size};

            static inline {size_t} {struct_name}_Size(const {struct_name}* value)
            {{
            ''').format(**globals))
        
        if node.is_message_size_fixed():
            self.buf.write('\t(void)value; // suppress warning\n')
            self.buf.write('\treturn {max_size};\n'.format(**globals))
        else:
            self.buf.write('\treturn ')
            
            visitor = SizeVisitor(buf=self.buf)
            if node.members():
                for i, member in enumerate(node.members()):
                    if i > 0:
                        self.buf.write(' +\n\t\t')
                    visitor.visit(member.type, member_name=('value->' + member.name))
            else:
                self.buf.write('0')
            self.buf.write(';\n')
            
        self.buf.write('}\n\n')
        
    def visit_UnionDecl(self, node):
        #UnionEmitter().visit(node)
        pass

class HUnionEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_UnionDecl(self, node):
        self.checkSubUnions(node)
        
        globals = dict(
            union_name=node.name,
            max_size=node.max_message_size(),
            padding_length=node.alignment() - node.tag_storage_type.max_message_size(),
            size_t=size_t,
            byte=byte,
            enum_storage=byte,
            tag_name='{0}Tag'.format(node.name),
            tag_member='tag')
        
        self.buf.write('// UNION {union_name}\n'.format(**globals))
        self.emitEnum(node, globals)
        self.emitStruct(node, globals)
        self.emitMethods(node, globals)
        self.buf.write('\n')
    
    def checkSubUnions(self, node):
        for member in node.members():
            if isinstance(member.type, ast.CompoundType):
                if isinstance(member.type.type_decl, ast.UnionDecl) and member.type.alignment() > 1:
                    sys.exit('Error: Unable to nest union types in C emitter due to padding issues.')
        
    def emitEnum(self, node, globals):
        self.buf.write('enum {\n')
        
        # reserve max value for INVALID
        invalid_value = node.tag_storage_type.max
        if len(node.members()) > invalid_value:
            sys.exit('Error: Union {union_name} has more than {tag_max} members.'.format(
                tag_max=invalid_value, **globals))
        
        starts = []
        ends = []
        for i, member in enumerate(node.members()):
            starts.append('\t{tag_name}_{member_name},'.format(member_name=member.name, **globals))
            ends.append(' // {value}\n'.format(value=i))
        
        starts.append('\t{tag_name}_INVALID'.format(**globals))
        ends.append('\n')
        
        full_length = max(len(start) for start in starts)
        for start, end in zip(starts, ends):
            self.buf.write(start)
            self.buf.write(' ' * (full_length - len(start)))
            self.buf.write(end)
        
        self.buf.write('};\n')
        self.buf.write('typedef {enum_storage} {tag_name};\n\n'.format(**globals))
    
    def emitStruct(self, node, globals):
        self.buf.write('typedef struct {\n')
        
        if globals['padding_length'] > 0:
            self.buf.write('\t{byte} _padding[{padding_length}];\n'.format(**globals))
        self.buf.write('\t{tag_name} {tag_member};\n'.format(**globals))
        
        self.buf.write('\tunion {\n')
        visitor = MemberVisitor(buf=self.buf)
        for member in node.members():
            self.buf.write('\t\t')
            visitor.visit(member.type, member_name=member.name)
            self.buf.write(';\n')
        self.buf.write('\t};\n')
        
        self.buf.write('}} {union_name};\n\n'.format(**globals))
    
    def emitMethods(self, node, globals):
        
        # sanity assert: alignment is power-of-two
        assert(node.alignment() != 0)
        assert((node.alignment() & (node.alignment() - 1)) == 0)
        
        self.buf.write(textwrap.dedent('''\
            static const {size_t} {union_name}_MaxSize = {max_size};

            static inline {size_t} {union_name}_Size(const {union_name}* value)
            {{
            ''').format(**globals))
            
        if node.is_message_size_fixed():
            self.buf.write('\t(void)value; // suppress warning\n')
            self.buf.write('\treturn {max_size};\n'.format(**globals))
        else:
            self.buf.write('\tswitch (value->{tag_member})\n\t{{\n'.format(**globals))
        
            visitor = SizeVisitor(buf=self.buf)
            for member in node.members():
                self.buf.write('\tcase {tag_name}_{member_name}:\n'.format(member_name=member.name, **globals))
            
                self.buf.write('\t\treturn 1 + ')
                visitor.visit(member.type, member_name=('value->' + member.name))
                self.buf.write(';\n')
            
            self.buf.write('\tdefault:\n')
            self.buf.write('\t\treturn 1;\n')
            self.buf.write('\t}\n')
            
        self.buf.write(textwrap.dedent('''\
            }}

            static inline {byte}* {union_name}_Cast({union_name}* value)
            {{
            \treturn (({byte}*)&value->{tag_member});
            }}

            ''').format(**globals))

class MemberVisitor(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf
    
    def visit_PascalStringType(self, member, member_name):
        self.visit_VariableArrayType(member, member_name, member_type=ast.builtin_types['int_8'])

    def visit_BuiltinType(self, node, member_name):
        #Visit a builtin type for the message member decl
        self.buf.write("{type} {member_name}".format(
            type=type_translations[node.name], member_name=member_name))

    def visit_DefinedType(self, node, member_name):
        self.buf.write("{type} {member_name}".format(
            type=node.name, member_name=member_name))

    def visit_CompoundType(self, node, member_name):
        self.buf.write("{type} {member_name}".format(
            type=node.name, member_name=member_name))

    def visit_VariableArrayType(self, node, member_name):
        sys.exit('C emitter does not support variable-length arrays.')
    
    def visit_FixedArrayType(self, node, member_name):
        self.visit(node.member_type, member_name=member_name)
        self.buf.write("[{fixed_length}]".format(fixed_length=node.length))

class SizeVisitor(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, accessor=''):
        self.buf = buf
        self.accessor = accessor
    
    def visit_PascalStringType(self, member, member_name):
        sys.exit('C emitter does not support variable-length strings.')

    def visit_BuiltinType(self, node, member_name):
        self.buf.write('{fixed_size}'.format(fixed_size=node.max_message_size()))

    def visit_DefinedType(self, node, member_name):
        self.buf.write('{fixed_size}'.format(fixed_size=node.max_message_size()))

    def visit_CompoundType(self, node, member_name):
        if node.is_message_size_fixed():
            self.buf.write('{fixed_size}'.format(fixed_size=node.max_message_size()))
        else:
            self.buf.write('{member_storage}_Size(&{accessor}{member_name})'.format(
                member_name=member_name, accessor=self.accessor, member_storage=node.name))

    def visit_VariableArrayType(self, node, member_name):
        sys.exit('C emitter does not support variable-length arrays.')
    
    def visit_FixedArrayType(self, node, member_name):
        self.visit(node.member_type, member_name=member_name)
        self.output.write(' * {fixed_length}'.format(fixed_length=node.length))

if __name__ == '__main__':
    from clad import emitterutil
    emitterutil.c_main(language='C', extension='.h',
        emitter_types=[HGlobalEmitter],
        allow_custom_extension=True, allow_override_output=False,
        use_inclusion_guards=True,
        system_headers=['stddef.h', 'stdint.h'])
