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

#include "engine/voiceCommands/languagePhraseData.h"

#include "engine/voiceCommands/contextConfig.h"
#include "engine/voiceCommands/phraseData.h"
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
  
  const std::string& kContextConfigListKey = "contextConfigList";
  const std::string& kContextTypeKey = "contextType";
  const std::string& kSearchBeamKey = "searchBeam";
  const std::string& kSearchNOTAKey = "searchNOTA";
  const std::string& kRecogMinDurationKey = "recogMinDuration";
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
  
  // Load any context configs
  {
    if (dataObject.isMember(kContextConfigListKey))
    {
      if (!dataObject[kContextConfigListKey].isArray())
      {
        PRINT_NAMED_ERROR("LanguagePhraseData.Init.JsonData",
                          "Context config list is not a list\n%s",
                          Json::StyledWriter().write(dataObject).c_str());
        return false;
      }
      
      const Json::Value& configListObject = dataObject[kContextConfigListKey];
      for (int i=0; i < configListObject.size(); ++i)
      {
        const Json::Value& curItem = configListObject[i];
        AddContextConfig(curItem);
      }
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
  if (VoiceCommandType::Invalid == commandType)
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

bool LanguagePhraseData::AddContextConfig(const Json::Value& dataObject)
{
  if (!dataObject.isObject())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddContextConfig.JsonData",
                      "Context config list item is not an object?\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  if (!dataObject.isMember(kContextTypeKey) || !dataObject[kContextTypeKey].isString())
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddContextConfig.JsonData",
                      "Context config list does not contain context type\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const std::string& contextTypeString = dataObject[kContextTypeKey].asString();
  VoiceCommandListenContext contextType = VoiceCommandListenContextFromString(contextTypeString.c_str());
  if (VoiceCommandListenContext::Invalid == contextType)
  {
    PRINT_NAMED_ERROR("LanguagePhraseData.AddContextConfig.JsonData",
                      "Context config list's context type was not found\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Now that we have the minimum data set up the container object
  auto newContextConfigData = ContextConfigSharedPtr(new ContextConfig);
  
  // Get params, if they've been set
  float tmpFloatParam = 0;
  if (JsonTools::GetValueOptional(dataObject, kSearchBeamKey, tmpFloatParam))
  {
    newContextConfigData->SetSearchBeam(tmpFloatParam);
  }
  if (JsonTools::GetValueOptional(dataObject, kSearchNOTAKey, tmpFloatParam))
  {
    newContextConfigData->SetSearchNOTA(tmpFloatParam);
  }
  if (JsonTools::GetValueOptional(dataObject, kRecogMinDurationKey, tmpFloatParam))
  {
    newContextConfigData->SetRecogMinDuration(tmpFloatParam);
  }
  
  _contextConfigMap[contextType] = newContextConfigData;
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
    auto resultVector = GetPhraseDataList(commandType);
    returnList.insert(returnList.end(),
                      std::make_move_iterator(resultVector.begin()),
                      std::make_move_iterator(resultVector.end()));
  }

  return returnList;
}

std::vector<PhraseDataSharedPtr> LanguagePhraseData::GetPhraseDataList(VoiceCommandType commandType) const
{
  std::vector<PhraseDataSharedPtr> returnList;
  const auto& iter = _commandToPhraseDataMap.find(commandType);
  if (iter != _commandToPhraseDataMap.end() && iter->second.size() > 0)
  {
    returnList.insert(returnList.end(),
                      iter->second.begin(),
                      iter->second.end());
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
