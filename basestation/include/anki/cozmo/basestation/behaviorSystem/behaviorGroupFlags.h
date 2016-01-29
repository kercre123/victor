/**
 * File: behaviorGroupFlags
 *
 * Author: Mark Wesley
 * Created: 01/21/16
 *
 * Description: Behavior groups stored as bit flags - a behavior can belong to many groups
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorGroupFlags_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorGroupFlags_H__


#include "clad/types/behaviorGroup.h"
#include "util/bitFlags/bitFlags.h"
#include <stdint.h>


namespace Anki {
namespace Cozmo {
  

using BehaviorGroupFlags = Util::BitFlags<uint32_t, BehaviorGroup>;


} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorGroupFlags_H__
