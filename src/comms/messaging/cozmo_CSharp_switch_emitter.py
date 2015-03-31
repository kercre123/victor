#! /usr/bin/python

# here is a quick snippet to allow including the parser stuff.

import os, sys, inspect
# realpath() will make your script run, even if you symlink it :)
cmd_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

# use this if you want to include modules from a subfolder
cmd_subfolder = os.path.realpath(os.path.abspath(os.path.join(os.path.split(inspect.getfile( inspect.currentframe() ))[0], "..", "clad")))
if cmd_subfolder not in sys.path:
    sys.path.insert(0, cmd_subfolder)

# Info:
# cmd_folder = os.path.dirname(os.path.abspath(__file__)) # DO NOT USE __file__ !!!
# __file__ fails if script is called in different ways on Windows
# __file__ fails if someone does os.chdir() before
# sys.argv[0] also fails because it doesn't not always contains the path

import clad
import ast
import sys
import textwrap

# primitive type translation map
type_translations = {
    'bool': 'bool',
    'int_8': 'sbyte',
    'int_16': 'short',
    'int_32': 'int',
    'int_64': 'long',
    'uint_8': 'byte',
    'uint_16': 'ushort',
    'uint_32': 'uint',
    'uint_64': 'ulong',
    'float_32': 'float',
    'float_64': 'double'
}

# primitive type reading translation map (for BinaryReader)
read_translations = {
    'bool': 'ReadBoolean',
    'int_8': 'ReadSByte',
    'int_16': 'ReadInt16',
    'int_32': 'ReadInt32',
    'int_64': 'ReadInt64',
    'uint_8': 'ReadByte',
    'uint_16': 'ReadUInt16',
    'uint_32': 'ReadUInt32',
    'uint_64': 'ReadUInt64',
    'float_32': 'ReadSingle',
    'float_64': 'ReadDouble'
}

def primitive_type_name(type):
    type_name = type.name
    return type_translations[type_name] if type_name in type_translations else None

def cast_type(type):
    if primitive_type_name(type) is not None:
        return type_translations[type.name]
    elif isinstance(type, ast.PascalStringType):
        return "string"
    else:
        return type.fully_qualified_name('.')

def get_namespace_header_footer(node):
    namespace_header = ""
    namespace_footer = ""
    namespace = node.namespace
    while True:
        if namespace is None:
            break
        namespace_header = "namespace {0} {{\n".format(namespace.name) + namespace_header
        namespace_footer = "}} // namespace {0}\n".format(namespace.name) + namespace_footer
        namespace = namespace.namespace
    return (namespace_header, namespace_footer)

class CSharpEnumEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf
        
    def visit_EnumDecl(self, node):
        namespace_header, namespace_footer = get_namespace_header_footer(node)
        #print the header
        header = textwrap.dedent("""
        public enum {0}
        {{
        """.format(node.name))
        self.buf.write(namespace_header)
        self.buf.write(header)
        enum_val = 0
        members = node.members()
        #print each member of the enum
        for i, member in enumerate(members):
            enum_val = self.emitEnum(member, enum_val, i < len(members) - 1, prefix='')
        #print the footer
        footer = '};\n\n'
        self.buf.write(footer)
        self.buf.write(namespace_footer)

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

class CSharpStructEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout):
        self.buf = buf

    def visit_MessageDecl(self, node):
        namespace_header, namespace_footer = get_namespace_header_footer(node)
        header = 'public class %s\n{\n' % node.name
        footer = '}\n\n'
        self.buf.write(namespace_header)
        self.buf.write(header)
        members = node.members()
        for i, member in enumerate(members):
            CSharpDefsEmitter_MessageMemberVisitor(buf=self.buf,
                                                   prefix='',
                                                   member_name=member.name).visit(member)
        CSharpStruct_ConstructorEmitter(buf=self.buf, prefix='').visit(node)
        CSharpStruct_PackEmitter(buf=self.buf, prefix='').visit(node)
        CSharpStruct_UnpackEmitter(buf=self.buf, prefix='').visit(node)
        CSharpStruct_SizeEmitter(buf=self.buf, prefix='').visit(node)
        self.buf.write(footer)
        self.buf.write(namespace_footer)

    def visit_UnionDecl(self, node):
        pass

