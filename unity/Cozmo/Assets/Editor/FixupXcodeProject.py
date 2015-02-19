#!/usr/bin/python
import os
import sys
import uuid
import fnmatch
import itertools
import pprint
import string
import pickle

project_root = os.path.join(os.getcwd(), './')
#project_root = './'
print 'Project path: ' + project_root
target_path = sys.argv[1]

default_allow_filter = ['*']
resource_allow_filter = ['*.png','*.xib']

class FixupXCodeSettings:
    def __init__(self):
        # files/folders to ignore
        self.ignore_filters = [
            '.*',
            '*.xcodeproj',
            '*.meta',
            '*.pch',
            'build',
            '*.txt',
            '*.plist'
        ]
        
        # folders to recursively add
        self.folders = [
            # ('path to file', ['listof', '*filters','.toinclude'])
        ]
        
        # folder references to add
        self.folder_refs = [
            # ('path to folder', 'group/path/')
        ]
        
        # files to individually add
        self.files = [
            # ('path to file', 'group/path/')
        ]

        # files to remove e.g. Default@2x.png
        self.remove_files = [
            # ('path to file', 'group/path/')
            
        ]
        
        # system frameworks to link
        self.frameworks = []
        
        # system frameworks to weak link
        self.weak_frameworks = []
        
        # additional steps to run
        self.additional_steps = []
        
    def ignore(self, filter):
        self.ignore_filters.append(filter)
        
    def add_folder(self, path, filter = default_allow_filter):
        self.folders.append((path, filter))
        
    def add_folder_ref(self, path, group_path = ''):
        self.folder_refs.append((path, group_path))
        
    def add_file(self, path, group_path):
        self.files.append((path, group_path))

    def remove_file(self, path, group_path):
        self.remove_files.append((path, group_path))

    def add_framework(self, name):
        self.frameworks.append(name)
        
    def add_weak_framework(self, name):
        self.weak_frameworks.append(name)
        
    def add_additional_step(self, step):
        self.additional_steps.append(step)

cwd = os.getcwd()
print "FixupXcodeProject.py invoking  params.py"
os.chdir(project_root + "Assets/Editor/")
execfile("./params.py")
xcode_fixup = FixupXCodeSettings()
config = "./configure_xcode_fixup.py"
print "FixupXcodeProject.py invoking " + config
execfile(config)
os.chdir(cwd)

xcodeproj_path = target_path + '/Unity-iPhone.xcodeproj'

ignore_list = [
    '.*',
    '*.xcodeproj',
    '*.meta',
    '*.pch',
    'build',
    '*.txt'
]

# extension to file type dictionary, be sure to update the type lists below also
file_type_by_extension_dict = {
    '.xib': 'file.xib',
    '.png': 'image.png',
    '.h': 'sourcecode.c.h',
    '.c': 'sourcecode.c.c',
    '.m': 'sourcecode.c.objc',
    '.cpp': 'sourcecode.cpp.cpp',
    '.mm': 'sourcecode.cpp.objcpp',
    '.a': 'archive.ar',
    '.framework': 'wrapper.framework',
    '.dylib': 'compiled.mach-o.dylib',
    '.strings': 'text.plist.strings'
}

# types to add to the source code build phase
source_types = [
    'sourcecode.c.c',
    'sourcecode.c.objc',
    'sourcecode.cpp.cpp',
    'sourcecode.cpp.objcpp',
    'sourcecode.asm'
]

# the directories where files of this type are located are added to the header search path
header_types = [
    'sourcecode.c.h'
]

# types to add to the resource build phase
resource_types = [
    'file.xib',
    'image.png',
    'folder',
    'text.plist.strings'
]

# types to add to the framework build phases
library_types = [
    'archive.ar',
    'compiled.mach-o.dylib',
    'wrapper.framework',
]

def generate_id():
    return uuid.uuid4().hex[:24].upper()

def matches_pattern(basename, pattern_list):
    return any((fnmatch.fnmatch(basename, pattern) for pattern in pattern_list))

def is_ignored(basename):
    return matches_pattern(basename, xcode_fixup.ignore_filters)

# characters that are escaped inside a "string literal"
plist_escapes = { '"': '"', 'n': '\n', 't': '\t' }
# characters that are considered valid characters inside a token
plist_token_characters = string.ascii_letters + string.digits + './_-'

