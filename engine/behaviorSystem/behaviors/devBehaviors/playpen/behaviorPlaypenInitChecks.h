/**
 * File: behaviorPlaypenInitChecks.h
 *
 * Author: Al Chaussee
 * Created: 08/09/17
 *
 * Description: Quick check of initial robot state for playpen. Checks things like firmware version,
 *              battery voltage, cliff sensors, etc
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenInitChecks : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenInitChecks(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void AlwaysHandle(const RobotToEngineEvent& event, const Robot& robot) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:
  
  std::string _fwBuildType                = "";
  u32         _fwVersion                  = 0;
  bool        _failedToParseVersionHeader = false;
  
  int32_t     _hwVersion = -1;
  bool        _gotMfgID  = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenInitChecks_H__