class CSharpDefsEmitter_MessageMemberVisitor(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name='', depth=1):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name
        self.depth = depth

    def visit_MessageMemberDecl(self, member):
        # Visiting a plain member decl for a message
        # first, print the type
        self.buf.write('%spublic ' % ('\t' * self.depth))
        self.visit(member.type)
        self.buf.write(" %s%s;\n" % (self.prefix, self.member_name))

    def visit_MessageVariableArrayMemberDecl(self, member):
        # Visiting a v-array member decl for a message
        # C# arrays are automatically variable length
        # should look like
        # public int[] foo;
        self.buf.write('%spublic ' % ('\t' * self.depth))
        self.visit(member.type.member_type)
        self.buf.write('[] ' + self.prefix + member.name + ';\n')
        
    def visit_BuiltinType(self, node):
        #Visiting a builtin type for the message member decl
        self.buf.write(type_translations[node.name])

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name('.'))

    def visit_CompoundType(self, node):
        self.buf.write(node.fully_qualified_name('.'))

    def visit_PascalStringType(self, node):
        self.buf.write('string')
        
    def visit_VariableArrayType(self, node):
        #Arrays of arrays are not supported currently
        # TODO: print the error and die here
        pass
    
    def visit_FixedArrayType(self, node):
        self.visit(node.member_type)
        self.buf.write('[]')


class CSharpStruct_ConstructorEmitter(ast.NodeVisitor):
    # Should emit an unpack constructor of the form:
    # public Foo(System.IO.Stream stream)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix
        
    def visit_MessageDecl(self, node):
        self.buf.write('\n\t/**** Constructors ****/\n')
        # default constructor
        self.emitDefaultConstructor(node)
        # value constructor
        self.emitValueConstructor(node)
        #unpack constructor
        self.buf.write('\n\tpublic %s(System.IO.Stream stream)\n\t\t: this()\n\t{\n' % node.name)
        if node.members():
            self.buf.write('\t\tSystem.IO.BinaryReader reader = new System.IO.BinaryReader(stream);\n')
        for member in node.members():
            CSharpStruct_UnpackEmitter(buf=self.buf, prefix=self.prefix).visit(member)
        self.buf.write('\t}\n')

    def emitDefaultConstructor(self, node):
        self.buf.write('\n\tpublic %s()\n\t{\n' % node.name)
        for member in node.members():
            if isinstance(member.type, ast.FixedArrayType):
                self.buf.write('\t\tthis.%s = new %s[%s];\n' % (self.prefix + member.name, cast_type(member.type.member_type), member.type.length))
            elif isinstance(member.type, ast.PascalStringType):
                self.buf.write('\t\tthis.%s = System.String.Empty;\n' % (self.prefix + member.name))
            elif isinstance(member.type, ast.VariableArrayType):
                self.buf.write('\t\tthis.%s = new %s[0];\n' % (self.prefix + member.name, cast_type(member.type.member_type)))
        self.buf.write('\t}\n')

    def emitValueConstructor(self, node):
        members = node.members()
        if(0 >= len(members)):
            return
        self.buf.write('\n\tpublic %s(' % node.name)
        for i, member in enumerate(node.members()):
            self.visit(member)
            self.buf.write(' %s' % member.name) #print name
            if (i < len(members) - 1):
                self.buf.write(',\n\t\t')
        self.buf.write(')\n\t{\n')
        for i, member in enumerate(node.members()):
            if isinstance(member.type, ast.FixedArrayType):
                self.buf.write('\t\tif (%s.Length != %d) {\n' % (member.name, member.type.length))
                self.buf.write('\t\t\tthrow new System.ArgumentException("Argument has wrong length. Expected length %d.", "%s");\n' % (member.type.length, member.name))
                self.buf.write('\t\t}\n')
            self.buf.write('\t\tthis.%s = %s;\n' % (self.prefix + member.name, member.name))
        self.buf.write('\t}\n')
        
    def visit_CompoundType(self, node):
        self.buf.write(node.fully_qualified_name('.'))

    def visit_DefinedType(self, node):
        self.buf.write(node.fully_qualified_name('.'))

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_VariableArrayType(self, node):
        self.buf.write('%s[]' % cast_type(node.member_type))

    def visit_FixedArrayType(self, node):
        self.visit_VariableArrayType(node)
    
    def visit_PascalStringType(self, node):
        self.buf.write('string')
    
