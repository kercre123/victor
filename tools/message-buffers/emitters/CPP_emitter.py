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
    'bool': 'bool',
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

def primitive_type_name(type):
    type_name = type.name
    return type_translations[type_name] if type_name in type_translations else None

def cast_type(type):
    if primitive_type_name(type) is not None:
        return type_translations[type.name]
    elif isinstance(type, ast.PascalStringType):
        return "std::string";
    else:
        return type.fully_qualified_name()

class HNamespaceEmitter(ast.NodeVisitor):
    
    def __init__(self, buf=sys.stdout, include_extension='.h'):
        self.buf = buf
        self.include_extension = include_extension
    
    def visit_IncludeDecl(self, node, *args, **kwargs):
        new_header_file_name = emitterutil.get_included_file(node.name, self.include_extension)
        self.buf.write('#include "{0}"\n\n'.format(new_header_file_name))

    def visit_NamespaceDecl(self, node, *args, **kwargs):
        self.buf.write('namespace {namespace_name} {{\n\n'.format(namespace_name=node.name))
        for c_name, c in node.children():
            self.visit(c, *args, **kwargs)
        self.buf.write('}} // namespace {namespace_name}\n\n'.format(namespace_name=node.name))
    
    def visit_EnumDecl(self, node, *args, **kwargs):
        HEnumEmitter(self.buf).visit(node, *args, **kwargs)
    
    def visit_MessageDecl(self, node, *args, **kwargs):
        HStructEmitter(self.buf).visit(node, *args, **kwargs)
    
    def visit_UnionDecl(self, node, *args, **kwargs):
        HUnionEmitter(self.buf).visit(node, *args, **kwargs)

class CPPNamespaceEmitter(HNamespaceEmitter):
    
    def visit_EnumDecl(self, node, *args, **kwargs):
        CPPEnumEmitter(self.buf).visit(node, *args, **kwargs)

    def visit_IncludeDecl(self, node, *args, **kwargs):
        pass
    
    def visit_MessageDecl(self, node, *args, **kwargs):
        CPPStructEmitter(self.buf).visit(node, *args, **kwargs)
    
    def visit_UnionDecl(self, node, *args, **kwargs):
        CPPUnionEmitter(self.buf).visit(node, *args, **kwargs)

class HEnumEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_EnumDecl(self, node):
        # print the header
        self.buf.write(textwrap.dedent('''\
        // ENUM {0}
        enum class {0} : {1} {{
        ''').format(node.name, type_translations[node.storage_type.builtin_type().name]));
        enum_val = 0
        members = node.members()
        #print each member of the enum
        for i, member in enumerate(members):
            enum_val = self.emitEnum(member, enum_val, i < len(members) - 1, prefix='')
        #print the footer
        footer = '};\n\n'
        self.buf.write(footer)
        self.buf.write('const char* {0}ToString(const {0} m);\n'.format(node.name))

    def emitEnum(self, member, enum_val, trailing_comma=True, prefix=''):
        separator = ',' if trailing_comma else ''
        if not member.initializer:
            self.buf.write('\t' + prefix + member.name + separator + '\t// %d\n' % enum_val)
            return enum_val + 1
        else:
            initial_value = member.initializer
            enum_val = initial_value.value
            enum_str = hex(enum_val) if initial_value.type == "hex" else str(enum_val)
            self.buf.write("\t" + "%s%s = %s"
                           % (prefix, member.name, enum_str) + separator
                           + '\t// %d\n' % enum_val)
            return enum_val + 1

class CPPEnumEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_EnumDecl(self, node):
        self.buf.write(textwrap.dedent('''\
        const char* {0}ToString(const {0} m)
        {{
        \tswitch(m) {{
        ''').format(node.name))
        for member in node.members():
            self.buf.write('\t\tcase {0}::{1}: return "{1}"; break;\n'
                           .format(node.name, member.name))
        self.buf.write(textwrap.dedent('''\
        \t\tdefault: return nullptr;
        \t}
        \treturn nullptr;
        }
        '''))

class HStructEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_MessageDecl(self, node):
        header = textwrap.dedent('''\
            // {obj_type} {message_name}
            struct {message_name}
            {{
            ''').format(obj_type=node.object_type().upper(), message_name=node.name)
        footer = '};\n\n'
        self.buf.write(header)
        members = node.members()
        for i, member in enumerate(members):
            HStructEmitter_MessageMemberVisitor(buf=self.buf,
                                                prefix='',
                                                member_name=member.name,
                                                depth=1).visit(member)
        HStruct_ConstructorEmitter(buf=self.buf, prefix='').visit(node)
        HStruct_PackEmitter(buf=self.buf, prefix='').visit(node)
        HStruct_UnpackEmitter(buf=self.buf, prefix='').visit(node)
        HStruct_SizeEmitter(buf=self.buf, prefix='').visit(node)
        HStruct_EqualityEmitter(buf=self.buf, prefix='').visit(node)
        self.buf.write(footer)

    def visit_UnionDecl(self, node):
        pass

class CPPStructEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_MessageDecl(self, node):
        header = "// {obj_type} {message_name}\n\n".format(obj_type=node.object_type().upper(),
                                                           message_name=node.name)
        footer = "\n"
        self.buf.write(header)
        CPPStruct_ConstructorEmitter(buf=self.buf, prefix='').visit(node)
        CPPStruct_PackEmitter(buf=self.buf, prefix='').visit(node)
        CPPStruct_UnpackEmitter(buf=self.buf, prefix='').visit(node)
        CPPStruct_SizeEmitter(buf=self.buf, prefix='').visit(node)
        CPPStruct_EqualityEmitter(buf=self.buf, prefix='').visit(node)
        self.buf.write(footer)

    def visit_UnionDecl(self, node):
        pass

class HStructEmitter_MessageMemberVisitor(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name='', depth=0):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name
        self.depth = depth

    def visit_MessageMemberDecl(self, member):
        # Visiting a plain member decl for a message
        # first, print the type
        self.buf.write("\t" * self.depth)
        self.visit(member.type)
        self.buf.write(" %s%s;\n" % (self.prefix, self.member_name))

    def visit_MessageVariableArrayMemberDecl(self, member):
        # Visiting a v-array member decl for a message
        # C++ Vectors are automatically variable length
        # should look like
        # vector<int> foo;
        self.buf.write('\t' * self.depth + 'std::vector<')
        self.visit(member.type.member_type)
        self.buf.write('> %s%s;\n' % (self.prefix, self.member_name))

    ## Type translators ##
    def visit_PascalStringType(self, member):
        # Visiting a string type will always generate a string member
        #  The length is encoded differently, but that is only important
        #  to packing and unpacking.
        self.buf.write('std::string')

    def visit_BuiltinType(self, node):
        #Visit a builtin type for the message member decl
        self.buf.write(type_translations[node.name])

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_CompoundType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_VariableArrayType(self, node):
        #Arrays of arrays are not supported currently
        sys.stderr.write('Variable array of variable array is not yet supported in cpp!\n')
        exit(1)
        pass
    
    def visit_FixedArrayType(self, node):
        self.buf.write('std::array<')
        self.visit(node.member_type)
        self.buf.write(', %d>' % (node.length,))

class HStruct_ConstructorEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        class_name = self.prefix + node.name
        self.buf.write('\t' + textwrap.dedent('''\

        /**** Constructors ****/
        {0}() = default;
        {0}(const {0}& other) = default;
        {0}({0}& other) = default;
        {0}({0}&& other) noexcept = default;
        {0}& operator=(const {0}& other) = default;
        {0}& operator=({0}&& other) noexcept = default;
        '''.format(class_name)).replace('\n', '\n\t'))

        self.emitValueConstructor(node)

        self.buf.write('\t' + textwrap.dedent('''\
        explicit {0}(const uint8_t* buff, size_t len);
        explicit {0}(const CLAD::SafeMessageBuffer& buffer);
        '''.format(class_name)).replace('\n', '\n\t'))


    def emitValueConstructor(self, node):
        members = node.members()
        if(0 >= len(members)):
            return
        self.buf.write('\n\texplicit %s(' % (self.prefix + node.name))
        for i, member in enumerate(members):
            self.visit(member) #print type
            self.buf.write(' %s' % member.name) #print name
            if (i < len(members) - 1):
                self.buf.write('\n\t\t,')
        self.buf.write(')\n\t:')
        for i, member in enumerate(members):
            #print initializer
            self.buf.write('%s(%s)' % (member.name, member.name))
            if (i < len(members) - 1):
                self.buf.write('\n\t,')
        self.buf.write('\n\t{}\n\n')

    #these guys are visited during the convenience constructor
    # we have to print ref types differently
    def visit_CompoundType(self, node):
        self.buf.write('const %s&' % node.fully_qualified_name())

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_VariableArrayType(self, node):
        self.buf.write('const std::vector<%s>&' % cast_type(node.member_type))

    def visit_FixedArrayType(self, node):
        self.buf.write('const std::array<%s, %d>&' % (cast_type(node.member_type), node.length))


    def visit_PascalStringType(self, node):
        self.buf.write('const std::string&')
    
class CPPStruct_ConstructorEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        class_name = self.prefix + node.name
        self.buf.write(textwrap.dedent('''\
        {0}::{0}(const uint8_t* buff, size_t len)
        {{
        \tconst CLAD::SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
        \tUnpack(buffer);
        }}

        {0}::{0}(const CLAD::SafeMessageBuffer& buffer)
        {{
        \tUnpack(buffer);
        }}

        '''.format(class_name)))

    #these guys are visited during the convenience constructor
    # we have to print ref types differently
    def visit_CompoundType(self, node):
        self.buf.write('const %s&' % node.fully_qualified_name())

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_VariableArrayType(self, node):
        self.buf.write('const std::vector<%s>&' % cast_type(node.member_type))

    def visit_FixedArrayType(self, node):
        self.buf.write('const std::array<%s, %d>&' % (cast_type(node.member_type), node.length))


    def visit_PascalStringType(self, node):
        self.buf.write('const std::string&')

