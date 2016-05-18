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


#ifndef __Cozmo_Basestation_Events_GameEventHelpers_H__
#define __Cozmo_Basestation_Events_GameEventHelpers_H__


#include "clad/types/gameEvent.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(GameEvent);

GameEvent GameEventFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Events_GameEventHelpers_H__

