/**
 * File: behaviorPlaypenDriveForwards.h
 *
 * Author: Al Chaussee
 * Created: 07/28/17
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__
#define __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__

#include "engine/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

namespace Anki {
namespace Cozmo {

class BehaviorPlaypenDriveForwards : public IBehaviorPlaypen
{
protected:
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorPlaypenDriveForwards(Robot& robot, const Json::Value& config);
  
protected:
  
  virtual Result         InternalInitInternal(Robot& robot)   override;
  virtual BehaviorStatus InternalUpdateInternal(Robot& robot) override;
  virtual void           StopInternal(Robot& robot)   override;
  
  virtual void HandleWhileRunningInternal(const EngineToGameEvent& event, Robot& robot) override;
  
  virtual bool ShouldRunWhileOnCharger() const override { return true; }
  
private:

  void TransitionToWaitingForBackCliffs(Robot& robot);
  void TransitionToWaitingForBackCliffUndetected(Robot& robot);
  
  enum State {
    NONE,
    WAITING_FOR_FRONT_CLIFFS,
    WAITING_FOR_FRONT_CLIFFS_UNDETCTED,
    WAITING_FOR_BACK_CLIFFS,
    WAITING_FOR_BACK_CLIFFS_UNDETECTED
  };
  State _waitingForCliffsState = NONE;
  bool _frontCliffsDetected = false;
  bool _backCliffsDetected = false;
  
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__
