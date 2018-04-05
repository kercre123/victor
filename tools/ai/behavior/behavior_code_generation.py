'''
Code to generate C++ and CLAD source based on behavior
'''

import os

from string import Template

################################################################################
# C++ generation
################################################################################

auto_generated_comment =  '''\
// This file is manually generated using ./tools/ai/generateBehaviorCode.py
// To add a behavior, just create the .cpp and .h files in the correct places
// and re-run ./tools/ai/generateBehavior.py
'''

factory_file_template = '''\
${auto_generated_comment}
// This factory creates behaviors from behavior JSONs with a specified
// `behavior_class` where the behavior C++ class name and file name both match
// Behavior{behavior_class}.h/cpp

#include "engine/aiComponent/behaviorComponent/behaviorFactory.h"

$includes

#include "clad/types/behaviorComponent/behaviorTypes.h"

namespace Anki {
namespace Cozmo {

ICozmoBehaviorPtr BehaviorFactory::CreateBehavior(const Json::Value& config)
{
  const BehaviorClass behaviorType = ICozmoBehavior::ExtractBehaviorClassFromConfig(config);

  ICozmoBehaviorPtr newBehavior;

  switch (behaviorType)
  {
$factory_case_statements
  }
    
  return newBehavior;
}

}
}
'''

def generate_include(behavior, path_to_behavior_code):
    return '#include "{}"'.format(os.path.join(path_to_behavior_code, behavior.rel_path))

def generate_factory_case(behavior):
    return '''\
    case BehaviorClass::{behavior_class}:
    {{
      newBehavior = ICozmoBehaviorPtr(new Behavior{behavior_class}(config));
      break;
    }}
    '''.format(behavior_class = behavior.class_name)


def write_behavior_factory_cpp(filename, behaviors, path_to_behavior_code):
    '''
    Write a behavior factory cpp file to `filename` that contains a factory for
    making all of the BehaviorClass objects in the `behaviors` list
    '''
    with open(filename, 'w') as outfile:
        t = Template(factory_file_template)
        d = {}
        d['includes'] = os.linesep.join( [generate_include(x, path_to_behavior_code) for x in behaviors] )
        d['factory_case_statements'] = os.linesep.join( [generate_factory_case(x) for x in behaviors] )
        d['auto_generated_comment'] = auto_generated_comment
        outfile.write(t.substitute(d))

################################################################################
# CLAD generation
################################################################################

behavior_class_clad_prefix = '''\

namespace Anki {
namespace Cozmo {

// BehaviorClass identifies the ICozmoBehavior child class which the behavior is
// an instance of. The names of the BehaviorClass, the code file, and the actual C++ class
// all match, with the filename and the C++ class being prefixed with 'Behavior'
// (e.g. BehaviorRollBlock is defined in behaviorRollBlock.h and is associated with
//  BehaviorClass::RollBlock).
enum uint_8 BehaviorClass {
'''

behavior_class_clad_suffix = '''\
}

}
}
'''

def write_behavior_class_clad(filename, behaviors):
    '''
    write a behavior class CLAD file to `filename` based on the BehaviorClass
    objects in the `behaviors` list
    '''
    with open(filename, 'w') as outfile:
        outfile.write(auto_generated_comment)
        outfile.write(behavior_class_clad_prefix)

        last_dir = None
        for behavior in behaviors:
            this_dir = os.path.dirname(behavior.rel_path)
            if last_dir != this_dir:
                print_dir = this_dir if this_dir else '<root>'
                outfile.write('\n  // {}\n'.format(print_dir))
                last_dir = this_dir

            outfile.write('  {},\n'.format(behavior.class_name))
        
        outfile.write(behavior_class_clad_suffix)


behavior_id_clad_prefix = '''\

namespace Anki {
namespace Cozmo {

// BehaviorID is a unique identifier for each instance of a behavior
// so that they can be referenced by other parts of the behavior system

enum uint_8 BehaviorID {

  // required (not based on config)
  Anonymous,
  DevExecuteBehaviorRerun,
  Wait_TestInjectable,
'''

behavior_id_clad_suffix = '''\
}

}
}
'''

def write_behavior_id_clad(filename, behaviors):
    '''
    write a behaviorID CLAD file to `filename` based on the Behavior instances
    objects in the `behaviors` list
    '''
    with open(filename, 'w') as outfile:
        outfile.write(auto_generated_comment)
        outfile.write(behavior_id_clad_prefix)

        last_dir = None
        for behavior in behaviors:
            this_dir = os.path.dirname(behavior.rel_path)
            if last_dir != this_dir:
                print_dir = this_dir if this_dir else '<root>'
                outfile.write('\n  // {}\n'.format(print_dir))
                last_dir = this_dir

            outfile.write('  {},\n'.format(behavior.behavior_id))
        
        outfile.write(behavior_id_clad_suffix)
        
