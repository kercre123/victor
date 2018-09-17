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

def get_anonymous_behavior_classes(json, full_path):
    '''
    given behavior json, return all values of the key behaviorClass (this is different than
    get_behavior_instance since that ignores anonymous behaviors
    '''
    if "anonymousBehaviors" not in json:
        return None

    behavior_classes = set()
    for anon_behavior in json["anonymousBehaviors"]:
        if "behaviorClass" not in anon_behavior:
            print("Error: file {} has an anonymous behavior without a behavior class".format(full_path))
            return None
        behavior_classes.add( anon_behavior["behaviorClass"] )
        # just in case someone makes a nested anonymous behavior
        child_classes = get_anonymous_behavior_classes( anon_behavior, full_path )
        if child_classes is not None:
            behavior_classes |= child_classes
    return behavior_classes


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

    behavior_class = J["behaviorClass"]
    all_classes = set([behavior_class])
    anonymous_classes = get_anonymous_behavior_classes( J, full_path )
    if anonymous_classes is not None:
        all_classes |= anonymous_classes

    rel_path = os.path.relpath(full_path, behaviors_config_path)
    return BehaviorInstance(rel_path=rel_path, behavior_id=J["behaviorID"], behavior_class=behavior_class), all_classes

if __name__ == '__main__':
    import sys
    path = sys.argv[1]
    bi = get_behavior_instance(path, '.')
    print(bi)
