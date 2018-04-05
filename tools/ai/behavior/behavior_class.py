'''
Code for parsing behavior classes
'''

import collections
import os
import re

BehaviorClass = collections.namedtuple('BehaviorClass', ['rel_path', 'file_name', 'class_name'])

def get_behavior_class(full_path, filename):
    '''
    given a behavior class header filename (.h file), return the behavior class, or None if there's an error
    '''
    behavior_len = len('behavior')
    base = os.path.splitext(filename)[0]
    if base[:behavior_len].lower() != 'behavior':
        # print("Error: file name '{}' does not begin with 'behavior'".format(filename))
        return None

    ## read the file and make sure there is a matching class name defined within
    found_match = False
    class_name_re = re.compile(' *class ([a-z,A-Z,0-9]*)')

    mismatches = []
    
    with open(full_path, 'r') as infile:
        for line in infile:
            match = class_name_re.match(line)
            if match:
                class_name = match.group(1)
                if class_name[:behavior_len] == 'Behavior':
                    if class_name[behavior_len:] == base[behavior_len:]:
                        found_match = True
                        break
                    else:
                        mismatches.append(class_name)
                

    if found_match:
        return base[behavior_len:]
    else:
        if mismatches:
            print("Warning: file '{}' does not contain a valid behavior definition. It contained {}".format(
                full_path,
                ', '.join(mismatches)))
        else:
            print("Warning: file '{}' does not contain any behavior definitions".format(full_path))

        return None
