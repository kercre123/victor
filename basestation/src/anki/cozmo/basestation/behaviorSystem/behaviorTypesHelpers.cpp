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


#include "anki/cozmo/basestation/behaviorSystem/behaviorTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

IMPLEMENT_ENUM_INCREMENT_OPERATORS(BehaviorClass);
IMPLEMENT_ENUM_INCREMENT_OPERATORS(ReactionTrigger);
IMPLEMENT_ENUM_INCREMENT_OPERATORS(ExecutableBehaviorType);

} // namespace Cozmo
} // namespace Anki