class CSharpStruct_PackEmitter(ast.NodeVisitor):
    # Should emit a packer of the form:
    # public System.IO.Stream Pack(System.IO.Stream stream)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix
    
    def visit_MessageDecl(self, node):
        self.buf.write('\n\t/**** Pack ****/\n')
        self.buf.write('\tpublic System.IO.Stream Pack(System.IO.Stream stream)\n\t{\n')
        if node.members():
            self.buf.write('\t\tSystem.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);\n')
        for member in node.members():
            self.visit(member)
        self.buf.write('\t\treturn stream;\n')
        self.buf.write('\t}\n')
    
    def write_PascalStringType(self, length_type, node, indent='\t\t', suffix=''):
        self.buf.write(indent + textwrap.dedent('''\
        writer.Write(({0})({1}{3}.Length));
        {2}writer.Write(System.Text.Encoding.UTF8.GetBytes({1}{3}));
        ''').format(primitive_type_name(length_type), node.name, indent, suffix))

    def write_PascalStringTypeArrayMember(self, node):
        self.write_PascalStringType(node.type.member_type.length_type, node, '\t\t\t', '[i]')

    def visit_MessageMemberDecl(self, node):
        if (isinstance(node.type, ast.PascalStringType)):
            self.write_PascalStringType(node.type.length_type, node)
        elif (isinstance(node.type, ast.FixedArrayType)):
            self.buf.write('\t\tfor (int i = 0; i < %s.Length; i++) {\n' % node.name)
            if isinstance(node.type.member_type, ast.PascalStringType):
                self.write_PascalStringTypeArrayMember(node)
            else:
                self.buf.write('\t\t\twriter.Write((%s)%s[i]);\n' %
                               (type_translations[node.type.member_type.builtin_type().name], node.name))
            self.buf.write('\t\t}\n')
        else:
            self.buf.write('\t\twriter.Write((%s)%s);\n' %
                           (type_translations[node.type.builtin_type().name], node.name))
    
    def visit_MessageVariableArrayMemberDecl(self, node):
        # write the length
        self.buf.write('\t\twriter.Write((%s)(%s.Length));\n' %
                       (type_translations[node.type.length_type.builtin_type().name], node.name))
        #write the array
        self.buf.write('\t\tfor (int i = 0; i < %s.Length; i++) {\n' % node.name)
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.write_PascalStringTypeArrayMember(node)
        else:
            self.buf.write('\t\t\twriter.Write((%s)%s[i]);\n' %
                           (type_translations[node.type.member_type.builtin_type().name],
                            node.name))
        self.buf.write('\t\t}\n')

