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
#include <set>
#include <string>
#include <vector>

namespace Json
{
  class Value;
}

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
class CommandPhraseData {
public:
  bool Init(const Json::Value& commandPhraseData);
  
  // NOTE: The char* items in the vector should be used immedately, and NOT held onto,
  // as any alterations to the phrase list will invalidate the pointers.
  std::vector<const char*> GetPhraseListRaw() const;
  
  VoiceCommandType GetCommandForPhrase(const std::string& phrase) const;
  const char* GetFirstPhraseForCommand(VoiceCommandType commandType) const;
  
  // Simple struct for holding the data of a context and associated list of commands
  struct ContextData
  {
    bool                        _isPhraseSpotted = false;
    bool                        _allowsFollowup = false;
    std::set<VoiceCommandType>  _commandsSet;
  };
  using ContextDataMap = std::map<VoiceCommandListenContext, ContextData>;
  const ContextDataMap& GetContextData() const { return _contextDataMap; }
  
private:
  
  // A couple maps for holding the data of the phrase and associated command
  std::map<std::string, VoiceCommandType>                 _phraseToTypeMap;
  std::map<VoiceCommandType, std::vector<std::string>>    _commandToPhrasesMap;
  
  std::map<VoiceCommandListenContext, ContextData>    _contextDataMap;
  
  bool AddPhraseCommandMap(const Json::Value& dataObject);
  bool AddContextData(const Json::Value& dataObject);
};


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
#endif // __Cozmo_Basestation_VoiceCommands_CommandPhraseData_H_
