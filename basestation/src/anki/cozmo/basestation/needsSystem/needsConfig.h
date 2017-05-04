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

DECLARE_ENUM_INCREMENT_OPERATORS(NeedId);

NeedId NeedIdFromString(const char* inString);


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

  float _minNeedLevel;
  float _maxNeedLevel;
  float _decayPeriod;

  NeedsState::CurNeedsMap _initialNeedsLevels;

  using BracketThresholds = std::vector<float>;
  using NeedsBrackets = std::map<NeedId, BracketThresholds>;
  NeedsBrackets _needsBrackets;

  DecayConfig _decayConnected;
  DecayConfig _decayUnconnected;

private:
  void InitDecay(const Json::Value& json, const std::string& decayKey, DecayConfig& decayConfig);

};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsConfig_H__

