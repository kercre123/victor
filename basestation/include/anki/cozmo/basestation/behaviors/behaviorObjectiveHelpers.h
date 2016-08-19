/**
 * File: behaviorObjectiveHelpers
 *
 * Author: Kevin M. Karol
 * Created: 08/18/16
 *
 * Description: Helper functions for dealing with CLAD generated behaviorObjectives
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_Animations_AnimEventHelpers_H__
#define __Cozmo_Basestation_Animations_AnimEventHelpers_H__


#include "clad/types/behaviorObjectives.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorObjective);

BehaviorObjective BehaviorObjectiveFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Animations_AnimEventHelpers_H__

