/**
 * File: unlockIdsHelpers.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-04
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_UnlocksIdsHelpers_H__
#define __Anki_Cozmo_Basestation_Components_UnlocksIdsHelpers_H__

#include "clad/types/unlockTypes.h"
#include "util/enums/enumOperators.h"
#include <string>

namespace Anki {
namespace Cozmo {

DECLARE_ENUM_INCREMENT_OPERATORS(UnlockId);

UnlockId UnlockIdsFromString(const char* inString);

inline UnlockId UnlockIdsFromString(const std::string& inString)
{
  return UnlockIdsFromString(inString.c_str());
}

}
}


#endif
