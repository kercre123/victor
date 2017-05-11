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


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__


#include "clad/types/behaviorTypes.h"
#include "clad/types/reactionTriggers.h"
#include "util/enums/enumOperators.h"
#include <string>


namespace Anki {
namespace Cozmo {

DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorClass);
DECLARE_ENUM_INCREMENT_OPERATORS(ReactionTrigger);
DECLARE_ENUM_INCREMENT_OPERATORS(ExecutableBehaviorType);  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorTypesHelpers_H__

