/**
 * File: moodEventTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/09/15
 *
 * Description: Helper functions for dealing with CLAD generated moodEventTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_MoodEventTypesHelpers_H__
#define __Cozmo_Basestation_MoodSystem_MoodEventTypesHelpers_H__


#include "clad/types/moodEventTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {


DECLARE_ENUM_INCREMENT_OPERATORS(MoodEventType);

MoodEventType MoodEventTypeFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_MoodEventTypesHelpers_H__

