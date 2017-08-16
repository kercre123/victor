/**
* File: contextConfig.cpp
*
* Author: Lee Crippen
* Created: 07/07/17
*
* Description: Class for containing context configuration data.
*
* Copyright: Anki, Inc. 2017
*
**/

#include "engine/voiceCommands/contextConfig.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace VoiceCommand {

void ContextConfig::SetSearchBeam(float newValue)
{
  _searchBeam = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::SearchBeam, true);
}

void ContextConfig::SetSearchNOTA(float newValue)
{
  _searchNOTA = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::SearchNOTA, true);
}

void ContextConfig::SetRecogMinDuration(float newValue)
{
  _recogMinDuration = newValue;
  _dataTypesSet.SetBitFlag(DataValueType::RecogMinDuration, true);
}

bool ContextConfig::HasDataValueSet(DataValueType testType) const
{
  return _dataTypesSet.IsBitFlagSet(testType);
}

float ContextConfig::GetSearchBeam() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::SearchBeam), "ContextConfig.GetSearchBeam.IsSet");
  return _searchBeam;
}

float ContextConfig::GetSearchNOTA() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::SearchNOTA), "ContextConfig.GetSearchNOTA.IsSet");
  return _searchNOTA;
}

float ContextConfig::GetRecogMinDuration() const
{
  DEV_ASSERT(HasDataValueSet(DataValueType::RecogMinDuration), "ContextConfig.GetRecogMinDuration.IsSet");
  return _recogMinDuration;
}

ContextConfig& ContextConfig::Combine(const ContextConfig& otherConfig)
{
  if (otherConfig.HasDataValueSet(DataValueType::SearchBeam))
  {
    SetSearchBeam(otherConfig.GetSearchBeam());
  }
  
  if (otherConfig.HasDataValueSet(DataValueType::SearchNOTA))
  {
    SetSearchNOTA(otherConfig.GetSearchNOTA());
  }
  
  if (otherConfig.HasDataValueSet(DataValueType::RecogMinDuration))
  {
    SetRecogMinDuration(otherConfig.GetRecogMinDuration());
  }
  
  return *this;
}

} // namespace VoiceCommand
} // namespace Cozmo
} // namespace Anki
