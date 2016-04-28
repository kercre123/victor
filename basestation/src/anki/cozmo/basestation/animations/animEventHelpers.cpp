/**
 * File: animEventHelpers
 *
 * Author: Kevin Yoon
 * Created: 04/26/16
 *
 * Description: Helper functions for dealing with CLAD generated animation types
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/animations/animEventHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(AnimEvent);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<AnimEvent> gStringToAnimEventMapper;

AnimEvent AnimEventFromString(const char* inString)
{
  return gStringToAnimEventMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

