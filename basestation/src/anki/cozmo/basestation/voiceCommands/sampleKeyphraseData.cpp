/**
* File: sampleKeyphraseData.cpp
*
* Author: Lee Crippen
* Created: 06/21/2017
*
* Description: Loads and holds onto Json data for audio samples.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "anki/cozmo/basestation/voiceCommands/sampleKeyphraseData.h"
#include "util/logging/logging.h"
#include "json/json.h"

namespace {
  const std::string kFilenameKey = "sampleFilename";
  const std::string kCommandCountListKey = "commandCountList";
  const std::string kCommandKey = "command";
  const std::string kCommandCountKey = "commandCount";
}

namespace Anki {
namespace Cozmo {

bool SampleKeyphraseData::Init(const Json::Value& sampleKeyphraseData)
{
  if (!sampleKeyphraseData.isArray())
  {
    PRINT_NAMED_ERROR("SampleKeyphraseData.Init",
                      "Sample keyphrase data list is not a list\n%s",
                      Json::StyledWriter().write(sampleKeyphraseData).c_str());
    return false;
  }
  
  _fileCommandDataMap.clear();
  for (int i=0; i < sampleKeyphraseData.size(); ++i)
  {
    const Json::Value& curItem = sampleKeyphraseData[i];
    AddFileCommandData(curItem);
  }
  
  return true;
}

bool SampleKeyphraseData::AddFileCommandData(const Json::Value& dataObject)
{
  if (!dataObject.isObject())
  {
    PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                      "List item is not an object\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  if (!dataObject.isMember(kFilenameKey) || !dataObject[kFilenameKey].isString())
  {
    PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                      "Sample Keyphrase data does not contain filename\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const std::string& fileName = dataObject[kFilenameKey].asString();
  
  // If there's no list of command counts, this is simply a "noise" sample which is OK
  if (!dataObject.isMember(kCommandCountListKey))
  {
    // Make sure there's an entry for this filename, otherwise leave the map of command -> count empty
    _fileCommandDataMap.insert(std::make_pair(fileName, CommandCounts{}));
    return true;
  }
  
  if (!dataObject[kCommandCountListKey].isArray())
  {
    PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                      "Sample Keyphrase data's data list is not an array\n%s",
                      Json::StyledWriter().write(dataObject).c_str());
    return false;
  }
  
  const Json::Value& commandsListObject = dataObject[kCommandCountListKey];
  for (int i=0; i < commandsListObject.size(); ++i)
  {
    const Json::Value& curItem = commandsListObject[i];
    if (!curItem.isObject())
    {
      PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                        "Sample Keyphrase data command data list item %d is not an object\n%s",
                        i, Json::StyledWriter().write(dataObject).c_str());
      continue;
    }
    
    // Pull out the command type
    if (!curItem.isMember(kCommandKey) || !curItem[kCommandKey].isString())
    {
      PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                        "Sample Keyphrase data command data list item %d's command is not a string\n%s",
                        i, Json::StyledWriter().write(dataObject).c_str());
      continue;
    }
    
    const std::string& commandString = curItem[kCommandKey].asString();
    VoiceCommand::VoiceCommandType command = VoiceCommand::VoiceCommandTypeFromString(commandString);
    
    // Pull out the number of this type
    if (!curItem.isMember(kCommandCountKey) || !curItem[kCommandCountKey].isUInt())
    {
      PRINT_NAMED_ERROR("SampleKeyphraseData.AddFileCommandData",
                        "Sample Keyphrase data command data list item %d command count is not a UInt\n%s",
                        i, Json::StyledWriter().write(dataObject).c_str());
      continue;
    }
    
    auto commandCount = curItem[kCommandCountKey].asUInt();
    _fileCommandDataMap[fileName][command] = commandCount;
  }
  
  return true;
}
  
const CommandCounts& SampleKeyphraseData::GetCommandCountsForFile(const std::string& filename) const
{
  const auto& dataIter = _fileCommandDataMap.find(filename);
  if (_fileCommandDataMap.end() != dataIter)
  {
    return dataIter->second;
  }
  
  static const CommandCounts emptyResult;
  return emptyResult;
}

} // namespace Cozmo
} // namespace Anki