def escape_string(input):
    """ Escape a string by observed plist string escaping rules """
    if len(input) > 0 and all(char in plist_token_characters for char in input): return input
    input = input.replace('\\', '\\\\')
    for escape, char in plist_escapes.items():
        input = input.replace(char, '\\' + escape)
    return '"' + input + '"'

def file_by_char(f):
    """ Return an iterator for (character, line) from a file """
    line_count = 0
    for line in f:
        line_count += 1
        for char in line: yield char, line_count
        yield '\n', line_count

def get_plist_token(input):
    """ Return an iterator for (token, is_literal, line) from an iterator that returns (character, line) """
    token = ''
    for c, line in input:
        if c == '"':
            for c, line in input:
                if c == '\\':
                    c = input.next()[0]
                    if c == '\\': token += '\\'
                    if c in plist_escapes: token += plist_escapes[c]
                    else: token += '\\' + c # disable this if you want to ignore escape codes we don't know
                    continue
                elif c == '"':
                    break
                else: token += c
            yield (token, True, line)
            token = ''
            continue
        elif c == '/':
            c = input.next()[0]
            if c == '*':
                for c, line in input:
                    if c == '*':
                        c = input.next()[0]
                        if c == '/': break
                continue
            elif c == '/':
                for c, line in input:
                    if c == '\n': break
                continue
            else:
                token += '/' + c
        elif c.isspace(): continue
        elif c in plist_token_characters:
            token += c
        else:
            if len(token) > 0:
                yield (token, True, line)
                token = ''
            yield (c, False, line)

def parser_error(line, text):
    #print 'parser error on line ' + str(line) + ': ' + text
    raise Exception, 'parser error on line ' + str(line) + ': ' + text
    
class PListDictionary:
    """ Ordered dictionary because that isn't around till Python 2.7 """
    def __init__(self, entries = None):
        self.dictionary = {}
        self.entries = []
        if entries:
            for key, value in entries:
                self[key] = value
        
    def __repr__(self):
        return repr(self.entries)
        
    def __len__(self):
        return len(self.entries)
        
    def __getitem__(self, key):
        if key in self.dictionary:
            return self.dictionary[key][1]
        return None
    
    def __setitem__(self, key, value):
        if key not in self.dictionary:
            value = [key, value]
            self.dictionary[key] = value
            self.entries.append(value)
        else:
            self.dictionary[key][1] = value
            
    def __delitem__(self, key):
        if key not in self.dictionary:
            raise KeyError
        self.entries.remove(self.dictionary[key])
        del self.dictionary[key]
    
    def __iter__(self):
        return iter(self.entries)
    
    def __contains__(self, key):
        return key in self.dictionary
    
    def items(self):
        return self.entries
    
    def keys(self):
        return (entry[1] for entry in entries)

def parse_plist(input):
    """ Return a PListDictionary from an input iterator that returns (character, line) """
    root_dictionary = None
    input_iter = get_plist_token(input)
    for token, is_literal, line in input_iter:
        if token == '{' and not is_literal:
            root_dictionary = PListDictionary()
            for var in parse_plist_dictionary(input_iter):
                root_dictionary[var[0]] = var[1]
        else:
            parser_error(line, 'unexpected token ' + token)
    return root_dictionary

def parse_plist_dictionary(input_iter):
    object_name = None
    found_seperator = False
    object = None
    for token, is_literal, line in input_iter:
        if object_name == None:
            if is_literal:
                object_name = token
            elif token == '}':
                raise StopIteration
            else:
                parser_error(line, 'expected literal, got ' + token)
        elif not found_seperator:
            if token == '=' and not is_literal:
                found_seperator = True
            else:
                parser_error(line, 'expected =, got ' + token)
        elif object == None:
            if is_literal:
                object = token
            elif token == '{':
                object = PListDictionary()
                for var in parse_plist_dictionary(input_iter):
                    object[var[0]] = var[1]
            elif token == '(':
                object = []
                for var in parse_plist_list(input_iter):
                    object.append(var)
            else:
                parser_error(line, 'expected value, got ' + token)
        elif token == ';' and not is_literal:
            yield (object_name, object)
            object_name = None
            found_seperator = False
            object = None
        else:
            parser_error(line, 'unexpected token ' + token)
    
