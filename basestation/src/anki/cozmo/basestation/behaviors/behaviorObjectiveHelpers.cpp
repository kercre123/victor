/**
 * File: behaviorObjectiveHelpers
 *
 * Author: Kevin M. Karol
 * Created: 08/18/16
 *
 * Description: Helper functions for dealing with CLAD generated behaviorObjectives
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/behaviors/behaviorObjectiveHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(BehaviorObjective);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<BehaviorObjective> gStringToAnimEventMapper;

BehaviorObjective BehaviorObjectiveFromString(const char* inString)
{
  return gStringToAnimEventMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

