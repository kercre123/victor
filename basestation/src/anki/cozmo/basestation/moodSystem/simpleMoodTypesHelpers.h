/**
 * File: simpleMoodTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/06/15
 *
 * Description: Helper functions for dealing with CLAD generated simpleMoodTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_SimpleMoodTypesHelpers_H__
#define __Cozmo_Basestation_MoodSystem_SimpleMoodTypesHelpers_H__


#include "clad/types/simpleMoodTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
  namespace Cozmo {
    
    
    DECLARE_ENUM_INCREMENT_OPERATORS(SimpleMoodType);
    
    SimpleMoodType SimpleMoodTypeFromString(const char* inString);
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_SimpleMoodTypesHelpers_H__

