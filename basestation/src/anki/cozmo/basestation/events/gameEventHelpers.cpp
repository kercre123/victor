/**
 * File: gameEventHelpers
 *
 * Author: Molly Jameson
 * Created: 05/16/16
 *
 * Description: Helper functions for dealing with CLAD generated gameEvent types
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/events/gameEventHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(GameEvent);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<GameEvent> gStringToGameEventMapper;

GameEvent GameEventFromString(const char* inString)
{
  return gStringToGameEventMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

