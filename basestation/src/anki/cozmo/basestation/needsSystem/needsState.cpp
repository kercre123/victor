/**
 * File: needsState
 *
 * Author: Paul Terry
 * Created: 04/12/2017
 *
 * Description: State data for Cozmo's Needs system
 *
 * Copyright: Anki, Inc. 2017
 *
 **/


#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/robot.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {


NeedsState::NeedsState()
: _timeLastWritten(Time())
, _robotSerialNumber(0)
, _rng(nullptr)
, _curNeedsLevels()
, _curNeedsBracketsCache()
, _partIsDamaged()
, _curNeedsUnlockLevel(0)
, _numStarsAwarded(0)
, _numStarsForNextUnlock(1)
, _timeLastStarAwarded(Time())
, _needsConfig(nullptr)
, _starRewardsConfig(nullptr)
, _needsBracketsDirty(true)
{
}

NeedsState::~NeedsState()
{
  Reset();
}


void NeedsState::Init(NeedsConfig& needsConfig, const u32 serialNumber,
                      const std::shared_ptr<StarRewardsConfig> starRewardsConfig, Util::RandomGenerator* rng)
{
  Reset();

  _timeLastWritten = Time();  // ('never')

  _needsConfig = &needsConfig;

  _robotSerialNumber = serialNumber;

  _rng = rng;

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    const auto& needId = static_cast<NeedId>(i);
    _curNeedsLevels[needId] = needsConfig._initialNeedsLevels[needId];
  }

  _needsBracketsDirty = true;
  UpdateCurNeedsBrackets(needsConfig._needsBrackets);
  
  for (int i = 0; i < static_cast<int>(RepairablePartId::Count); i++)
  {
    const auto& repairablePartId = static_cast<RepairablePartId>(i);
    _partIsDamaged[repairablePartId] = false;
  }

  _starRewardsConfig = starRewardsConfig;
  
  _curNeedsUnlockLevel = 0;
  _numStarsAwarded = 0;
  _numStarsForNextUnlock = _starRewardsConfig->GetMaxStarsForLevel(0);
}


void NeedsState::Reset()
{
  _curNeedsLevels.clear();
  _curNeedsBracketsCache.clear();
  _partIsDamaged.clear();

  _needsBracketsDirty = true;
}


void NeedsState::SetDecayMultipliers(const DecayConfig& decayConfig, std::array<float, (size_t)NeedId::Count>& multipliers)
{
  PRINT_CH_INFO(NeedsManager::kLogChannelName, "NeedsState.SetDecayMultipliers",
                "Setting needs decay multipliers");

  // Set some decay rate multipliers, based on config data, and the CURRENT needs levels:

  // Note that for long time periods (i.e. unconnected), we won't handle the progression across
  // multiple tiers of brackets FOR MULTIPLIER PURPOSES, but design doesn't want any multipliers
  // for unconnected decay anyway.  We do, however handle multiple tiers properly when we apply
  // decay in ApplyDecay.

  multipliers.fill(1.0f);

  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const DecayModifiers& modifiers = decayConfig._decayModifiersByNeed[needIndex];
    if (!modifiers.empty()) // (It's OK for there to be no modifiers)
    {
      const float curNeedLevel = _curNeedsLevels[static_cast<NeedId>(needIndex)];

      // Note that the modifiers are assumed to be in descending order by threshold
      int modifierIndex = 0;
      for ( ; modifierIndex < modifiers.size(); modifierIndex++)
      {
        if (curNeedLevel >= modifiers[modifierIndex]._threshold)
        {
          break;
        }
      }
      // We can get here with an out of range index, because the last threshold
      // in the list does not have to be zero...
      if (modifierIndex < modifiers.size())
      {
        const OtherNeedModifiers& otherNeedModifiers = modifiers[modifierIndex]._otherNeedModifiers;

        for (const auto& onm : otherNeedModifiers)
        {
          int otherNeedIndex = static_cast<int>(onm._otherNeedID);
          multipliers[otherNeedIndex] *= onm._multiplier;
        }
      }
    }
  }
}

