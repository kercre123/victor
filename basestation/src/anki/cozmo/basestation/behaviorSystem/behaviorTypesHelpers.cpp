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


#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_ENUM_INCREMENT_OPERATORS(BehaviorType);

// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<BehaviorType> gStringToBehaviorTypeMapper;

BehaviorType BehaviorTypeFromString(const char* inString)
{
  return gStringToBehaviorTypeMapper.GetTypeFromString(inString);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IMPLEMENT_ENUM_INCREMENT_OPERATORS(ExecutableBehaviorType);
  
// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<ExecutableBehaviorType> gStringToExecutableBehaviorTypeMapper;

ExecutableBehaviorType ExecutableBehaviorTypeFromString(const char* inString)
{
  return gStringToExecutableBehaviorTypeMapper.GetTypeFromString(inString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// BehaviorGameFlag
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorGameFlag BehaviorGameFlagFromString(const std::string& inString)
{
  // can't use StringToEnumMapper because the enum class is a flag enum (discontinuous)
  // todo rsam Should clad generate stringToEnum as well?
  if ( inString == "NoGame" )
  {
    return BehaviorGameFlag::NoGame;
  }
  else if ( inString == "SpeedTap" )
  {
    return BehaviorGameFlag::SpeedTap;
  }
  else if ( inString == "MemoryMatch" ) {
    return BehaviorGameFlag::MemoryMatch;
  }
  else if ( inString == "CubePounce" ) {
    return BehaviorGameFlag::CubePounce;
  }
  else if ( inString == "All" ) {
    return BehaviorGameFlag::All;
  }
  else if ( inString == "KeepAway"){
    return BehaviorGameFlag::KeepAway;
  }

  ASSERT_NAMED(false, "BehaviorGameFlagFromString.InvalidString");
  return BehaviorGameFlag::NoGame;
}

} // namespace Cozmo
} // namespace Anki

