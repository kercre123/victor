/**
 * File: progressionStatTypesHelpers
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Helper functions for dealing with CLAD generated progressionStatTypes
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "anki/cozmo/basestation/progressionSystem/progressionStatTypesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(ProgressionStatType);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<ProgressionStatType> gStringToProgressionStatTypeMapper;

ProgressionStatType ProgressionStatTypeFromString(const char* inString)
{
  return gStringToProgressionStatTypeMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