class HStruct_PackEmitter(ast.NodeVisitor):
    # Should emit a packer of the form:
    # (public) size_t Pack(uint8_t* buff, size_t buffLen)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write('\n\t' + textwrap.dedent('''\
        /**** Pack ****/
        \tsize_t Pack(uint8_t* buff, size_t len) const;
        \tsize_t Pack(CLAD::SafeMessageBuffer& buffer) const;
        '''))

class CPPStruct_PackEmitter(ast.NodeVisitor):
    # Should emit a packer of the form:
    # (public) size_t Pack(uint8_t* buff, size_t buffLen)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        class_name = self.prefix + node.name
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Pack(uint8_t* buff, size_t len) const
        {{
        \tCLAD::SafeMessageBuffer buffer(buff, len, false);
        \treturn Pack(buffer);
        }}

        size_t {0}::Pack(CLAD::SafeMessageBuffer& buffer) const
        {{
        '''.format(class_name)))
        for member in node.members():
            self.visit(member)
        self.buf.write(textwrap.dedent('''\
        \tconst size_t bytesWritten {buffer.GetBytesWritten()};
        \treturn bytesWritten;
        }

        '''))

    def visit_MessageMemberDecl(self, node):
        if (isinstance(node.type, ast.PascalStringType)):
            # this is a string!
            self.buf.write('\tbuffer.WritePString<%s>(this->%s);\n' %
                           (primitive_type_name(node.type.length_type),
                            (self.prefix + node.name)))
        elif (isinstance(node.type, ast.FixedArrayType)):
            #write the array (vector)
            if isinstance(node.type.member_type, ast.PascalStringType):
                self.buf.write('\tbuffer.WritePStringFArray<%d, %s>(this->%s);\n' %
                               (node.type.length,
                                type_translations[node.type.member_type.length_type.name],
                                (self.prefix + node.name)))
            elif isinstance(node.type.member_type, ast.CompoundType):
                self.emitCompoundTypeArrayPacker(self.prefix + node.name)
            else:
                self.buf.write('\tbuffer.WriteFArray<%s, %d>(this->%s);\n' %
                               (cast_type(node.type.member_type),
                                node.type.length,
                                (self.prefix + node.name)))
        elif (isinstance(node.type, ast.CompoundType)):
            self.buf.write('\tthis->%s.Pack(buffer);\n' % (self.prefix + node.name))
        else:
            self.buf.write('\tbuffer.Write(this->%s);\n' %
                           (self.prefix + node.name))

    def visit_MessageVariableArrayMemberDecl(self, node):
        #write the array (vector)
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.buf.write('\tbuffer.WritePStringVArray<%s, %s>(this->%s);\n' %
                           (type_translations[node.type.length_type.name],
                            type_translations[node.type.member_type.length_type.name],
                            (self.prefix + node.name)))
        elif isinstance(node.type.member_type, ast.CompoundType):
            self.buf.write('\tbuffer.Write(static_cast<{0}>({1}.size()));\n'
                           .format(type_translations[node.type.length_type.name],
                                   self.prefix + node.name));
            self.emitCompoundTypeArrayPacker(self.prefix + node.name)
        else:
            self.buf.write('\tbuffer.WriteVArray<%s, %s>(this->%s);\n' %
                           (cast_type(node.type.member_type),
                            type_translations[node.type.length_type.name],
                            (self.prefix + node.name)))

    def emitCompoundTypeArrayPacker(self, name):
        self.buf.write(textwrap.dedent('''\
        \tfor (const auto& m : {0}) {{
        \t\tm.Pack(buffer);
        \t}}
        #''')[:-1].format(name))

