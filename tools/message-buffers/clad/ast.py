"""
AnkiBuffer parser AST types
heavily based on c_ast.py in py_c_parser
"""

from __future__ import absolute_import
from __future__ import print_function

import os.path
import sys

class Node(object):
    """ Base object for AST Nodes.
    """
    def children(self):
        """ A sequence of all children that are Nodes
        """
        return []

    def show(self, buf=sys.stdout, offset=0,
             nodenames=False, attrnames=False, showcoord=False, _node_name=None):
        """ Print the Node and its attributes to a buffer.
        TODO: describe all of the kw_args
        """
        lead = ' ' * offset
        if nodenames and _node_name is not None:
            buf.write(lead + self.__class__.__name__ + ' <' + _node_name + '>: ')
        else:
            buf.write(lead + self.__class__.__name__ + ': ')
        
        if self.attr_names:
            if attrnames:
                nvlist = [(n, getattr(self, n) if not callable(getattr(self, n))
                           else getattr(self, n)()) for n in self.attr_names]
                attrstr = ', '.join('%s=%s' % nv for nv in nvlist)
            else:
                vlist = [getattr(self, n) if not callable(getattr(self, n))
                           else getattr(self, n)() for n in self.attr_names]
                attrstr = ', '.join('%s' % v for v in vlist)
            buf.write(attrstr)
            
        if showcoord:
            buf.write(' (at %s)' % self.coord)
            buf.write('\n')
        
class NodeVisitor(object):
    """ A base Node Visitor class for visiting ast nodes.
        Subclass this and define your own visit_XXX methods, 
        where XXX is the class name you want to visit with these methods.
    """
    def visit(self, node, *args, **kwargs):
        """ Visit a node.
        """
        method = 'visit_' + node.__class__.__name__
        visitor = getattr(self, method, None)
        if visitor is not None:
            return visitor(node, *args, **kwargs)
        else:
            for base in node.__class__.__bases__:
                method = 'visit_' + base.__name__ + '_subclass'
                visitor = getattr(self, method, None)
                if visitor is not None:
                    return visitor(node, *args, **kwargs)
            return self.generic_visit(node, *args, **kwargs)
    
    def generic_visit(self, node, *args, **kwargs):
        """ Called if no explicit visitor function exists for a node.
            Implements preorder visiting of the node.
        """
        for c_name, c in node.children():
            self.visit(c, *args, **kwargs)

# Concrete node subclasses

class DeclList(Node):
    def __init__(self, decl_list, coord=None):
        self.decl_list = decl_list
        self.coord = coord

    def children(self):
        nodelist = []
        for i, child in enumerate(self.decl_list or []):
            nodelist.append(("decl[%d]" % i, child))
        return tuple(nodelist)

    def append(self, decl):
        self.decl_list.append(decl)
        return self
    
    def length(self):
        return len(self.decl_list)
    
    attr_names = ()

class Decl(Node):
    def __init__(self, name, coord=None, namespace=None):
        self.name = name
        self.coord = coord
        self._namespace = namespace
    
    def relative_qualified_name(self, base_namespace, separator='::'):
        "The fully-qualified name relative to the given namespace."
        if self.namespace == base_namespace:
            return self.name
        elif self.namespace:
            return self.namespace.relative_qualified_name(base_namespace, separator) + (separator + self.name)
        else:
            raise ValueError('Node not in namespace {0}.'.format(base_namespace.name))
    
    def fully_qualified_name(self, separator='::'):
        """ List of namespaces, with the most specific being first """
        result = self.name
        if self.namespace is not None:
            result = self.namespace.fully_qualified_name(separator) + (separator + result)
        return result

    @property
    def namespace(self):
        return self._namespace

    @namespace.setter
    def namespace(self, value):
        self._namespace = value
    
    def namespace_list(self):
        if self.namespace:
            return self.namespace.get_hierarchy_list()
        else:
            return []

class NamespaceDecl(Decl):
    def __init__(self, name, decl_list, coord=None, namespace=None):
        super(self.__class__, self).__init__(name, coord=coord, namespace=namespace)
        self.decl_list = decl_list
    
    def __contains__(self, decl):
        while decl:
            decl = decl.namespace
            if self == decl:
                return True
        return False
    
    def __eq__(self, other):
        if type(self) is type(other):
            return self.name == other.name and self.namespace == other.namespace
        else:
            return NotImplemented

    def __ne__(self, other):
        if type(self) is type(other):
            return not self.__eq__(other)
        else:
            return NotImplemented
    
    def children(self):
        nodelist = []
        if self.decl_list is not None: nodelist.append(("members", self.decl_list))
        return tuple(nodelist)

    def members(self):
        return self.decl_list.members
    
    def get_hierarchy_list(self):
        backwards_list = []
        current = self
        while current:
            backwards_list.append(current)
            current = current.namespace
        return reversed(backwards_list)
    
    attr_names = tuple(['name', 'fully_qualified_name'])

