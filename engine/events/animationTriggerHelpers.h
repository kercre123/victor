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
#include "json/json-forwards.h"


namespace Anki {
  
namespace JsonTools
{
  bool GetValueOptional(const Json::Value& config, const std::string& key, Cozmo::AnimationTrigger& value);
}
} // namespace Anki


#endif // __Cozmo_Basestation_Events_AnimationTriggerHelpers_H__

