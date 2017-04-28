/**
 * File: needsConfig
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: Configuration data for the Cozmo Needs system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "util/enums/stringToEnumMapper.hpp"
#include <assert.h>


namespace Anki {
namespace Cozmo {

static const std::string kInitialNeedsLevelsArrayKey = "InitialNeedsLevels";
static const std::string kBracketLevelsArrayKey = "BracketLevels";

static const std::string kDecayConnectedKey = "DecayConnected";
static const std::string kDecayUnconnectedKey = "DecayUnconnected";

static const std::string kDecayRatesKey = "DecayRates";
static const std::string kThresholdKey = "Threshold";
static const std::string kDecayPerMinuteKey = "DecayPerMinute";

static const std::string kDecayModifiersKey = "DecayModifiers";
static const std::string kOtherNeedsAffectedKey = "OtherNeedsAffected";
static const std::string kOtherNeedIDKey = "OtherNeedID";
static const std::string kMultiplierKey = "Multiplier";



IMPLEMENT_ENUM_INCREMENT_OPERATORS(NeedId);

// One global instance, created at static initialization on app launch
static Util::StringToEnumMapperI<NeedId> gStringToNeedIdMapper;

NeedId NeedIdFromString(const char* inString)
{
  return gStringToNeedIdMapper.GetTypeFromString(inString);
}

NeedsConfig::NeedsConfig()
: _initialNeedsLevels()
, _needsBrackets()
{
}

void NeedsConfig::Init(const Json::Value& json)
{
  _initialNeedsLevels.clear();
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    _initialNeedsLevels[static_cast<NeedId>(i)] = json[kInitialNeedsLevelsArrayKey][EnumToString(static_cast<NeedId>(i))].asFloat();
  }

  _needsBrackets.clear();
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const Json::Value& jsonForNeedId = json[kBracketLevelsArrayKey][EnumToString(static_cast<NeedId>(i))];
    BracketThresholds thresholds;
    for (size_t i = 0; i < (size_t)NeedBracketId::Count; i++)
    {
      const float value = jsonForNeedId[EnumToString(static_cast<NeedBracketId>(i))].asFloat();
      thresholds.push_back(value);
    }
    _needsBrackets[static_cast<NeedId>(i)] = std::move(thresholds);
  }

  InitDecay(json, kDecayConnectedKey, _decayConnected);
  InitDecay(json, kDecayUnconnectedKey, _decayUnconnected);
}


void NeedsConfig::InitDecay(const Json::Value& json, const std::string& decayKey, DecayConfig& decayInfo)
{
  for (size_t needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const auto& jsonForNeed = json[decayKey][EnumToString(static_cast<NeedId>(needIndex))];

    DecayRates decayRates;
    for (const auto& item : jsonForNeed[kDecayRatesKey])
    {
      const float threshold = item[kThresholdKey].asFloat();
      const float rate = item[kDecayPerMinuteKey].asFloat();
      decayRates.push_back(DecayRate(threshold, rate));
    }
    decayInfo._decayRates = std::move(decayRates);

    DecayModifers decayModifiers;
    for (const auto& item : jsonForNeed[kDecayModifiersKey])
    {
      const float threshold = item[kThresholdKey].asFloat();

      OtherNeedModifiers otherNeedModifiers;
      for (const auto& otherNeedItem : item[kOtherNeedsAffectedKey])
      {
        const NeedId otherNeedID = NeedIdFromString(otherNeedItem[kOtherNeedIDKey].asString().c_str());
        const float multiplier = otherNeedItem[kMultiplierKey].asFloat();
        otherNeedModifiers.push_back(OtherNeedModifier(otherNeedID, multiplier));
      }
      decayModifiers.push_back(DecayModifier(threshold, otherNeedModifiers));
    }
    decayInfo._decayModifiers = std::move(decayModifiers);
  }
}

} // namespace Cozmo
} // namespace Anki

