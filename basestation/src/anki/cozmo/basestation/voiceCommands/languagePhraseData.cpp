/**
* File: languagePhraseData.cpp
*
* Author: Lee Crippen
* Created: 05/26/17
*
* Description: Class for loading and containing voice command phrase language data.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/voiceCommands/languagePhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "anki/common/basestation/jsonTools.h"
#include "util/logging/logging.h"
#include "util/string/stringUtils.h"
#include "json/json.h"

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

namespace {
  const std::string& kLanguageCodeKey = "languageCode";
  
  const std::string& kLanguageDataDirKey = "languageDataDir";
  const std::string& kNetfileFilenameKey = "netfileFilename";
  const std::string& kLtsFilenameKey = "ltsFilename";
  
  const std::string& kAltCountryDataListKey = "altCountryDataList";
  const std::string& kCountryCodeKey = "countryCode";
  
  const std::string& kPhraseCommandsMapKey = "phraseCommandsMap";
  const std::string& kPhraseCommandsMapPhraseKey = "phrase";
  const std::string& kPhraseCommandsMapCommandKey = "commandType";
  const std::string& kParamAKey = "paramA";
  const std::string& kParamBKey = "paramB";
  const std::string& kParamCKey = "paramC";
  const std::string& kMinRecogScoreKey = "minRecogScore";
}

bool LanguagePhraseData::Init(const Json::Value& dataObject)
{
  // Get the language code
  {
    if (!dataObject.isMember(kLanguageCodeKey))
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData", "Language data does not contain language code");
      return false;
    }
    
    const std::string& languageCodeString = dataObject[kLanguageCodeKey].asString();
    _languageType = Anki::Util::Locale::LanguageFromString(languageCodeString);
  }
  
  // Load the default language filenames for this language
  if (!LoadLanguageFilenames(dataObject, _languageFilenames))
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData", "Language data does not contain expected filenames");
    return false;
  }
  
  // If there are any alternate language data files for particular countries, load those
  if (dataObject.isMember(kAltCountryDataListKey))
  {
    const Json::Value& altCountryDataList = dataObject[kAltCountryDataListKey];
    if (!altCountryDataList.isArray())
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData",
                        "Language %s alternate country data is not a list?",
                        Anki::Util::Locale::LanguageToString(_languageType).c_str());
      return false;
    }
    
    // Make our local copy of the phrase list
    for (int i=0; i < altCountryDataList.size(); ++i)
    {
      const Json::Value& curItem = altCountryDataList[i];
      
      if (!curItem.isMember(kCountryCodeKey))
      {
        PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData", "Alt country data does not contain country code");
        return false;
      }
      
      const std::string& countryCodeString = curItem[kCountryCodeKey].asString();
      CountryType countryType = Anki::Util::Locale::CountryISO2FromString(countryCodeString);
      
      // Try to get the language filenames for this alt country data
      LanguageFilenames altFilenames;
      if (!LoadLanguageFilenames(curItem, altFilenames))
      {
        PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData", "Alt country data does not contain expected filenames");
        return false;
      }
      
      _languageFilenamesAltMap[countryType] = std::move(altFilenames);
    }
  }
  
  // Get the phrase command map data
  {
    if (!dataObject.isMember(kPhraseCommandsMapKey))
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData",
                        "Language %s Keyphrase data does not contain phrase command map",
                        Anki::Util::Locale::LanguageToString(_languageType).c_str());
      return false;
    }
    
    const Json::Value& phraseMapData = dataObject[kPhraseCommandsMapKey];
    if (!phraseMapData.isArray())
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData",
                        "Language %s Keyphrase data phrase command map is not a list?",
                        Anki::Util::Locale::LanguageToString(_languageType).c_str());
      return false;
    }
    
    // Make our local copy of the phrase list
    for (int i=0; i < phraseMapData.size(); ++i)
    {
      const Json::Value& curItem = phraseMapData[i];
      AddPhraseCommandMap(curItem);
    }
  }
  
  return true;
}

bool LanguagePhraseData::AddPhraseCommandMap(const Json::Value& dataObject)
{
  if (!dataObject.isObject())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map item is not an object?\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  if (!dataObject.isMember(kPhraseCommandsMapPhraseKey) || !dataObject[kPhraseCommandsMapPhraseKey].isString())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map does not contain phrase\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Store off the lower case version of phrases for consistency
  const std::string& phraseKey = Util::StringToLowerUTF8(dataObject[kPhraseCommandsMapPhraseKey].asString());
  
  // We want this to be a unique entry in the map
  auto phraseMapIter = _phraseToDataMap.find(phraseKey);
  if (phraseMapIter != _phraseToDataMap.end())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData", "Phrase command map contains phrase already stored once: %s", phraseKey.c_str());
    return false;
  }
  
  if (!dataObject.isMember(kPhraseCommandsMapCommandKey) || !dataObject[kPhraseCommandsMapCommandKey].isString())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map does not contain command type\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const std::string& commandKey = dataObject[kPhraseCommandsMapCommandKey].asString();
  VoiceCommandType commandType = VoiceCommandTypeFromString(commandKey.c_str());
  if (VoiceCommandType::Count == commandType)
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map's command was not found\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Now that we have the minimum data (a phrase and a command type) set up the container object
  auto newPhraseData = PhraseDataSharedPtr(new PhraseData);
  newPhraseData->SetPhrase(phraseKey);
  newPhraseData->SetVoiceCommandType(commandType);
  
  // Get the three int params, if they've been set
  int tmpIntParam = 0;
  if (JsonTools::GetValueOptional(dataObject, kParamAKey, tmpIntParam))
  {
    newPhraseData->SetParamA(tmpIntParam);
  }
  if (JsonTools::GetValueOptional(dataObject, kParamBKey, tmpIntParam))
  {
    newPhraseData->SetParamB(tmpIntParam);
  }
  if (JsonTools::GetValueOptional(dataObject, kParamCKey, tmpIntParam))
  {
    newPhraseData->SetParamC(tmpIntParam);
  }
  
  // Try getting the min score param
  double tmpDoubleParam = 0;
  if (JsonTools::GetValueOptional(dataObject, kMinRecogScoreKey, tmpDoubleParam))
  {
    newPhraseData->SetMinRecogScore(tmpDoubleParam);
  }
  
  _phraseToDataMap[phraseKey] = newPhraseData;
  // There might be multiple phrases that match the command type
  _commandToPhraseDataMap[commandType].push_back(newPhraseData);
  
  return true;
}

PhraseDataSharedPtr LanguagePhraseData::GetDataForPhrase(const std::string& phrase) const
{
  const auto& phraseDataIter = _phraseToDataMap.find(phrase);
  if (phraseDataIter == _phraseToDataMap.end())
  {
    return PhraseDataSharedPtr{};
  }
  
  return phraseDataIter->second;
}
  
std::vector<PhraseDataSharedPtr> LanguagePhraseData::GetPhraseDataList(const std::set<VoiceCommandType>& typeSet) const
{
  std::vector<PhraseDataSharedPtr> returnList;
  for (const auto& commandType : typeSet)
  {
    const auto& iter = _commandToPhraseDataMap.find(commandType);
    if (iter != _commandToPhraseDataMap.end() && iter->second.size() > 0)
    {
      for (const auto& listItem : iter->second)
      {
        returnList.push_back(listItem);
      }
    }
  }

  return returnList;
}

const char* LanguagePhraseData::GetFirstPhraseForCommand(VoiceCommandType commandType) const
{
  const auto& iter = _commandToPhraseDataMap.find(commandType);
  if (iter != _commandToPhraseDataMap.end() && iter->second.size() > 0)
  {
    return iter->second.front()->GetPhrase().c_str();
  }
  
  return nullptr;
}

const LanguageFilenames& LanguagePhraseData::GetLanguageFilenames(CountryType countryType) const
{
  // First see whether there's an entry in the alternates data for this country
  const auto& iter = _languageFilenamesAltMap.find(countryType);
  if (iter != _languageFilenamesAltMap.end())
  {
    return iter->second;
  }
  
  // Otherwise return the default
  return _languageFilenames;
}

bool LanguagePhraseData::LoadLanguageFilenames(const Json::Value& dataObject, LanguageFilenames& out_data)
{
  LanguageFilenames filenameData;
  // Get the language data dir
  {
    if (!dataObject.isMember(kLanguageDataDirKey))
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.LoadLanguageFilenames.JsonData",
                        "Data key %s was not found in Json object:\n%s",
                        kLanguageDataDirKey.c_str(),
                        Json::StyledWriter().write(dataObject).c_str());
      return false;
    }
    filenameData._languageDataDir = dataObject[kLanguageDataDirKey].asString();
  }
  
  // Get the netfile filename
  {
    if (!dataObject.isMember(kNetfileFilenameKey))
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.LoadLanguageFilenames.JsonData",
                        "Data key %s was not found in Json object:\n%s",
                        kNetfileFilenameKey.c_str(),
                        Json::StyledWriter().write(dataObject).c_str());
      return false;
    }
    filenameData._netfileFilename = dataObject[kNetfileFilenameKey].asString();
  }
  
  // Get the lts filename
  {
    if (!dataObject.isMember(kLtsFilenameKey))
    {
      PRINT_NAMED_ERROR("LanguagePhraseData.LoadLanguageFilenames.JsonData",
                        "Data key %s was not found in Json object:\n%s",
                        kLtsFilenameKey.c_str(),
                        Json::StyledWriter().write(dataObject).c_str());
      return false;
    }
    filenameData._ltsFilename = dataObject[kLtsFilenameKey].asString();
  }
  
  out_data = std::move(filenameData);
  
  return true;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