class CSharpStruct_UnpackEmitter(ast.NodeVisitor):
    # Should emit an unpacker of the form:
    # public System.IO.Stream UnpackFoo(System.IO.Stream stream)
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write('\n\t/**** Unpack ****/\n')
        self.buf.write('\tpublic System.IO.Stream Unpack(System.IO.Stream stream)\n\t{\n')
        if node.members():
            self.buf.write('\t\tSystem.IO.BinaryReader reader = new System.IO.BinaryReader(stream);\n')
        for member in node.members():
            self.visit(member)
        self.buf.write('\t\treturn stream;\n')
        self.buf.write('\t}\n')

    def read_PascalStringType(self, length_type, node, var_prefix, indent='\t\t', suffix=''):
        self.buf.write(indent + textwrap.dedent('''\
        {length_type} {prefix}len = reader.{read_func}();
        {indent}byte[] {prefix}buf = reader.ReadBytes((int){prefix}len);
        {indent}{name}{suffix} = System.Text.Encoding.UTF8.GetString({prefix}buf,0,{prefix}len);
        ''').format(length_type=primitive_type_name(length_type),
                    prefix=var_prefix,
                    read_func=read_translations[length_type.name],
                    indent=indent,
                    name=node.name,
                    suffix=suffix))

    def read_PascalStringTypeArrayMember(self, node):
        self.read_PascalStringType(node.type.member_type.length_type, node, '', '\t\t\t', '[i]')

    def visit_MessageMemberDecl(self, node):
        self.buf.write('\t\t//%s\n' % node.name)
        if (isinstance(node.type, ast.PascalStringType)):
            self.read_PascalStringType(node.type.length_type, node, node.name + "_")
        elif (isinstance(node.type, ast.FixedArrayType)):
            self.buf.write('\t\tfor (int i = 0; i < %s; i++) {\n' % node.type.length)
            if isinstance(node.type.member_type, ast.PascalStringType):
                self.read_PascalStringTypeArrayMember(node)
            else:
                self.buf.write('\t\t\t%s[i] = (%s)reader.%s();\n' %
                               (node.name,
                                cast_type(node.type.member_type),
                                read_translations[node.type.member_type.builtin_type().name]))
            self.buf.write('\t\t}\n')
        else:
            self.buf.write('\t\t%s = (%s)(reader.%s());\n' %
                           (node.name, cast_type(node.type),
                            read_translations[node.type.builtin_type().name]))
        
    def visit_MessageVariableArrayMemberDecl(self, node):
        self.buf.write('\n\t\t' + textwrap.dedent('''\
        //{node_name}[]
        \t\t{length_type} {length_var_name} = ({length_type})(reader.{read_func}());
        \t\t{node_name} = new {member_length_type}[{length_var_name}];
        \t\tfor ({length_type} i = 0; i < {length_var_name}; i++) {{
        ''').format(node_name=node.name, length_var_name=node.name + "_length",
                    length_type=type_translations[node.type.length_type.name],
                    read_func=read_translations[node.type.length_type.name],
                    member_length_type=cast_type(node.type.member_type)))
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.read_PascalStringTypeArrayMember(node)
        else:
            self.buf.write('\t\t\t%s[i] = (%s)reader.%s();\n' %
                           (node.name,
                            cast_type(node.type.member_type),
                            read_translations[node.type.member_type.builtin_type().name]))
        self.buf.write('\t\t}\n')

class CSharpStruct_SizeEmitter(ast.NodeVisitor):
    #Should emit a member function of the form:
    # ulong Size()
    # Which returns the expected buffer size of this packed struct
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_MessageDecl(self, node):
        self.buf.write("\tpublic int Size\n")
        self.buf.write("\t{\n")
        self.buf.write("\t\tget {\n")
        self.buf.write("\t\t\tint result = 0;\n")
        for i, member in enumerate(node.members()):
            self.buf.write('\t\t\t//%s\n' % member.name)
            self.visit(member)
        self.buf.write('\t\t\treturn result;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t}\n')
        
    def visit_MessageMemberDecl(self, node):
        if (isinstance(node.type, ast.PascalStringType)):
            self.emitPascalStringSize(node.type, node.name)
        elif (isinstance(node.type, ast.FixedArrayType)):
            self.emitFixedSize(node.type.member_type, node.name, node.type.length)
        else:
            self.emitBasicSize(node.type, node.name)
            
    def visit_MessageVariableArrayMemberDecl(self, node):
        self.buf.write('\t\t\tresult += %d; //length = %s\n' %
                       (node.type.length_type.size,
                        node.type.length_type.name))
        if isinstance(node.type.member_type, ast.PascalStringType):
            self.buf.write('\t\t\t' + textwrap.dedent('''\
            foreach (string str in {0}) {{
            \t\t\t\tresult += {1}; //length = {1}
            \t\t\t\tresult += str.Length;
            \t\t\t}}
            ''').format(node.name, node.type.member_type.length_type.size))
        else:
            self.buf.write('\t\t\tresult += %d * %s.Length; //member = %s\n' %
                           (node.type.member_type.size,
                            node.name,
                            node.type.member_type.builtin_type().name))
        
    def emitPascalStringSize(self, type, name):
        self.buf.write('\t\t\tresult += %d; //length = %s\n' %
                       (type.length_type.size,
                        type.length_type.name))
        self.buf.write('\t\t\tresult += %d * %s.Length; //string\n' %
                       (type.member_type.size,
                        name))
    
    def emitFixedSize(self, type, name, length):
        if isinstance(type, ast.PascalStringType):
            self.buf.write('\t\t\t' + textwrap.dedent('''\
            foreach (string str in {0}) {{
            \t\t\t\tresult += {1}; //length = {1}
            \t\t\t\tresult += str.Length;
            \t\t\t}}
            ''').format(name, type.length_type.size))
        else:
            self.buf.write('\t\t\tresult += %d * %d; // = %s * %d\n' %
                           (type.size,
                            length,
                            type.name,
                            length))
    
    def emitBasicSize(self, type, name):
        self.buf.write('\t\t\tresult += %d; // = %s\n' %
                       (type.size,
                        type.name))
        
class CSharpUnionEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix

    def visit_UnionDecl(self, node):
        header = "public class %s {\n" % (self.prefix + node.name)
        footer = '}\n\n'
        members = node.members()
        self.buf.write(header)
        #union stuff
        self.emitTags(members)
        self.emitTagMember()
        self.emitGetTag(members)
        self.emitUnion(members)
        self.emitAccessors(members)
        self.emitUnpack(members)
        self.emitPack(members)
        self.emitSize(members)
        #end union stuff
        self.buf.write(footer)
        
    def emitTags(self, members):
        self.buf.write('\tpublic enum Tag {\n')
        for i, member in enumerate(members):
            self.buf.write('\t\t%s,\t//%d\n' % (member.name, i))
        self.buf.write('\t\tINVALID\n')
        self.buf.write('\t};\n\n')
        
    def emitTagMember(self):
        self.buf.write('\tprivate Tag _tag = Tag.INVALID;\n\n')
        
    def emitUnion(self, members):
        self.buf.write('\tprivate object _state = null;\n\n')
    def emitGetTag(self, members):
        self.buf.write('\tpublic Tag GetTag() { return _tag; }\n\n')
        
    def emitAccessors(self, members):
        for i, member in enumerate(members):
            CSharpUnion_AccessorEmitter(buf=self.buf,
                                     prefix='_').visit(member)
    
    def emitPack(self, members):
        self.buf.write('\tpublic System.IO.Stream Pack(System.IO.Stream stream)\n\t{\n')
        self.buf.write('\t\tSystem.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);\n')
        self.buf.write('\t\twriter.Write((%s)GetTag());\n' % type_translations['uint_8'])
        self.buf.write('\t\tswitch(GetTag()) {\n')
        for i, member in enumerate(members):
            self.buf.write('\t\tcase Tag.%s:\n' % member.name)
            CSharpUnion_PackHelper(buf=self.buf, prefix='', member_name=member.name).visit(member)
            self.buf.write('\t\t\tbreak;\n')
        self.buf.write('\t\tdefault:\n')
        self.buf.write('\t\t\tbreak;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t\treturn stream;\n')
        self.buf.write('\t}\n\n')

    def emitUnpack(self, members):
        self.buf.write('\tpublic System.IO.Stream Unpack(System.IO.Stream stream)\n\t{\n')
        # First read the type tag and save the current type
        self.buf.write('\t\tSystem.IO.BinaryReader reader = new System.IO.BinaryReader(stream);\n')
        self.buf.write('\t\tTag newTag = Tag.INVALID;\n')
        self.buf.write('\t\tnewTag = (Tag)reader.%s();\n' % read_translations['uint_8'])
        # Then switch on the type to unpack the actual type
        self.buf.write('\t\tswitch(newTag) {\n')
        for i, member in enumerate(members):
            self.buf.write('\t\tcase Tag.%s:\n' % member.name)
            CSharpUnion_UnpackHelper(buf=self.buf, prefix='', member_name=member.name).visit(member)
            self.buf.write('\t\t\tbreak;\n')
        self.buf.write('\t\tdefault:\n')
        self.buf.write('\t\t\tbreak;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t\t_tag = newTag;\n')
        self.buf.write('\t\treturn stream;\n')
        self.buf.write('\t}\n\n')

    def emitSize(self, members):
        self.buf.write('\tpublic int Size\n\t{\n')
        self.buf.write('\t\tget {\n')
        self.buf.write('\t\t\tint result = %d; // tag = %s\n' % (1, 'uint_8'))
        self.buf.write('\t\t\tswitch(GetTag()) {\n')
        for i, member in enumerate(members):
            self.buf.write('\t\t\tcase Tag.%s:\n' % member.name)
            CSharpUnion_SizeHelper(buf=self.buf, prefix='', member_name=member.name).visit(member)
            self.buf.write('\t\t\t\tbreak;\n')
        self.buf.write('\t\t\tdefault:\n')
        self.buf.write('\t\t\t\treturn 0;\n')
        self.buf.write('\t\t\t}\n')
        self.buf.write('\t\t\treturn result;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t}\n')
        
