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


#include "clad/types/behaviorType.h"
#include "util/enums/enumOperators.h"
#include <string>


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorType);

BehaviorType BehaviorTypeFromString(const char* inString);

inline BehaviorType BehaviorTypeFromString(const std::string& inString)
{
  return BehaviorTypeFromString(inString.c_str());
}


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

