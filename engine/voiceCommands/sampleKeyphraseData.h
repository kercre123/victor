/**
* File: sampleKeyphraseData.h
*
* Author: Lee Crippen
* Created: 06/21/2017
*
* Description: Loads and holds onto Json data for audio samples.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __Cozmo_Basestation_VoiceCommands_SampleKeyphraseData_H_
#define __Cozmo_Basestation_VoiceCommands_SampleKeyphraseData_H_

#include "clad/types/voiceCommandTypes.h"

#include <map>
#include <string>

namespace Json
{
  class Value;
}

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {
  
using CommandCounts = std::map<VoiceCommand::VoiceCommandType, uint32_t>;
using FileToDataMap = std::map<std::string, CommandCounts>;

class SampleKeyphraseData {
public:
  bool Init(const Json::Value& sampleKeyphraseData);
  
  const CommandCounts& GetCommandCountsForFile(const std::string& filename) const;
  const FileToDataMap& GetFileToDataMap() const { return _fileCommandDataMap; }
  
private:
  FileToDataMap  _fileCommandDataMap;
  
  bool AddFileCommandData(const Json::Value& dataObject);
};
  
} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_VoiceCommands_SampleKeyphraseData_H_