class HStruct_UnpackEmitter(ast.NodeVisitor):
    # Should emit an unpacker of the form:
    # (public) size_t Unpack(const uint8_t* buff, size_t& buffLen)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write('\n\t' + textwrap.dedent('''\
        /**** Unpack ****/
        \tsize_t Unpack(const uint8_t* buff, const size_t len);
        \tsize_t Unpack(const CLAD::SafeMessageBuffer& buffer);
        '''))

class CPPStruct_UnpackEmitter(ast.NodeVisitor):
    # Should emit an unpacker of the form:
    # (public) size_t Unpack(const uint8_t* buff, size_t& buffLen)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        class_name = self.prefix + node.name;
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Unpack(const uint8_t* buff, const size_t len)
        {{
        \tconst CLAD::SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
        \treturn Unpack(buffer);
        }}

        size_t {0}::Unpack(const CLAD::SafeMessageBuffer& buffer)
        {{
        ''').format(class_name));
        for member in node.members():
            self.visit(member)
        self.buf.write(textwrap.dedent('''\
        \treturn buffer.GetBytesRead();
        }

        '''))

    def visit_MessageMemberDecl(self, node):
        if (isinstance(node.type, ast.PascalStringType)):
            # string type!
            self.buf.write('\tbuffer.ReadPString<%s>(this->%s);\n' %
                           (primitive_type_name(node.type.length_type),
                            (self.prefix + node.name)))
        elif (isinstance(node.type, ast.FixedArrayType)):
            if isinstance(node.type.member_type, ast.PascalStringType):
                self.buf.write('\tbuffer.ReadPStringFArray<%d, %s>(this->%s);\n' %
                               (node.type.length,
                                type_translations[node.type.member_type.length_type.name],
                                node.name))
            elif isinstance(node.type.member_type, ast.CompoundType):
                self.buf.write('\tbuffer.ReadCompoundTypeFArray<{0}, {1}>(this->{2});\n'
                               .format(cast_type(node.type.member_type),
                                       node.type.length,
                                       node.name))
            else:
                self.buf.write('\tbuffer.ReadFArray<%s, %d>(this->%s);\n' %
                               (cast_type(node.type.member_type),
                                node.type.length,
                                node.name))
        elif (isinstance(node.type, ast.CompoundType)):
            self.buf.write('\tthis->%s.Unpack(buffer);\n' % node.name)
        else:
            self.buf.write('\tbuffer.Read(this->%s);\n' %
                           (self.prefix + node.name))

    def visit_MessageVariableArrayMemberDecl(self, node):
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.buf.write('\tbuffer.ReadPStringVArray<%s,%s>(this->%s);\n' %
                           (type_translations[node.type.length_type.name],
                            type_translations[node.type.member_type.length_type.name],
                            node.name))
        elif isinstance(node.type.member_type, ast.CompoundType):
            self.buf.write('\tbuffer.ReadCompoundTypeVArray<{0}, {1}>(this->{2});\n'
                           .format(cast_type(node.type.member_type),
                                   type_translations[node.type.length_type.name],
                                   node.name))
        else:
            self.buf.write('\tbuffer.ReadVArray<%s, %s>(this->%s);\n' %
                           (cast_type(node.type.member_type),
                            type_translations[node.type.length_type.name],
                            node.name))

class HStruct_SizeEmitter(ast.NodeVisitor):
    # Should emit a size member of the form:
    # (public) size_t Size()
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write('\n\tsize_t Size() const;\n')

