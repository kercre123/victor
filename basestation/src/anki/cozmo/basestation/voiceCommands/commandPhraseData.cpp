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
#if (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)

#include "anki/cozmo/basestation/voiceCommands/commandPhraseData.h"
#include "anki/cozmo/basestation/voiceCommands/voiceCommandTypeHelpers.h"
#include "util/logging/logging.h"
#include "json/json.h"

#include <iostream>
#include <fstream>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

namespace {
  const std::string& kPhraseListKey = "phraseList";
  const std::string& kPhraseCommandsMapKey = "phraseCommandsMap";
  const std::string& kPhraseCommandsMapPhraseKey = "phrase";
  const std::string& kPhraseCommandsMapCommandsKey = "commands";
  const std::string& kPhraseCommandsMapCommandKey = "commandType";
  
  const std::string& kContextDataMapKey = "contextDataMap";
  const std::string& kContextDataTypeKey = "contextType";
  const std::string& kContextDataIsPhraseSpottedKey = "contextIsPhraseSpotted";
  const std::string& kContextDataAllowsFollowupKey = "contextAllowsFollowup";
  const std::string& kContextDataCommandsKey = "contextCommandList";
}
  
bool CommandPhraseData::Init(const Json::Value& commandPhraseData)
{
  if (!commandPhraseData.isObject())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data is not an object.");
    return false;
  }
  
  // Set up the phrase to command map
  {
    if (!commandPhraseData.isMember(kPhraseCommandsMapKey))
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data does not contain phrase command map!");
      return false;
    }
    
    const Json::Value& phraseMapData = commandPhraseData[kPhraseCommandsMapKey];
    if (!phraseMapData.isArray())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data phrase command map is not a list?");
      return false;
    }
    
    // Make our local copy of the phrase list
    for (int i=0; i < phraseMapData.size(); ++i)
    {
      const Json::Value& curItem = phraseMapData[i];
      AddPhraseCommandMap(curItem);
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

bool CommandPhraseData::AddPhraseCommandMap(const Json::Value& dataObject)
{
  if (!dataObject.isObject())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map item is not an object?\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  if (!dataObject.isMember(kPhraseCommandsMapPhraseKey) || !dataObject[kPhraseCommandsMapPhraseKey].isString())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map does not contain phrase!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const std::string& phraseKey = dataObject[kPhraseCommandsMapPhraseKey].asString();
  
  // We want this to be a unique entry in the map
  auto phraseMapIter = _phraseToTypeMap.find(phraseKey);
  if (phraseMapIter != _phraseToTypeMap.end())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Phrase command map contains phrase already stored once: %s", phraseKey.c_str());
    return false;
  }
  
  if (!dataObject.isMember(kPhraseCommandsMapCommandKey) || !dataObject[kPhraseCommandsMapCommandKey].isString())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map does not contain command type!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const std::string& commandKey = dataObject[kPhraseCommandsMapCommandKey].asString();
  VoiceCommandType commandType = VoiceCommandTypeFromString(commandKey.c_str());
  if (VoiceCommandType::Count == commandType)
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map's command was not found.\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  // There might be multiple phrases that match the command type

  
  _phraseToTypeMap[phraseKey] = commandType;
  _commandToPhrasesMap[commandType].push_back(phraseKey);
  
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

std::vector<const char*> CommandPhraseData::GetPhraseListRaw() const
{
  std::vector<const char*> rawList;
  for (const auto& phraseData : _phraseToTypeMap)
  {
    rawList.push_back(phraseData.first.c_str());
  }
  return rawList;
}
  
VoiceCommandType CommandPhraseData::GetCommandForPhrase(const std::string& phrase) const
{
  const auto& phraseDataIter = _phraseToTypeMap.find(phrase);
  if (phraseDataIter == _phraseToTypeMap.end())
  {
    return VoiceCommandType::Count;
  }
  
  return phraseDataIter->second;
}
  
const char* CommandPhraseData::GetFirstPhraseForCommand(VoiceCommandType commandType) const
{
  const auto& iter = _commandToPhrasesMap.find(commandType);
  if (iter != _commandToPhrasesMap.end() && iter->second.size() > 0)
  {
    return iter->second.front().c_str();
  }
  
  return nullptr;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
