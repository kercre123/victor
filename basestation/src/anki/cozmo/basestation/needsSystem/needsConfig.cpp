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
#include "clad/types/needsSystemTypes.h"
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

static const std::string kInitialUnlockLevelsArrayKey = "UnlockLevels";

static const std::string kActionDeltasKey = "ActionDeltas";
static const std::string kActionIDKey = "ActionID";
static const std::string kDeltasKey = "Deltas";
static const std::string kNeedIDKey = "NeedID";
static const std::string kDeltaKey = "Delta";
static const std::string kRandomRangeKey = "RandomRange";


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

void StarRewardsConfig::Init(const Json::Value& json)
{
  _UnlockLevels.clear();
  const auto& jsonInitialUnlockLevels = json[kInitialUnlockLevelsArrayKey];
  if( jsonInitialUnlockLevels.isArray())
  {
    const s32 numEntries = jsonInitialUnlockLevels.size();
    _UnlockLevels.reserve(numEntries);
    
    for(int i = 0; i < numEntries; ++i)
    {
      const Json::Value& jsonEntry = jsonInitialUnlockLevels[i];
      UnlockLevel level;
      level.SetFromJSON(jsonEntry);
      _UnlockLevels.emplace_back(level);
    }
  }
}
  
int StarRewardsConfig::GetMaxStarsForLevel(int level)
{
  if( level < _UnlockLevels.size() )
  {
    return _UnlockLevels[level].numStarsToUnlock;
  }
  // return the last one repeatedly if player has hit max level.
  // But the stars keep going up.
  if( _UnlockLevels.size() >= 1 )
  {
    unsigned int maxRewardedLevel = (unsigned int)(_UnlockLevels.size() - 1);
    return _UnlockLevels[maxRewardedLevel].numStarsToUnlock;
  }
  return 0;
}
  
void StarRewardsConfig::GetRewardsForLevel(int level, std::vector<NeedsReward>& rewards)
{
  if( level < _UnlockLevels.size() )
  {
    rewards = _UnlockLevels[level].rewards;
  }
  else if( _UnlockLevels.size() > 0 )
  {
    rewards = _UnlockLevels[_UnlockLevels.size() - 1].rewards;
  }
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


ActionsConfig::ActionsConfig()
: _actionDeltas()
{
}

void ActionsConfig::Init(const Json::Value& json)
{
  _actionDeltas.clear();
  _actionDeltas.resize(static_cast<size_t>(NeedsActionId::Count));

  const auto& jsonActionDeltas = json[kActionDeltasKey];

  for (const auto& item : jsonActionDeltas)
  {
    const auto& actionIdStr = JsonTools::ParseString(item, kActionIDKey.c_str(),
                                                     "Failed to parse an action ID");
    const NeedsActionId actionId = NeedsActionIdFromString(actionIdStr.c_str());
    ActionDelta& actionDelta = _actionDeltas[static_cast<int>(actionId)];

    const auto& jsonDeltas = item[kDeltasKey];
    for (const auto& deltaItem : jsonDeltas)
    {
      const auto& needIdStr = JsonTools::ParseString(deltaItem, kNeedIDKey.c_str(),
                                                     "Failed to parse a need ID");
      const auto& needId = NeedIdFromString(needIdStr.c_str());
      const int needIdIndex = static_cast<int>(needId);

      const float deltaValue = JsonTools::ParseFloat(deltaItem, kDeltaKey.c_str(),
                                                     "Failed to parse a delta");
      const float randomRangeValue = JsonTools::ParseFloat(deltaItem, kRandomRangeKey.c_str(),
                                                           "Failed to parse a random range");
      actionDelta._needDeltas[needIdIndex]._delta = deltaValue;
      actionDelta._needDeltas[needIdIndex]._randomRange = randomRangeValue;
    }
  }
}


} // namespace Cozmo
} // namespace Anki

