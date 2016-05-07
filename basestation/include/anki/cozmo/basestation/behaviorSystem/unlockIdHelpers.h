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

#ifndef __Cozmo_Basestation_BehaviorSystem_UnlockIdHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_UnlockIdHelpers_H__


#include "clad/types/unlockTypes.h"
#include "util/enums/enumOperators.h"
#include <string>

namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(UnlockId);

UnlockId UnlockIdFromString(const char* inString);

inline UnlockId UnlockIdFromString(const std::string& inString)
{
  return UnlockIdFromString(inString.c_str());
}


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_UnlockIdHelpers_H__