class CPPStruct_SizeEmitter(ast.NodeVisitor):
    # Should emit a size member of the form:
    # (public) size_t Size()
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        class_name = self.prefix + node.name
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Size() const
        {{
        \tsize_t result = 0;
        ''').format(class_name))
        for i, member in enumerate(node.members()):
            self.buf.write('\t//%s\n' % member.name)
            self.visit(member)
        self.buf.write(textwrap.dedent('''\
        \treturn result;
        }

        '''))

    def visit_MessageMemberDecl(self, node):
        if (isinstance(node.type, ast.PascalStringType)):
            self.emitPascalStringSize(node.type, node.name)
        elif (isinstance(node.type, ast.FixedArrayType)):
            self.emitFixedSize(node.type.member_type, node.name, node.type.length)
        elif (isinstance(node.type, ast.CompoundType)):
            self.buf.write('\tresult += {0}.Size(); // = {0}\n'.format(node.name))
        else:
            self.emitBasicSize(node.type, node.name)

    def visit_MessageVariableArrayMemberDecl(self, node):
        self.buf.write('\tresult += %d; //length = %s\n' %
                       (node.type.length_type.size,
                        node.type.length_type.name))
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.emitPascalStringArraySize(node.type.member_type, node.name)
        elif isinstance(node.type.member_type, ast.CompoundType):
            self.emitCompoundTypeArraySize(node.name)
        else:
            self.buf.write('\tresult += %d * %s.size(); //member = %s\n' %
                           (node.type.member_type.size,
                            node.name,
                            node.type.member_type.builtin_type().name))
    
    def emitPascalStringArraySize(self, type, name):
        self.buf.write('\tresult += {0} * {1}.size(); // sizeof({2}) * array size\n'
                       .format(type.length_type.size, name, type.length_type.name))
        self.emitCompoundTypeArraySize(name, 'length')

    def emitCompoundTypeArraySize(self, name, size_method='Size'):
        self.buf.write(textwrap.dedent('''\
        \tfor (const auto& m : {0}) {{
        \t\tresult += m.{1}();
        \t}}
        #''')[:-1].format(name, size_method))

    def emitFixedSize(self, type, name, length):
        if isinstance(type, ast.PascalStringType):
            self.emitPascalStringArraySize(type, name)
        elif isinstance(type, ast.CompoundType):
            self.emitCompoundTypeArraySize(name)
        else:
            self.buf.write('\tresult += %d * %d; // = %s * %d\n' %
                           (type.size,
                            length,
                            type.name,
                            length))
    
    def emitPascalStringSize(self, type, name):
        self.buf.write('\tresult += %d; // length = %s\n' %
                       (type.length_type.size,
                        type.length_type.name))
        self.buf.write('\tresult += %d * %s.size(); //string\n' %
                       (type.member_type.size,
                        name))

    def emitBasicSize(self, type, name):
        self.buf.write('\tresult += %d; // = %s\n' %
                       (type.size,
                        type.name))

class HStruct_EqualityEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write('\n\t' + textwrap.dedent('''\
        bool operator==(const {0}& other) const;
        \tbool operator!=(const {0}& other) const;
        ''').format(node.name))

class CPPStruct_EqualityEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write(textwrap.dedent('''\
        bool {0}::operator==(const {0}& other) const
        {{
        '''.format(node.name)))
        if(len(node.members()) > 0):
            self.buf.write('\tif (')
            self.buf.write('%s != other.%s' %  (node.members()[0].name, node.members()[0].name))
            for member in node.members()[1:]:
                self.buf.write('\n\t|| %s != other.%s' % (member.name, member.name))
            self.buf.write(') {\n')
            self.buf.write('\t\treturn false;\n\t}\n')
        self.buf.write(textwrap.dedent('''\
        \treturn true;
        }}

        bool {0}::operator!=(const {0}& other) const
        {{
        \treturn !(operator==(other));
        }}

        '''.format(node.name)))

class HUnionEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_UnionDecl(self, node):
        header = textwrap.dedent('''\
            // UNION {union_name}
            class {union_name}
            {{
            ''').format(union_name=node.name)
        footer = '};\n\n'
        members = node.members()
        self.buf.write(header)
        #public stuff
        self.buf.write('public:\n')
        self.emitUsingTag(node)
        self.emitConstructors(node)
        self.emitDestructor(node)
        self.emitGetTag(node)
        self.emitAccessors(members)
        self.emitUnpack(members)
        self.emitPack(members)
        self.emitSize(node, members)
        #private stuff
        self.buf.write('private:\n')
        self.emitClearCurrent(members)
        self.emitTagMember(node)
        self.emitUnion(members)
        self.buf.write(footer)

    def emitUsingTag(self, node):
        self.buf.write('\tusing Tag = {0}Tag;\n'.format(node.name))

    def emitConstructors(self, node):
        self.buf.write('\t' + textwrap.dedent('''\
        /**** Constructors ****/
        \t{0}() :_tag(Tag::INVALID) {{ }}
        \texplicit {0}(const CLAD::SafeMessageBuffer& buff);
        \texplicit {0}(const uint8_t* buffer, size_t length);

        ''').format(node.name))

    def emitDestructor(self, node):
        self.buf.write('\t~%s() { ClearCurrent(); }\n' % node.name)
        
    def emitUnion(self, members):
        self.buf.write('\n\tunion {\n')
        for i, member in enumerate(members):
            HStructEmitter_MessageMemberVisitor(buf=self.buf,
                                                prefix='_',
                                                member_name=member.name,
                                                depth=2).visit(member)
        self.buf.write('\t};\n')

    def emitGetTag(self, node):
        self.buf.write('\tTag GetTag() const { return _tag; }\n\n')

    def emitTagMember(self, node):
        self.buf.write('\tTag _tag;\n')

    def emitAccessors(self, members):
        for i, member in enumerate(members):
            HUnion_AccessorEmitter(buf=self.buf,
                                   prefix='_').visit(member)

    def emitClearCurrent(self, members):
        self.buf.write('\tvoid ClearCurrent();\n')

    def emitPack(self, members):
        self.buf.write('\n\tsize_t Pack(uint8_t* buff, size_t len) const;\n')
        self.buf.write('\tsize_t Pack(CLAD::SafeMessageBuffer& buffer) const;\n')

    def emitUnpack(self, members):
        self.buf.write('\n\tsize_t Unpack(const uint8_t* buff, const size_t len);\n')
        self.buf.write('\tsize_t Unpack(const CLAD::SafeMessageBuffer& buffer);\n')

    def emitSize(self, node, members):
        self.buf.write('\n\tsize_t Size() const;\n')

class TagHUnionTagEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_IncludeDecl(self, node, *args, **kwargs):
        pass

    def visit_NamespaceDecl(self, node, *args, **kwargs):
        self.buf.write('namespace {namespace_name} {{\n\n'.format(namespace_name=node.name))
        for c_name, c in node.children():
            self.visit(c, *args, **kwargs)
        self.buf.write('}} // namespace {namespace_name}\n\n'.format(namespace_name=node.name))

    def visit_UnionDecl(self, node):
        members = node.members()
        self.emitTags(node, members)
        self.buf.write(textwrap.dedent('''\
        const char* {0}ToString(const {0} type);
        '''.format(node.name + 'Tag')))

    def emitTags(self, node, members):
        # Tags are 1 byte only for now!
        self.buf.write('enum class {0}Tag : uint8_t {{\n'.format(node.name))
        for i, member in enumerate(members):
            self.buf.write('\t%s,\t// %d\n' % (member.name, i))
        self.buf.write(textwrap.dedent('''\
        \tINVALID
        };

        '''))

class TagHHashEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_IncludeDecl(self, node, *args, **kwargs):
        pass

    def visit_UnionDecl(self, node):
        self.buf.write(textwrap.dedent('''\
        template<>
        struct std::hash<{0}>
        {{
        \tsize_t operator()({0} t) const
        \t{{
        \t\treturn static_cast<std::underlying_type<{0}>::type>(t);
        \t}}
        }};
        
        ''').format(node.fully_qualified_name() + 'Tag'))

class CPPUnionEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_UnionDecl(self, node):
        header = "// UNION {union_name}\n\n".format(union_name=node.name)
        footer = "\n"
        members = node.members()
        self.buf.write(header)
        self.emitTagToString(node, members)
        self.emitConstructors(node)
        self.emitAccessors(node, members)
        self.emitUnpack(node, members)
        self.emitPack(node, members)
        self.emitSize(node, members)
        self.emitClearCurrent(node, members)
        self.buf.write(footer)

    def emitTagToString(self, node, members):
        self.buf.write(textwrap.dedent('''\
        const char* {0}TagToString(const {0}Tag tag) {{
        \tswitch (tag) {{
        '''.format(node.name)))
        for i, member in enumerate(members):
            self.buf.write(textwrap.dedent('''\
            \tcase {0}Tag::{1}:
            \t\treturn "{1}";
            #''')[:-1].format(node.name, member.name))
        self.buf.write(textwrap.dedent('''\
        \tdefault:
        \t\treturn "INVALID";
        \t}
        }

        '''))

    def emitConstructors(self, node):
        self.buf.write(textwrap.dedent('''\
        {0}::{0}(const CLAD::SafeMessageBuffer& buff) :_tag(Tag::INVALID)
        {{
        \tUnpack(buff);
        }}

        {0}::{0}(const uint8_t* buffer, size_t length) :_tag(Tag::INVALID)
        {{
        \tCLAD::SafeMessageBuffer buff(const_cast<uint8_t*>(buffer), length);
        \tUnpack(buff);
        }}

        ''').format(node.name))

    def emitAccessors(self, node, members):
        for i, member in enumerate(members):
            CPPUnion_AccessorEmitter(buf=self.buf,
                                     prefix='_',
                                     class_name=node.name).visit(member)

    def emitClearCurrent(self, node, members):
        self.buf.write(textwrap.dedent('''\

        void {0}::ClearCurrent()
        {{
        \tswitch(GetTag()) {{
        ''').format(node.name))
        for i, member in enumerate(members):
            self.buf.write('\tcase Tag::{0}:\n'.format(member.name))
            if (not primitive_type_name(member.type)):
                # need to emit a destructor
                self.buf.write('\t\t_%s.~%s();\n' % (member.name,
                                                      member.type.name))
            self.buf.write('\t\tbreak;\n')
        self.buf.write(textwrap.dedent('''\
        \tdefault:
        \t\tbreak;
        \t}
        \t_tag = Tag::INVALID;
        }
        '''))

    def emitPack(self, node, members):
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Pack(uint8_t* buff, size_t len) const
        {{
        \tCLAD::SafeMessageBuffer buffer(buff, len, false);
        \treturn Pack(buffer);
        }}

        size_t {0}::Pack(CLAD::SafeMessageBuffer& buffer) const
        {{
        \tbuffer.Write(_tag);
        \tswitch(GetTag()) {{
        ''').format(node.name))
        for i, member in enumerate(members):
            self.buf.write('\tcase Tag::{0}:\n'.format(member.name))
            CPPUnion_PackHelper(buf=self.buf, prefix='_', member_name=member.name).visit(member)
            self.buf.write('\t\tbreak;\n')
        self.buf.write(textwrap.dedent('''\
        \tdefault:
        \t\tbreak;
        \t}
        \tconst size_t bytesWritten {buffer.GetBytesWritten()};
        \treturn bytesWritten;
        }

        '''))

    def emitUnpack(self, node, members):
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Unpack(const uint8_t* buff, const size_t len)
        {{
        \tconst CLAD::SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
        \treturn Unpack(buffer);
        }}

        size_t {0}::Unpack(const CLAD::SafeMessageBuffer& buffer)
        {{
        \tTag newTag {{Tag::INVALID}};
        \tconst Tag oldTag {{GetTag()}};
        \tbuffer.Read(newTag);
        \tif (newTag != oldTag) {{
        \t\tClearCurrent();
        \t}}
        \tswitch(newTag) {{
        ''').format(node.name))
        for i, member in enumerate(members):
            self.buf.write('\tcase Tag::{0}:\n'.format(member.name))
            CPPUnion_UnpackHelper(buf=self.buf, prefix='_', member_name=member.name).visit(member)
            self.buf.write('\t\tbreak;\n')
        self.buf.write(textwrap.dedent('''\
        \tdefault:
        \t\tbreak;
        \t}
        \t_tag = newTag;
        \treturn buffer.GetBytesRead();
        }

        '''))

    def emitSize(self, node, members):
        # Tags are one byte only for now!
        self.buf.write(textwrap.dedent('''\
        size_t {0}::Size() const
        {{
        \tsize_t result {{1}}; // tag = uint_8
        \tswitch(GetTag()) {{
        ''').format(node.name))
        for i, member in enumerate(members):
            self.buf.write('\tcase Tag::{0}:\n'.format(member.name))
            CPPUnion_SizeHelper(buf=self.buf, prefix='_', member_name=member.name).visit(member)
            self.buf.write('\t\tbreak;\n')
        self.buf.write(textwrap.dedent('''\
        \tdefault:
        \t\treturn 0;
        \t}
        \treturn result;
        }

        '''))
            

class CPPUnion_PackHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name

    def visit_CompoundType(self, node):
        self.buf.write('\t\tthis->%s.Pack(buffer);\n' % (self.prefix + self.member_name))

    def visit_BuiltinType(self, node):
        self.buf.write('\t\tbuffer.Write(this->%s);\n' % (self.prefix + self.member_name))

    def visit_VariableArrayType(self, node):
        self.buf.write('\t\tbuffer.WriteVArray<%s, %s>(this->%s);\n' %
                       (cast_type(node.type.member_type),
                        primitive_type_name(node.type.length_type),
                        self.prefix + self.member_name))

class CPPUnion_UnpackHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name

    def visit_CompoundType(self, node):
        self.buf.write('\t\tif (newTag != oldTag) {\n')
        self.buf.write('\t\t\tnew(&(this->%s)) %s(buffer);\n' % (self.prefix + self.member_name,
                                                                 node.fully_qualified_name()))
        self.buf.write('\t\t}\n')
        self.buf.write('\t\telse {\n')
        self.buf.write('\t\t\tthis->%s.Unpack(buffer);\n' % (self.prefix + self.member_name))
        self.buf.write('\t\t}\n')
        
    def visit_BuiltinType(self, node):
        self.buf.write('\t\tbuffer.Read(this->%s);\n' % (self.prefix + self.member_name))

    def visit_VariableArrayType(self, node):
        self.buf.write('\t\tbuffer.ReadVArray<%s, %s>(this->%s);\n' %
                       (cast_type(node.type.member_type),
                        primitive_type_name(node.type.length_type),
                        self.prefix + self.member_name))
            
class HUnion_AccessorEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageMemberDecl(self, node):
        self.buf.write('\t/** %s **/\n' % node.name)
        self.emitGetter(node)
        self.emitSetter(node)
        self.buf.write('\n')
        
    def emitGetter(self, node):
        self.buf.write('\t')
        self.visit(node.type)
        self.buf.write(' Get_%s() const;\n' % node.name)

    def emitSetter(self, node):
        self.buf.write('\tvoid Set_%s(' % node.name)
        self.visit(node.type)
        self.buf.write(' new_%s);\n' % node.name)
        # emit r-val setter if this is a complex type
        if(not (isinstance(node.type, ast.BuiltinType) or isinstance(node.type, ast.BuiltinType))):
            self.emitRValSetter(node)

    def emitRValSetter(self, node):
        self.buf.write('\tvoid Set_{0}({1}&& new_{0});\n'.format(node.name,
                                                                 node.type.fully_qualified_name()))

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_CompoundType(self, node):
        self.buf.write('const %s&' % node.fully_qualified_name())

class CPPUnion_AccessorEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', class_name=''):
        self.buf = buf
        self.prefix = prefix
        self.class_name = class_name

    def visit_MessageMemberDecl(self, node):
        self.emitGetter(node)
        self.emitSetter(node)
        self.buf.write('\n')

    def emitGetter(self, node):
        self.visit(node.type)
        self.buf.write(textwrap.dedent('''\
         {0}::Get_{1}() const
        {{
        \tassert(_tag == Tag::{1});
        \treturn {2}{1};
        }}
        ''').format(self.class_name, node.name, self.prefix))

    def emitSetter(self, node):
        self.buf.write('void {0}::Set_{1}('.format(self.class_name, node.name))
        self.visit(node.type)
        self.buf.write(textwrap.dedent('''\
         new_{0})
        {{
        \tif(this->_tag == Tag::{0}) {{
        \t\t{1} = new_{0};
        \t}}
        \telse {{
        \t\tClearCurrent();
        ''').format(node.name, self.prefix + node.name))
        if(not primitive_type_name(node.type)):
            self.buf.write('\t\tnew(&%s) %s{new_%s};\n' % (self.prefix + node.name,
                                                             cast_type(node.type),
                                                             node.name))
        else:
            self.buf.write('\t\t%s = new_%s;\n' % (self.prefix + node.name, node.name))
        self.buf.write(textwrap.dedent('''\
        \t\t_tag = Tag::{0};
        \t}}
        }}
        ''').format(node.name))
        # emit r-val setter if this is a complex type
        if(not (isinstance(node.type, ast.BuiltinType) or isinstance(node.type, ast.BuiltinType))):
            self.emitRValSetter(node)

    def emitRValSetter(self, node):
        self.buf.write(textwrap.dedent('''\
        void {0}::Set_{1}({2}&& new_{1})
        {{
        \tif(this->_tag == Tag::{1}) {{
        \t\t{3} = std::move(new_{1});
        \t}}
        \telse {{
        \t\tClearCurrent();
        \t\tnew(&{3}) {4}{{std::move(new{3})}};
        \t\t_tag = Tag::{1};
        \t}}
        }}

        ''').format(self.class_name, node.name, node.type.fully_qualified_name(),
                    self.prefix + node.name, cast_type(node.type)))

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name())

    def visit_CompoundType(self, node):
        self.buf.write('const %s&' % node.fully_qualified_name())

