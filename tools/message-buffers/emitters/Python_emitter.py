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

from clad import clad
from clad import ast

type_translations = {
    'bool': 'b',
    'int_8': 'b',
    'int_16': 'h',
    'int_32': 'i',
    'int_64': 'q',
    'uint_8': 'B',
    'uint_16': 'H',
    'uint_32': 'I',
    'uint_64': 'Q',
    'float_32': 'f',
    'float_64': 'd'
}

support_module = 'msgbuffers'

def sort_key_lowercase(key):
    return (key.lower(), key)

class BaseEmitter(ast.NodeVisitor):
    "Base class for emitters."
    
    def __init__(self, buf=sys.stdout):
        self.buf = buf
    
    # ignore includes unless explicitly allowed
    def visit_IncludeDecl(self, node, *args, **kwargs):
        pass

class Module(object):
    "A record of all the symbols within a module."
    
    def __init__(self, include_name):
        self.name = include_name[:include_name.find('.')].replace('/', '.')
        self.global_namespaces = set()
        self.global_decls = set()
        self.included_modules = []
    
    def include(self, other):
        self.global_namespaces.update(other.global_namespaces)
        self.global_decls.update(other.global_decls)
        self.included_modules.append(other)
    
    def sort_key(self):
        return sort_key_lowercase(self.name)
    
    def get_unique_name(self, name):
        while name in self:
            name = '_' + name
        return name
    
    def is_symbol_in_includes(self, symbol):
        for included_module in self.included_modules:
            if symbol in included_module:
                return True
        return False
    
    def __contains__(self, symbol):
        return (symbol in self.global_namespaces or symbol in self.global_decls)

class IncludeSearcher(ast.NodeVisitor):
    "A visitor that searches through all the symbols, looking for names."
    
    def __init__(self):
        self.all_namespaces = set()
        self.module = Module('__main__')
        self.included_modules = []
    
    def visit_IncludeDecl(self, node):
        saved_module = self.module
        self.module = Module(node.name)
        self.generic_visit(node)
        saved_module.include(self.module)
        self.module = saved_module
    
    def visit_NamespaceDecl(self, node):
        self.all_namespaces.add(node.fully_qualified_name('.'))
        if node.namespace is None:
            self.module.global_namespaces.add(node.name)
        self.generic_visit(node)
    
    def visit_Decl_subclass(self, node):
        if node.namespace is None:
            self.module.global_decls.add(node.name)

class DeclEmitter(BaseEmitter):
    "An emitter that redirects to DeclBodyEmitter after cleaning up stray symbols."
    
    class DeclBodyEmitter(BaseEmitter):
    
        def visit_EnumDecl(self, node, *args, **kwargs):
            EnumEmitter(self.buf, *args, **kwargs).visit(node, *args, **kwargs)
    
        def visit_MessageDecl(self, node, *args, **kwargs):
            MessageEmitter(self.buf, *args, **kwargs).visit(node, *args, **kwargs)
    
        def visit_UnionDecl(self, node, *args, **kwargs):
            UnionEmitter(self.buf, *args, **kwargs).visit(node, *args, **kwargs)

    def __init__(self, buf, module):
        super(DeclEmitter, self).__init__(buf)
        self.module = module
        self.body_emitter = self.DeclBodyEmitter(self.buf)
    
    def visit_NamespaceDecl(self, node, *args, **kwargs):
        self.generic_visit(node, *args, **kwargs)
    
    def visit_Decl_subclass(self, node, *args, **kwargs):
        
        globals = dict(
            decl_name=node.name,
            qualified_decl_name=node.fully_qualified_name('.'),
            saved_name=self.module.get_unique_name('_' + node.name))
        
        if node.namespace:
            need_to_save_name = self.module.is_symbol_in_includes(node.name)
            if need_to_save_name:
                self.buf.write('{saved_name} = {decl_name}\n'.format(**globals))
        
        self.body_emitter.visit(node, *args, **kwargs)
        
        if node.namespace:
            self.buf.write('{qualified_decl_name} = {decl_name}\n'.format(**globals))
            if need_to_save_name:
                self.buf.write('{decl_name} = {saved_name}\n\n'.format(**globals))
            else:
                self.buf.write('del {decl_name}\n\n'.format(**globals))
        
        self.buf.write('\n')
        
class EnumEmitter(BaseEmitter):
    
    def visit_EnumDecl(self, node):
        
        globals = dict(
            support_module=support_module,
            enum_name=node.name,
            enum_type=node.storage_type.name,
        )
        
        self.emitHeader(node, globals)
        self.emitMembers(node, globals)
        
    def emitHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            class {enum_name}(object):
            \t"Automatically-generated {enum_type} enumeration."
            #''')[:-1].format(**globals))

    def emitMembers(self, node, globals):        
        if node.members():
            starts = []
            ends = []
            enum_val = 0
            for member in node.members():
                if member.initializer:
                    enum_val = member.initializer.value
                    initializer = hex(enum_val) if member.initializer.type == "hex" else str(enum_val)
                else:
                    initializer = str(enum_val)
                start = '\t{member_name}'.format(member_name=member.name)
                end = ' = {initializer}\n'.format(initializer=initializer)
                enum_val += 1
                
                starts.append(start)
                ends.append(end)
            
            full_length = max(len(start) for start in starts)
            for start, end in zip(starts, ends):
                self.buf.write(start)
                self.buf.write(' ' * (full_length - len(start)))
                self.buf.write(end)
        
        self.buf.write('\n')

