/**
* File: recognitionSetupData.h
*
* Author: Lee Crippen
* Created: 07/11/17
*
* Description: Simple struct containing setup data for a speech recognition context
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_RecognitionSetupData_H_
#define __Cozmo_Basestation_VoiceCommands_RecognitionSetupData_H_

#include <memory>
#include <vector>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
class PhraseData;
using PhraseDataSharedPtr = std::shared_ptr<PhraseData>;

class ContextConfig;
using ContextConfigSharedPtr = std::shared_ptr<ContextConfig>;

class RecognitionSetupData
{
public:
  std::vector<PhraseDataSharedPtr>  _phraseList;
  bool                              _isPhraseSpotted = false;
  bool                              _allowsFollowup = false;
  ContextConfigSharedPtr            _contextConfig;
}; // class RecognitionSetupData


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_RecognitionSetupData_H_