class IncludeDecl(Node):
    def __init__(self, name, decl_list, coord=None):
        self.name = name
        self.decl_list = decl_list
        self.coord = coord

    def children(self):
        nodelist = []
        if self.decl_list is not None: nodelist.append(("members", self.decl_list))
        return tuple(nodelist)

    def members(self):
        return self.decl_list.members

    attr_names = tuple(['name'])

class MessageDecl(Decl):
    def __init__(self, name, decl_list, coord=None, namespace=None, is_structure=False):
        super(self.__class__, self).__init__(name, coord=coord, namespace=namespace)
        self.decl_list = decl_list
        self._is_structure = is_structure
    
    def children(self):
        nodelist = []
        if self.decl_list is not None: nodelist.append(("members", self.decl_list))
        return tuple(nodelist)

    def members(self):
        return self.decl_list.members
    
    def alignment(self):
        "The alignment requirements on the message when it is a C struct."
        if self.members():
            return max(member.type.alignment() for member in self.members())
        else:
            return 1
    
    def max_message_size(self):
        "The maximum size that the transmitted message can possibly be."
        if self.members():
            return sum(member.type.max_message_size() for member in self.members())
        else:
            return 0

    def is_message_size_fixed(self):
        "Returns true if the transmitted message will always be the same size."
        if self.members():
            return all(member.type.is_message_size_fixed() for member in self.members())
        else:
            return True
    
    def object_type(self):
        return "structure" if self._is_structure else "message"
    
    attr_names = tuple(['name', 'fully_qualified_name'])

class MessageMemberDeclList(Node):
    def __init__(self, members, coord=None):
        self.members = members
        self.coord = coord
    
    def children(self):
        nodelist = []
        for i, child in enumerate(self.members or []):
            nodelist.append(("member[%d]" % i, child))
        return tuple(nodelist)

    def append(self, member):
        self.members.append(member)
        return self
    
    def length(self):
        return len(self.members)
    
    attr_names = tuple(['length'])

    
class MessageMemberDecl(Node):
    def __init__(self, name, type, init=None, coord=None):
        self.name = name
        self.type = type
        self.init = init
        self.coord = coord

    def children(self):
        nodelist = []
        if self.type is not None: nodelist.append(("type", self.type))
        if self.init is not None: nodelist.append(("init", self.init))
        return tuple(nodelist)
    
    attr_names = ('name', 'type')

class MessageVariableArrayMemberDecl(MessageMemberDecl):
    def __init__(self, name, type, init, coord=None):
        super(self.__class__, self).__init__(name, type, init)
        self.coord = coord
    

    
class EnumDecl(Decl):
    def __init__(self, name, storage_type="int_8", member_list=None, coord=None, namespace=None):
        super(self.__class__, self).__init__(name, coord=coord, namespace=namespace)
        self.storage_type = storage_type
        self.member_list = member_list or []
    
    def children(self):
        nodelist = [("member_list", self.member_list)]
        return tuple(nodelist)

    def members(self):
        return self.member_list.members
    
    def alignment(self):
        return self.storage_type.alignment()
    
    def max_message_size(self):
        return self.storage_type.max_message_size()
    
    def is_message_size_fixed(self):
        return self.storage_type.is_message_size_fixed()
    
    attr_names = tuple(['name', 'fully_qualified_name', 'storage_type'])
        
class EnumMemberList(Node):
    def __init__(self, members, coord=None):
        self.members = members
        self.coord = coord

    def children(self):
        nodelist = []
        for i, child in enumerate(self.members):
            nodelist.append(("member[%d]" % i, child))
        return tuple(nodelist)

    def append(self, member):
        self.members.append(member)
        return self
    
    attr_names = ()
    
class EnumMember(Node):
    def __init__(self, name, initializer=None, coord=None):
        self.name = name
        self.initializer = initializer
        self.coord = coord
    
    def children(self):
        nodelist = []
        if self.initializer:
            nodelist.append(("initializer", self.initializer))
        return tuple(nodelist)
    
    attr_names = tuple(["name"])


