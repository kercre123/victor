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

#include "util/environment/locale.h"
#include "clad/types/voiceCommandTypes.h"

#include <map>
#include <memory>
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
  
class LanguagePhraseData;
struct LanguageFilenames;

class PhraseData;
using PhraseDataSharedPtr = std::shared_ptr<PhraseData>;
  
class CommandPhraseData {
public:
  CommandPhraseData();
  ~CommandPhraseData();
  
  bool Init(const Json::Value& commandPhraseData);
  
  using LanguageType = Anki::Util::Locale::Language;
  using CountryType = Anki::Util::Locale::CountryISO2;
  
  std::vector<PhraseDataSharedPtr> GetPhraseDataList(LanguageType languageType, VoiceCommandListenContext context) const;
  
  PhraseDataSharedPtr GetDataForPhrase(LanguageType languageType, const std::string& phrase) const;
  const char* GetFirstPhraseForCommand(LanguageType languageType, VoiceCommandType commandType) const;
  
  // Simple struct for holding the data of a context and associated list of commands
  struct ContextData
  {
    bool                        _isPhraseSpotted = false;
    bool                        _allowsFollowup = false;
    std::set<VoiceCommandType>  _commandsSet;
  };
  using ContextDataMap = std::map<VoiceCommandListenContext, ContextData>;
  const ContextDataMap& GetContextData() const { return _contextDataMap; }
  
  const LanguageFilenames& GetLanguageFilenames(LanguageType languageType, CountryType countryType) const;
  
private:
  std::map<LanguageType, std::unique_ptr<LanguagePhraseData>> _languagePhraseDataMap;
  std::map<VoiceCommandListenContext, ContextData>            _contextDataMap;
  
  bool AddLanguagePhraseData(const Json::Value& dataObject);
  bool AddContextData(const Json::Value& dataObject);
};


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_CommandPhraseData_H_
