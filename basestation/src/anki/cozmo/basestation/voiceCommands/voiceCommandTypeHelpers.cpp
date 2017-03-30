/**
* File: voiceCommandTypeHelpers.cpp
*
* Author: Lee Crippen
* Created: 03/17/17
*
* Description: Helper functions for dealing with CLAD generated voice command types.
*
* Copyright: Anki, Inc. 2017
*
**/

#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "anki/cozmo/basestation/voiceCommands/voiceCommandTypeHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(VoiceCommandType);


// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<VoiceCommandType> gStringToVoiceCommandTypeMapper;

VoiceCommandType VoiceCommandTypeFromString(const char* inString)
{
  return gStringToVoiceCommandTypeMapper.GetTypeFromString(inString);
}
  

} // namespace Cozmo
} // namespace Anki
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
