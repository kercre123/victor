#!/usr/bin/env python

"""
The parser for AnkiBuffers
Author: Mark Pauley
Date: 12/12/2014
"""

from __future__ import absolute_import
from __future__ import print_function

import os.path
import re

from .ply import yacc

from . import ast
from .lexer import AnkiBufferLexer
from .plyparser import PLYParser, Coord, ParseError

THIS_DIR = os.path.dirname(os.path.realpath(__file__))

class AnkiBufferParser(PLYParser):
    """ A parser for the Anki Buffer definition language.
    """
    
    def __init__(self,
                 input_directories=(".",),
                 lex_optimize=False,
                 lextab='clad.lextab',
                 yacc_optimize=False,
                 yacctab='clad.yacctab',
                 yacc_debug=False,
                 lex_debug=False):
        """ Create a new AnkiBuffer parser
        TODO: explain the options
        """
        self.lexer = AnkiBufferLexer(
            lex_debug=lex_debug,
            error_func=self._lex_error_func,
            type_lookup_func=self._lex_type_lookup_func)
        
        self.lexer.build(optimize=lex_optimize, lextab=lextab, outputdir=THIS_DIR)
        self.tokens = self.lexer.tokens
        self.parser = yacc.yacc(
            module=self,
            start='start',
            debug=yacc_debug,
            optimize=yacc_optimize,
            tabmodule=yacctab,
            outputdir=THIS_DIR
        )
        self._input_directories = input_directories
        self._last_yielded_token = None
        self.declared_types = dict()
        self._include_directive_re = re.compile('^#include\s+\"(.*)\"(?:\s*/.*)*$')
        self._namespace_stack = [ None ]
        self._message_types = []
        self._auto_union_decls = []

    def lineno_to_coord(self, lineno, column=None):
        c = self._lines_to_coords[lineno - 1]
        if column is None:
            column = self.lexer.find_tok_column()
        c.column = column
        return c

    def preprocess(self, text, filename):
        lines = text.splitlines(True)
        processed_text = ""
        for lineno,line in enumerate(lines):
            self._lines_to_coords.append(Coord(filename, lineno + 1))
            m = re.match(self._include_directive_re, line);
            if m:
                include_file_path = m.group(1)
                for input_directory in self._input_directories:
                    real_include_file_path = os.path.join(input_directory, include_file_path)
                    if os.path.exists(real_include_file_path):
                        chosen_input_directory = input_directory
                        break
                else:
                    raise ParseError(self._lines_to_coords[-1],
                                     'Could not find file "{0}"\nInclude directories seached:\n\t{1}\n'.format(
                                        include_file_path, '\n\t'.join(self._input_directories)))
                processed_text += "// " + line
                try:
                    with open(real_include_file_path, "r") as f:
                        data = f.read()
                except IOError as e:
                    raise ParseError(self._lines_to_coords[-1],
                                     'Error reading file "{0}" in directory "{1}"'.format(
                                        include_file_path, chosen_input_directory))
                processed_text += 'include "{0}" {{\n'.format(include_file_path)
                lineno += 1
                self._lines_to_coords.append(Coord(None, lineno + 1))
                if m.group(1) not in self._included_files:
                    self._included_files.append(include_file_path)
                    processed_text += self.preprocess(data, include_file_path)
                processed_text += '}\n'
                lineno += 1
                self._lines_to_coords.append(Coord(None, lineno + 1))
            else:
                processed_text += line
        return processed_text
        
    def parse(self, text, filename='', debuglevel=0):
        """ Parses buffer definitions and returns an AST.
        
          text:
            A String containing the source code
            
          filename:
            Name of the file being parsed (for meaningful
            error messages)

          debuglevel:
            Debug level to yacc
        """
        self._included_files = []
        self._lines_to_coords = []
        processed_text = self.preprocess(text, filename)
        self._last_yielded_token = None
        syntax_tree = self.parser.parse(
            input=processed_text,
            lexer=self.lexer,
            debug=debuglevel)
        
        self._postprocess_auto_unions()

        return syntax_tree
    
    def _postprocess_auto_unions(self):
        for auto_union in self._auto_union_decls:
            coord = auto_union.coord
            auto_union.member_list = ast.MessageMemberDeclList([], coord)
            
            names = dict()
            for message_type in self._message_types:
                if message_type.name in names:
                    raise ParseError(
                        auto_union.coord,
                        "Autounion '{0}' would contain two members with the name '{1}': '{2}' and '{3}'".format(
                        auto_union.fully_qualified_name(),
                        message_type.name,
                        names[message_type.name].fully_qualified_name(),
                        message_type.fully_qualified_name()))
                names[message_type.name] = message_type
            
            for message_type in self._message_types:
                decl = ast.MessageMemberDecl(message_type.name, message_type, None, coord)
                auto_union.member_list.append(decl)
    
    ###### implementation ######

    def _get_current_namespace(self):
        return self._namespace_stack[-1]

    def _get_type_from_fully_qualified_name(self, typename):
        if typename is None:
            return self.declared_types
        namespace_list = typename.split('::')
        t = self.declared_types
        for namespace in namespace_list:
            #add some error handling stuff here
            if not namespace in t:
                return None
            t = t[namespace]
        return t

    def _get_type(self, m_typename):
        namespace = self._get_current_namespace();
        while True:
            typename = m_typename
            if namespace is not None:
                typename = namespace.fully_qualified_name() + '::' + typename
            t = self._get_type_from_fully_qualified_name(typename)
            if t is not None:
                return t
            if namespace is None:
                break;
            namespace = namespace.namespace
        return None;

    def _add_namespace(self, new_namespace):
        namespace_name = new_namespace.fully_qualified_name()
        namespace_list = namespace_name.split('::')
        current_types = self.declared_types
        for namespace in namespace_list[:-1]:
            if not namespace in current_types:
                #namespace not found, error
                raise ParseError(new_namespace.coord,
                                 'Unknown namespace {0}'.format(namespace_name))
            current_types = current_types[namespace]
        final_namespace = namespace_list[-1]
        if not final_namespace in current_types:
            current_types[final_namespace] = dict()
        return current_types[final_namespace]

    def _add_type(self, lineno, m_type):
        """ Adds a new message type
        """
        fully_qualified_name = None
        current_namespace = self._get_current_namespace()
        if current_namespace is not None:
            fully_qualified_name = current_namespace.fully_qualified_name()
        namespace_types = self._get_type_from_fully_qualified_name(fully_qualified_name)
        if m_type.name in namespace_types:
            raise ParseError(self.lineno_to_coord(lineno),
                             "Name '{0}' already exists".format(m_type.fully_qualified_name()))
        namespace_types[m_type.name] = m_type

    def _is_type_in_scope(self, name):
        """ Is <name> a type in the current-scope?
        """
    
    def _lex_error_func(self, msg, line, column):
        c = self.lineno_to_coord(line, column)
        self._parse_error(msg, c)
    
    def _lex_type_lookup_func(self, name):
        """ Looks up types that were previously defined.
        """
        is_type = self._is_type_in_scope(name)
    
    def _get_yacc_lookahead_token(self):
        """ The last token yacc requested from the lexer.
        Saved in the lexer.
        """
        return self.lexer.last_token

    ###### productions #######

    def p_start(self, p):
        """ start : decl_list
        """
        p[0] = p[1]

    def p_decl_list(self, p):
        """ decl_list : decl
                      | decl_list decl
        """
        if len(p) == 2:
            p[0] = ast.DeclList([p[1]], p[1].coord)
        else:
            p[0] = p[1].append(p[2])

    def p_decl(self, p):
        """ decl : namespace_decl
                 | message_decl
                 | enum_decl
                 | union_decl
                 | include_decl
        """
        p[0] = p[1]

    def p_include_decl(self, p):
        """ include_decl : include_begin decl_list RBRACE
                         | include_begin RBRACE
        """
        include = p[1]
        if len(p) >= 4:
            include.decl_list = p[2]
        p[0] = include

    def p_include_begin(self, p):
        """ include_begin : INCLUDE QUOTED_PATH LBRACE
        """
        new_include = ast.IncludeDecl(p[2][1:-1], None, self.lineno_to_coord(p.lineno(2)))
        p[0] = new_include

    def p_namespace_decl(self, p):
        """ namespace_decl : namespace_begin decl_list RBRACE
        """
        namespace = p[1]
        namespace.decl_list = p[2]
        self._namespace_stack.pop()
        p[0] = namespace

    def p_namespace_begin(self, p):
        """ namespace_begin : NAMESPACE ID LBRACE
        """
        # push namespace onto stack
        new_namespace = ast.NamespaceDecl(p[2], None,
                                          self.lineno_to_coord(p.lineno(2)),
                                          self._get_current_namespace())
        namespace_types = self._add_namespace(new_namespace)
        self._namespace_stack.append(new_namespace)
        p[0] = new_namespace

    def p_message_decl(self, p):
        """ message_decl : message_decl_begin ID LBRACE message_member_decl_list RBRACE
                         | message_decl_begin ID LBRACE message_member_decl_list COMMA RBRACE
                         | message_decl_begin ID LBRACE RBRACE
        """
        if len(p) >= 6:
            members = p[4]
        else:
            members = ast.MessageMemberDeclList([], self.lineno_to_coord(p.lineno(3)))

        is_structure = p[1] == "structure"
        p[0] = ast.MessageDecl(p[2], members,
                               self.lineno_to_coord(p.lineno(2)),
                               self._get_current_namespace(),
                               is_structure)
        type = ast.CompoundType(p[0].name, p[0], p[0].coord)
        if not is_structure:
            self._message_types.append(type)
        self._add_type(p.lineno(2), type)

    def p_message_decl_begin(self, p):
        """ message_decl_begin : MESSAGE
                               | STRUCTURE
        """
        p[0] = p[1]
        
    def p_message_member_decl_list(self, p):
        """ message_member_decl_list : message_member_decl
                                     | message_member_decl_list COMMA message_member_decl
        """
        if len(p) == 2:
            p[0] = ast.MessageMemberDeclList([p[1]], p[1].coord)
        else:
            p[0] = p[1].append(p[3])

    def p_message_member_decl(self, p):
        """ message_member_decl : message_member
                                | message_array_member
        """
        p[0] = p[1]

    def p_message_member(self, p):
        """ message_member : type ID
        """
        p[0] = ast.MessageMemberDecl(
            p[2], #name
            p[1], #type
            None, #init
            self.lineno_to_coord(p.lineno(2))
        )

    def p_message_array_member(self, p):
        """ message_array_member : message_variable_array_member
                                 | message_fixed_array_member
        """
        p[0] = p[1]

    def p_message_variable_array_member(self, p):
        """ message_variable_array_member : type ID LSQ builtin_int_type RSQ
        """
        type = ast.VariableArrayType(p[1], p[4], self.lineno_to_coord(p.lineno(1)))
        p[0] = ast.MessageVariableArrayMemberDecl(p[2], type, None,
                                                  self.lineno_to_coord(p.lineno(1)))
        
    def p_message_fixed_array_member(self, p):
        """ message_fixed_array_member : type ID LSQ int_constant RSQ
        """
        type = ast.FixedArrayType(p[1], p[4].value, self.lineno_to_coord(p.lineno(1)))
        p[0] = ast.MessageMemberDecl(p[2], type, None, self.lineno_to_coord(p.lineno(1)))
        
    def p_enum_decl(self, p):
        """ enum_decl : ENUM builtin_type ID LBRACE enum_member_list RBRACE
                      | ENUM builtin_type ID LBRACE enum_member_list COMMA RBRACE
        """
        decl = ast.EnumDecl(p[3], p[2], p[5],
                            self.lineno_to_coord(p.lineno(1)),
                            self._get_current_namespace())
        decl_type = ast.DefinedType(decl.name, decl, self.lineno_to_coord(p.lineno(1)))
        self._add_type(p.lineno(1), decl_type)
        p[0] = decl
        
    def p_enum_member_list(self, p):
        """ enum_member_list : enum_member
                             | enum_member_list COMMA enum_member
        """
        if len(p) == 2:
            p[0] = ast.EnumMemberList([p[1]], p[1].coord)
        else:
            p[0] = p[1].append(p[3])
        
    def p_enum_member(self, p):
        """ enum_member : ID
                        | ID EQ int_constant
        """
        value = p[3] if len(p) == 4 else None
        p[0] = ast.EnumMember(p[1], value, self.lineno_to_coord(p.lineno(1)))

    def p_union_decl(self, p):
        """ union_decl : UNION ID LBRACE union_member_list RBRACE
                       | UNION ID LBRACE union_member_list COMMA RBRACE
                       | UNION ID LBRACE RBRACE
        """
        member_list = p[4] if len(p) > 5 else None
        decl = ast.UnionDecl(p[2], member_list,
                             self.lineno_to_coord(p.lineno(2)),
                             self._get_current_namespace())
        p[0] = decl
        if member_list is None:
            self._auto_union_decls.append(decl)
        # TODO: Uncomment to add recursive union support
        #type = ast.CompoundType(p[0].name, p[0], p[0].coord)
        #self._add_type(type)

    def p_union_member_list(self, p):
        """ union_member_list : union_member
                              | union_member_list COMMA union_member
        """
        if len(p) == 2:
            p[0] = ast.MessageMemberDeclList([p[1]], p[1].coord)
        else:
            p[0] = p[1].append(p[3])

    def p_union_member(self, p):
        """ union_member : type ID
        """
        p[0] = ast.MessageMemberDecl(p[2], p[1], None, p[1].coord)

    def p_type(self, p):
        """ type : string_type
                 | non_array_type
        """
        p.set_lineno(0, p.lineno(1))
        p[0] = p[1]

    def p_string_type(self, p):
        """ string_type : STRING
                        | STRING LSQ builtin_int_type RSQ
        """
        if len(p) == 2:
            length_type = 'uint_8'
        else:
            length_type = p[3].name
        p[0] = ast.string_types[length_type]

    def p_non_array_type(self, p):
        """ non_array_type : builtin_type
                           | declared_type
        """
        p.set_lineno(0, p.lineno(1))
        p[0] = p[1]
        
    def p_builtin_type(self, p):
        """ builtin_type : builtin_float_type
                         | builtin_int_type
        """
        p.set_lineno(0, p.lineno(1))
        p[0] = p[1]

    def p_builtin_float_type(self,p):
        """ builtin_float_type : FLOAT_32 
                               | FLOAT_64 
        """
        p.set_lineno(0, p.lineno(1))
        p[0] = ast.builtin_types[p[1]]
        
    def p_builtin_int_type(self, p):
        """ builtin_int_type : INT_8
                             | INT_16 
                             | INT_32
                             | INT_64
                             | UINT_8
                             | UINT_16 
                             | UINT_32
                             | UINT_64
                             | BOOL
        """
        p.set_lineno(0, p.lineno(1))
        p[0] = ast.builtin_types[p[1]]

    def p_declared_type(self, p):
        """ declared_type : ID
        """
        t = self._get_type(p[1])
        if t is not None:
            p[0] = t
        else:
            self._parse_error("Unknown type: %s" % p[1], self.lineno_to_coord(p.lineno(1)))

    def p_int_constant(self, p):
        """ int_constant : int_constant_dec
                         | int_constant_hex
        """
        p[0] = p[1]

    def p_int_constant_hex(self, p):
        """ int_constant_hex : INT_CONST_HEX
        """
        p[0] = ast.IntConst(p[1], 'hex', self.lineno_to_coord(p.lineno(1)))

    def p_int_constant_dec(self, p):
        """ int_constant_dec : INT_CONST_DEC
        """
        p[0] = ast.IntConst(p[1], 'dec', self.lineno_to_coord(p.lineno(1)))
    
    def p_error(self, p):
        if p:
            self._parse_error('before \'%s\'' % p.value,
                              self.lineno_to_coord(p.lineno, self.lexer.find_tok_column(p)))
        else:
            self._parse_error('At end of input', self.lineno_to_coord(0))
    
            

