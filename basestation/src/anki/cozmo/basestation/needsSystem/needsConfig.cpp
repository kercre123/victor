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
#include "anki/cozmo/basestation/robot.h"
#include "anki/common/basestation/jsonTools.h"
#include "clad/types/needsSystemTypes.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {

static const std::string kMinNeedLevelKey = "MinimumNeedLevel";
static const std::string kMaxNeedLevelKey = "MaximumNeedLevel";
static const std::string kDecayPeriodSecondsKey = "DecayPeriodSeconds";
static const std::string kInitialNeedLevelKey = "InitialNeedLevel";
static const std::string kBracketLevelKey = "BracketLevel";
static const std::string kBrokenPartThresholdKey = "BrokenPartThreshold";

static const std::string kDecayRatesKey = "DecayRates";
static const std::string kConnectedDecayRatesKey = "ConnectedDecayRates";
static const std::string kUnconnectedDecayRatesKey = "UnconnectedDecayRates";
static const std::string kThresholdKey = "Threshold";
static const std::string kDecayPerMinuteKey = "DecayPerMinute";

static const std::string kDecayModifiersKey = "DecayModifiers";
static const std::string kConnectedDecayModifiersKey = "ConnectedDecayModifiers";
static const std::string kUnconnectedDecayModifiersKey = "UnconnectedDecayModifiers";
static const std::string kOtherNeedsAffectedKey = "OtherNeedsAffected";
static const std::string kOtherNeedIDKey = "OtherNeedID";
static const std::string kMultiplierKey = "Multiplier";

static const std::string kInitialUnlockLevelsArrayKey = "UnlockLevels";

static const std::string kActionDeltasKey = "actionDeltas";
static const std::string kActionIDKey = "actionId";
static const std::string kRepairDeltaKey = "repairDelta";
static const std::string kRepairRangeKey = "repairRange";
static const std::string kEnergyDeltaKey = "energyDelta";
static const std::string kEnergyRangeKey = "energyRange";
static const std::string kPlayDeltaKey = "playDelta";
static const std::string kPlayRangeKey = "playRange";


