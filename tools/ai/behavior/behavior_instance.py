'''
Code for handling behavior instances
'''

import collections
import os
import sys

## the default json package doesn't support comments, so load demjson
scriptPath = os.path.dirname(os.path.realpath(__file__))
sys.path.append(scriptPath + "/demjson")
import demjson

BehaviorInstance = collections.namedtuple('BehaviorInstance', ['rel_path', 'behavior_id', 'behavior_class'])

def get_behavior_instance(full_path, behaviors_config_path):
    '''
    given a behavior instance json file, return the behavior instance, or None if there's an error
    '''
    filename = os.path.basename(full_path)
    base_filename = os.path.splitext(filename)[0]
    
    try:
        J = demjson.decode_file(full_path)
    except demjson.JSONException as e:
        print("Error: could not read json file {}: {}".format(full_path, e))
        return None

    if "behaviorClass" not in J:
        print("Error: file {} does not contain a behavior class".format(full_path))
        return None

    if "behaviorID" not in J:
        print("Error: file {} does not contain a behavior ID".format(full_path))
        return None

    if J["behaviorID"].lower() != base_filename.lower():
        print("Warning: file name {} does not match behavior ID {}".format(filename, J["behaviorID"]))

    rel_path = os.path.relpath(full_path, behaviors_config_path)
    return BehaviorInstance(rel_path=rel_path, behavior_id=J["behaviorID"], behavior_class=J["behaviorClass"])

if __name__ == '__main__':
    import sys
    path = sys.argv[1]
    bi = get_behavior_instance(path, '.')
    print(bi)
