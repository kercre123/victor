#! /usr/bin/python

# here is a quick snippet to allow including the parser stuff.

import os, sys, inspect
# realpath() will make your script run, even if you symlink it :)
cmd_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

# use this if you want to include modules from a subfolder
library_folder = os.path.realpath(os.path.join(cmd_folder, "../../../lib/anki/cozmo-engine/tools/message-buffers"))
clad_folder = os.path.join(library_folder, 'clad')
emitter_folder = os.path.join(library_folder, 'emitters')
for folder in (clad_folder, emitter_folder):
    if folder not in sys.path:
        sys.path.insert(0, folder)

import clad
import ast
import sys
import textwrap

class TargetedEmitter(ast.NodeVisitor):
    "An emitter that targets a specific message or union."

    def __init__(self, options):
        self.options = options
        self.buf = sys.stdout
    
    def __call__(self, outbuf):
        "A hack to allow initialization in two parts."
        self.buf = outbuf
        return self
    
    def visit_MessageDecl(self, node):
        if node.name == options.handled_message_name:
            sys.exit('--handled-message-name parameter {0} matches a Message and not a Union type.'.format(
                options.handled_message_name))
    
    def visit_UnionDecl(self, node):
        if node.name == options.handled_message_name:
            namespaces = []
            namespace = node.namespace
            while namespace:
                namespaces.append(namespace)
                namespace = namespace.namespace
            namespaces = reversed(namespaces)
            
            globals = dict(
                message_name=node.fully_qualified_name(),
                handler_name=options.handler_class_name,
                handler_base=options.handler_base_class_name,
            )
            self.generate_UnionDecl(node, namespaces, globals)


class UnionDeclarationEmitter(TargetedEmitter):
    "An emitter that generates the handler declaration."
    
    def generate_UnionDecl(self, node, globals):
        self.writeNamespaces(node, globals)
        self.writeHeader(node, globals)
        self.writeUnionMethod(node, globals)
        self.writeSeparator(node, globals)
        self.writeMemberMethods(node, globals)
        self.writeFooter(node, globals)
        self.writeEndNamespaces(node, globals)
    
    def writeNamespaces(self, node, globals):
        if node.namespace:
            namespace = node.namespace
            while namespace:
                self.buf.write('namespace {namespace} {{\n'.format(namespace=namespace.name))
                namespace = namespace.namespace
            self.buf.write('\n')
    
    def writeEndNamespaces(self, node, globals):
        if node.namespace:
            namespace = node.namespace
            while namespace:
                self.buf.write('}} // namespace {namespace}\n'.format(namespace=namespace.name))
                namespace = namespace.namespace
            self.buf.write('\n')
        
    def writeHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
        class {handler_name} : {handler_base}
        {{
        public:
        '''.format(**globals)))
    
    def writeSeparator(self, node, globals):
        self.buf.write('private:\n')
        
    def writeFooter(self, node, globals):
        self.buf.write('};\n\n')

    def writeUnionMethod(self, node, globals):
        self.buf.write('\tvoid ProcessMessage(const {message_name}& msg);\n'.format(**globals))
        
    def writeMemberMethods(self, node, globals):
        for member in node.members():
            # TODO: Non-Message types
            self.buf.write('\tvoid Process_{member_name}(const {member_type}& msg);\n'.format(
                member_name=member.name, member_type=member.type.fully_qualified_name(), **globals))

class UnionSwitchEmitter(TargetedEmitter):
    "An emitter that generates the handler declaration."
    
    def generate_UnionDecl(self, node, globals):
        self.writeHeader(node, globals)
        self.writeMemberCases(node, globals)
        self.writeFooter(node, globals)
        
    def writeHeader(self, node, globals):
        self.buf.write(textwrap.dedent('''\
        void {namespace}::{handler_name}::ProcessMessage(const {message_name}& msg)
        {{
        \tswitch(msg.GetType()) {{
        \tdefault:
        \t// should we throw an error here?
        \t\tbreak;
        '''.format(namespace=node.namespace.fully_qualified_name(), **globals)))
    
    def writeFooter(self, node, globals):
        self.buf.write('\t}\n}\n\n')
        
    def writeMemberCases(self, node, globals):
        for member in node.members():
            self.buf.write(textwrap.dedent('''\
                \tcase {message_name}::Type::{member_name}:
                \t\tProcess_{member_name}(msg.Get_{member_name}());
                #''')[:-1].format(member_name=member.name, **globals))

if __name__ == '__main__':
    import emitterutil
    
    # basic parameters
    language = 'C++'
    allow_namespaces=True
    use_inclusion_guards=True
    
    # argument parsing
    option_parser = emitterutil.ArgumentParser(language=language)
    if allow_namespaces:
        option_parser.add_argument('-n', '--namespace', dest='namespaces', nargs='*',
            help='The optional namespaces to wrap the emitter with.')
    
    option_parser.add_argument('-r', '--output-header')
    option_parser.add_argument('--include-headers', nargs='+')
    option_parser.add_argument('--handler-base-class-name', required=True)
    option_parser.add_argument('--handler-class-name', required=True)
    option_parser.add_argument('--handled-message-name', required=True)
    
    options = option_parser.parse_args()
    
    if options.output_header and options.output_header != '-':
        options.output_header = emitterutil.open_linux(options.output_header, 'w')
    else:
        options.output_header = sys.stdout
    
    # dependent parameters
    header_emitters = [UnionDeclarationEmitter(options)]
    source_emitters = [UnionSwitchEmitter(options)]
    system_headers = []
    local_headers = options.include_headers
    usings = []
    
    # emission
    try:
        emitterutil._write_c_header(options.output_header, options, header_emitters,
            system_headers, local_headers, usings, allow_namespaces, use_inclusion_guards)
        emitterutil._write_c_source(options.output_file, options, source_emitters,
            allow_namespaces)
    finally:
        emitterutil.close_file_arguments(options)
        if options.output_header != sys.stdout:
            options.output_header.close()
