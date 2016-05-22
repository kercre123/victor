/**
 * File: simpleMoodTypesHelpers
 *
 * Author: Trevor Dasch
 * Created: 02/17/16
 *
 * Description: Helper functions for dealing with CLAD generated simpleMoodTypes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "anki/cozmo/basestation/moodSystem/simpleMoodTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
  namespace Cozmo {
    
    
    IMPLEMENT_ENUM_INCREMENT_OPERATORS(SimpleMoodType);
    
    
    // One global instance, created at static initialization on app launch
    static Anki::Util::StringToEnumMapper<SimpleMoodType> gStringToSimpleMoodTypeMapper;
    
    SimpleMoodType SimpleMoodTypeFromString(const char* inString)
    {
      return gStringToSimpleMoodTypeMapper.GetTypeFromString(inString);
    }
    
    
  } // namespace Cozmo
} // namespace Anki