def parse_plist_list(input_iter):
    object = None
    for token, is_literal, line in input_iter:
        if object == None:
            if is_literal:
                object = token
            elif token == '{':
                object = []
                for var in parse_plist_dictionary(input_iter):
                    object.append(var)
            elif token == '(':
                object = []
                for var in parse_plist_list(input_iter):
                    object.append(var)
            elif token == ')':
                raise StopIteration
            else:
                parser_error(line, 'expected value, got ' + token)
        elif not is_literal:
            if token == ')': 
                if object != None: yield object
                raise StopIteration
            elif token == ',':
                yield object
                object = None
            else:
                parser_error(line, 'unexpected token ' + token)
        else:
            parser_error(line, 'unexpected token ' + token)

def serialize_key(key, value):
    try:
        return value.serialize_key()
    except:
        return escape_string(str(key))

def serialize_value(value, indent):
    try: return value.serialize()
    except AttributeError:
        if isinstance(value, PListDictionary): return serialize_dictionary(value, indent)
        elif isinstance(value, list): return serialize_list(value, indent)
        else: return escape_string(str(value))

def serialize_list_inline(data):
    """ Return a list serialized in PList format as a single line """
    out = '('
    for value in data:
        if isinstance(value, list): out += serialize_list_inline(value)
        elif isinstance(value, PListDictionary): out += serialize_dictionary_inline(value)
        elif isinstance(value, str): out += escape_string(value)
        else: out += str(value)
        out += ', '
    return out + ')'

def serialize_dictionary_inline(data):
    """ Return a PListDictionary serialized in PList format as a single line """
    out = '{'
    for key, value in data:
        out += serialize_key(key, value) + ' = '
        if isinstance(value, list): out += serialize_list_inline(value)
        elif isinstance(value, PListDictionary): out += serialize_dictionary_inline(value)
        elif isinstance(value, str): out += escape_string(value)
        else: out += str(value)
        out += '; '
    return out + '}'

def serialize_list(data, indent):
    indent_inner = indent + 1
    out = '('
    for value in data:
        out += '\n' + '\t' * indent_inner + serialize_value(value, indent_inner) +','
    return out + '\n' + '\t' * indent + ')'

def serialize_dictionary(data, indent):
    indent_inner = indent + 1
    out = '{'
    for key, value in data:
        out += '\n' + '\t' * indent_inner + serialize_key(key, value) + ' = ' + serialize_value(value, indent_inner) + ';'
    return out + '\n' + '\t' * indent + '}'

class PBXObjectReference:
    def __init__(self, object):
        self.object = object
    
    def serialize(self):
        return self.object.serialize_key()

class PBXBuildFile:
    def __init__(self):
        self.id = None
        self.fileref = None
        self.settings = None
    
    def serialize_key(self):
        return self.id + ' /* Buildfile for ' + self.fileref.friendly_name() + ' */'
    
    def serialize(self):
        out = '{isa = PBXBuildFile; fileRef = ' + self.fileref.serialize_key() + ''
        if self.settings: out += '; settings = ' + serialize_dictionary_inline(self.settings)
        return out + '; }'
    
    @staticmethod
    def unserialize(id, data):
        self = PBXBuildFile()
        self.id = id
        self.fileref = get_object(data['fileRef'])
        self.settings = data['settings']
        return self

