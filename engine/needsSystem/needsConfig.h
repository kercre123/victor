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


#ifndef __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__
#define __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__

#include "anki/common/types.h"
#include "engine/needsSystem/needsState.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {

class CozmoContext;


struct DecayRate
{
  DecayRate(float t, float d) : _threshold(t), _decayPerMinute(d) {}

  float _threshold;
  float _decayPerMinute;
};


struct OtherNeedModifier
{
  OtherNeedModifier(NeedId n, float m) : _otherNeedID(n), _multiplier(m) {}

  NeedId _otherNeedID;
  float  _multiplier;
};


using OtherNeedModifiers = std::vector<OtherNeedModifier>;

struct DecayModifier
{
  DecayModifier(float t, OtherNeedModifiers onms) : _threshold(t), _otherNeedModifiers(onms) {}

  float              _threshold;
  OtherNeedModifiers _otherNeedModifiers;
};


using DecayRates = std::vector<DecayRate>;
using DecayRatesByNeed = std::vector<DecayRates>;

using DecayModifiers = std::vector<DecayModifier>;
using DecayModifiersByNeed = std::vector<DecayModifiers>;

struct DecayConfig
{
  DecayRatesByNeed     _decayRatesByNeed;
  DecayModifiersByNeed _decayModifiersByNeed;
};


class NeedsConfig
{
public:
  NeedsConfig(const CozmoContext* cozmoContext);

  void Init(const Json::Value& json);
  void InitDecay(const Json::Value& json);

  float NeedLevelForNeedBracket(const NeedId needId, const NeedBracketId bracketId) const;

  void SetUnconnectedDecayTestVariation(const std::string& baseFilename, const std::string& variationKey,
                                        const Util::AnkiLab::AssignmentStatus assignmentStatus);
  const std::string& GetUnconnectedDecayTestVariation() const { return _unconnectedDecayTestVariationKey; };

  float _minNeedLevel;
  float _maxNeedLevel;
  float _decayPeriod;

  NeedsState::CurNeedsMap _initialNeedsLevels;

  using BracketThresholds = std::vector<float>;
  using NeedsBrackets = std::map<NeedId, BracketThresholds>;
  NeedsBrackets _needsBrackets;

  using FullnessDecayCooldownTimes_s = std::map<NeedId, float>;
  FullnessDecayCooldownTimes_s _fullnessDecayCooldownTimes_s;

  using BrokenPartThresholds = std::vector<float>;
  BrokenPartThresholds _brokenPartThresholds;

  DecayConfig _decayConnected;
  DecayConfig _decayUnconnected;

  float _localNotificationMaxFutureMinutes;

  int _repairRounds;

private:
  void InitDecayRates(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo);
  void InitDecayModifiers(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo);

  const CozmoContext* _cozmoContext;

  std::string _unconnectedDecayTestVariationKey = "Unknown (unknown)";
};


class StarRewardsConfig
{
public:
  void Init(const Json::Value& json);
  
  int GetMaxStarsForLevel(int level) const;
  
  void GetRewardsForLevel(int level, std::vector<NeedsReward>& rewards) const;

  int GetTargetSparksTotalForLevel(int level) const { return GetLevelOrLastLevel(level).targetSparksTotal; }
  int GetMaxPriorUnlocksForLevel(int level)   const { return GetLevelOrLastLevel(level).maxPriorLevelUnlocks; }
  float GetMinSparksPctForLevel(int level)    const { return GetLevelOrLastLevel(level).minSparksPct; }
  float GetMaxSparksPctForLevel(int level)    const { return GetLevelOrLastLevel(level).maxSparksPct; }
  int GetMinSparksForLevel(int level)         const { return GetLevelOrLastLevel(level).minSparks; }
  int GetMinMaxSparksForLevel(int level)      const { return GetLevelOrLastLevel(level).minMaxSparks; }
  int GetFreeplayTargetSparksTotalForLevel(int level)    const { return GetLevelOrLastLevel(level).freeplayTargetSparksTotal; }
  float GetFreeplayMinSparksRewardPctForLevel(int level) const { return GetLevelOrLastLevel(level).freeplayMinSparksRewardPct; }
  float GetFreeplayMinSparksPctForLevel(int level)       const { return GetLevelOrLastLevel(level).freeplayMinSparksPct; }
  float GetFreeplayMaxSparksPctForLevel(int level)       const { return GetLevelOrLastLevel(level).freeplayMaxSparksPct; }
  int GetFreeplayMinSparksForLevel(int level)            const { return GetLevelOrLastLevel(level).freeplayMinSparks; }
  int GetFreeplayMinMaxSparksForLevel(int level)         const { return GetLevelOrLastLevel(level).freeplayMinMaxSparks; }

  int GetNumLevels() const { return static_cast<int>(_UnlockLevels.size()); }

private:
  const UnlockLevel& GetLevelOrLastLevel(int level) const;

  std::vector<UnlockLevel> _UnlockLevels;
};


struct NeedDelta
{
  NeedDelta(float delta, float randomRange, NeedsActionId cause) { _delta = delta; _randomRange = randomRange; _cause = cause; };
  NeedDelta() { _delta = 0.0f; _randomRange = 0.0f; _cause = NeedsActionId::NoAction; }

  float         _delta;
  float         _randomRange;
  NeedsActionId _cause;
};

struct ActionDelta
{
  ActionDelta() : _cooldown_s(0.0f), _freeplaySparksRewardWeight(0.0f) {}

  std::array<NeedDelta, static_cast<size_t>(NeedId::Count)> _needDeltas;
  float _cooldown_s;
  float _freeplaySparksRewardWeight;
};

class ActionsConfig
{
public:
  ActionsConfig();

  void Init(const Json::Value& json);

  std::vector<ActionDelta> _actionDeltas;

private:
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__

