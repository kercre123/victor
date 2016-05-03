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


#ifndef __Cozmo_Basestation_Animations_AnimEventHelpers_H__
#define __Cozmo_Basestation_Animations_AnimEventHelpers_H__


#include "clad/types/animationEvents.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(AnimEvent);

AnimEvent AnimEventFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Animations_AnimEventHelpers_H__

