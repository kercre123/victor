/**
* File: contextConfig.h
*
* Author: Lee Crippen
* Created: 07/07/17
*
* Description: Class for containing context configuration data.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_ContextConfig_H_
#define __Cozmo_Basestation_VoiceCommands_ContextConfig_H_

#include "clad/types/voiceCommandTypes.h"
#include "util/bitFlags/bitFlags.h"

#include <string>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

class ContextConfig
{
public:
  void SetSearchBeam(float newValue);
  void SetSearchNOTA(float newValue);
  void SetRecogMinDuration(float newValue);
  
  float GetSearchBeam() const;
  float GetSearchNOTA() const;
  float GetRecogMinDuration() const;
  
  enum class DataValueType {
    RecogMinDuration,
    SearchBeam,
    SearchNOTA
  };
  
  bool HasDataValueSet(DataValueType testType) const;
  
  ContextConfig& Combine(const ContextConfig& otherConfig);
  
private:
  float                     _recogMinDuration = 0.f;
  float                     _searchBeam = 0.f;
  float                     _searchNOTA = 0.f;
  
  Util::BitFlags32<DataValueType> _dataTypesSet;

}; // class ContextConfig


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_ContextConfig_H_
