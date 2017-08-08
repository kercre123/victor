/**
* File: languagePhraseData.h
*
* Author: Lee Crippen
* Created: 05/26/17
*
* Description: Class for loading and containing voice command phrase language data.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_LanguagePhraseData_H_
#define __Cozmo_Basestation_VoiceCommands_LanguagePhraseData_H_

#include "util/environment/locale.h"
#include "clad/types/voiceCommandTypes.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace Json
{
  class Value;
}

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
class PhraseData;
using PhraseDataSharedPtr = std::shared_ptr<PhraseData>;

class ContextConfig;
using ContextConfigSharedPtr = std::shared_ptr<ContextConfig>;

// Simple struct for returning filename data for a language
struct LanguageFilenames
{
  std::string _languageDataDir = "";
  std::string _netfileFilename = "";
  std::string _ltsFilename = "";
};

class LanguagePhraseData {
public:
  
  bool Init(const Json::Value& dataObject);
  bool AddPhraseCommandMap(const Json::Value& dataObject);
  
  std::vector<PhraseDataSharedPtr> GetPhraseDataList(const std::set<VoiceCommandType>& typeSet) const;
  std::vector<PhraseDataSharedPtr> GetPhraseDataList(VoiceCommandType commandType) const;
  PhraseDataSharedPtr GetDataForPhrase(const std::string& phrase) const;
  const char* GetFirstPhraseForCommand(VoiceCommandType commandType) const;
  
  using LanguageType = Anki::Util::Locale::Language;
  using CountryType = Anki::Util::Locale::CountryISO2;
  const LanguageType& GetLanguageType() const { return _languageType; }
  
  const LanguageFilenames& GetLanguageFilenames(CountryType countryType) const;
  const std::map<VoiceCommandListenContext, ContextConfigSharedPtr>& GetContextConfigs() const { return _contextConfigMap; }
  
private:
  // A couple maps for holding the data of the phrase and associated command
  std::map<std::string, PhraseDataSharedPtr>                    _phraseToDataMap;
  std::map<VoiceCommandType, std::vector<PhraseDataSharedPtr>>  _commandToPhraseDataMap;
  
  LanguageType                                                  _languageType = LanguageType::en;
  LanguageFilenames                                             _languageFilenames{};
  std::map<CountryType, LanguageFilenames>                      _languageFilenamesAltMap;
  
  std::map<VoiceCommandListenContext, ContextConfigSharedPtr>   _contextConfigMap;
  
  static bool LoadLanguageFilenames(const Json::Value& dataObject, LanguageFilenames& out_data);
  
  bool AddContextConfig(const Json::Value& dataObject);
};


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_LanguagePhraseData_H_