class CPPUnion_SizeHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name

    def visit_BuiltinType(self, node):
        self.buf.write('\t\tresult += %d; // = %s\n' %
                       (self.prefix + self.member_name))

    def visit_DefinedType(self, node):
        self.buf.write('\t\tresult += %d; // = %s\n' %
                       (node.size, node.builtin_type().name))

    def visit_CompoundType(self, node):
        self.buf.write('\t\tresult += %s.Size();\n' %
                       (self.prefix + self.member_name))

class UnionDiscoverer(ast.NodeVisitor):
    
    found = False
    
    def visit_IncludeDecl(self, node):
        pass
    
    def visit_UnionDecl(self, node):
        self.found = True

if __name__ == '__main__':
    from clad import emitterutil
    
    language = 'C++'
    header_extension = '.h'
    source_extension = '.cpp'
    
    option_parser = emitterutil.StandardArgumentParser(language)
    option_parser.add_argument('-r', '--header-output-directory', metavar='dir',
        help='The directory to output the {language} header file(s) to.'.format(language=language))
    option_parser.add_argument('--header-output-extension', default=header_extension, metavar='ext',
        help='The extension to use for header files. (Helps work around a CMake Xcode issue.)')
    
    options = option_parser.parse_args()
    if not options.header_output_directory:
        options.header_output_directory = options.output_directory
    header_extension = options.header_output_extension
    
    tree = emitterutil.parse(options)
    comment_lines = emitterutil.get_comment_lines(options, language)
    
    union_discoverer = UnionDiscoverer()
    union_discoverer.visit(tree)
    
    if union_discoverer.found:
        
        def tag_output_header_callback(output):
            for emitter_type in [TagHUnionTagEmitter, TagHHashEmitter]:
                emitter_type(output).visit(tree)
        
        tag_output_header = emitterutil.get_output_file(options, 'Tag' + header_extension)
        emitterutil.write_c_file(options.header_output_directory, tag_output_header,
            tag_output_header_callback,
            comment_lines, use_inclusion_guards=True,
            system_headers=['functional'])
        
        main_output_header_local_headers = [tag_output_header]
    else:
        main_output_header_local_headers = None
    
    
    def main_output_header_callback(output):
        HNamespaceEmitter(output, include_extension=header_extension).visit(tree)
    
    main_output_header = emitterutil.get_output_file(options, header_extension)
    emitterutil.write_c_file(options.header_output_directory, main_output_header,
        main_output_header_callback,
        comment_lines=comment_lines,
        use_inclusion_guards=True,
        system_headers=['array', 'cassert', 'cstdint', 'string', 'vector', 'CLAD/SafeMessageBuffer.h'],
        local_headers=main_output_header_local_headers)
    
    
    def main_output_source_callback(output):
        CPPNamespaceEmitter(output).visit(tree)
    
    main_output_source = emitterutil.get_output_file(options, source_extension)
    emitterutil.write_c_file(options.output_directory, main_output_source,
        main_output_source_callback,
        comment_lines=comment_lines,
        use_inclusion_guards=False,
        local_headers=[main_output_header])