class PBXGroup:
    path_dict = {}
    new_groups = []
    
    def __init__(self):
        self.id = None
        self.parent = None
        self.children = []
        self.name = None
        self.path = None
        self.source_tree = None

    def friendly_name(self):
        if self.name: return self.name
        else: return os.path.basename(self.path)

    def build_path(self):
        path = self.name if self.name else self.path
        if self.parent: return os.path.join(self.parent.build_path(), path)
        else: return ''

    def add_child(self, child):
        child.parent = self
        self.children.append(child)

    def remove_child(self, child):
        if (child in self.children):
            self.children.remove(child)
    
    def rebuild_path_dictionary(self):
        PBXGroup.path_dict[self.build_path()] = self
        for child in self.children:
            if hasattr(child, 'children'):
                child.rebuild_path_dictionary()
  
    def serialize_key(self):
        return self.id + ' /* ' + self.friendly_name() + ' */'
        
    def serialize(self):
        out = '{\n\t\t\tisa = PBXGroup;\n\t\t\tchildren = (\n'
        for child in self.children:
            out += '\t\t\t\t' + child.serialize_key() + ',\n'
        out += '\t\t\t)'
        if self.name:           out += ';\n\t\t\tname = ' + escape_string(self.name)
        if self.path:           out += ';\n\t\t\tpath = ' + escape_string(self.path)
        if self.source_tree:    out += ';\n\t\t\tsourceTree = ' + escape_string(self.source_tree)
        return out + ';\n\t\t}'
    
    @staticmethod
    def unserialize(id, data):
        self = PBXGroup()
        self.id = id
        self.parent = None
        self.children = []
        for child_id in data['children']:
            self.add_child(get_object(child_id))
        self.name = data['name']
        self.path = data['path']
        self.source_tree = data['sourceTree']
        #print ("pbxbroup.unserialize %s %s %s" %(self.name, self.path, self.children))
        #for child in self.children:
        #    print ("   %s" % child.name)
        return self
    
    @staticmethod
    def from_path(path):
        if path in PBXGroup.path_dict:
            return PBXGroup.path_dict[path]
        
        split_path = os.path.split(path)
        group = PBXGroup()
        group.id = generate_id()
        group.name = split_path[1]
        group.source_tree = '<group>'
        
        PBXGroup.from_path(split_path[0]).add_child(group)
        PBXGroup.path_dict[path] = group
        PBXGroup.new_groups.append(group)
        return group

class PBXFileReference:
    def __init__(self):
        self.id = None
        self.parent = None
        self.encoding = None
        self.type = None
        self.explicit_type = None
        self.include_in_index = None
        self.name = None
        self.path = None
        self.source_tree = None
        self.has_buildfile = False
    
    def friendly_name(self):
        if self.name: return self.name
        else: return os.path.basename(self.path)
    
    def build_path(self):
        path = self.name if self.name else self.path
        if self.parent: return os.path.join(self.parent.build_path(), path)
        else: return path
    
    def serialize_key(self):
        return self.id + ' /* ' + self.build_path() + ' */'
    
    def serialize(self):
        out = '{ isa = PBXFileReference'
        if self.encoding:           out += '; fileEncoding = ' + str(self.encoding)
        if self.type:               out += '; lastKnownFileType = ' + escape_string(self.type)
        if self.explicit_type:      out += '; explicitFileType = ' + escape_string(self.explicit_type)
        if self.include_in_index:   out += '; includeInIndex = ' + self.include_in_index
        if self.name:               out += '; name = ' + escape_string(self.name)
        if self.path:               out += '; path = ' + escape_string(self.path)
        if self.source_tree:        out += '; sourceTree = ' + escape_string(self.source_tree)
        return out + '; }'

    @staticmethod
    def unserialize(id, data):
        self = PBXFileReference()
        self.id = id
        self.parent = None
        self.encoding = data['fileEncoding']
        self.type = data['lastKnownFileType']
        self.explicit_type = data['explicitFileType']
        self.include_in_index = data['includeInIndex']
        self.name = data['name']
        self.path = data['path']
        self.source_tree = data['sourceTree']
        self.has_buildfile = True
        return self
        
        
objects_by_id = {}
def get_object(object_id):
    object = None
    if object_id not in objects_by_id:
        data = object_data[object_id]
        if data['isa'] == 'PBXBuildFile': object = PBXBuildFile.unserialize(object_id, data)
        elif data['isa'] == 'PBXGroup': object = PBXGroup.unserialize(object_id, data)
        elif data['isa'] == 'PBXFileReference': object = PBXFileReference.unserialize(object_id, data)
        objects_by_id[object_id] = object
    return objects_by_id[object_id]

def add_new_file(path, group_path, type = None, source_tree = 'SOURCE_ROOT'):
    basename = os.path.basename(path)
    
    fileref = None
    group = PBXGroup.from_path(group_path)
    for child in group.children:
        if not hasattr(child, 'children'):
            if child.name == basename:
                fileref = child
                break
    else:
        if type == None:
            extension = os.path.splitext(basename)[1]
            if extension not in file_type_by_extension_dict:
                print 'unknown extension ' + extension + ' in ' + basename
                return None
            type = file_type_by_extension_dict[extension]
        
        fileref = PBXFileReference()
        fileref.id = generate_id()
        fileref.name = basename
        fileref.type = type
        fileref.source_tree = source_tree
        group.add_child(fileref)

    fileref.path = path
    return fileref

