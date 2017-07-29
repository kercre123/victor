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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/devBehaviors/playpen/iBehaviorPlaypen.h"

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
  
  virtual void GetResultsInternal() override;
  
private:
  
  void TransitionToWaitingForFrontCliffs(Robot& robot);
  void TransitionToWaitingForBackCliffs(Robot& robot);
};

}
}

#endif // __Cozmo_Basestation_Behaviors_BehaviorPlaypenDriveForwards_H__
