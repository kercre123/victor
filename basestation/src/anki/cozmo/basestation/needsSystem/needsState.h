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
#include <assert.h>
#include <map>

namespace Anki {
namespace Cozmo {


namespace ExternalInterface {
  class MessageGameToEngine;
}

class NeedsConfig;


class NeedsState
{
public:
  
  NeedsState();
  ~NeedsState();
  
  // Init
  void Init(const NeedsConfig& needsConfig);
  
  // Reset
  void Reset();
  
  // Decay the needs values, according to how much time has passed since last decay, and config data
  void Decay(float timeElasped_secs);
  
  void DecayUnconnected(float timeElapsed_secs);  //?
  
  float         GetNeedLevelByIndex(size_t i)     { return _curNeedsLevels[(NeedId)i]; }
  NeedBracketId GetNeedBracketByIndex(size_t i)   { return _curNeedsBrackets[(NeedId)i]; };
  bool          GetPartIsDamagedByIndex(size_t i) { return _partIsDamaged[(RepairablePartId)i]; };
  
  // Set current needs bracket levels from current levels
  void UpdateCurNeedsBrackets(const NeedsConfig& needsConfig);

  // todo:  DateTime	LastConnectedDateTime

  using CurNeedsMap = std::map<NeedId, float>;
  CurNeedsMap _curNeedsLevels;

  using CurNeedsBrackets = std::map<NeedId, NeedBracketId>;
  CurNeedsBrackets _curNeedsBrackets;

  using PartIsDamagedMap = std::map<RepairablePartId, bool>;
  PartIsDamagedMap _partIsDamaged;
  
  int _curNeedsUnlockLevel;
  int _numStarsAwarded;
  int _numStarsForNextUnlock;

  // accessors:  for need level (by type); need bracket (by type), Part daamaged, stars awarded, stars for next unlcok , curneedsunlock lvel

  // todo: read/write to storage (read includes any re-gen of other fields such as CurNeedsBrackets)

private:
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_NeedsSystem_NeedsState_H__

