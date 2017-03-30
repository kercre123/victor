/**
* File: commandPhraseData.h
*
* Author: Lee Crippen
* Created: 03/17/17
*
* Description: Class for loading and containing voice command phrase config data.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_CommandPhraseData_H_
#define __Cozmo_Basestation_VoiceCommands_CommandPhraseData_H_
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "clad/types/voiceCommandTypes.h"

#include <map>
#include <string>
#include <vector>

namespace Json
{
  class Value;
}

namespace Anki {
namespace Cozmo {
  
class CommandPhraseData {
public:
  bool Init(const Json::Value& commandPhraseData);
  
  // NOTE: The char* items in the vector should be used immedately, and NOT held onto,
  // as any alterations to the phrase list will invalidate the pointers.
  std::vector<const char*> GetPhraseListRaw() const;
  
  const std::vector<VoiceCommandType>& GetCommandsInPhrase(const std::string& phrase) const;
  
private:
  
  // Simple struct for holding the data of the phrase and associate list of commands
  struct PhraseData
  {
    std::string                       _phrase;
    std::vector<VoiceCommandType>     _commandsList;
  };
  
  std::vector<PhraseData>  _phraseDataList;
  
  bool AddPhraseCommandMap(const Json::Value& dataObject);
  
  std::vector<PhraseData>::const_iterator FindPhraseIter(const std::string& phrase) const;
  std::vector<PhraseData>::iterator FindPhraseIter(const std::string& phrase);
};


} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
#endif // __Cozmo_Basestation_VoiceCommands_CommandPhraseData_H_