NeedsConfig::NeedsConfig()
: _minNeedLevel(0.0f)
, _maxNeedLevel(1.0f)
, _decayPeriod(60.0f)
, _initialNeedsLevels()
, _needsBrackets()
, _brokenPartThresholds()
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
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    const std::string keyStr = kInitialNeedLevelKey + EnumToString(static_cast<NeedId>(i));
    const auto value = JsonTools::ParseFloat(json, keyStr.c_str(),
                                             "Failed to parse an initial need level");
    _initialNeedsLevels[static_cast<NeedId>(i)] = value;
  }

  _needsBrackets.clear();
  for (size_t i = 0; i < (size_t)NeedId::Count; i++)
  {
    BracketThresholds thresholds;
    float prevValue = 1.0f;
    for (size_t j = 0; j < (size_t)NeedBracketId::Count; j++)
    {
      const std::string keyStr = kBracketLevelKey + EnumToString(static_cast<NeedId>(i))
                                 + EnumToString(static_cast<NeedBracketId>(j));
      const auto value = JsonTools::ParseFloat(json, keyStr.c_str(),
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

  _brokenPartThresholds.clear();
  float prevValue = 1.0f;
  for (int i = 0; i < MAX_REPAIRABLE_PARTS; i++)
  {
    const std::string keyStr = kBrokenPartThresholdKey + std::to_string(i);

    // Since we don't really know how many repairable parts there are yet (MAX_REPAIRABLE_PARTS
    // is set to something higher than we allow now, for future expansion), just keep trying
    // for a higher number in the key, until the new key doesn't exist, and then stop silently
    const auto & val = json[keyStr];
    if (!val.isNumeric())
    {
      break;
    }
    const auto value = val.asFloat();

    DEV_ASSERT_MSG(value <= prevValue,
                   "NeedsConfig.Init",
                   "Broken part thresholds not in descending order (%f vs %f)", value, prevValue);
    prevValue = value;
    _brokenPartThresholds.push_back(value);
  }
}


void NeedsConfig::InitDecay(const Json::Value& json)
{
  InitDecayRates(json[kDecayRatesKey], kConnectedDecayRatesKey, _decayConnected);
  InitDecayRates(json[kDecayRatesKey], kUnconnectedDecayRatesKey, _decayUnconnected);

  InitDecayModifiers(json[kDecayModifiersKey], kConnectedDecayModifiersKey, _decayConnected);
  InitDecayModifiers(json[kDecayModifiersKey], kUnconnectedDecayModifiersKey, _decayUnconnected);
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


void NeedsConfig::InitDecayRates(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo)
{
  DecayRatesByNeed decayRatesByNeed;

  for (size_t needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const std::string keyStr = baseKey + EnumToString(static_cast<NeedId>(needIndex));
    const auto& jsonForNeed = json[keyStr.c_str()];

    DecayRates decayRates;
    for (const auto& item : jsonForNeed)
    {
      const auto threshold = JsonTools::ParseFloat(item, kThresholdKey.c_str(),
                                                   "Failed to parse a decay rate threshold");
      const auto rate = JsonTools::ParseFloat(item, kDecayPerMinuteKey.c_str(),
                                              "Failed to parse a decay rate");
      DEV_ASSERT_MSG(rate >= 0.0f, "NeedConfig.InitDecayRates",
                     "A decay rate needs to be positive but is %f", rate);
      decayRates.push_back(DecayRate(threshold, rate));
    }
    // Sort the decay rates by descending threshold
    std::sort(decayRates.begin(), decayRates.end(), SortDecayRatesByThresholdDescending());
    
    decayRatesByNeed.push_back(decayRates);
  }

  decayInfo._decayRatesByNeed = std::move(decayRatesByNeed);
}


void NeedsConfig::InitDecayModifiers(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo)
{
  DecayModifiersByNeed decayModifiersByNeed;

  for (size_t needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const std::string keyStr = baseKey + EnumToString(static_cast<NeedId>(needIndex));
    const auto& jsonForNeed = json[keyStr.c_str()];

    DecayModifiers decayModifiers;
    for (const auto& item : jsonForNeed)
    {
      const auto threshold = JsonTools::ParseFloat(item, kThresholdKey.c_str(),
                                                   "Failed to parse a decay modifier threshold");
      OtherNeedModifiers otherNeedModifiers;
      for (const auto& otherNeedItem : item[kOtherNeedsAffectedKey])
      {
        const auto otherNeedStr = JsonTools::ParseString(otherNeedItem, kOtherNeedIDKey.c_str(),
                                                         "Failed to parse a modifier 'other need id'");
        const NeedId otherNeedID = NeedIdFromString(otherNeedStr.c_str());
        const auto multiplier = JsonTools::ParseFloat(otherNeedItem, kMultiplierKey.c_str(),
                                                      "Failed to parse a decay modifier multiplier");
        otherNeedModifiers.push_back(OtherNeedModifier(otherNeedID, multiplier));
      }
      decayModifiers.push_back(DecayModifier(threshold, otherNeedModifiers));
    }
    // Sort the decay modifiers by descending threshold
    std::sort(decayModifiers.begin(), decayModifiers.end(), SortDecayModifiersByThresholdDescending());

    decayModifiersByNeed.push_back(decayModifiers);
  }

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

    const float repairDelta = JsonTools::ParseFloat(item, kRepairDeltaKey.c_str(),
                                                    "Failed to parse a repair delta");
    const float repairRange = JsonTools::ParseFloat(item, kRepairRangeKey.c_str(),
                                                    "Failed to parse a repair range");
    const float energyDelta = JsonTools::ParseFloat(item, kEnergyDeltaKey.c_str(),
                                                    "Failed to parse a energy delta");
    const float energyRange = JsonTools::ParseFloat(item, kEnergyRangeKey.c_str(),
                                                    "Failed to parse a energy range");
    const float playDelta = JsonTools::ParseFloat(item, kPlayDeltaKey.c_str(),
                                                    "Failed to parse a play delta");
    const float playRange = JsonTools::ParseFloat(item, kPlayRangeKey.c_str(),
                                                    "Failed to parse a play range");

    ActionDelta& actionDelta = _actionDeltas[static_cast<int>(actionId)];
    actionDelta._needDeltas[static_cast<int>(NeedId::Repair)]._delta = repairDelta;
    actionDelta._needDeltas[static_cast<int>(NeedId::Repair)]._randomRange = repairRange;
    actionDelta._needDeltas[static_cast<int>(NeedId::Energy)]._delta = energyDelta;
    actionDelta._needDeltas[static_cast<int>(NeedId::Energy)]._randomRange = energyRange;
    actionDelta._needDeltas[static_cast<int>(NeedId::Play)]._delta = playDelta;
    actionDelta._needDeltas[static_cast<int>(NeedId::Play)]._randomRange = playRange;
  }
}


} // namespace Cozmo
} // namespace Anki

