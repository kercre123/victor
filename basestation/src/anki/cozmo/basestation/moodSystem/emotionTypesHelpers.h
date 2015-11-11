/**
 * File: emotionTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/06/15
 *
 * Description: Helper functions for dealing with CLAD generated emotionTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionTypesHelpers_H__
#define __Cozmo_Basestation_MoodSystem_EmotionTypesHelpers_H__


#include "clad/types/emotionTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(EmotionType);

EmotionType EmotionTypeFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionTypesHelpers_H__