void NeedsState::ApplyDecay(const DecayConfig& decayConfig, const int needIndex, const float timeElasped_s, const NeedsMultipliers& multipliers)
{
  PRINT_CH_INFO(NeedsManager::kLogChannelName, "NeedsState.ApplyDecay",
                "Decaying need index %d with elapsed time of %f seconds", needIndex, timeElasped_s);
  
  // This handles any time elapsed passed in
  const NeedId needId = static_cast<NeedId>(needIndex);
  float curNeedLevel = _curNeedsLevels[needId];
  const DecayRates& rates = decayConfig._decayRatesByNeed[needIndex];

  // Find the decay 'bracket' the level is currently in
  // Note that the rates are assumed to be in descending order by threshold
  int rateIndex = 0;
  for ( ; rateIndex < rates.size(); rateIndex++)
  {
    if (curNeedLevel >= rates[rateIndex]._threshold)
    {
      break;
    }
  }
  if (rateIndex >= rates.size())
  {
    // Can happen if bottom bracket is non-zero threshold, or there are
    // no brackets at all; in those cases, just don't decay
    return;
  }

  float timeRemaining_min = (timeElasped_s / 60.0f);
  while (timeRemaining_min > 0.0f)
  {
    const DecayRate& rate = rates[rateIndex];
    const float bottomThreshold = rate._threshold;
    const float decayRatePerMin = rate._decayPerMinute * multipliers[needIndex];

    if (decayRatePerMin <= 0.0f)
    {
      break;  // Done if no decay (and avoid divide by zero below)
    }

    const float timeToBottomThreshold_min = (curNeedLevel - bottomThreshold) / decayRatePerMin;
    if (timeRemaining_min > timeToBottomThreshold_min)
    {
      timeRemaining_min -= timeToBottomThreshold_min;
      curNeedLevel = bottomThreshold;
      if (++rateIndex >= rates.size())
        break;
    }
    else
    {
      curNeedLevel -= (timeRemaining_min * decayRatePerMin);
      break;
    }
  }

  if (curNeedLevel < _needsConfig->_minNeedLevel)
  {
    curNeedLevel = _needsConfig->_minNeedLevel;
  }

  _curNeedsLevels[needId] = curNeedLevel;
  _needsBracketsDirty = true;

  if (needId == NeedId::Repair)
  {
    PossiblyDamageParts();
  }
}


void NeedsState::ApplyDelta(const NeedId needId, const NeedDelta& needDelta)
{
  float needLevel = _curNeedsLevels[needId];

  const float randDist = _rng->RandDbl(needDelta._randomRange * 2.0f) - needDelta._randomRange;
  const float delta = (needDelta._delta + randDist);
  needLevel += delta;
  needLevel = Util::Clamp(needLevel, _needsConfig->_minNeedLevel, _needsConfig->_maxNeedLevel);

  if ((needId == NeedId::Repair) && (delta > 0.0f))
  {
    // If Repair level is going up, clamp the delta so that it stays within the range of
    // thresholds for broken parts, according to the actual current number of broken parts
    const int numDamagedParts = NumDamagedParts();
    const static float epsilon = 0.00001f;
    const size_t numThresholds = _needsConfig->_brokenPartThresholds.size();

    // FIRST:  Clamp against going too high
    float maxLevel = _needsConfig->_maxNeedLevel;
    if (numDamagedParts >= numThresholds)
    {
      maxLevel = _needsConfig->_brokenPartThresholds[numThresholds - 1];
    }
    else if (numDamagedParts > 0)
    {
      maxLevel = _needsConfig->_brokenPartThresholds[numDamagedParts - 1];
    }
    if (needLevel > maxLevel)
    {
      needLevel = maxLevel - epsilon;
    }

    // SECOND:  Clamp against not going high enough
    float minLevel = _needsConfig->_minNeedLevel;
    if (numDamagedParts < numThresholds)
    {
      minLevel = _needsConfig->_brokenPartThresholds[numDamagedParts];
    }
    if (needLevel < minLevel)
    {
      needLevel = minLevel + epsilon;
    }
  }

  _curNeedsLevels[needId] = needLevel;
  _needsBracketsDirty = true;

  if ((needId == NeedId::Repair) && (delta < 0.0f))
  {
    PossiblyDamageParts();
  }
}