def remove_file(path, group_path, type = None, source_tree = 'SOURCE_ROOT'):
    basename = os.path.basename(path)

    fileref = None
    group = PBXGroup.from_path(group_path)
    for child in group.children:
        if basename == os.path.basename(child.path):
            group.remove_child(child)
            fileref = child
            break

    return fileref

def add_system_framework(name):
    extension = os.path.splitext(name)[1]
    if extension not in file_type_by_extension_dict:
        print 'unknown extension ' + extension + ' in ' + name
        return None
    
    type = file_type_by_extension_dict[extension]
    path_prefix = ''
    if type == 'wrapper.framework':
        path_prefix = 'System/Library/Frameworks/'
    elif type == 'compiled.mach-o.dylib':
        path_prefix = 'usr/lib/'
    
    path = os.path.join(path_prefix, name)
    return add_new_file(path, 'Frameworks', type, 'SDKROOT')
    
def add_new_folder(path, pattern_list = ('*')):
    split_dir = os.path.split(path)
    if split_dir[1] == '': split_dir = os.path.split(split_dir[0])
    base_dir = split_dir[0]
    
    for root, subfolders, files in os.walk(path):
        for subfolder in subfolders[:]:
            if is_ignored(subfolder) or not matches_pattern(subfolder, pattern_list):
                subfolders.remove(subfolder)
                continue

        for filename in itertools.ifilterfalse(is_ignored, files):
            if matches_pattern(filename, pattern_list):
                path = os.path.relpath(os.path.join(root, filename), target_path)
                group_path = os.path.relpath(root, base_dir)
                fileref = add_new_file(path, group_path)
                if fileref: yield fileref


pbxproj_path = os.path.join(xcodeproj_path, 'project.pbxproj')
pickle_path = os.path.join(xcodeproj_path, 'project.pickle')

xcode_project = None
if os.path.exists(pickle_path):
    with open(pickle_path, 'rb') as pickle_file:
        xcode_project = pickle.load(pickle_file)
else:
    with open(pbxproj_path, 'rt') as pbxproj_file:
        xcode_project = parse_plist(file_by_char(pbxproj_file))
    with open(pickle_path, 'wb') as pickle_file:
        pickle.dump(xcode_project, pickle_file, -1)
root_group = None

object_data = xcode_project['objects']
project_id = xcode_project['rootObject']
project = object_data[project_id]

# load existing groups so we can modify to them
root_group = get_object(project['mainGroup'])
root_group.rebuild_path_dictionary()

#swap out loaded dictionaries with real objects for somewhat nicer output
for key, value in object_data.items():
    try:
        isa = value['isa']
        if isa == 'PBXBuildFile' or isa == 'PBXFileReference' or isa == 'PBXGroup':
            object_data[key] = get_object(key)
    except AttributeError:
        # we must have already processed it
        continue

# move frameworks from the root group into the Frameworks group
framework_group = PBXGroup.from_path('Frameworks')
root_children = []
for child in root_group.children:
    if isinstance(child, PBXFileReference):
        if child.type in library_types:
            framework_group.add_child(child)
            continue
    root_children.append(child)
root_group.children = root_children

# add new files
new_files = []
for folder, pattern in xcode_fixup.folders:
    for fileref in add_new_folder(folder, pattern):
        new_files.append(fileref)

for path, group_path in xcode_fixup.folder_refs:
    fileref = add_new_file(os.path.relpath(path, target_path), group_path, 'folder')
    if not fileref or fileref.has_buildfile: continue
    new_files.append(fileref)

for path, group_path in xcode_fixup.remove_files:
    fileref = remove_file(os.path.relpath(path, target_path), group_path)
    if fileref:
        print("success removing %s" % path)
    else:
        print("failure removing %s" % path)

    
for path, group_path in xcode_fixup.files:
    fileref = add_new_file(os.path.relpath(path, target_path), group_path)
    if not fileref or fileref.has_buildfile: continue
    new_files.append(fileref)



# sort out all the new files
new_libraries = []
new_resources = []
new_sources = []
new_user_framework_paths = []