class UnionDecl(Decl):
    def __init__(self, name, member_list=None, coord=None, namespace=None):
        super(self.__class__, self).__init__(name, coord=coord, namespace=namespace)
        self.member_list = member_list
        self.tag_storage_type = builtin_types['uint_8']
        
    def children(self):
        nodelist = [("member_list", self.member_list)]
        return tuple(nodelist)
    
    def members(self):
        return self.member_list.members

    def alignment(self):
        "The alignment requirements on the message when it is a C struct."
        if self.members():
            alignment = max(member.type.alignment() for member in self.members())
            return max(alignment, self.tag_storage_type.alignment())
        else:
            return self.tag_storage_type.alignment()
    
    def max_message_size(self):
        "The maximum size that the transmitted message can possibly be."
        if self.members():
            return self.tag_storage_type.max_message_size() + max(member.type.max_message_size() for member in self.members())
        else:
            return self.tag_storage_type.max_message_size()
    
    def is_message_size_fixed(self):
        "Returns true if the transmitted message will always be the same size."
        if self.members():
            size = self.members()[0].type.max_message_size()
            for member in self.members():
                if not member.type.is_message_size_fixed():
                    return False
                if member.type.max_message_size() != size:
                    return False
        return self.tag_storage_type.is_message_size_fixed()
    
    attr_names = tuple(["name", "fully_qualified_name", "alignment"])
    
##### Types #####        
class Type(Node):
    def __init__(self, name, coord=None):
        self.name = name
        self.coord = coord

    def builtin_type(self):
        return None

    def children(self):
        return tuple([])
    
    def alignment(self):
        "The alignment requirements on the type in C."
        return None
    
    def max_message_size(self):
        "The maximum size that the transmitted type can possibly be."
        return None

    def is_message_size_fixed(self):
        "Returns true if the transmitted type will always be the same size."
        return False

    def fully_qualified_name(self, separator='::'):
        return self.name
    
class BuiltinType(Type):
    # to be used for builtin stuff like ints and floats
    def __init__(self, name, type, size, unsigned=False, min=None, max=None, coord=None):
        self.name = name
        self.size = size
        self.type = type
        self.unsigned = unsigned
        self.min = min
        self.max = max
        self.coord = coord

    def builtin_type(self):
        return self

    def alignment(self):
        return self.size
    
    def max_message_size(self):
        return self.size
    
    def is_message_size_fixed(self):
        return True
    
    attr_names = tuple(["name", "size", "type", "unsigned", "min", "max"])

class VariableArrayType(Type):
    def __init__(self, member_type, length_type, coord=None):
        self.name = '%s[%s]' % (member_type.name, length_type.name)
        self.member_type = member_type
        self.length_type = length_type
        self.coord = coord
        
    def builtin_type(self):
        return self.member_type.builtin_type()

    def children(self):
        nodelist = [
            ("member_type", self.member_type),
            ("length_type", self.length_type)
        ]
        return tuple(nodelist)
            
    def alignment(self):
        return max(self.length_type.alignment(), self.member_type.alignment())
        
    def max_message_size(self):
        return self.length_type.max_message_size() + self.length_type.max * self.member_type.max_message_size()
    
    attr_names = tuple(["length_type"])

# String type is just a variable array, member type must be uint_8
class PascalStringType(VariableArrayType):
    def __init__(self, length_type, coord=None):
        super(PascalStringType, self).__init__(builtin_types['uint_8'], length_type, coord)
        if length_type.type == 'uint_8':
            self.name = 'string'
        else:
            self.name = 'string[%s]' % length_type.name

class FixedArrayType(Type):
    def __init__(self, member_type, length, coord=None):
        self.name = '%s[%d]' % (member_type.name, length)
        self.member_type = member_type
        self.length = length
        self.coord = coord

    def builtin_type(self):
        return self.member_type.builtin_type()

    def children(self):
        nodelist = [
            ("member_type", self.member_type),
        ]
        return nodelist

    def alignment(self):
        if self.length == 0:
            return 0
        else:
            return self.member_type.alignment()
    
    def max_message_size(self):
        return self.member_type.max_message_size() * self.length
    
    def is_message_size_fixed(self):
        if self.length == 0:
            return True
        else:
            return self.member_type.is_message_size_fixed()
    
    attr_names = tuple(["length"])
        