NeedBracketId NeedsState::GetNeedBracketByIndex(size_t i)
{
  UpdateCurNeedsBrackets(_needsConfig->_needsBrackets);

  return _curNeedsBracketsCache[static_cast<NeedId>(i)];
}


void NeedsState::SetStarLevel(int newLevel)
{
  _curNeedsUnlockLevel = newLevel;
  _numStarsAwarded = 0;
  _numStarsForNextUnlock = _starRewardsConfig->GetMaxStarsForLevel(_curNeedsUnlockLevel);
}

void NeedsState::UpdateCurNeedsBrackets(const NeedsBrackets& needsBrackets)
{
  if (!_needsBracketsDirty)
    return;

  // Set each of the needs' "current bracket" based on the current level for that need
  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    const NeedId needId = static_cast<NeedId>(needIndex);
    const float curNeedLevel = _curNeedsLevels[needId];

    const auto& bracketThresholds = needsBrackets.find(needId)->second;
    size_t bracketIndex = 0;
    const auto numBracketThresholds = bracketThresholds.size();
    for ( ; bracketIndex < numBracketThresholds; bracketIndex++)
    {
      if (curNeedLevel >= bracketThresholds[bracketIndex])
      {
        break;
      }
    }
    if (bracketIndex >= numBracketThresholds)
    {
      bracketIndex = numBracketThresholds - 1;
    }
    _curNeedsBracketsCache[needId] = static_cast<NeedBracketId>(bracketIndex);
  }

  _needsBracketsDirty = false;
}

int NeedsState::NumDamagedParts() const
{
  int numDamagedParts = 0;
  for (const auto& part : _partIsDamaged)
  {
    if (part.second)
    {
      numDamagedParts++;
    }
  }
  return numDamagedParts;
}

void NeedsState::PossiblyDamageParts()
{
  const int numDamagedParts = NumDamagedParts();
  const int numPartsTotal = static_cast<int>(_partIsDamaged.size());
  if (numDamagedParts >= numPartsTotal)
    return;

  const float curRepairLevel = _curNeedsLevels[NeedId::Repair];
  int newNumDamagedParts = 0;
  for ( ; newNumDamagedParts < _needsConfig->_brokenPartThresholds.size(); newNumDamagedParts++)
  {
    if (curRepairLevel > _needsConfig->_brokenPartThresholds[newNumDamagedParts])
    {
      break;
    }
  }
  if (newNumDamagedParts > numPartsTotal)
  {
    newNumDamagedParts = numPartsTotal;
  }

  const int partsToDamage = newNumDamagedParts - numDamagedParts;
  if (partsToDamage <= 0)
    return;

  for (int i = 0; i < partsToDamage; i++)
  {
    _partIsDamaged[PickPartToDamage()] = true;
  }
}

RepairablePartId NeedsState::PickPartToDamage() const
{
  const int numUndamagedParts = static_cast<int>(_partIsDamaged.size()) - NumDamagedParts();
  int undamagedPartIndex = _rng->RandInt(numUndamagedParts);
  int i = 0;
  for (const auto& part : _partIsDamaged)
  {
    if (!part.second)
    {
      if (undamagedPartIndex == 0)
      {
        break;
      }
      undamagedPartIndex--;
    }
    i++;
  }
  return static_cast<RepairablePartId>(i);
}


#if ANKI_DEV_CHEATS
void NeedsState::DebugFillNeedMeters()
{
  Reset();

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    _curNeedsLevels[static_cast<NeedId>(i)] = _needsConfig->_maxNeedLevel;
  }

  _needsBracketsDirty = true;
  UpdateCurNeedsBrackets(_needsConfig->_needsBrackets);
  
  for (int i = 0; i < static_cast<int>(RepairablePartId::Count); i++)
  {
    const auto& repairablePartId = static_cast<RepairablePartId>(i);
    _partIsDamaged[repairablePartId] = false;
  }
}
#endif


} // namespace Cozmo
} // namespace Anki

