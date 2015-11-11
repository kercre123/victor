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


#include "anki/cozmo/basestation/moodSystem/emotionTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"
#include "util/logging/logging.h"
#include <assert.h>
#include <map>


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(EmotionType);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<EmotionType> gStringToEmotionTypeMapper;

EmotionType EmotionTypeFromString(const char* inString)
{
  return gStringToEmotionTypeMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