class CSharpUnion_PackHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name

    def visit_CompoundType(self, node):
        self.buf.write('\t\t\t%s.Pack(stream);\n' % (self.prefix + self.member_name))

    def visit_BuiltinType(self, node):
        self.buf.write('\t\t\twriter.Write((%s)%s);\n' % (cast_type(node),
                                                          self.prefix + self.member_name))

    def visit_VariableArrayType(self, node):
        sys.stderr.write('Variable array directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass

    def visit_FixedArrayType(self, node):
        sys.stderr.write('Fixed array directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass

    def visit_PascalStringType(self, node):
        sys.stderr.write('String directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass
    
class CSharpUnion_UnpackHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name
        
    def visit_CompoundType(self, node):
        self.buf.write('\t\t\t_state = new %s(stream);\n' % node.fully_qualified_name('.'))
        
    def visit_BuiltinType(self, node):
        self.buf.write('\t\t\t_state = (%s)reader.%s();\n' % (primitive_type_name(node),
                                                          read_translations[node.name]))
        
    def visit_VariableArrayType(self, node):
        sys.stderr.write('Variable array directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass

    def visit_FixedArrayType(self, node):
        sys.stderr.write('Fixed array directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass

    def visit_PascalStringType(self, node):
        sys.stderr.write('String directly in a union is not yet supported in c-sharp!\n')
        exit(1)
        pass
    
class CSharpUnion_AccessorEmitter(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix=''):
        self.buf = buf
        self.prefix = prefix
        
    def visit_MessageMemberDecl(self, node):
        self.emitProperty(node)
        self.buf.write('\n')
        
    def emitProperty(self, node):
        self.buf.write('\tpublic ')
        self.visit(node.type)
        self.buf.write(' %s\n' % node.name)
        self.buf.write('\t{\n')
        self.buf.write('\t\tget {\n')
        self.buf.write('\t\t\tif (_tag != Tag.%s) {\n' % node.name)
        self.buf.write('\t\t\t\tthrow new System.InvalidOperationException(string.Format(\n')
        self.buf.write('\t\t\t\t\t"Cannot access union member \\"%s\\" when a value of type {0} is stored.",\n' % node.name)
        self.buf.write('\t\t\t\t\t_tag.ToString()));\n')
        self.buf.write('\t\t\t}\n')
        self.buf.write('\t\t\treturn (')
        self.visit(node.type)
        self.buf.write(')this._state;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t\t\n')
        self.buf.write('\t\tset {\n')
        self.buf.write('\t\t\t_tag = (value != null) ? Tag.%s : Tag.INVALID;\n' % node.name)
        self.buf.write('\t\t\t_state = value;\n')
        self.buf.write('\t\t}\n')
        self.buf.write('\t}\n')

    def visit_BuiltinType(self, node):
        self.buf.write(primitive_type_name(node))

    def visit_DefinedType(self, node):
        self.buf.write(node.type.name)

    def visit_CompoundType(self, node):
        self.buf.write(node.fully_qualified_name('.'))

    def visit_PascalStringType(self, node):
        self.buf.write('string')

class CSharpUnion_SizeHelper(ast.NodeVisitor):
    def __init__(self, buf=sys.stdout, prefix='', member_name=None):
        self.buf = buf
        self.prefix = prefix
        self.member_name = member_name

    def visit_BuiltinType(self, node):
        self.buf.write('\t\t\t\tresult += %d; // = %s\n' %
                       (node.size, node.name))

    def visit_DefinedType(self, node):
        self.buf.write('\t\t\t\tresult += %d; // = %s\n' %
                       (node.size, node.builtin_type().name))

    def visit_CompoundType(self, node):
        self.buf.write('\t\t\t\tresult += %s.Size;\n' %
                       (self.prefix + self.member_name))


if __name__ == '__main__':
    import emitterutil
    emitterutil.c_main(language='C#',
        emitters=[CSharpEnumEmitter, CSharpStructEmitter, CSharpUnionEmitter],
        use_inclusion_guards=False)
