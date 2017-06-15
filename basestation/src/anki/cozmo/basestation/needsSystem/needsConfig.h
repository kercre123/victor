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
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "util/enums/enumOperators.h"


namespace Anki {
namespace Cozmo {


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
  NeedsConfig();

  void Init(const Json::Value& json);
  void InitDecay(const Json::Value& json);

  float _minNeedLevel;
  float _maxNeedLevel;
  float _decayPeriod;

  NeedsState::CurNeedsMap _initialNeedsLevels;

  using BracketThresholds = std::vector<float>;
  using NeedsBrackets = std::map<NeedId, BracketThresholds>;
  NeedsBrackets _needsBrackets;

  using BrokenPartThresholds = std::vector<float>;
  BrokenPartThresholds _brokenPartThresholds;

  DecayConfig _decayConnected;
  DecayConfig _decayUnconnected;

private:
  void InitDecayRates(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo);
  void InitDecayModifiers(const Json::Value& json, const std::string& baseKey, DecayConfig& decayInfo);

};


class StarRewardsConfig
{
public:
  void Init(const Json::Value& json);
  
  int GetMaxStarsForLevel(int level);
  
  void GetRewardsForLevel(int level, std::vector<NeedsReward>& rewards);
private:
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
  std::array<NeedDelta, static_cast<size_t>(NeedId::Count)> _needDeltas;
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

