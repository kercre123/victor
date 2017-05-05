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


#include "anki/cozmo/basestation/voiceCommands/voiceCommandTypeHelpers.h"
#include "util/enums/stringToEnumMapper.hpp"


namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

  
IMPLEMENT_ENUM_INCREMENT_OPERATORS(VoiceCommandType);

// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<VoiceCommandType> gStringToVoiceCommandTypeMapper;

VoiceCommandType VoiceCommandTypeFromString(const char* inString)
{
  return gStringToVoiceCommandTypeMapper.GetTypeFromString(inString);
}


IMPLEMENT_ENUM_INCREMENT_OPERATORS(VoiceCommandListenContext);

// One global instance, created at static initialization on app launch
static Anki::Util::StringToEnumMapper<VoiceCommandListenContext> gStringToVoiceCommandListenContextMapper;

VoiceCommandListenContext VoiceCommandListenContextFromString(const char* inString)
{
  return gStringToVoiceCommandListenContextMapper.GetTypeFromString(inString);
}
  
} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
