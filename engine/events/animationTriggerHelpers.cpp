/**
 * File: AnimationTriggerHelpers
 *
 * Author: Molly Jameson
 * Created: 05/16/16
 *
 * Description: Helper functions for dealing with CLAD generated AnimationTrigger types
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "engine/events/animationTriggerHelpers.h"
#include "coretech/common/engine/jsonTools.h"
#include "util/logging/logging.h"


namespace Anki {
namespace JsonTools{

// Will not set anything if no value is found, however if something is filled in that key,
// Will assert error if not found in list.
bool GetValueOptional(const Json::Value& config, const std::string& key, Cozmo::AnimationTrigger& value)
{
  const Json::Value& child(config[key]);
  if(child.isNull())
    return false;
  std::string str = GetValue<std::string>(child);
  value = Cozmo::AnimationTriggerFromString(str.c_str());
    
  return true;
}


Cozmo::AnimationTrigger ParseAnimationTrigger(const Json::Value& config, const char* key, const std::string& debugName) {
  const auto& val = config[key];
  DEV_ASSERT_MSG(val.isString(), (debugName + ".ParseString.NotValidString").c_str(), "%s", key);
  Cozmo::AnimationTrigger trigger;
  if( AnimationTriggerFromString(val.asCString(), trigger) ) {
    return trigger;
  } else {
    return Cozmo::AnimationTrigger::Count;
  }
};


}
} // namespace Anki

