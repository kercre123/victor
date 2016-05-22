/**
 * File: behaviorGroupHelpers
 *
 * Author: Mark Wesley
 * Created: 01/14/15
 *
 * Description: Helper functions for dealing with CLAD generated behaviorGroup
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/behaviorGroupHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(BehaviorGroup);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<BehaviorGroup> gStringToBehaviorGroupMapper;

BehaviorGroup BehaviorGroupFromString(const char* inString)
{
  return gStringToBehaviorGroupMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