class DefinedType(Type):
    # to be used for aliases and enums
    def __init__(self, name, underlying_type, coord=None):
        self.name = name
        self.underlying_type = underlying_type
        self.coord = coord
        self.size = underlying_type.storage_type.size

    def builtin_type(self):
        if self.underlying_type:
            return self.underlying_type.storage_type.builtin_type()
        return None
    
    def children(self):
        nodelist = []
        return tuple(nodelist)
    
    def alignment(self):
        return self.underlying_type.alignment()
    
    def max_message_size(self):
        return self.underlying_type.max_message_size()
    
    def is_message_size_fixed(self):
        return self.underlying_type.is_message_size_fixed()
    
    def relative_qualified_name(self, base_namespace, separator='::'):
        "The fully-qualified name relative to the given namespace."
        return self.underlying_type.relative_qualified_name(base_namespace, separator)
    
    def fully_qualified_name(self, separator='::'):
        return self.underlying_type.fully_qualified_name(separator)
    
    @property
    def namespace(self):
        return self.underlying_type.namespace
    
    attr_names = tuple(['name'])

class CompoundType(Type):
    # to be used for structs and unions
    def __init__(self, name, type_decl, coord=None):
        self.name = name
        self.type_decl = type_decl
        self.coord = coord
        
    def children(self):
        nodelist = []
        if self.type_decl:
            nodelist.append(("type_decl", self.type_decl))
        return tuple(nodelist)

    def alignment(self):
        return self.type_decl.alignment()
    
    def max_message_size(self):
        return self.type_decl.max_message_size()
    
    def is_message_size_fixed(self):
        return self.type_decl.is_message_size_fixed()
    
    def relative_qualified_name(self, base_namespace, separator='::'):
        return self.type_decl.relative_qualified_name(base_namespace, separator)
    
    def fully_qualified_name(self, separator='::'):
        return self.type_decl.fully_qualified_name(separator)

    @property
    def namespace(self):
        return self.type_decl.namespace
    
    attr_names = tuple(['name'])

#Fill out builtin types    
#concrete builtin types
builtin_types = {
    "bool":    BuiltinType("bool", "int", 1, min=0, max=1, coord="builtin"),
    "int_8":   BuiltinType("int_8",  "int", 1, min=-2**7,  max=2**7-1,  coord="builtin"),
    "int_16":  BuiltinType("int_16", "int", 2, min=-2**15, max=2**15-1, coord="builtin"),
    "int_32":  BuiltinType("int_32", "int", 4, min=-2**31, max=2**31-1, coord="builtin"),
    "int_64":  BuiltinType("int_64", "int", 8, min=-2**63, max=2**63-1, coord="builtin"),
    "uint_8":   BuiltinType("uint_8",  "int", 1, unsigned=True, min=0, max=2**8-1,  coord="builtin"),
    "uint_16":  BuiltinType("uint_16", "int", 2, unsigned=True, min=0, max=2**16-1, coord="builtin"),
    "uint_32":  BuiltinType("uint_32", "int", 4, unsigned=True, min=0, max=2**32-1, coord="builtin"),
    "uint_64":  BuiltinType("uint_64", "int", 8, unsigned=True, min=0, max=2**64-1, coord="builtin"),
    "float_32":     BuiltinType("float_32", "float", 4, coord="builtin"),
    "float_64":     BuiltinType("float_64", "float", 8, coord="builtin"),
}

#builtin 8bit, 16bit and 32bit length string types
string_length_types = [
    'uint_8', 'uint_16', 'uint_32'
]
string_types = dict()
for length_type in string_length_types:
    string_types[length_type] = PascalStringType(builtin_types[length_type],coord='builtin')

class IntConst(Node):
    def __init__(self, value, type="dec", coord=None):
        self.value = value
        self.type = type
        self.coord = coord

    def children(self):
        return tuple()
    
    attr_names = tuple(["value", "type"])
    
    
# DEBUG emitter
class ASTDebug(NodeVisitor):
    def __init__(self):
        self.depth = 0
        self.node_name = None
        
    def generic_visit(self, node):
        if node:
            node.show(buf=sys.stdout,
                      offset=self.depth,
                      _node_name=self.node_name,
                      nodenames=True,
                      attrnames=True,
                      showcoord=True)
            
            self.depth = self.depth + 1
            for (name, child) in node.children():
                self.node_name = name
                self.visit(child)
            self.node_name = None
            self.depth = self.depth - 1
