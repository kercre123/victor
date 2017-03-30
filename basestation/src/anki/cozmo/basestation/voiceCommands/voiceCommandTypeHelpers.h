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
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "clad/types/voiceCommandTypes.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

  
DECLARE_ENUM_INCREMENT_OPERATORS(VoiceCommandType);

VoiceCommandType VoiceCommandTypeFromString(const char* inString);


} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
#endif // __Cozmo_Basestation_VoiceCommands_VoiceCommandTypeHelpers_H__
