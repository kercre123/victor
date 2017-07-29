/**
 * File: behaviorPlaypenDriftCheck.h
 *
 * Author: Al Chaussee
 * Created: 07/27/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDriftCheck : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDriftCheck(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual void GetResultsInternal() override;
  
private:
  
  void TransitionToPlayingSound(Robot& robot);
  void CheckDrift(Robot& robot);
  
  Radians _startingRobotOrientation;
  
  bool _soundComplete = false;
  bool _driftCheckComplete = false;
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriftCheck_H__
