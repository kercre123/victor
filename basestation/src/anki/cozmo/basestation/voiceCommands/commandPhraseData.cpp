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
}
  
bool CommandPhraseData::Init(const Json::Value& commandPhraseData)
{
  // Get the phrase list
  {
    if (!commandPhraseData.isObject())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data is not an object!");
      return false;
    }
    
    if (!commandPhraseData.isMember(kPhraseListKey))
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data does not contain phrase list!");
      return false;
    }
    
    const Json::Value& phraseListData = commandPhraseData[kPhraseListKey];
    if (!phraseListData.isArray())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Keyphrase data phrase list is not a list?");
      return false;
    }
    
    // Make our local copy of the phrase list
    _phraseDataList.clear();
    for (int i=0; i < phraseListData.size(); ++i)
    {
      const Json::Value& curItem = phraseListData[i];
      if (!curItem.isString())
      {
        PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData",
                          "Keyphrase data phrase list item index %d is not a string?\n%s",
                          i, Json::StyledWriter().write(curItem).c_str());
        continue;
      }
      _phraseDataList.push_back( {._phrase = curItem.asString()} );
    }
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
  
  std::string phraseKey = dataObject[kPhraseCommandsMapPhraseKey].asString();
  auto phraseIter = FindPhraseIter(phraseKey);
  if (phraseIter == _phraseDataList.end())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.Init.JsonData", "Phrase command map contains phrase not found in list: %s", phraseKey.c_str());
    return false;
  }
  
  if (!dataObject.isMember(kPhraseCommandsMapCommandsKey) || !dataObject[kPhraseCommandsMapCommandsKey].isArray())
  {
    PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                      "Phrase command map does not contain command list!\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const Json::Value& commandsListObject = dataObject[kPhraseCommandsMapCommandsKey];
  for (int i=0; i < commandsListObject.size(); ++i)
  {
    const Json::Value& curItem = commandsListObject[i];
    if (!curItem.isString())
    {
      PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                        "Phrase command map's command list item is not a string.\n%s",
                        Json::StyledWriter().write(curItem).c_str());
      continue;
    }
    
    VoiceCommandType command = VoiceCommandTypeFromString(curItem.asString().c_str());
    if (VoiceCommandType::Count == command)
    {
      PRINT_NAMED_ERROR("CommandPhraseData.AddPhraseCommandMap.JsonData",
                        "Phrase command map's command list item was not found.\n%s",
                        Json::StyledWriter().write(curItem).c_str());
      continue;
    }
    
    phraseIter->_commandsList.push_back(std::move(command));
  }
  
  return true;
}
  
std::vector<CommandPhraseData::PhraseData>::const_iterator CommandPhraseData::FindPhraseIter(const std::string& phrase) const
{
  auto phraseIter = _phraseDataList.begin();
  while (phraseIter != _phraseDataList.end())
  {
    if (phraseIter->_phrase == phrase)
    {
      return phraseIter;
    }
    ++phraseIter;
  }
  
  return _phraseDataList.end();
}

std::vector<CommandPhraseData::PhraseData>::iterator CommandPhraseData::FindPhraseIter(const std::string& phrase)
{
  auto phraseIter = _phraseDataList.begin();
  while (phraseIter != _phraseDataList.end())
  {
    if (phraseIter->_phrase == phrase)
    {
      return phraseIter;
    }
    ++phraseIter;
  }
  
  return _phraseDataList.end();
}

std::vector<const char*> CommandPhraseData::GetPhraseListRaw() const
{
  std::vector<const char*> rawList;
  for (const auto& phraseData : _phraseDataList)
  {
    rawList.push_back(phraseData._phrase.c_str());
  }
  return rawList;
}
  
const std::vector<VoiceCommandType>& CommandPhraseData::GetCommandsInPhrase(const std::string& phrase) const
{
  static std::vector<VoiceCommandType> emptyList;
  
  const auto phraseIter = FindPhraseIter(phrase);
  if (_phraseDataList.end() == phraseIter)
  {
    return emptyList;
  }
  
  return phraseIter->_commandsList;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
#endif // (VOICE_RECOG_PROVIDER != VOICE_RECOG_NONE)
