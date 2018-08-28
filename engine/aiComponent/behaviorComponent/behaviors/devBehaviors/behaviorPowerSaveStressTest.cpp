/**
 * File: BehaviorPowerSaveStressTest.cpp
 *
 * Author: Al Chaussee
 * Created: 8/15/2018
 *
 * Description: Simple behavior to stress test power save mode (and nothing else)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/devBehaviors/behaviorPowerSaveStressTest.h"
#include "engine/aiComponent/behaviorComponent/behaviorComponents_fwd.h"
#include "engine/components/powerStateManager.h"
#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Vector {

  void BehaviorPowerSaveStressTest::OnBehaviorActivated()
  {
    _then = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    SmartRequestPowerSaveMode();
  }
  
  void BehaviorPowerSaveStressTest::BehaviorUpdate()
  {
    if(!IsActivated())
    {
      return;
    }
    
    const auto now = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    const int kToggleTime_sec = 5;
    if(now - _then > kToggleTime_sec)
    {
      _then = now;
      auto& powerSaveManager = GetBehaviorComp<PowerStateManager>();
      static bool enable = false;
      if(enable)
      {
        powerSaveManager.RequestPowerSaveMode(GetDebugLabel());
      }
      else
      {
        powerSaveManager.RemovePowerSaveModeRequest(GetDebugLabel());
      }
      enable = !enable;
    }
  }
  
}
}
