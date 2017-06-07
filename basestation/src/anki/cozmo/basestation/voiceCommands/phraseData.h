/**
* File: phraseData.h
*
* Author: Lee Crippen
* Created: 06/02/17
*
* Description: Class for containing voice command phrase data.
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_VoiceCommands_PhraseData_H_
#define __Cozmo_Basestation_VoiceCommands_PhraseData_H_

#include "clad/types/voiceCommandTypes.h"
#include "util/bitFlags/bitFlags.h"

#include <string>

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

class PhraseData
{
public:
  void SetPhrase(const std::string& newPhrase);
  void SetParamA(int newValue);
  void SetParamB(int newValue);
  void SetParamC(int newValue);
  void SetMinRecogScore(double newValue);
  void SetVoiceCommandType(VoiceCommandType newValue);
  
  const std::string& GetPhrase() const;
  int GetParamA() const;
  int GetParamB() const;
  int GetParamC() const;
  double GetMinRecogScore() const;
  VoiceCommandType GetVoiceCommandType() const;
  
  enum class DataValueType {
    ParamA,
    ParamB,
    ParamC,
    Phrase,
    MinRecogScore,
    VoiceCommandType
  };
  
  bool HasDataValueSet(DataValueType testType) const;
  
private:
  std::string       _phrase = "";
  double            _minRecogScore = 0;
  int               _paramAVal = 0;
  int               _paramBVal = 0;
  int               _paramCVal = 0;
  VoiceCommandType  _voiceCommandType = VoiceCommandType::Count;
  
  Util::BitFlags32<DataValueType> _dataTypesSet;

}; // class PhraseData


} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_VoiceCommands_PhraseData_H_
