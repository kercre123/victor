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


#include "anki/cozmo/basestation/moodSystem/moodEventTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"
#include "util/logging/logging.h"
#include <assert.h>
#include <map>


namespace Anki {
namespace Cozmo {


IMPLEMENT_ENUM_INCREMENT_OPERATORS(MoodEventType);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<MoodEventType> gStringToMoodEventTypeMapper;

MoodEventType MoodEventTypeFromString(const char* inString)
{
  return gStringToMoodEventTypeMapper.GetTypeFromString(inString);
}


} // namespace Cozmo
} // namespace Anki

