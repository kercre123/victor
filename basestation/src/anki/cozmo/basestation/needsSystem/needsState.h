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

#include <json/json.h>

#include <assert.h>
#include <map>

namespace Anki {
namespace Cozmo {


namespace ExternalInterface {
  class MessageGameToEngine;
}

class NeedsConfig;
struct DecayConfig;

using Time = std::chrono::time_point<std::chrono::system_clock>;


class NeedsState
{
public:
  
  NeedsState();
  ~NeedsState();
  
  void Init(NeedsConfig& needsConfig, u32 serialNumber);
  
  void Reset();
  
  // Decay the needs values, according to how much time has passed since last decay, and config data
  void ApplyDecay(const float timeElasped_s, const DecayConfig& decayConfig);
  
  float         GetNeedLevelByIndex(size_t i)     { return _curNeedsLevels[static_cast<NeedId>(i)]; }
  NeedBracketId GetNeedBracketByIndex(size_t i)   { return _curNeedsBracketsCache[static_cast<NeedId>(i)]; };
  bool          GetPartIsDamagedByIndex(size_t i) { return _partIsDamaged[static_cast<RepairablePartId>(i)]; };
  
  // Set current needs bracket levels from current levels
  void UpdateCurNeedsBrackets();


  // Serialization versions; increment this when the format of the serialization changes
  // This is stored in the serialized file
  // Note that changing format of robot storage serialization will be more difficult,
  // because it serializes a CLAD structure, so for backward compatibility we'd have
  // to preserve older versions of that CLAD structure.
  static const int kDeviceStorageVersion = 1;
  static const int kRobotStorageVersion = 1;

  Time _timeLastWritten;

  u32 _robotSerialNumber;

  using CurNeedsMap = std::map<NeedId, float>;
  CurNeedsMap _curNeedsLevels;

  // These are meant for convenience to the game, and indicate the 'severity bracket' of the
  // corresponding need level (e.g. 'full', 'normal', 'warning', 'critical')
  using CurNeedsBrackets = std::map<NeedId, NeedBracketId>;
  CurNeedsBrackets _curNeedsBracketsCache;

  using PartIsDamagedMap = std::map<RepairablePartId, bool>;
  PartIsDamagedMap _partIsDamaged;
  
  int _curNeedsUnlockLevel;
  int _numStarsAwarded;
  int _numStarsForNextUnlock;

private:

  const NeedsConfig* _needsConfig;  // Do I really need this?

};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsState_H__

