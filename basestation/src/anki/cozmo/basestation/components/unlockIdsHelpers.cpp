/**
 * File: unlockIdsHelpers.cpp
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/components/unlockIdsHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"

namespace Anki {
namespace Cozmo {

IMPLEMENT_ENUM_INCREMENT_OPERATORS(UnlockId);

// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<UnlockId> gStringToUnlockIdsMapper;

UnlockId UnlockIdsFromString(const char* inString)
{
  return gStringToUnlockIdsMapper.GetTypeFromString(inString);
}

}
}
