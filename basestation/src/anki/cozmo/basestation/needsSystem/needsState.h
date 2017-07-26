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


#ifndef __Cozmo_Basestation_NeedsSystem_NeedsState_H__
#define __Cozmo_Basestation_NeedsSystem_NeedsState_H__

#include "anki/common/types.h"
#include "clad/types/needsSystemTypes.h"
#include "clad/types/unlockTypes.h"
#include "util/global/globalDefinitions.h"
#include "util/random/randomGenerator.h"

#include <assert.h>
#include <vector>
#include <map>

namespace Anki {
namespace Cozmo {


namespace ExternalInterface {
  class MessageGameToEngine;
}

class NeedsConfig;
struct NeedDelta;

class StarRewardsConfig;
struct DecayConfig;

using BracketThresholds = std::vector<float>;
using NeedsBrackets = std::map<NeedId, BracketThresholds>;

using Time = std::chrono::time_point<std::chrono::system_clock>;
using NeedsMultipliers = std::array<float, (size_t)NeedId::Count>;

class NeedsState
{
public:
  
  NeedsState();
  ~NeedsState();
  
  void Init(NeedsConfig& needsConfig, const u32 serialNumber,
            const std::shared_ptr<StarRewardsConfig> starRewardsConfig, Util::RandomGenerator* rng);

  // Set up decay multipliers (we do this prior to calling ApplyDecay)
  void SetDecayMultipliers(const DecayConfig& decayConfig, std::array<float, (size_t)NeedId::Count>& multipliers);
  
  // Decay a need's level, according to how much time has passed since last decay, and config data
  void ApplyDecay(const DecayConfig& decayConfig, const int needIndex, const float timeElasped_s, const NeedsMultipliers& multipliers);

  // Apply a given delta to a given need
  bool ApplyDelta(const NeedId needId, const NeedDelta& needDelta, const NeedsActionId cause);


  float         GetNeedLevel(NeedId need) const;
  NeedBracketId GetNeedBracket(NeedId need);
  
  float         GetNeedLevelByIndex(size_t needIndex)     { return _curNeedsLevels[static_cast<NeedId>(needIndex)]; }
  NeedBracketId GetNeedBracketByIndex(size_t needIndex);
  bool          GetPartIsDamagedByIndex(size_t needIndex) { return _partIsDamaged[static_cast<RepairablePartId>(needIndex)]; };

  // Return true if all needs are "met"
  bool AreNeedsMet();
  
  // Set current needs bracket levels from current levels
  void UpdateCurNeedsBrackets(const NeedsBrackets& needsBrackets);

  bool IsNeedAtBracket(const NeedId need, const NeedBracketId bracket);
  
  // Get the lowest need and what bracket it is in
  // Lowest need is based on which need has the lowest actual value
  void GetLowestNeedAndBracket(NeedId& lowestNeedId, NeedBracketId& lowestNeedBracketId) const;

  void SetNeedsBracketsDirty() { _needsBracketsDirty = true; };

  void SetPrevNeedsBrackets();

  NeedBracketId GetPrevNeedBracketByIndex(size_t i) { return _prevNeedsBracketsCache[static_cast<NeedId>(i)]; }
  
  int NumDamagedParts() const;
  int NumDamagedPartsForRepairLevel(const float level) const;
  RepairablePartId PickPartToRepair() const;

  // Serialization versions; increment this when the format of the serialization changes
  // This is stored in the serialized file
  // Note that changing format of robot storage serialization will be more difficult,
  // because it serializes a CLAD structure, so for backward compatibility we'd have
  // to preserve older versions of that CLAD structure.
  static const int kDeviceStorageVersion = 4;
  static const int kRobotStorageVersion = 4;

  Time _timeLastWritten;

  // Time of last disconnect:  This is for DAS purposes, to find out 'how long has
  // the user left Cozmo disconnected.'  We can't really store this on the robot,
  // because on disconnect it's too late to write to the robot.  Therefore this is
  // device-storage-only, and consequently if the device connects to a different
  // robot, this time will be reset.
  Time _timeLastDisconnect;

  // Time of last app backgrounding:  This is for DAS purposes, to find out 'how
  // long has it been since the user exited (backgrounded) the app.  Similar to
  // the above, we don't bother storing this on the robot.
  Time _timeLastAppBackgrounded;

  // Also for DAS purposes, this is the number of times the user has opened or
  // unbackgrounded the app, since the last robot disconnect.
  int  _timesOpenedSinceLastDisconnect;

  u32 _robotSerialNumber;

  Util::RandomGenerator* _rng;

  using CurNeedsMap = std::map<NeedId, float>;
  CurNeedsMap _curNeedsLevels;

  using PartIsDamagedMap = std::map<RepairablePartId, bool>;
  PartIsDamagedMap _partIsDamaged;

  int      _curNeedsUnlockLevel;
  int      _numStarsAwarded;
  int      _numStarsForNextUnlock;
  Time     _timeLastStarAwarded;
  UnlockId _forceNextSong;
  
  void SetStarLevel(int newLevel);

  const NeedsConfig* _needsConfig;

  std::shared_ptr<StarRewardsConfig> _starRewardsConfig;

#if ANKI_DEV_CHEATS
  void DebugFillNeedMeters();
#endif

private:

  // These are meant for convenience to the game, and indicate the 'severity bracket' of the
  // corresponding need level (e.g. 'full', 'normal', 'warning', 'critical')
  using CurNeedsBrackets = std::map<NeedId, NeedBracketId>;
  CurNeedsBrackets _curNeedsBracketsCache;
  CurNeedsBrackets _prevNeedsBracketsCache;

  bool _needsBracketsDirty;

  void Reset();

  void PossiblyDamageParts(const NeedsActionId cause);
  RepairablePartId PickPartToDamage() const;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsState_H__