for fileref in new_files:
    if fileref.type in source_types: new_sources.append(fileref)
    elif fileref.type in resource_types: new_resources.append(fileref)
    elif fileref.type in library_types:
        new_libraries.append((fileref, False))
        if fileref.type == 'wrapper.framework':
            new_user_framework_paths.append(fileref.path)

for framework in xcode_fixup.frameworks:
    fileref = add_system_framework(framework)
    new_libraries.append((fileref, False))
    new_files.append(fileref)

for framework in xcode_fixup.weak_frameworks:
    fileref = add_system_framework(framework)
    new_libraries.append((fileref, True))
    new_files.append(fileref)

#update the various build configurations with the new files and settings
new_buildfiles = []
for target_id in project['targets']:
    target = object_data[target_id]

    if target['name'].endswith('-simulator'):
        targets = project['targets']
        del targets[targets.index(target_id)]
        del object_data[target_id]
        continue

    build_config_list_id = target['buildConfigurationList']
    build_config_list = object_data[build_config_list_id]
    for build_config_id in build_config_list['buildConfigurations']:
        build_config = object_data[build_config_id]
        build_settings = build_config['buildSettings']
        build_settings['GCC_VERSION'] = 'com.apple.compilers.llvm.clang.1_0'
        build_settings['GCC_ENABLE_OBJC_EXCEPTIONS'] = 'YES'
        
        if not build_settings['LIBRARY_SEARCH_PATHS']: build_settings['LIBRARY_SEARCH_PATHS'] = ['$(inherited)']
        for library, is_weak in new_libraries:
            if library.type == 'archive.ar':
                path = '"$(SRCROOT)/' + os.path.dirname(library.path) + '"'
                build_settings['LIBRARY_SEARCH_PATHS'].append(path)
        
        if not build_settings['FRAMEWORK_SEARCH_PATHS']: build_settings['FRAMEWORK_SEARCH_PATHS'] = ['$(inherited)']
        for rel_path in new_user_framework_paths:
            path = '"$(SRCROOT)/' + os.path.dirname(rel_path) +  '"'
            build_settings['FRAMEWORK_SEARCH_PATHS'].append(path)

        build_settings['OTHER_LDFLAGS'] = ''
        build_settings['DEBUG_INFORMATION_FORMAT'] = 'dwarf-with-dsym'
        build_settings['GCC_GENERATE_DEBUGGING_SYMBOLS']  = 'YES' # yes, we really want the symbols we told you to generate...
        build_settings['DEPLOYMENT_POSTPROCESSING'] = 'YES'
    
    for build_phase_id in target['buildPhases']:
        build_phase = object_data[build_phase_id]
        
        # replace file reference ids with file reference objects
        phase_files = []
        for buildfile_id in build_phase['files']:
            phase_files.append(PBXObjectReference(get_object(buildfile_id)))
        
        # add new files based off type
        isa = build_phase['isa']
        if isa == 'PBXFrameworksBuildPhase':
            for fileref, is_weak in new_libraries:
                buildfile = PBXBuildFile()
                buildfile.id = generate_id()
                buildfile.fileref = fileref
                if is_weak: buildfile.settings = PListDictionary([('ATTRIBUTES', ['Weak'])])
                new_buildfiles.append(buildfile)
                phase_files.append(PBXObjectReference(buildfile))
        elif isa == 'PBXSourcesBuildPhase':
            for fileref in new_sources:
                buildfile = PBXBuildFile()
                buildfile.id = generate_id()
                buildfile.fileref = fileref
                new_buildfiles.append(buildfile)
                phase_files.append(PBXObjectReference(buildfile))
        elif isa == 'PBXResourcesBuildPhase':
            for fileref in new_resources:
                buildfile = PBXBuildFile()
                buildfile.id = generate_id()
                buildfile.fileref = fileref
                new_buildfiles.append(buildfile)
                phase_files.append(PBXObjectReference(buildfile))
                
        # replace file list with the one we built
        build_phase['files'] = phase_files

# add any new objects onto the end of the object data
for resource in itertools.chain(PBXGroup.new_groups, new_buildfiles, new_files):
    object_data.items().append((resource.id, resource))

# write out new object data
with open(pbxproj_path, 'wt') as pbxproj_file:
    pbxproj_file.write(serialize_dictionary(xcode_project, 0))
#print serialize_dictionary(xcode_project, 0)

for step in xcode_fixup.additional_steps:
    step(target_path)
