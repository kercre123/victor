/**
 * File: CladFromJSONHelpers.h
 *
 * Author: Kevin M. Karol
 * Created: 4/3/18
 *
 * Description: Helper functions for parsing CLAD types from JSON
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Cozmo_Basestation_Events_CladFromJSONHelpers_H__
#define __Cozmo_Basestation_Events_CladFromJSONHelpers_H__


#include "clad/types/animationTrigger.h"
#include "json/json-forwards.h"


namespace Anki {  
namespace JsonTools {

template<class CladEnum>
bool GetCladEnumFromJSON(const Json::Value& config, const std::string& key,  CladEnum& value, 
                         const std::string& debugName, bool shouldAssertIfMissing = true)
{
  const Json::Value& child = config[key];
  if(child.isNull()){
    const char* debugMsg = (debugName + ".GetCladEnumFromJSON.ParseString.KeyMissing").c_str();
    DEV_ASSERT_MSG(!shouldAssertIfMissing, debugMsg, "%s", key.c_str());
    return false;
  }
  std::string str = child.asString();
  // Note - this functionality can only be used in engine until VIC-2545 is imlemented
  const bool foundValue = Cozmo::EnumFromString(str.c_str(), value);
  if(!foundValue){
    const char* debugMsg = (debugName + "GetCladEnumFromJSON.ParseString.InvalidValue").c_str();
    DEV_ASSERT_MSG(!shouldAssertIfMissing, debugMsg, "%s", key.c_str());
  }
    
  return foundValue;
}

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Events_CladFromJSONHelpers_H__

