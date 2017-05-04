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


#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"
#include "anki/cozmo/basestation/components/nvStorageComponent.h"
#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/needsSystem/needsManager.h"
#include "anki/cozmo/basestation/needsSystem/needsState.h"
#include "anki/cozmo/basestation/needsSystem/needsConfig.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {


NeedsState::NeedsState()
: _timeLastWritten(Time())
, _robotSerialNumber(0)
, _curNeedsLevels()
, _curNeedsBracketsCache()
, _partIsDamaged()
, _curNeedsUnlockLevel(0)
, _numStarsAwarded(0)
, _numStarsForNextUnlock(1)
, _needsConfig(nullptr)
{
}

NeedsState::~NeedsState()
{
  Reset();
}


void NeedsState::Init(NeedsConfig& needsConfig, u32 serialNumber)
{
  Reset();

  _timeLastWritten = Time();  // ('never')

  _needsConfig = &needsConfig;

  _robotSerialNumber = serialNumber;

  for (int i = 0; i < static_cast<int>(NeedId::Count); i++)
  {
    const auto& needId = static_cast<NeedId>(i);
    _curNeedsLevels[needId] = needsConfig._initialNeedsLevels[needId];
  }

  UpdateCurNeedsBrackets(needsConfig._needsBrackets);
  
  for (int i = 0; i < static_cast<int>(RepairablePartId::Count); i++)
  {
    const auto& repairablePartId = static_cast<RepairablePartId>(i);
    _partIsDamaged[repairablePartId] = false;
  }

  _curNeedsUnlockLevel = 0;
  _numStarsAwarded = 0;
  _numStarsForNextUnlock = 3; // todo set this from config data
}


void NeedsState::Reset()
{
  _curNeedsLevels.clear();
  _curNeedsBracketsCache.clear();
  _partIsDamaged.clear();
}


void NeedsState::ApplyDecay(const float timeElasped_s, const DecayConfig& decayConfig)
{
  PRINT_CH_INFO(NeedsManager::kLogChannelName, "NeedsState.ApplyDecay",
                "Decaying needs with elapsed time of %f seconds", timeElasped_s);

  // First, set up some decay rate multipliers, based on config data, and the CURRENT needs levels:

  // Note that for long time periods (i.e. unconnected), we won't handle the progression across
  // multiple tiers of brackets FOR MULTIPLIER PURPOSES, but design doesn't want any multipliers
  // for unconnected decay anyway.  We do, however handle multiple tiers properly when we apply
  // decay below.

  std::array<float, (size_t)NeedId::Count> multipliers;
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

  // Second, now apply decay:
  // This handles any time elapsed passed in
  for (int needIndex = 0; needIndex < (size_t)NeedId::Count; needIndex++)
  {
    float curNeedLevel = _curNeedsLevels[static_cast<NeedId>(needIndex)];
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
      continue;
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
    _curNeedsLevels[static_cast<NeedId>(needIndex)] = curNeedLevel;
  }
}


void NeedsState::UpdateCurNeedsBrackets(const NeedsBrackets& needsBrackets)
{
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
}


} // namespace Cozmo
} // namespace Anki

