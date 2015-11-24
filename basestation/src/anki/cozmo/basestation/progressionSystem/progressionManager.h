/**
 * File: progressionManager
 *
 * Author: Mark Wesley
 * Created: 11/10/15
 *
 * Description: Manages the Progression (ratings for a selection of stats) for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_ProgressionSystem_ProgressionManager_H__
#define __Cozmo_Basestation_ProgressionSystem_ProgressionManager_H__


#include "anki/cozmo/basestation/progressionSystem/progressionStat.h"
#include "clad/types/progressionStatTypes.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {
  
  
template <typename Type>
class AnkiEvent;


namespace ExternalInterface {
  class MessageGameToEngine;
}


class Robot;
  

class ProgressionManager
{
public:
  
  explicit ProgressionManager(Robot* inRobot = nullptr);
  
  void Reset();

  void Update(double currentTime);
  
  void HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);

  const ProgressionStat&  GetStat(ProgressionStatType statType) const { return GetStatByIndex((size_t)statType); }
  ProgressionStat&        GetStat(ProgressionStatType statType)       { return GetStatByIndex((size_t)statType); }
  
  void SendStatsToGame() const;

private:
  
  // ============================== Private Member Funcs ==============================
  
  const ProgressionStat&  GetStatByIndex(size_t index) const
  {
    assert((index >= 0) && (index < (size_t)ProgressionStatType::Count));
    return _stats[index];
  }
  
  ProgressionStat&  GetStatByIndex(size_t index)
  {
    assert((index >= 0) && (index < (size_t)ProgressionStatType::Count));
    return _stats[index];
  }

  // ============================== Private Member Vars ==============================

  ProgressionStat     _stats[(size_t)ProgressionStatType::Count];
  Robot*              _robot;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_ProgressionSystem_ProgressionManager_H__

