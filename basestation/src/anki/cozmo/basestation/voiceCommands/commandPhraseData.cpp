/**
* File: commandPhraseData.cpp
*
* Author: Lee Crippen
* Created: 03/17/17
*
* Description: Class for loading and containing voice command phrase config data.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/languagePhraseData.h"
#include "util/logging/logging.h"
#include "json/json.h"

#include <iostream>
#include <fstream>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

namespace {
  const std::string& kLanguagePhraseListKey = "languagePhraseList";
  
  const std::string& kContextDataMapKey = "contextDataMap";
  const std::string& kContextDataTypeKey = "contextType";
  const std::string& kContextDataIsPhraseSpottedKey = "contextIsPhraseSpotted";
  const std::string& kContextDataAllowsFollowupKey = "contextAllowsFollowup";
  const std::string& kContextDataCommandsKey = "contextCommandList";
}

// unique_ptr of a forward-declared class type demands this sacrifice
CommandPhraseData::CommandPhraseData() = default;
CommandPhraseData::~CommandPhraseData() = default;

bool CommandPhraseData::Init(const Json::Value& commandPhraseData)
{
  if (!commandPhraseData.isObject())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data is not an object.");
    return false;
  }
  
  // Set up the phrase to command maps for each language
  {
    if (!commandPhraseData.isMember(kLanguagePhraseListKey))
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data does not contain language phrase data list.");
      return false;
    }
    
    const Json::Value& languageDataList = commandPhraseData[kLanguagePhraseListKey];
    if (!languageDataList.isArray())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data language data is not a list?");
      return false;
    }
    
    for (int i=0; i < languageDataList.size(); ++i)
    {
      auto languageData = std::make_unique<LanguagePhraseData>();
      if (languageData->Init(languageDataList[i]))
      {
        auto languageType = languageData->GetLanguageType();
        
        // If no data entry yet exists for this language, add in this one
        if (_languagePhraseDataMap.find(languageType) == _languagePhraseDataMap.end())
        {
          _languagePhraseDataMap[languageType] = std::move(languageData);
        }
        else
        {
          PRINT_NAMED_ERROR("CommandPhraseData.Init.LanguagePhraseDataIgnored",
                            "Secondary language data for language %s is ignored.",
                            Anki::Util::Locale::LanguageToString(languageType).c_str());
        }
      }
    }
  }
  
  // Set up the context data map
  {
    if (!commandPhraseData.isMember(kContextDataMapKey))
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data does not contain context data map!");
      return false;
    }
    
    const Json::Value& contextData = commandPhraseData[kContextDataMapKey];
    if (!contextData.isArray())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data context data map is not a list?");
      return false;
    }
    
    // Parse out the context data
    for (int i=0; i < contextData.size(); ++i)
    {
      const Json::Value& curItem = contextData[i];
      AddContextData(curItem);
    }
  }
  
  return true;
}

bool CommandPhraseData::AddContextData(const Json::Value& dataObject)
{
  if (!dataObject.isObject())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item is not an object?\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Get the context type from the json
  if (!dataObject.isMember(kContextDataTypeKey) || !dataObject[kContextDataTypeKey].isString())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item does not contain context type!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  std::string contextTypeStr = dataObject[kContextDataTypeKey].asString();
  VoiceCommandListenContext context = VoiceCommandListenContextFromString(contextTypeStr.c_str());
  if (VoiceCommandListenContext::Count == context)
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item's context type was not found.\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Verify there hasn't already been an entry added for this type
  if (_contextDataMap.find(context) != _contextDataMap.end())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item's context type already used.\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // Go ahead and create the item in the map now:
  ContextData& contextData = _contextDataMap[context];
  
  // Get the isPhraseSpotted bool value from the json
  if (!dataObject.isMember(kContextDataIsPhraseSpottedKey) || !dataObject[kContextDataIsPhraseSpottedKey].isBool())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item does not contain IsPhraseSpotted value!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  contextData._isPhraseSpotted = dataObject[kContextDataIsPhraseSpottedKey].asBool();
  
  // Get the allowsFollowup bool value from the json
  if (!dataObject.isMember(kContextDataAllowsFollowupKey) || !dataObject[kContextDataAllowsFollowupKey].isBool())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item does not contain allowsFollowup value!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  contextData._allowsFollowup = dataObject[kContextDataAllowsFollowupKey].asBool();
  
  // Get the list of commands in this context
  if (!dataObject.isMember(kContextDataCommandsKey) || !dataObject[kContextDataCommandsKey].isArray())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                      "Context data item does not contain command list!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const Json::Value& commandsListObject = dataObject[kContextDataCommandsKey];
  for (int i=0; i < commandsListObject.size(); ++i)
  {
    const Json::Value& curItem = commandsListObject[i];
    if (!curItem.isString())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                        "Context data item's command list item is not a string.\n%s",
                        Json::StyledWriter().write(curItem).c_str());
      continue;
    }
    
    VoiceCommandType command = VoiceCommandTypeFromString(curItem.asString().c_str());
    if (VoiceCommandType::Count == command)
    {
      PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                        "Context data item's command list item was not found.\n%s",
                        Json::StyledWriter().write(curItem).c_str());
      continue;
    }
    
    // Try inserting the command. If it's not unique, complain
    const auto& insertResult = contextData._commandsSet.insert(std::move(command));
    if (!insertResult.second)
    {
      PRINT_NAMED_ERROR("CommandPhraseData.AddContextData.JsonData",
                        "Context data item's command list item was not unique.\n%s",
                        Json::StyledWriter().write(curItem).c_str());
      continue;
    }
  }
  
  return true;
}

std::vector<PhraseDataSharedPtr> CommandPhraseData::GetPhraseDataList(LanguageType languageType, VoiceCommandListenContext context) const
{
  const auto& iter = _languagePhraseDataMap.find(languageType);
  if (iter == _languagePhraseDataMap.end())
  {
    return std::vector<PhraseDataSharedPtr>{};
  }
  
  const auto& contextDataMap = GetContextData();
  const auto& contextIter = contextDataMap.find(context);
  if (contextIter == contextDataMap.end())
  {
    return std::vector<PhraseDataSharedPtr>{};
  }
  
  return iter->second->GetPhraseDataList(contextIter->second._commandsSet);
}

PhraseDataSharedPtr CommandPhraseData::GetDataForPhrase(LanguageType languageType, const std::string& phrase) const
{
  const auto& iter = _languagePhraseDataMap.find(languageType);
  if (iter != _languagePhraseDataMap.end())
  {
    return iter->second->GetDataForPhrase(phrase);
  }
  
  return PhraseDataSharedPtr{};
}
  
const char* CommandPhraseData::GetFirstPhraseForCommand(LanguageType languageType, VoiceCommandType commandType) const
{
  const auto& iter = _languagePhraseDataMap.find(languageType);
  if (iter != _languagePhraseDataMap.end())
  {
    return iter->second->GetFirstPhraseForCommand(commandType);
  }
  
  return nullptr;
}

const LanguageFilenames& CommandPhraseData::GetLanguageFilenames(LanguageType languageType, CountryType countryType) const
{
  const auto& iter = _languagePhraseDataMap.find(languageType);
  if (iter == _languagePhraseDataMap.end())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.GetLanguageFilenames",
                      "Could not find language phrase data for %s",
                      Anki::Util::Locale::LanguageToString(languageType).c_str());
    
    static LanguageFilenames emptyFilenames{};
    return emptyFilenames;
  }
  
  return iter->second->GetLanguageFilenames(countryType);
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
