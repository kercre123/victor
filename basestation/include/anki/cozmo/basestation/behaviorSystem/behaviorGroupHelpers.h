/**
 * File: behaviorGroupHelpers
 *
 * Author: Mark Wesley
 * Created: 01/14/15
 *
 * Description: Helper functions for dealing with CLAD generated behaviorGroup
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorGroupHelpers_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorGroupHelpers_H__


#include "clad/types/behaviorGroup.h"
#include "util/enums/enumOperators.h"
#include <string>


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(BehaviorGroup);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorGroupHelpers_H__

