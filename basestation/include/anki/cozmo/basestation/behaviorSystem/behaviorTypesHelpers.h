/**
 * File: behaviorTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/20/15
 *
 * Description: Helper functions for dealing with CLAD generated behaviorTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__


#include "clad/types/behaviorTypes.h"
#include "util/enums/enumOperators.h"
#include <string>


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorClass);

BehaviorClass BehaviorClassFromString(const char* inString);

inline BehaviorClass BehaviorClassFromString(const std::string& inString)
{
  return BehaviorClassFromString(inString.c_str());
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_ENUM_INCREMENT_OPERATORS(ReactionTrigger);

ReactionTrigger ReactionTriggerFromString(const char* inString);

inline ReactionTrigger ReactionTriggerFromString(const std::string& inString)
{
  return ReactionTriggerFromString(inString.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
DECLARE_ENUM_INCREMENT_OPERATORS(ExecutableBehaviorType);
  
ExecutableBehaviorType ExecutableBehaviorTypeFromString(const char* inString);
  
inline ExecutableBehaviorType ExecutableBehaviorTypeFromString(const std::string& inString)
{
  return ExecutableBehaviorTypeFromString(inString.c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGameFlag BehaviorGameFlagFromString(const std::string& inString);

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

