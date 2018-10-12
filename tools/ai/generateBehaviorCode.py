#!/usr/bin/env python3

import os

from behavior.behavior_class import *
from behavior.behavior_code_generation import *
from behavior.behavior_instance import *

import argparse


TOOLS_PATH='tools/ai'
BEHAVIOR_CODE_PATH='engine/aiComponent/behaviorComponent/behaviors'
BEHAVIOR_CONFIG_PATH='resources/config/engine/behaviorComponent/behaviors'

GENERATED_BEHAVIOR_FACTORY_CODE_PATH='engine/aiComponent/behaviorComponent/behaviorFactory.cpp'
GENERATED_BEHAVIOR_CLASS_CLAD_PATH='clad/src/clad/types/behaviorComponent/behaviorClasses.clad'
GENERATED_BEHAVIOR_ID_CLAD_PATH='clad/src/clad/types/behaviorComponent/behaviorIDs.clad'

def get_repo_root(filename, expected_path_input):
    '''
    Check that filename is in the path in expected_path (list of directories).
    If so, return the root (filename - expected_path), otherwise reutrn None
    '''

    curr_path = os.path.dirname( os.path.abspath( filename ) )
    expected_path = expected_path_input
    
    tries = 0
    while expected_path:
        tries += 1
        if tries > 100:
            print("infinite loop bug! input '{}', '{}'".format(filename, expected_path_input))
            return None
        
        expected_path, expected_dir = os.path.split(expected_path)
        curr_path, found_dir = os.path.split(curr_path)

        if expected_dir != found_dir:
            print("path mismatch: '{}' does not end with '{}'".format(filename, os.path.join(*expected_path)))
            return None

    return curr_path


def rel_path_sorter(x):
    if os.path.dirname(x.rel_path) == '':
        # no directory, so add a space to the front so that this sorts to the top
        return ' ' + x.rel_path
    else:
        return x.rel_path

def get_behavior_classes(behaviors_code_path):
    '''
    Return a list of BehaviorClass objects for every behavior defined in behaviors_code_path
    Return list will be sorted by relative path and filename
    '''
    
    behavior_classes = []
    
    for root, dirs, files in os.walk(behaviors_code_path):
        # skip "helper" folders which may contain things that are named like behaviors
        # e.g. BehaviorScoringWrapper (which is a wrapper for behavior scores, not a
        # behavior itself)
        if "helpers" in dirs:
            dirs.remove("helpers")
            
        for filename in files:
            if os.path.splitext(filename)[1] in ['.h', '.hpp']:
                full_path = os.path.join(root, filename)
                rel_path = os.path.relpath(full_path, behaviors_code_path)
                behavior_class = get_behavior_class(full_path, filename)
                if behavior_class:
                    behavior_classes.append( BehaviorClass(rel_path, filename, behavior_class) )

    behavior_classes.sort(key=rel_path_sorter)
    
    return behavior_classes

def get_behavior_instances(behaviors_config_path):
    behavior_instances = []
    all_classes = set()

    for root, dirs, files in os.walk(behaviors_config_path):            
        for filename in files:
            if os.path.splitext(filename)[1] == '.json':
                full_path = os.path.join(root, filename)
                behavior_instance, instance_classes = get_behavior_instance(full_path, behaviors_config_path)
                all_classes |= instance_classes
                if behavior_instance:
                    behavior_instances.append(behavior_instance)

    behavior_instances.sort(key=rel_path_sorter)
    return behavior_instances, all_classes

def generate_all(options=None):


    
    repo_root = get_repo_root(__file__, TOOLS_PATH)
    print('repo root is {}'.format(repo_root))

    behaviors_code_path = os.path.join(repo_root, BEHAVIOR_CODE_PATH)
    print('reading behavior code from {}'.format(behaviors_code_path))

    if not os.path.isdir(behaviors_code_path):
        print("error: invalid path '{}'".format(behaviors_code_path))
        return False

    behavior_classes = get_behavior_classes(behaviors_code_path)
    print('found {} behavior class files'.format(len(behavior_classes)))

    factory_cpp_path = os.path.join(repo_root, GENERATED_BEHAVIOR_FACTORY_CODE_PATH)        
    write_behavior_factory_cpp(factory_cpp_path, behavior_classes, BEHAVIOR_CODE_PATH)

    class_clad_path = os.path.join(repo_root, GENERATED_BEHAVIOR_CLASS_CLAD_PATH)
    write_behavior_class_clad(class_clad_path, behavior_classes)

    behaviors_config_path = os.path.join(repo_root, BEHAVIOR_CONFIG_PATH)
    print('reading behavior jsons from {}'.format(behaviors_config_path))

    behavior_instances, all_instance_classes = get_behavior_instances(behaviors_config_path)
    print('found {} behavior instance json files'.format(len(behavior_instances)))

    id_clad_path = os.path.join(repo_root, GENERATED_BEHAVIOR_ID_CLAD_PATH)
    write_behavior_id_clad(id_clad_path, behavior_instances)

    ## now check that the behavior classes in the instances match real behavior classes
    all_behavior_classes = set([ x.class_name for x in behavior_classes ])

    anyError = False
    for instance in behavior_instances:
        if instance.behavior_class not in all_behavior_classes:
            print('ERROR: file {} specifies behavior invalid behavior class: {}'.format(
                instance.rel_path, instance.behavior_class))
            anyError = True
        
    if options and options.dangling_classes:   
        for behavior_class in sorted(all_behavior_classes):
            if behavior_class not in all_instance_classes:
                print('WARNING: behavior class {} not found in any behavior instances'.format(behavior_class))

    if anyError:
        return False
    else:
        return True

def parse_argv():
    desc = 'Generate behavior code (CLAD files for BehaviorID and BehaviorClass '
    'as well as factory methods) from C++ classes and behavior instance json files'
    parser = argparse.ArgumentParser( description=desc )
    parser.add_argument( '-d', '--dangling-classes', action="store_true", default=False,
                        help='display behavior classes without a behavior instance' )

    options = parser.parse_args()
    return options


if __name__ == "__main__":
    options = parse_argv()
    if not generate_all(options):
        exit(-1)
