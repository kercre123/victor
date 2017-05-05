/**
* File: voiceCommandTypeHelpers.h
*
* Author: Lee Crippen
* Created: 03/17/17
*
* Description: Helper functions for dealing with CLAD generated voice command types.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_VoiceCommandTypeHelpers_H__
#define __Cozmo_Basestation_VoiceCommands_VoiceCommandTypeHelpers_H__

#include "clad/types/voiceCommandTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

  
DECLARE_ENUM_INCREMENT_OPERATORS(VoiceCommandType);

VoiceCommandType VoiceCommandTypeFromString(const char* inString);

  
DECLARE_ENUM_INCREMENT_OPERATORS(VoiceCommandListenContext);

VoiceCommandListenContext VoiceCommandListenContextFromString(const char* inString);


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_VoiceCommandTypeHelpers_H__
