/**
 * File: unlockIdHelpers
 *
 * Author: Raul Sampedro
 * Created: 05/06/16
 *
 * Description: Helper functions for dealing with CLAD generated unlockId
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/behaviorSystem/unlockIdHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(UnlockId);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<UnlockId> gStringToUnlockIdMapper;

UnlockId UnlockIdFromString(const char* inString)
{
  return gStringToUnlockIdMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

