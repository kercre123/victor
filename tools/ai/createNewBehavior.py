#!/usr/bin/env python3

from generateBehaviorCode import *

import argparse
from datetime import datetime
from string import Template
import os

TEMPLATE_H='tools/ai/behavior/b_template.h'
TEMPLATE_CPP='tools/ai/behavior/b_template.cpp'

def lowerFirst(s):
    ''' return a string which matches `s` but with a lowercase first letter '''
    return s[0].lower() + s[1:]

def upperFirst(s):
    ''' return a string which matches `s` but with a uppercase first letter '''
    return s[0].upper() + s[1:]

def correctCodePath(repo_root, path):
    '''
    Take the user supplied `path` and return an absolute path which correctly includes the behavior code
    subfolder.
    Return None if there's an error
    '''
    
    full_path = os.path.abspath( path )
    
    # if the path doesn't start with the behavior code path, add it automatically
    behavior_code_path = os.path.join(repo_root, BEHAVIOR_CODE_PATH)

    if not os.path.commonprefix([ behavior_code_path, full_path ]) == behavior_code_path:
        print('NOTE: passed in path "{}" expanded to "{}" does not contain the behavior code path "{}"'.format(
            args.path,
            full_path,
            behavior_code_path))
        
        # assume their passed in path was relative to the behaviors code folder
        full_path = os.path.join( behavior_code_path, args.path )
        print('NOTE: assuming full path is {}'.format(full_path))

    if os.path.isfile(full_path):
        print('ERROR: specified path must be a directory, not a file')
        return None

    return full_path
    

if __name__ == "__main__":

    var_descriptions = {'class_name': 'name of the C++ class. "Behavior" will be prepended if missing',
                        'author': 'name of the author of this new behavior',
                        'description': 'short description of the behavior'}

    
    parser = argparse.ArgumentParser(description='Create a new behavior class')
    parser.add_argument('path',
                        help='path to directory where this new behavior should be added (engine code path)')
    parser.add_argument('-x', '--non-interactive', action="store_false", dest='interactive', default=True,
                        help='Force non-interactive mode. By default, user will be prompted for missing information')

    for var_name in var_descriptions:
        # add each variable as an argument, with it's description as the help
        parser.add_argument('--{}'.format( var_name.replace('_', '-') ),
                            help=var_descriptions[var_name])
    
    args = parser.parse_args()

    values = {}

    for var_name in var_descriptions:
        if getattr(args, var_name):
            values[var_name] = getattr(args, var_name)
        elif args.interactive:
            values[var_name] = input(var_descriptions[var_name] + ': ')
        else:
            values[var_name] = var_name.upper()

    today = datetime.date( datetime.today() )
    values['date'] = today.isoformat()
    values['year'] = today.year
    
    # make sure first letter is capital
    values['class_name'] = upperFirst(values['class_name'])

    if values['class_name'].find('Behavior') != 0:
        print('NOTE: Behavior class name {} does not begin with "Behavior", automatically prepending'.format(
            values['class_name']))
        values['class_name'] = 'Behavior' + values['class_name']

    repo_root = get_repo_root(__file__, TOOLS_PATH)

    full_path = correctCodePath(repo_root, args.path)
    if full_path is None:
        exit(-1)
        
    if not os.path.exists(full_path):
        print('NOTE: behavior path {} does not previously exist'.format(args.path))
        if args.interactive:
            reply = input('create directory {}? (y/N) '.format(full_path))
            if len(reply) == 0 or reply[0].lower() != 'y':
                exit(-1)

        try:
            os.makedirs(full_path)
        except os.error as E:
            print('ERROR: could not create path {}'.format(full_path))
            print(E)
            exit(-1)
    
    h_filename = lowerFirst(values['class_name']) + '.h'
    cpp_filename = lowerFirst(values['class_name']) + '.cpp'

    full_h_path = os.path.join(full_path, h_filename)
    full_cpp_path = os.path.join(full_path, cpp_filename)

    for p in [full_h_path, full_cpp_path]:
        if os.path.exists(p):
            if args.interactive:
                reply = input('Warning: file {} already exists, really overwrite with new template? (y/N)'.format(p))
                if len(reply) == 0 or reply[0].lower() != 'y':
                    exit(-1)
            else:
                print('Error: file {} already exists, refusing to overwrite'.format(p))

    values['include_path'] = os.path.relpath(full_h_path, repo_root)

    import pprint
    pprint.pprint(values)

    template_h_path = os.path.join(repo_root, TEMPLATE_H)
    
    with open(template_h_path, 'r') as infile:
        t = Template(infile.read())
        with open(full_h_path, 'w') as outfile:
            outfile.write( t.substitute(values) )

    template_cpp_path = os.path.join(repo_root, TEMPLATE_CPP)
    
    with open(template_cpp_path, 'r') as infile:
        t = Template(infile.read())
        with open(full_cpp_path, 'w') as outfile:
            outfile.write( t.substitute(values) )

    # regenerate all generated code / CLAD
    generate_all()
