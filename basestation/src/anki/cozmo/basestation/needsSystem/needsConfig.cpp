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
#include "anki/common/basestation/jsonTools.h"
#include "util/enums/stringToEnumMapper.hpp"
#include <assert.h>


namespace Anki {
namespace Cozmo {

static const std::string kMinNeedLevelKey = "MinimumNeedLevel";
static const std::string kMaxNeedLevelKey = "MaximumNeedLevel";
static const std::string kDecayPeriodSecondsKey = "DecayPeriodSeconds";

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
: _minNeedLevel(0.0f)
, _maxNeedLevel(1.0f)
, _decayPeriod(60.0f)
, _initialNeedsLevels()
, _needsBrackets()
, _decayConnected()
, _decayUnconnected()
{
}

void NeedsConfig::Init(const Json::Value& json)
{
  _minNeedLevel = JsonTools::ParseFloat(json, kMinNeedLevelKey.c_str(), "Failed to parse min need level");
  _maxNeedLevel = JsonTools::ParseFloat(json, kMaxNeedLevelKey.c_str(), "Failed to parse max need level");
  _decayPeriod = JsonTools::ParseFloat(json, kDecayPeriodSecondsKey.c_str(), "Failed to parse decay frequency");

  _initialNeedsLevels.clear();
  const auto& jsonInitialNeedsLevels = json[kInitialNeedsLevelsArrayKey];
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const auto value = JsonTools::ParseFloat(jsonInitialNeedsLevels,
                                   EnumToString(static_cast<NeedId>(i)),
                                   "Failed to parse an initial need level");
    _initialNeedsLevels[static_cast<NeedId>(i)] = value;
  }

  _needsBrackets.clear();
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const auto& jsonForNeedId = json[kBracketLevelsArrayKey][EnumToString(static_cast<NeedId>(i))];
    BracketThresholds thresholds;
    float prevValue = 1.0f;
    for (size_t i = 0; i < (size_t)NeedBracketId::Count; i++)
    {
      const auto value = JsonTools::ParseFloat(jsonForNeedId,
                                               EnumToString(static_cast<NeedBracketId>(i)),
                                               "Failed to parse an initial need level");
      DEV_ASSERT_MSG(value <= prevValue,
                     "NeedsConfig.Init",
                     "Bracket level thresholds not in descending order (%f vs %f)", value, prevValue);
      prevValue = value;
      thresholds.push_back(value);
    }
    DEV_ASSERT_MSG((thresholds.back() == 0.0f),
                   "NeedsConfig.Init",
                   "Last threshold in bracket list should be zero but is %f", thresholds.back());
    _needsBrackets[static_cast<NeedId>(i)] = std::move(thresholds);
  }

  InitDecay(json, kDecayConnectedKey, _decayConnected);
  InitDecay(json, kDecayUnconnectedKey, _decayUnconnected);
}

struct SortDecayRatesByThresholdDescending
{
  bool operator()(const DecayRate& a, const DecayRate& b) const
  {
    return (a._threshold >= b._threshold);
  }
};

struct SortDecayModifiersByThresholdDescending
{
  bool operator()(const DecayModifier& a, const DecayModifier& b) const
  {
    return (a._threshold >= b._threshold);
  }
};


void NeedsConfig::InitDecay(const Json::Value& json, const std::string& decayKey, DecayConfig& decayInfo)
{
  DecayRatesByNeed decayRatesByNeed;
  DecayModifiersByNeed decayModifiersByNeed;

  for (size_t needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const auto& jsonForNeed = json[decayKey][EnumToString(static_cast<NeedId>(needIndex))];

    DecayRates decayRates;
    for (const auto& item : jsonForNeed[kDecayRatesKey])
    {
      const auto threshold = JsonTools::ParseFloat(item, kThresholdKey.c_str(),
                                                   "Failed to parse a decay threshold");
      const auto rate = JsonTools::ParseFloat(item, kDecayPerMinuteKey.c_str(),
                                              "Failed to parse a decay rate");
      DEV_ASSERT_MSG(rate >= 0.0f,
                     "NeedsConfig.InitDecay",
                     "A decay rate needs to be positive but is %f", rate);
      decayRates.push_back(DecayRate(threshold, rate));
    }
    // Sort the decay rates by descending threshold
    std::sort(decayRates.begin(), decayRates.end(), SortDecayRatesByThresholdDescending());

    decayRatesByNeed.push_back(decayRates);

    DecayModifiers decayModifiers;
    for (const auto& item : jsonForNeed[kDecayModifiersKey])
    {
      const auto threshold = JsonTools::ParseFloat(item, kThresholdKey.c_str(),
                                                   "Failed to parse a modifier threshold");

      OtherNeedModifiers otherNeedModifiers;
      for (const auto& otherNeedItem : item[kOtherNeedsAffectedKey])
      {
        const auto otherNeedStr = JsonTools::ParseString(otherNeedItem, kOtherNeedIDKey.c_str(),
                                                         "Failed to parse a modifier 'other need id'");
        const NeedId otherNeedID = NeedIdFromString(otherNeedStr.c_str());
        const auto multiplier = JsonTools::ParseFloat(otherNeedItem, kMultiplierKey.c_str(),
                                                      "Failed to parse a modifier multiplier");
        otherNeedModifiers.push_back(OtherNeedModifier(otherNeedID, multiplier));
      }
      decayModifiers.push_back(DecayModifier(threshold, otherNeedModifiers));
    }
    // Sort the decay modifiers by descending threshold
    std::sort(decayModifiers.begin(), decayModifiers.end(), SortDecayModifiersByThresholdDescending());

    decayModifiersByNeed.push_back(decayModifiers);
  }

  decayInfo._decayRatesByNeed = std::move(decayRatesByNeed);
  decayInfo._decayModifiersByNeed = std::move(decayModifiersByNeed);
}

} // namespace Cozmo
} // namespace Anki

