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


#ifndef __Cozmo_Basestation_Events_AnimationTriggerHelpers_H__
#define __Cozmo_Basestation_Events_AnimationTriggerHelpers_H__


#include "clad/types/animationTrigger.h"
#include "util/enums/enumOperators.h"
#include "json/json-forwards.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(AnimationTrigger);

AnimationTrigger AnimationTriggerFromString(const char* inString, bool assertOnInvalidEnum = true);
  
bool IsAnimationTrigger(const char* inString);


} // namespace Cozmo
  
namespace JsonTools
{
  bool GetValueOptional(const Json::Value& config, const std::string& key, Cozmo::AnimationTrigger& value);
}
} // namespace Anki


#endif // __Cozmo_Basestation_Events_AnimationTriggerHelpers_H__

