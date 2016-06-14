/**
* File: visionModesHelpers
*
* Author: Lee Crippen
* Created: 06/12/16
*
* Description: Helper functions for dealing with CLAD generated visionModes
*
* Copyright: Anki, Inc. 2016
*
**/


#include "anki/cozmo/basestation/visionModesHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(VisionMode);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<VisionMode> gStringToVisionModeMapper;

VisionMode VisionModeFromString(const char* inString)
{
  return gStringToVisionModeMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki

