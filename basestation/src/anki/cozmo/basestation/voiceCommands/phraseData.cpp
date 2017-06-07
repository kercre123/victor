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

#include "anki/cozmo/basestation/voiceCommands/phraseData.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

void PhraseData::SetPhrase(const std::string& newPhrase)
{
  _phrase = std::move(newPhrase);
  _dataTypesSet.SetBitFlag(DataValueType::Phrase, true);
}

void PhraseData::SetParamA(int newValue)
{
  _paramAVal = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::ParamA, true);
}

void PhraseData::SetParamB(int newValue)
{
  _paramBVal = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::ParamB, true);
}

void PhraseData::SetParamC(int newValue)
{
  _paramCVal = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::ParamC, true);
}

void PhraseData::SetMinRecogScore(double newValue)
{
  _minRecogScore = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::MinRecogScore, true);
}

void PhraseData::SetVoiceCommandType(VoiceCommandType newValue)
{
  _voiceCommandType = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::VoiceCommandType, true);
}

bool PhraseData::HasDataValueSet(DataValueType testType) const
{
  return _dataTypesSet.IsBitFlagSet(testType);
}

const std::string& PhraseData::GetPhrase() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::Phrase), "PhraseData.GetPhrase.IsSet");
  return _phrase;
}

int PhraseData::GetParamA() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::ParamA), "PhraseData.GetParamA.IsSet");
  return _paramAVal;
}

int PhraseData::GetParamB() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::ParamB), "PhraseData.GetParamB.IsSet");
  return _paramBVal;
}

int PhraseData::GetParamC() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::ParamC), "PhraseData.GetParamC.IsSet");
  return _paramCVal;
}

double PhraseData::GetMinRecogScore() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::MinRecogScore), "PhraseData.GetMinRecogScore.IsSet");
  return _minRecogScore;
}

VoiceCommandType PhraseData::GetVoiceCommandType() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::VoiceCommandType), "PhraseData.GetVoiceCommandType.IsSet");
  return _voiceCommandType;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