class MessageEmitter(BaseEmitter):
    
    def visit_MessageDecl(self, node):
        
        globals = dict(
            support_module=support_module,
            struct_name=node.name,
        )
        
        self.emitHeader(node, globals)
        self.emitSlots(node, globals)
        self.emitProperties(node, globals)
        self.emitConstructor(node, globals)
        self.emitPackers(node, globals)
        self.emitComparisons(node, globals)
        self.emitLength(node, globals)
        self.emitStr(node, globals)
        self.emitRepr(node, globals)
    
    def emitHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            class {struct_name}(object):
            \t"Generated message-passing {obj_type}."
            
            #''')[:-1].format(obj_type=node.object_type(), **globals))
    
    def emitSlots(self, node, globals):
        if node.members():
            self.buf.write('\t__slots__ = (\n')
            
            starts = []
            ends = []
            for member in node.members():
                starts.append("\t\t'_{member_name}',".format(member_name=member.name))
                ends.append(' # {member_type}\n'.format(member_type=member.type.fully_qualified_name('.')))
            
            full_length = max(len(start) for start in starts)
            for start, end in zip(starts, ends):
                self.buf.write(start)
                self.buf.write(' ' * (full_length - len(start)))
                self.buf.write(end)
            
            self.buf.write('\t)\n\n')
        else:
            self.buf.write('\t__slots__ = ()\n\n')    
    
    def emitProperties(self, node, globals):
        setter_visitor = SetterVisitor(buf=self.buf, depth=2)
        for member in node.members():
            self.buf.write(textwrap.dedent('''\
                \t@property
                \tdef {member_name}(self):
                \t\t"{member_type} {member_name} struct property."
                \t\treturn self._{member_name}
                
                \t@{member_name}.setter
                \tdef {member_name}(self, value):
                #''').format(member_name=member.name, member_type=member.type.fully_qualified_name('.'), **globals)[:-1])
            self.buf.write('\t\tself._{member_name} = '.format(member_name=member.name))
            setter_visitor.visit(
                member.type,
                name="'{struct_name}.{member_name}'".format(member_name=member.name, **globals),
                value='value')
            self.buf.write('\n\n')
    
    def emitConstructor(self, node, globals):
        default_value_visitor = DefaultValueVisitor(buf=self.buf)
        self.buf.write('\tdef __init__(self')
        for member in node.members():
            self.buf.write(', {member_name}='.format(member_name=member.name))
            default_value_visitor.visit(member.type)
        self.buf.write('):\n')
        
        if node.members():
            for member in node.members():
                self.buf.write('\t\tself.{member_name} = {member_name}\n'.format(member_name=member.name))
        else:
            self.buf.write('\t\tpass\n')
        self.buf.write('\n')
            
    def emitPackers(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \t@classmethod
            \tdef unpack(cls, buffer):
            \t\t"Reads a new {struct_name} from the given buffer."
            \t\treader = {support_module}.BinaryReader(buffer)
            \t\tvalue = cls.unpack_from(reader)
            \t\tif reader.tell() != len(reader):
            \t\t\traise {support_module}.ReadError(
            \t\t\t\t('{struct_name}.unpack received a buffer of length {{length}}, ' +
            \t\t\t\t'but only {{position}} bytes were read.').format(
            \t\t\t\tlength=len(reader), position=reader.tell()))
            \t\treturn value
            
            \t@classmethod
            \tdef unpack_from(cls, reader):
            \t\t"Reads a new {struct_name} from the given BinaryReader."
            #''').format(**globals)[:-1])
        
        read_visitor = ReadVisitor(buf=self.buf)
        for member in node.members():
            self.buf.write('\t\t_{member_name} = '.format(member_name=member.name))
            read_visitor.visit(member.type, reader='reader')
            self.buf.write('\n')
        
        self.buf.write('\t\treturn cls(')
        for i, member in enumerate(node.members()):
            if i > 0:
                self.buf.write(', ')
            self.buf.write('_{member_name}'.format(member_name=member.name))
        self.buf.write(')\n\n')
            
        self.buf.write(textwrap.dedent('''\
            \tdef pack(self):
            \t\t"Writes the current {struct_name}, returning bytes."
            \t\twriter = {support_module}.BinaryWriter()
            \t\tself.pack_to(writer)
            \t\treturn writer.dumps()
            
            \tdef pack_to(self, writer):
            \t\t"Writes the current {struct_name} to the given BinaryWriter."
            #''').format(**globals)[:-1])
        
        write_visitor = WriteVisitor(buf=self.buf)
        for member in node.members():
            self.buf.write('\t\t')
            write_visitor.visit(member.type, writer='writer',
                value='self._{member_name}'.format(member_name=member.name))
            self.buf.write('\n')
        self.buf.write('\n')
    
    def emitComparisons(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __eq__(self, other):
            \t\tif type(self) is type(other):
            #''')[:-1])
        
        if node.members():
            self.buf.write('\t\t\treturn ')
            if len(node.members()) > 1:
                self.buf.write('(')
            for i, member in enumerate(node.members()):
                if i > 0:
                    self.buf.write('\t\t\t\t')
                self.buf.write('self._{member_name} == other._{member_name}'.format(member_name=member.name))
                if i < len(node.members()) - 1:
                    self.buf.write(' and\n')
            if len(node.members()) > 1:
                self.buf.write(')')
            self.buf.write('\n')
        else:
            self.buf.write('\t\t\treturn True\n')
        
        self.buf.write(textwrap.dedent('''\
            \t\telse:
            \t\t\treturn NotImplemented
            
            \tdef __ne__(self, other):
            \t\tif type(self) is type(other):
            \t\t\treturn not self.__eq__(other)
            \t\telse:
            \t\t\treturn NotImplemented
            
            #''')[:-1])
    
    def emitLength(self, node, globals):
        self.buf.write('\tdef __len__(self):\n')
        
        if node.members():
            size_visitor = SizeVisitor(buf=self.buf)
            self.buf.write('\t\treturn (')
            for i, member in enumerate(node.members()):
                if i > 0:
                    self.buf.write(' +\n\t\t\t')
                size_visitor.visit(member.type, value='self._{member_name}'.format(member_name=member.name))
            self.buf.write(')\n\n')
        else:
            self.buf.write('\t\treturn 0\n\n')
        
    def emitStr(self, node, globals):
        
        self.buf.write('\tdef __str__(self):\n')
        
        if node.members():
            self.buf.write("\t\treturn '{type}(")
            self.buf.write(', '.join('{%s}' % member.name for member in node.members()))
            self.buf.write(")'.format(\n")
            
            self.buf.write('\t\t\ttype=type(self).__name__,\n')
            
            str_visitor = StrVisitor(buf=self.buf)
            for i, member in enumerate(node.members()):
                self.buf.write('\t\t\t{member_name}='.format(member_name=member.name))
                str_visitor.visit(member.type,
                    value='self._{member_name}'.format(member_name=member.name))
                if i < len(node.members()) - 1:
                    self.buf.write(',\n')
                else:
                    self.buf.write(')\n')
        else:
            self.buf.write("\t\treturn '{type}()'.format(type=type(self).__name__)\n")
        self.buf.write('\n')
        
    def emitRepr(self, node, globals):
        self.buf.write('\tdef __repr__(self):\n')
        if node.members():
            self.buf.write("\t\treturn '{type}(")
            self.buf.write(', '.join('{%s}' % member.name for member in node.members()))
            self.buf.write(")'.format(\n")
            
            self.buf.write('\t\t\ttype=type(self).__name__,\n')
            
            for i, member in enumerate(node.members()):
                self.buf.write('\t\t\t{member_name}=repr(self._{member_name})'.format(member_name=member.name))
                if i < len(node.members()) - 1:
                    self.buf.write(',\n')
                else:
                    self.buf.write(')\n')
        else:
            self.buf.write("\t\treturn '{type}()'.format(type=type(self).__name__)\n")
        self.buf.write('\n')

class UnionEmitter(BaseEmitter):

    def visit_UnionDecl(self, node):
        
        globals = dict(
            support_module=support_module,
            union_name=node.name,
            tag_count=len(node.members()),
        )
        
        self.emitHeader(node, globals)
        self.emitSlots(node, globals)
        self.emitTags(node, globals)
        self.emitProperties(node, globals)
        self.emitConstructor(node, globals)
        self.emitPackers(node, globals)
        self.emitClear(node, globals)
        self.emitComparisons(node, globals)
        self.emitLength(node, globals)
        self.emitStr(node, globals)
        self.emitRepr(node, globals)
        self.emitDicts(node, globals)
    
    def emitHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            class {union_name}(object):
            \t"Generated message-passing union."
            
            #''')[:-1].format(**globals))
    
    def emitSlots(self, node, globals):
        self.buf.write("\t__slots__ = ('_tag', '_data')\n\n")
    
    def emitTags(self, node, globals):
        self.buf.write('\tclass Tag(object):\n')
        self.buf.write('\t\t"The type indicator for this union."\n')
        
        if node.members():
            starts = []
            mids = []
            ends = []
            for i, member in enumerate(node.members()):
                start = '\t\t{member_name}'.format(member_name=member.name)
                mid = ' = {initializer}'.format(initializer=str(i))
                end = ' # {member_type}\n'.format(
                    member_type=member.type.fully_qualified_name('.'))
                starts.append(start)
                mids.append(mid)
                ends.append(end)
        
            full_start_length = max(len(start) for start in starts)
            full_mid_length = max(len(mid) for mid in mids)
            for start, mid, end in zip(starts, mids, ends):
                self.buf.write(start)
                self.buf.write(' ' * (full_start_length - len(start)))
                self.buf.write(mid)
                self.buf.write(' ' * (full_mid_length - len(mid)))
                self.buf.write(end)
        else:
            self.buf.write('\t\tpass\n')
        
        self.buf.write('\n')
        
        self.buf.write(textwrap.dedent('''\
            \t@property
            \tdef tag(self):
            \t\t"The current tag for this union."
            \t\treturn self._tag
            
            \t@property
            \tdef tag_name(self):
            \t\t"The name of the current tag for this union."
            \t\tif 0 <= self._tag < {tag_count}:
            \t\t\treturn self._tags_by_value[self._tag]
            \t\telse:
            \t\t\treturn None
            
            \t@property
            \tdef data(self):
            \t\t"The data held by this union. None if no data is set."
            \t\treturn self._data
            
            #''')[:-1].format(**globals))
        
    def emitProperties(self, node, globals):
        setter_visitor = SetterVisitor(buf=self.buf, depth=2)
        for member in node.members():
            self.buf.write(textwrap.dedent('''\
                \t@property
                \tdef {member_name}(self):
                \t\t"{member_type} {member_name} union property."
                \t\t{support_module}.safety_check_tag('{member_name}', self._tag, self.Tag.{member_name}, self._tags_by_value)
                \t\treturn self._data
                
                \t@{member_name}.setter
                \tdef {member_name}(self, value):
                #''')[:-1].format(member_name=member.name, member_type=member.type.fully_qualified_name('.'), **globals))
            self.buf.write('\t\tself._data = '.format(member_name=member.name, value='value'))
            setter_visitor.visit(
                member.type,
                name="'{union_name}.{member_name}'".format(member_name=member.name, **globals),
                value='value')
            self.buf.write('\n')
            self.buf.write('\t\tself._tag = self.Tag.{member_name}\n\n'.format(member_name=member.name))
    
    def emitConstructor(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __init__(self, **kwargs):
            \t\tif not kwargs:
            \t\t\tself._tag = None
            \t\t\tself._data = None
            
            \t\telif len(kwargs) == 1:
            \t\t\tkey, value = next(iter(kwargs.items()))
            \t\t\tif key not in self._tags_by_name:
            \t\t\t\traise TypeError("'{{argument}}' is an invalid keyword argument for this method.".format(argument=key))
            \t\t\t# calls the correct property
            \t\t\tsetattr(self, key, value)
            
            \t\telse:
            \t\t\traise TypeError('This method only accepts up to one keyword argument.')
            
            #''')[:-1].format(**globals))
            
    def emitPackers(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \t@classmethod
            \tdef unpack(cls, buffer):
            \t\t"Reads a new {union_name} from the given buffer."
            \t\treader = {support_module}.BinaryReader(buffer)
            \t\tvalue = cls.unpack_from(reader)
            \t\tif reader.tell() != len(reader):
            \t\t\traise {support_module}.ReadError(
            \t\t\t\t('{union_name}.unpack received a buffer of length {{length}}, ' +
            \t\t\t\t'but only {{position}} bytes were read.').format(
            \t\t\t\tlength=len(reader), position=reader.tell()))
            \t\treturn value
            
            \t@classmethod
            \tdef unpack_from(cls, reader):
            \t\t"Reads a new {union_name} from the given BinaryReader."
            \t\ttag = reader.read('B')
            \t\tif 0 <= tag < {tag_count}:
            \t\t\tvalue = cls()
            \t\t\tsetattr(value, cls._tags_by_value[tag], cls._tag_unpack_methods[tag](reader))
            \t\t\treturn value
            \t\telse:
            \t\t\traise ValueError('{union_name} attempted to unpack unknown tag {{tag}}.'.format(tag=tag))
            
            \tdef pack(self):
            \t\t"Writes the current {union_name}, returning bytes."
            \t\twriter = {support_module}.BinaryWriter()
            \t\tself.pack_to(writer)
            \t\treturn writer.dumps()
    
            \tdef pack_to(self, writer):
            \t\t"Writes the current SampleUnion to the given BinaryWriter."
            \t\tif 0 <= self._tag < {tag_count}:
            \t\t\twriter.write(self._tag, 'B')
            \t\t\tself._tag_pack_methods[self._tag](writer, self._data)
            \t\telse:
            \t\t\traise ValueError('Cannot pack an empty {union_name}.')
            
            #''')[:-1].format(**globals))
    
    def emitClear(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef clear(self):
            \t\tself._tag = None
            \t\tself._data = None
            
            #''')[:-1])
        
    def emitComparisons(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __eq__(self, other):
            \t\tif type(self) is type(other):
            \t\t\treturn self._tag == other._tag and self._data == other._data
            \t\telse:
            \t\t\treturn NotImplemented
            
            \tdef __ne__(self, other):
            \t\tif type(self) is type(other):
            \t\t\treturn not self.__eq__(other)
            \t\telse:
            \t\t\treturn NotImplemented
            
            #''')[:-1])
    
    def emitLength(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __len__(self):
            \t\tif 0 <= self._tag < {tag_count}:
            \t\t\treturn self._tag_size_methods[self._tag](self._data)
            \t\telse:
            \t\t\treturn 1
            
            #''')[:-1].format(**globals))
    
    def emitStr(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __str__(self):
            \t\tif 0 <= self._tag < {tag_count}:
            \t\t\treturn '{{type}}({{name}}={{value}})'.format(
            \t\t\t\ttype=type(self).__name__,
            \t\t\t\tname=self.tag_name,
            \t\t\t\tvalue=self._data)
            \t\telse:
            \t\t\treturn '{{type}}()'.format(
            \t\t\t\ttype=type(self).__name__)
            
            #''')[:-1].format(**globals))
    
    def emitRepr(self, node, globals):
        self.buf.write(textwrap.dedent('''\
            \tdef __repr__(self):
            \t\tif 0 <= self._tag < {tag_count}:
            \t\t\treturn '{{type}}({{name}}={{value}})'.format(
            \t\t\t\ttype=type(self).__name__,
            \t\t\t\tname=self.tag_name,
            \t\t\t\tvalue=repr(self._data))
            \t\telse:
            \t\t\treturn '{{type}}()'.format(
            \t\t\t\ttype=type(self).__name__)
            
            #''')[:-1].format(**globals))
    
    def emitDicts(self, node, globals):
        self.buf.write('\t_tags_by_name = dict(\n')
        for i, member in enumerate(node.members()):
            self.buf.write("\t\t{member_name}={tag},\n".format(member_name=member.name, tag=i))
        self.buf.write('\t)\n\n')
        
        if node.members():
            self.buf.write('\t_tags_by_value = (\n')
            for member in node.members():
                self.buf.write("\t\t'{member_name}',\n".format(member_name=member.name, tag=i))
            self.buf.write('\t)\n\n')
        else:
            self.buf.write('\t_tags_by_value = ()\n\n')

        self.emitVisitorList(node, globals, '_tag_unpack_methods', ReadVisitor(buf=self.buf), 'reader')
        self.emitVisitorList(node, globals, '_tag_pack_methods', WriteVisitor(buf=self.buf), 'writer', 'value')
        self.emitVisitorList(node, globals, '_tag_size_methods', SizeVisitor(buf=self.buf), 'value')
        #self.emitVisitorList(node, globals, '_tag_str_methods', StrVisitor(buf=self.buf), 'value')
    
    def emitVisitorList(self, node, globals, dict_name, visitor, *args):
        if node.members():
            self.buf.write('\t{dict_name} = (\n'.format(dict_name=dict_name))
            for member in node.members():
                self.buf.write('\t\tlambda {args}: '.format(args=', '.join(args)))
                visitor.visit(member.type, *args)
                self.buf.write(',\n')
            self.buf.write('\t)\n\n')
        else:
            self.buf.write('\t{dict_name} = ()\n\n'.format(dict_name=dict_name))

class PythonMemberVisitor(BaseEmitter):
    
    def __init__(self, buf=sys.stdout, depth=1):
        super(PythonMemberVisitor, self).__init__(buf)
        self.depth = depth
    
    def visit_DefinedType(self, node, *args, **kwargs):
        self.visit(node.underlying_type, *args, **kwargs)
    
    def visit_EnumDecl(self, node, *args, **kwargs):
        self.visit(node.storage_type, *args, **kwargs)

class SetterVisitor(PythonMemberVisitor):
    
    def visit_BuiltinType(self, node, name, value):
        if node.name == 'bool':
            self.buf.write("{support_module}.validate_bool(\n".format(support_module=support_module))
            self.buf.write('\t' * (self.depth + 1))
            self.buf.write("{name}, {value})".format(
                name=name, value=value))
            
        elif node.type == 'int':
            self.buf.write("{support_module}.validate_integer(\n".format(support_module=support_module))
            self.buf.write('\t' * (self.depth + 1))
            self.buf.write("{name}, {value}, {minimum}, {maximum})".format(
                name=name, value=value, minimum=node.min, maximum=node.max))
        
        elif node.type == 'float':
            self.buf.write("{support_module}.validate_float(\n".format(support_module=support_module))
            self.buf.write('\t' * (self.depth + 1))
            self.buf.write("{name}, {value}, '{format}')".format(
                name=name, value=value, format=type_translations[node.name]))
        
        else:
            raise ValueError('Unknown primitive type {type}.'.format(node.type))
    
    def visit_CompoundType(self, node, name, value):
        self.buf.write("{support_module}.validate_object(\n".format(support_module=support_module))
        self.buf.write('\t' * (self.depth + 1))
        self.buf.write("{name}, {value}, {type})".format(
            name=name, value=value, type=node.fully_qualified_name('.')))
    
    def visit_PascalStringType(self, node, name, value):
        self.buf.write("{support_module}.validate_string(\n".format(support_module=support_module))
        self.buf.write('\t' * (self.depth + 1))
        self.buf.write("{name}, {value}, {maximum_length})".format(
            name=name, value=value, maximum_length=node.length_type.max))
    
    def visit_FixedArrayType(self, node, name, value):
        self.buf.write("{support_module}.validate_farray(\n".format(support_module=support_module))
        self.buf.write('\t' * (self.depth + 1))
        self.buf.write("{name}, {value}, {length},\n".format(
            name=name, value=value, length=node.length))
        self.buf.write('\t' * (self.depth + 1))
        inner = '{value}_inner'.format(value=value)
        self.buf.write('lambda name, {inner}: '.format(inner=inner))
        self.depth += 1
        self.visit(node.member_type, name='name', value=inner)
        self.depth -= 1
        self.buf.write(')')
    
    def visit_VariableArrayType(self, node, name, value):
        self.buf.write("{support_module}.validate_varray(\n".format(support_module=support_module))
        self.buf.write('\t' * (self.depth + 1))
        self.buf.write("{name}, {value}, {maximum_length},\n".format(
            name=name, value=value, maximum_length=node.length_type.max))
        self.buf.write('\t' * (self.depth + 1))
        inner = '{value}_inner'.format(value=value)
        self.buf.write('lambda name, {inner}: '.format(inner=inner))
        self.depth += 1
        self.visit(node.member_type, name='name', value=inner)
        self.depth -= 1
        self.buf.write(')')

class DefaultValueVisitor(PythonMemberVisitor):
    
    def visit_BuiltinType(self, node):
        if node.name == 'bool':
            self.buf.write('False')
        elif node.type == 'int':
            self.buf.write('0')
        elif node.type == 'float':
            self.buf.write('0.0')
        else:
            raise ValueError('Unknown primitive type {type}.'.format(node.type))
    
    def visit_EnumDecl(self, node):
        if not node.members():
            self.buf.write('0')
        else:
            member = node.members()[0]
            self.buf.write('{enum_name}.{enum_member}'.format(
                enum_name=node.fully_qualified_name('.'), enum_member=member.name))
    
    def visit_CompoundType(self, node):
        self.buf.write('{type}()'.format(type=node.fully_qualified_name('.')))
    
    def visit_PascalStringType(self, node):
        self.buf.write("''")
    
    def visit_FixedArrayType(self, node):
        self.buf.write('(')
        self.visit(node.member_type)
        self.buf.write(',) * {length}'.format(length=node.length))
    
    def visit_VariableArrayType(self, node):
        self.buf.write('()')

class ReadVisitor(PythonMemberVisitor):
    
    class ChildFixedArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, reader, length):
            if node.name == 'bool':
                self.buf.write('list(map(bool, ')
            self.buf.write("{reader}.read_farray('{format}', {length})".format(
                reader=reader, format=type_translations[node.name], length=length))
            if node.name == 'bool':
                self.buf.write('))')
    
        def visit_CompoundType(self, node, reader, length):
            self.buf.write("{reader}.read_object_farray({type}.unpack_from, {length})".format(
                reader=reader, type=node.fully_qualified_name('.'), length=length))
    
        def visit_PascalStringType(self, node, reader, length):
            self.buf.write("{reader}.read_string_farray('{string_length_format}', {array_length})".format(
                reader=reader, string_length_format=type_translations[node.length_type.name], array_length=length))
    
        def visit_FixedArrayType(self, node, reader, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, reader, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    class ChildVariableArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, reader, length_type):
            if node.name == 'bool':
                self.buf.write('list(map(bool, ')
            self.buf.write("{reader}.read_varray('{data_format}', '{length_format}')".format(
                reader=reader, data_format=type_translations[node.name],
                length_format=type_translations[length_type.name]))
            if node.name == 'bool':
                self.buf.write('))')
    
        def visit_CompoundType(self, node, reader, length_type):
            self.buf.write("{reader}.read_object_varray({type}.unpack_from, '{length_format}')".format(
                reader=reader, type=node.fully_qualified_name('.'), length_format=type_translations[length_type.name]))
    
        def visit_PascalStringType(self, node, reader, length_type):
            self.buf.write("{reader}.read_string_varray('{string_length_format}', '{array_length_format}')".format(
                reader=reader, string_length_format=type_translations[node.length_type.name],
                array_length_format=type_translations[length_type.name]))
    
        def visit_FixedArrayType(self, node, reader, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, reader, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    def __init__(self, *args, **kwargs):
        super(ReadVisitor, self).__init__(*args, **kwargs)
        self.fixed_visitor = self.ChildFixedArrayVisitor(*args, **kwargs)
        self.variable_visitor = self.ChildVariableArrayVisitor(*args, **kwargs)
    
    def visit_BuiltinType(self, node, reader):
        if node.name == 'bool':
            self.buf.write('bool(')
        self.buf.write("{reader}.read('{format}')".format(reader=reader, format=type_translations[node.name]))
        if node.name == 'bool':
            self.buf.write(')')
    
    def visit_CompoundType(self, node, reader):
        self.buf.write("{reader}.read_object({type}.unpack_from)".format(
            reader=reader, type=node.fully_qualified_name('.')))
    
    def visit_PascalStringType(self, node, reader):
        self.buf.write("{reader}.read_string('{format}')".format(
            reader=reader, format=type_translations[node.length_type.name]))
    
    def visit_FixedArrayType(self, node, reader):
        self.fixed_visitor.visit(node.member_type, reader=reader, length=node.length)
    
    def visit_VariableArrayType(self, node, reader):
        self.variable_visitor.visit(node.member_type, reader=reader, length_type=node.length_type)

class WriteVisitor(PythonMemberVisitor):
    
    class ChildFixedArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, writer, value, length):
            if node.name == 'bool':
                value = 'list(map(int, {value}))'.format(value=value)
            self.buf.write("{writer}.write_farray({value}, '{format}', {length})".format(
                writer=writer, value=value, format=type_translations[node.name], length=length))
    
        def visit_CompoundType(self, node, writer, value, length):
            self.buf.write("{writer}.write_object_farray({value}, {length})".format(
                writer=writer, value=value, length=length))
    
        def visit_PascalStringType(self, node, writer, value, length):
            self.buf.write("{writer}.write_string_farray({value}, '{string_length_format}', {array_length})".format(
                writer=writer, value=value, string_length_format=type_translations[node.length_type.name],
                array_length=length))
    
        def visit_FixedArrayType(self, node, writer, value, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, writer, value, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    class ChildVariableArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, writer, value, length_type):
            if node.name == 'bool':
                value = 'list(map(int, {value}))'.format(value=value)
            self.buf.write("{writer}.write_varray({value}, '{data_format}', '{length_format}')".format(
                writer=writer, value=value, data_format=type_translations[node.name],
                length_format=type_translations[length_type.name]))
    
        def visit_CompoundType(self, node, writer, value, length_type):
            self.buf.write("{writer}.write_object_varray({value}, '{length_format}')".format(
                writer=writer, value=value, length_format=type_translations[length_type.name]))
    
        def visit_PascalStringType(self, node, writer, value, length_type):
            self.buf.write("{writer}.write_string_varray({value}, '{string_length_format}', '{array_length_format}')".format(
                writer=writer, value=value, string_length_format=type_translations[node.length_type.name],
                array_length_format=type_translations[length_type.name]))
    
        def visit_FixedArrayType(self, node, writer, value, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, writer, value, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    def __init__(self, *args, **kwargs):
        super(WriteVisitor, self).__init__(*args, **kwargs)
        self.fixed_visitor = self.ChildFixedArrayVisitor(*args, **kwargs)
        self.variable_visitor = self.ChildVariableArrayVisitor(*args, **kwargs)
    
    def visit_BuiltinType(self, node, writer, value):
        if node.name == 'bool':
            value = 'int({value})'.format(value=value)
        self.buf.write("{writer}.write({value}, '{format}')".format(
            writer=writer, value=value, format=type_translations[node.name], support_module=support_module))
    
    def visit_CompoundType(self, node, writer, value):
        self.buf.write("{writer}.write_object({value})".format(
            writer=writer, value=value, support_module=support_module))
    
    def visit_PascalStringType(self, node, writer, value):
        self.buf.write("{writer}.write_string({value}, '{format}')".format(
            writer=writer, value=value, format=type_translations[node.length_type.name], support_module=support_module))
    
    def visit_FixedArrayType(self, node, writer, value):
        self.fixed_visitor.visit(node.member_type, writer=writer, value=value, length=node.length)
    
    def visit_VariableArrayType(self, node, writer, value):
        self.variable_visitor.visit(node.member_type, writer=writer, value=value, length_type=node.length_type)

class SizeVisitor(PythonMemberVisitor):
    
    class ChildFixedArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, value, length):
            self.buf.write("{support_module}.size_farray({value}, '{format}', {length})".format(
                value=value, format=type_translations[node.name], length=length, support_module=support_module))
    
        def visit_CompoundType(self, node, value, length):
            self.buf.write("{support_module}.size_object_farray({value}, {length})".format(
                value=value, length=length, support_module=support_module))
    
        def visit_PascalStringType(self, node, value, length):
            self.buf.write("{support_module}.size_string_farray({value}, '{string_length_format}', {array_length})".format(
                value=value, string_length_format=type_translations[node.length_type.name],
                array_length=length, support_module=support_module))
    
        def visit_FixedArrayType(self, node, value, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, value, length):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    class ChildVariableArrayVisitor(PythonMemberVisitor):
        def visit_BuiltinType(self, node, value, length_type):
            self.buf.write("{support_module}.size_varray({value}, '{data_format}', '{length_format}')".format(
                value=value, data_format=type_translations[node.name],
                length_format=type_translations[length_type.name], support_module=support_module))
    
        def visit_CompoundType(self, node, value, length_type):
            self.buf.write("{support_module}.size_object_varray({value}, '{length_format}')".format(
                value=value, length_format=type_translations[length_type.name], support_module=support_module))
    
        def visit_PascalStringType(self, node, value, length_type):
            self.buf.write("{support_module}.size_string_varray({value}, '{string_length_format}', '{array_length_format}')".format(
                value=value, string_length_format=type_translations[node.length_type.name],
                array_length_format=type_translations[length_type.name], support_module=support_module))
    
        def visit_FixedArrayType(self, node, value, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
        def visit_VariableArrayType(self, node, value, length_type):
            raise sys.exit('Python emitter does not support arrays of arrays.')
    
    def __init__(self, *args, **kwargs):
        super(SizeVisitor, self).__init__(*args, **kwargs)
        self.fixed_visitor = self.ChildFixedArrayVisitor(*args, **kwargs)
        self.variable_visitor = self.ChildVariableArrayVisitor(*args, **kwargs)
    
    def visit_BuiltinType(self, node, value):
        self.buf.write("{support_module}.size({value}, '{format}')".format(
            value=value, format=type_translations[node.name], support_module=support_module))
    
    def visit_CompoundType(self, node, value):
        self.buf.write('{support_module}.size_object({value})'.format(
            value=value, support_module=support_module))
    
    def visit_PascalStringType(self, node, value):
        self.buf.write("{support_module}.size_string({value}, '{length_format}')".format(
            value=value, length_format=type_translations[node.length_type.name], support_module=support_module))
    
    def visit_FixedArrayType(self, node, value):
        self.fixed_visitor.visit(node.member_type, value=value, length=node.length)
    
    def visit_VariableArrayType(self, node, value):
        self.variable_visitor.visit(node.member_type, value=value, length_type=node.length_type)

class StrVisitor(PythonMemberVisitor):
    
    def visit_BuiltinType(self, node, value):
        self.buf.write(value)
    
    def visit_CompoundType(self, node, value):
        self.buf.write(value)
    
    def visit_PascalStringType(self, node, value):
        self.buf.write('{support_module}.shorten_string({value})'.format(
            value=value, support_module=support_module))
    
    def visit_FixedArrayType(self, node, value):
        if isinstance(node.member_type, ast.PascalStringType):
            self.buf.write('{support_module}.shorten_sequence({value}, {support_module}.shorten_string)'.format(
                value=value, support_module=support_module))
        else:
            self.buf.write('{support_module}.shorten_sequence({value})'.format(
                value=value, support_module=support_module))
    
    def visit_VariableArrayType(self, node, value):
        self.visit_FixedArrayType(node, value)

def emit_body(tree, output):
    searcher = IncludeSearcher()
    searcher.visit(tree)
    
    # add all namespaces, even those not in use
    if searcher.all_namespaces:
        for namespace in sorted(searcher.all_namespaces, key=sort_key_lowercase):
            output.write('{namespace} = {support_module}.Namespace()\n'.format(
                namespace=namespace, support_module=support_module))
        output.write('\n')
    
    # add the imports for each module
    for current_module in sorted(searcher.module.included_modules, key=Module.sort_key):
        namespace_imports = []
        namespace_assignments = []
        for name in sorted(current_module.global_namespaces, key=sort_key_lowercase):
            unique_name = current_module.get_unique_name('_' + name)
            namespace_imports.append('{name} as {unique_name}'.format(
                name=name, unique_name=unique_name))
            # need to clone so we don't change the original module
            namespace_assignments.append('{name}.update({unique_name}.deep_clone())\n'.format(
                name=name, unique_name=unique_name))
        
        decl_imports = sorted(current_module.global_decls, key=sort_key_lowercase)
        
        if namespace_imports:
            output.write('from {module_name} import {imports}\n'.format(
                module_name=current_module.name,
                imports=', '.join(namespace_imports)))
            for line in namespace_assignments:
                output.write(line)
        if decl_imports:
            output.write('from {module_name} import {imports}\n'.format(
                module_name=current_module.name,
                imports=', '.join(decl_imports)))
        if namespace_imports or decl_imports:
            output.write('\n')
    
    DeclEmitter(output, module=searcher.module).visit(tree)

if __name__ == '__main__':
    local_current_path = os.path.dirname(inspect.getfile(inspect.currentframe()))
    local_support_path = os.path.normpath(os.path.join(local_current_path, '..', 'support', 'python'))
    
    from clad import emitterutil
    option_parser = emitterutil.StandardArgumentParser('python')
    options = option_parser.parse_args()
    
    tree = emitterutil.parse(options)
    main_output_file = emitterutil.get_output_file(options, '.py')
    comment_lines = emitterutil.get_comment_lines(options, 'python')
    
    emitterutil.write_python_file(options.output_directory, main_output_file,
        lambda output: emit_body(tree, output),
        comment_lines,
        additional_paths=[local_support_path],
        import_modules=[support_module])
    
    if options.output_directory != '-' and main_output_file != '-':
        current_path = options.output_directory
        for component in os.path.dirname(main_output_file).split('/'):
            if component:
                current_path = os.path.join(current_path, component)
                emitterutil.create_file(os.path.join(current_path, '__init__.py'))
